#include "TrainManager.h"
#include "EnemyStrategyManager.h"

using namespace MyBot;

TrainManager::TrainManager()
{
	reservedMinerals = 0;
	reservedGas = 0;
	saveGas = false;
	waitToProduce = false;
	nextVessleTime = 0;
}

TrainManager &TrainManager::Instance()
{
	static TrainManager instance;
	return instance;
}

void TrainManager::update()
{
	if (TIME % 4 != 0)
		return;

	
	if (SM.getNeedUpgrade())
	{
		return;
	}

	if (S->supplyUsed() >= 32 && INFO.getAllCount(Terran_Factory, S) == 0 &&
			(EIB != Zerg_4_Drone && EIB != Zerg_5_Drone && EIB != Zerg_sunken_rush && EIB != Toss_cannon_rush && EIB != Terran_bunker_rush && ESM.getEnemyGasRushRefinery() == nullptr))
	{
		return;
	}

	reservedMinerals = 0;
	reservedGas = 0;
	waitToProduce = false;

	try {
		factoryTraining();
	}
	catch (SAIDA_Exception e) {
		Logger::error("factoryTraining Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("factoryTraining Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("factoryTraining Unknown Error.\n");
		throw;
	}

	try {
		commandCenterTraining();
	}
	catch (SAIDA_Exception e) {
		Logger::error("commandCenterTraining Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("commandCenterTraining Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("commandCenterTraining Unknown Error.\n");
		throw;
	}

	try {
		barracksTraining();
	}
	catch (SAIDA_Exception e) {
		Logger::error("barracksTraining Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("barracksTraining Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("barracksTraining Unknown Error.\n");
		throw;
	}

	try {
		starportTraining();
	}
	catch (SAIDA_Exception e) {
		Logger::error("starportTraining Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("starportTraining Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("starportTraining Unknown Error.\n");
		throw;
	}
}

bool TrainManager::hasEnoughResources(UnitType unitType)
{
	// ÇöÀç ÇÁ·¹ÀÓÀ» ±âÁØÀ¸·Î BuildQueue ¿¡ Ãß°¡µÇ¾î ¿¹¾àµÈ ÀÚ¿ø°ú º´·Â »ý»ê¿¡ »ç¿ëµÈ ÀÚ¿øÀ» Á¦¿ÜÇÑ »ç¿ë °¡´ÉÇÑ ÀÚ¿ø°ú ºñ±³
	return (unitType.mineralPrice() <= getAvailableMinerals()) && (unitType.gasPrice() <= getAvailableGas());
}

void TrainManager::addReserveResources(UnitType unitType)
{
	reservedMinerals += unitType.mineralPrice();
	reservedGas += unitType.gasPrice();
}

int TrainManager::getAvailableMinerals()
{
	return BM.getAvailableMinerals() - BM.getFirstItemMineral() - reservedMinerals;
}

int TrainManager::getAvailableGas()
{
	return BM.getAvailableGas() - BM.getFirstItemGas() - reservedGas;
}

const Base *TrainManager::getEscapeBase(UnitInfo *commandCenter) {

	const Base *base = nullptr;

	bool mainCommandExist = false;
	bool firstExpansionCommandExist = false;

	for (auto c : INFO.getBuildings(Terran_Command_Center, S))
	{
		if (c->id() == commandCenter->unit()->getID())
			continue;

		if (INFO.getMainBaseLocation(S) && c->unit()->getDistance(INFO.getMainBaseLocation(S)->getPosition()) < 5 * TILE_SIZE)
			mainCommandExist = true;
		else if (INFO.getFirstExpansionLocation(S) && c->unit()->getDistance(INFO.getFirstExpansionLocation(S)->getPosition()) < 5 * TILE_SIZE)
			firstExpansionCommandExist = true;
	}

	if (!mainCommandExist)
	{
		if (INFO.getMainBaseLocation(S) && INFO.isBaseSafe(INFO.getMainBaseLocation(S)) && INFO.isBaseHasResourses(INFO.getMainBaseLocation(S), 0, 0))
			base = INFO.getMainBaseLocation(S);
	}
	else if (!firstExpansionCommandExist)
	{
		if (INFO.getFirstExpansionLocation(S) && INFO.isBaseSafe(INFO.getFirstExpansionLocation(S)) && INFO.isBaseHasResourses(INFO.getFirstExpansionLocation(S), 0, 0))
			base = INFO.getFirstExpansionLocation(S);
	}
	else {
		base = INFO.getClosestSafeEmptyBase(commandCenter);
	}

	return base;
}

void TrainManager::commandCenterTraining()
{
	uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

	if (commandCenterList.empty())
		return;

	// 9 Drone Ó¦¶Ô¼¼ÇÉ
	if (INFO.enemyRace == Races::Zerg && EIB <= Zerg_9_Drone &&
				INFO.getTypeUnitsInRadius(Zerg_Zergling, E, INFO.getMainBaseLocation(S)->getPosition(), 10 * TILE_SIZE).size())
		{
			return;
	}

	// ÃÊ¹Ý ÀÏ²Û·¯½¬ »ó´ë·Î ¸¶¸° 1±â »ý»êÇÏ±â À§ÇØ ÀÏ²Û »ý»ê Áß´Ü
	if (needStopTrainToBarracks())
		return;

	if (INFO.enemyRace == Races::Protoss)
	{
		if (needStopTrainToFactory())
		{
			return;
		}
	}

	int curScvCnt = INFO.getAllCount(Terran_SCV, S);
	int maxScvNeedCount = getMaxScvNeedCount();

	// °øÁß¿¡ ¶°ÀÖ´Â Ä¿¸Çµå°¡ ÀÖ´Â°¡
	bool isExistLiftAndMoveCommandCenter = false;

	for (auto c : commandCenterList)
	{
		if (c->getState() == "LiftAndMove")
		{
			isExistLiftAndMoveCommandCenter = true;
			break;
		}
	}


	for (auto c : commandCenterList)
	{
		if (!c->isComplete())
			continue;

		string state = c->getState();

		// ¿ì¸® º»Áø Ä¿¸Çµå¼¾ÅÍ°¡ ¾Æ´Ï¶ó¸é À§±â»óÈ² ÆÇ´ÜÇÏ¿© ¶ç¿î´Ù
		if (state != "LiftAndMove") 
		{
			// ÇöÀç º£ÀÌ½º¸¦ À¯ÁöÇÒ »óÈ²ÀÌ ¾ÈµÈ´Ù¸é
			if (c->isComplete() && isTimeToMoveCommandCenter(c))
			{
				// ³»°¡ ÀÖ´ø À§Ä¡°¡ Ãß°¡ ¸ÖÆ¼ Áö¿ªÀÌ¾úÀ¸¸é ³» Ãß°¡ ¸ÖÆ¼ ¸®½ºÆ®¿¡¼­ Á¤º¸ »èÁ¦
				Position pos = c->pos();

				auto targetBase = find_if(INFO.getAdditionalExpansions().begin(), INFO.getAdditionalExpansions().end(), [pos](Base * base) {
					return pos == base->getPosition();
				});

				if (targetBase != INFO.getAdditionalExpansions().end())
					INFO.removeAdditionalExpansionData(c->unit());

				const Base *b = getEscapeBase(c);

				if (b) {
					// addon°Ç¼³ÁßÀÌ°Å³ª À¯´Ö ÈÆ·ÃÁßÀÌ¸é Ãë¼ÒÇÑ´Ù
					if (c->unit()->canCancelAddon())
						c->unit()->cancelAddon();

					for (word i = 0, size = c->unit()->getTrainingQueue().size(); i < size; i++) {
						if (c->unit()->canCancelTrain())
							c->unit()->cancelTrain();
					}

					// ¾ÈÀüÇÑ °÷À¸·Î ³¯¶ó°£´Ù
					c->setState(new CommandCenterLiftAndMoveState(getEscapeBase(c)->getTilePosition()));
					state = c->getState();
				}
			}
		}

		if (state == "New" || state == "Idle")
		{
			// ¾Õ¸¶´ç ¾ð´ö¿¡¼­ ÁöÀº Ä¿¸ÇµåÀÎÁö ÆÇ´Ü
			if (c->isComplete() && c->unit()->getDistance(MYBASE) > 5 * TILE_SIZE && isSameArea(c->pos(), MYBASE)
					&& INFO.getCompletedCount(Terran_Command_Center, S) == 2) {


				if (INFO.getFirstExpansionLocation(S) && INFO.isTimeToMoveCommandCenterToFirstExpansion()) {
					c->setState(new CommandCenterLiftAndMoveState(INFO.getFirstExpansionLocation(S)->getTilePosition()));
					c->action();
				}
				else // ÀÏ´Ü ¶ç¿î´Ù.
				{
					if (!c->getLift())
						c->unit()->lift();
				}

				continue;
			}

			UnitType trainUnit = Terran_SCV;

			// ÀÚ¿ø ºÎÁ·
			if (!hasEnoughResources(trainUnit))
				continue;

			// ÀûÀÌ ³» º»Áø ±ÙÃ³¿¡ ÀÖ°í Factory°¡ ¾Èµ¹°í ÀÖÀ¸¸é SCV¸¦ »ÌÁö ¾ÊÀ½...
			if (waitToProduce && INFO.enemyInMyArea().size())
				continue;

			// ÄÄ»û½ºÅ×ÀÌ¼Ç ÁöÀ»¼ö ÀÖÀ¸¸é »óÅÂº¯°æ
			else if (INFO.getCompletedCount(Terran_Academy, S) >= 1 && c->unit()->getAddon() == nullptr && c->unit()->canBuildAddon()
					 && hasEnoughResources(Terran_Comsat_Station) && isSafeComsatPosition(c)) {
				c->setState(new CommandCenterBuildAddonState());
				addReserveResources(Terran_Comsat_Station);
			}
			// SCV ¸¸µé ¼ö ÀÖÀ¸¸é »Ì±â
			else if (curScvCnt <= maxScvNeedCount && c->unit()->canTrain())
			{
				if (INFO.getCompletedCount(Terran_Command_Center, S) > 2
						&& ScvManager::Instance().depotHasEnoughMineralWorkers(c->unit())) // ¾Õ¸¶´ç ÀÌ»óÀÏ ¶§
					continue;

				if (!SM.checkTurretFirst()) {
					c->setState(new CommandCenterTrainState(1));
					addReserveResources(trainUnit);
				}
			}
			// Ä¿¸Çµå¼¾ÅÍ ±ÙÃ³¿¡ ÀÖ´Â ÀÚ¿øÀÌ ´Ù ¶³¾îÁö¸é Ä¿¸Çµå¼¾ÅÍ ¿Å±ä´Ù.
			else {
				if (!INFO.isBaseHasResourses(INFO.getNearestBaseLocation(c->pos()), 0, 0) && INFO.getActivationMineralBaseCount() < 1
						&& !isExistLiftAndMoveCommandCenter && TIME % 24 * 30 == 0)
				{
					Base *multiBase = nullptr;

					for (int i = 1; i < 4; i++)
					{
						multiBase = INFO.getFirstMulti(S, false, false, i);

						if (multiBase != nullptr)
							break;
					}

					// ¸ÖÆ¼ ÇÒ ¼ö ÀÖ´Â À§Ä¡ ÀÖÀ¸¸é
					if (multiBase) {
						isExistLiftAndMoveCommandCenter = true;
						c->setState(new CommandCenterLiftAndMoveState(multiBase->getTilePosition()));
					}
				}
			}
		}

		c->action();
	}
}
//===============================================================================
void TrainManager::barracksTraining()
{

	//if (waitToProduce)
	//	return;

	uList barracksList = INFO.getBuildings(Terran_Barracks, S);

	if (barracksList.empty())
		return;

	int marineCount = INFO.getAllCount(Terran_Marine, S);
	int medicCount = INFO.getAllCount(Terran_Medic, S);
	int liftThreshold = 0;
	int basemarineCount = 2 ;
	int maxmarineCount = 0;
	int basemedicCount = 1;
	int maxmedicCount = 0;
	//================================
	if (INFO.enemyRace == Races::Terran)
	{
		if (INFO.getCompletedCount(Terran_Factory, S) == 0)
		{
			basemarineCount = 3;
		}
		if (INFO.getCompletedCount(Terran_Factory, S) != 0)
		{
			if (INFO.getCompletedCount(Terran_Machine_Shop, S) == 0)
			{
				basemarineCount = 4;
			}
			else
			{
				if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
				{
					if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 1 &&
						INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) <= 4)
					{
						basemarineCount = 10;
					}
					else if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 4)
					{
						basemarineCount = 40;
					}
				}
				else
				{
					if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 1 &&
						INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) <= 4)
					{
						basemarineCount = 3;
					}
					else if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 4)
					{
						basemarineCount = 6;
					}

				}
			}

		}
	}
	else
	{
		if (INFO.enemyRace == Races::Protoss)
		{
			if (SM.getMyBuild() == MyBuildTypes::Protoss_DragoonKiller || SM.getMyBuild() == MyBuildTypes::Protoss_CannonKiller)
			{
				if (INFO.getCompletedCount(Terran_Factory, S) == 0)
				{
					basemarineCount = 2;
				}
				if (INFO.getCompletedCount(Terran_Factory, S) != 0)
				{
					if (INFO.getCompletedCount(Terran_Machine_Shop, S) == 0 || INFO.getAllCount(Terran_Command_Center, S) < 2)
					{
						basemarineCount = 3;
					}
					else
					{
						if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 1 &&
							INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) <= 4)
						{
							basemarineCount = 6;
						}
						else if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 4)
						{
							basemarineCount = 8;
						}
						if (S->minerals() >= 900)
						{
							basemarineCount = 20;
						}
					}
				}
			}
			if (SM.getMyBuild() == MyBuildTypes::Protoss_TemplarKiller)
			{
				if (INFO.getAllCount(Terran_Factory, S) == 0)
				{
					basemarineCount = 2;
				}
				if (INFO.getCompletedCount(Terran_Factory, S) == 0)
				{
					basemarineCount = 3;
				}
				if (INFO.getCompletedCount(Terran_Factory, S) != 0)
				{
					if (INFO.getCompletedCount(Terran_Machine_Shop, S) == 0 || INFO.getAllCount(Terran_Command_Center, S) < 2)
					{
						basemarineCount = 4;
					}
					else
					{
						if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 1 &&
							INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) <4)
						{
							basemarineCount = 6;
						}
						else if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 4)
						{
							basemarineCount = 9;
						}
						if (S->minerals() >= 900)
						{
							basemarineCount = 30;
						}
					}
				}
			}
			if (SM.getMyBuild() == MyBuildTypes::Protoss_CarrierKiller)
			{
				if (INFO.getAllCount(Terran_Factory, S) == 0)
				{
					basemarineCount = 2;
				}
				if (INFO.getCompletedCount(Terran_Factory, S) == 0)
				{
					basemarineCount = 3;
				}
				if (INFO.getCompletedCount(Terran_Factory, S) != 0)
				{
					if (INFO.getCompletedCount(Terran_Machine_Shop, S) == 0 || INFO.getAllCount(Terran_Command_Center, S) < 2)
					{
						basemarineCount = 4;
					}
					else
					{
						if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 1 &&
							INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) <4)
						{
							basemarineCount = 6;
						}
						else if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 4)
						{
							basemarineCount = 12;
						}
						if (S->minerals() >= 900)
						{
							basemarineCount = 30;
						}
					}
				}
			}

			if (SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller)
			{
				if (INFO.getAllCount(Terran_Factory,S)<1)
					basemarineCount = 3;

				else if (INFO.getCompletedCount(Terran_Factory, S)<1)
					basemarineCount = 8;

				else if (INFO.getCompletedCount(Terran_Command_Center, S)<2)
					basemarineCount = 10;
				else 
					basemarineCount = 12;

				if (S->minerals() >= 900 || SM.getMainStrategy() == AttackAll)
				{
					basemarineCount = 30;
				}
			

			}
			if (SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller)
			{
				if ((INFO.getDestroyedCount(Protoss_Nexus,E)<1))
				basemarineCount = 30;
				else if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode,S)<2)
					basemarineCount = 4;

				else if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S)>=2)
					basemarineCount = 15;

				if (S->minerals() >=800)
				basemarineCount = INT_MAX;
			}


			if (SM.getMyBuild() == MyBuildTypes::Protoss_DragoonKiller || SM.getMyBuild() == MyBuildTypes::Protoss_CannonKiller)
			{
				if (marineCount >= 7)
					basemedicCount = 1;

				if (marineCount >= 12)
					basemedicCount = 2;

				if (marineCount >= 20)
					basemedicCount = 3;

			}

			else if (SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller)
			{
				if (marineCount >= 7)
					basemedicCount = 2;

				if (marineCount >= 12)
					basemedicCount = 3;

				if (marineCount >= 20)
					basemedicCount = 5;

			}

			else if (SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller)
			{
				if (marineCount >= 8)
					basemedicCount = 2;

				if (marineCount >= 14)
					basemedicCount = 3;

				if (marineCount >= 20)
					basemedicCount = 4;

			}
			else if (SM.getMyBuild() == MyBuildTypes::Protoss_TemplarKiller)
			{
				if (marineCount >= 6)
					basemedicCount = 2;

				if (marineCount >= 12)
					basemedicCount = 3;


			}
		

		}
		else
		{
			//========================¹¤³§»¹Ã»½¨Á¢ÆðÀ´µÄÇé¿ö========================
			if (INFO.getCompletedCount(Terran_Factory, S) == 0)
			{
				basemarineCount = 3;
			}
			//============================¹¤³§½¨Á¢ÆðÀ´µÄÇé¿ö=========================
			if (INFO.getCompletedCount(Terran_Factory, S) != 0)
			{
				//»úÐµÉÌµêÎ´½¨Á¢
				if (INFO.getCompletedCount(Terran_Machine_Shop, S) == 0)
				{
					basemarineCount = 4;
				}
				//»úÐµÉÌµê½¨Á¢
				else
				{
					if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 1 &&
						INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) <= 4)
					{
						basemarineCount = 6;
					}
					else if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 4)
					{
						basemarineCount = 30;
					}
				}


			}
		}
	}

	if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
	{
		liftThreshold = 6;
		if ((INFO.getUnits(Terran_Vulture, S).size() + INFO.getUnits(Terran_Goliath, S).size()
			+ INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S).size() > 4) && INFO.enemyRace == Races::Terran)
			liftThreshold = 0;
	}
	
	for (auto b : barracksList)
	{
		string state = b->getState();

		
		if (state == "New" && b->isComplete())
		{
			b->setState(new BarrackIdleState());
		}

		
		if (state == "Idle")
		{
			UnitType trainUnit = Terran_Marine;

			
			if (!hasEnoughResources(trainUnit))
				continue;

			
		//	if (INFO.enemyRace == Races::Protoss &&
		//			(EIB != UnknownBuild && EIB >= Toss_pure_double))
		//	{
		//		if (marineCount && INFO.getAllCount(Terran_Command_Center, S) < 2)
		//			continue;
		//	}

			if (marineCount < basemarineCount)
			{
				if (needStopTrainToFactory() && SM.getMyBuild() != MyBuildTypes::Protoss_MineKiller)
				{
					continue;
				}
				if (INFO.enemyRace == Races::Protoss)
				{
					if (((marineCount >= 7 && medicCount < basemedicCount)|| (marineCount >= 12 && medicCount < basemedicCount )||( marineCount >= 20 && medicCount < basemedicCount ))&& INFO.getCompletedCount(Terran_Academy, S) >= 1)
					{
						UnitType trainUnit = Terran_Medic;
						b->setState(new BarrackTrainState());
						b->action(trainUnit);
						addReserveResources(trainUnit);
						continue;
					}
				}
				else
				{
					if (marineCount >= 5 && medicCount < 1)
					{
						UnitType trainUnit = Terran_Medic;
						b->setState(new BarrackTrainState());
						b->action(trainUnit);
						addReserveResources(trainUnit);
						continue;
					}
				}
				b->setState(new BarrackTrainState());
				b->action(trainUnit);
				addReserveResources(trainUnit);
				continue;
			}
			if (marineCount >= liftThreshold && INFO.enemyRace == Races::Terran&&SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
			{

				//if (EIB == Terran_1fac_1star || EIB == Terran_1fac_2star || EIB == Terran_2fac_1star || EMB == Terran_1fac_1star || EMB == Terran_1fac_2star || EMB == Terran_2fac_1star || EMB == Terran_2fac)

				b->setState(new BarrackLiftAndMoveState());

			}
		}

		
		if (state == "Barricade")
		{
			

			if (!b->unit()->isLifted()/* && INFO.getUnitsInRadius(E, b->pos(), 10 * TILE_SIZE, true).size() <= 2*/) {
				if (marineCount < liftThreshold) {
					b->setState(new BarrackIdleState());
				}
			}
		}

		b->action();
	}
}

int TrainManager::getBaseVultureCount(InitialBuildType eib, MainBuildType emb)
{
	if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
		{
			if (S->getUpgradeLevel(UpgradeTypes::Ion_Thrusters) && S->hasResearched(TechTypes::Spider_Mines))
				return S->supplyUsed() / 100 + 3;
			else if (EnemyStrategyManager::Instance().getEnemyInitialBuild() == Terran_2b_forward)
				return 3;
			else
				return 1;
		}
		else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
			return 3;
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		if (eib <= Zerg_9_Balup || emb == Zerg_main_zergling) {
			if (E->getUpgradeLevel(UpgradeTypes::Metabolic_Boost) || E->isUpgrading(UpgradeTypes::Metabolic_Boost))
				return 3;
			// ¹úÃÄ¸¦ ÃÖ¼Ò 2¸¶¸® ~ 4¸¶¸® ±îÁö Àû º´·Â¿¡ ¸ÂÃç¼­ »Ì´Â´Ù.
			else if (INFO.getCompletedCount(Zerg_Zergling, E) > INFO.getAllCount(Terran_Vulture, S) * 4)
				return 3;
		}


		return 2;
	}
	else
	{
		if (SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller)
			return 4;
		else
			return 0;
	}
}

int TrainManager::getMaxVultureCount() 
{
	if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
		{
			return min((getAvailableMinerals() / 500) * 5 + 1, 6);
		}
	}

	return 20;
}

void TrainManager::factoryTraining()
{
	uList factoryList = INFO.getBuildings(Terran_Factory, S);

	if (factoryList.empty())
		return;

	int machineShopCount = INFO.getAllCount(Terran_Machine_Shop, S);
	int vultureAllCount = INFO.getAllCount(Terran_Vulture, S) + INFO.getDestroyedCount(Terran_Vulture, S);
	int vultureCount = INFO.getAllCount(Terran_Vulture, S);
	int tankCount = INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S);
	int goliathCount = INFO.getAllCount(Terran_Goliath, S);
	int baseVultureCount = getBaseVultureCount(EIB, EMB);
	int baseTankCount = 0;
	int baseGoliathCount = 0;
	int maxVultureCount = getMaxVultureCount();
	int maxTankCount = 30;
	int maxGoliathCount = INT_MAX;
	int mashineShopCount = 0;
	int currentFrameSetAddOnCount = 0;

	// Tank 
	if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
			baseTankCount = 2;
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		if (EMB == Zerg_main_lurker)
			baseTankCount = 0;
		else if (EMB == Zerg_main_hydra_mutal)
			baseTankCount = 3;
		else if (EMB == Zerg_main_hydra)
			baseTankCount = 5;
	}
	else
	{
		
			baseTankCount = 3;//4Ì¹¿ËÊÇ»ù´¡
		
	}

	// Goliath 
	if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
		{
			if (tankCount < 8)
				baseGoliathCount = 4;
			else
			{
				double ratio = 0.3;

				if (INFO.getAllCount(Terran_Wraith, E) + INFO.getAllCount(Terran_Battlecruiser, E))
					ratio = 0.6;

				baseGoliathCount = (int)(tankCount * ratio + 4);
			}

			// ¹èÆ²Å©·çÀú ³ª¿ÔÀ» ¶§ °ñ¸®¾Ñ Á¶±Ý ´õ Ãß°¡ »ý»ê
			baseGoliathCount += INFO.getCompletedCount(Terran_Battlecruiser, E);
			maxGoliathCount = tankCount + INFO.getCompletedCount(Terran_Battlecruiser, E);
		}
		else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
		{
			if (INFO.getCompletedCount(Terran_Valkyrie, E) > 0 || INFO.getCompletedCount(Terran_Goliath, E) > 2)
				baseGoliathCount = 2;
			else
				baseGoliathCount = 0;
		}
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		baseGoliathCount = S->supplyUsed() < 200 ? 6 : 12;

		if (EMB == Zerg_main_queen_hydra)
			baseGoliathCount += INFO.getCompletedCount(Zerg_Mutalisk, E) / 2;
	}
	else
	{
		if (EMB == Toss_arbiter)
			baseGoliathCount = INFO.getCompletedCount(Protoss_Arbiter, E);
		else if (EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier)
		{
			int maxGcnt = EMB == Toss_fast_carrier ? 10 : 2;
			int carrierCnt = INFO.getCompletedCount(Protoss_Carrier, E);
			baseGoliathCount = max(maxGcnt, carrierCnt * (int)(S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) >= 2 ? 3.5 : 5));

			if (INFO.getCompletedCount(Protoss_Arbiter, E) > 0)
			{
				baseGoliathCount = max(baseGoliathCount, 2 + INFO.getCompletedCount(Protoss_Arbiter, E));
			}

		}
		//else if (EMB == Toss_dark_drop || EMB == Toss_drop)
		//	baseGoliathCount = baseGoliathCount > 2 ? baseGoliathCount : 2;

		else if (INFO.getAllCount(Protoss_Scout, E) != 0) {
			baseGoliathCount = baseGoliathCount > INFO.getAllCount(Protoss_Scout, E) * 2 ? baseGoliathCount : INFO.getAllCount(Protoss_Scout, E) * 2;
		}
		else
		{
			baseGoliathCount =0;
		}
	}

	//mashineShopCount	
	if (INFO.enemyRace == Races::Zerg)
	{
		mashineShopCount = 1;

		if (EMB == Zerg_main_queen_hydra || (INFO.getAllCount(Zerg_Defiler, E) + INFO.getAllCount(Zerg_Defiler_Mound, E)))
			mashineShopCount = 2;
	}
	else if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
		{
			mashineShopCount = 1;

			if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) >= 2 && INFO.getAllCount(Terran_Goliath, S) >= 4)
				mashineShopCount = 2;


			if (INFO.getAllCount(Terran_Command_Center, S) == 2 && INFO.getAllCount(Terran_Machine_Shop, E) >= 3)
				mashineShopCount = 3;
		}
		else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
		{
			if (!SM.getNeedTank())
				mashineShopCount = 1;
			else
				mashineShopCount = 3;
		}
	}
	else
	{
		if (EMB == Toss_fast_carrier)
		{
			mashineShopCount = 1;
		}
		else
		{
			if (INFO.getCompletedCount(Terran_Factory, S) > 2 )
				mashineShopCount = 3;
			else
				mashineShopCount = 1;
		}
	}


	if (INFO.getActivationGasBaseCount() >= 3)
	{
		if (INFO.enemyRace == Races::Zerg)
		{
			if (INFO.getAllCount(Zerg_Defiler, E) + INFO.getAllCount(Zerg_Defiler_Mound, E))
			{
				mashineShopCount += 1;
			}
		}
		else if (INFO.enemyRace == Races::Terran)
		{
			mashineShopCount = min(4, INFO.getActivationGasBaseCount());
		}
		else
			mashineShopCount += 1;
	}

	// Factory with Machinshop 
	int addonIndex = 0;

	for (auto f : factoryList)
	{
		if (f->unit()->getAddon())
		{
			string state = f->getState();


			if (state == "New" && f->isComplete())
			{
				f->setState(new FactoryIdleState());
			}


			if (state == "Idle")
			{
				UnitType trainUnit = Terran_Vulture;


				if (!hasEnoughResources(trainUnit))
				{
					waitToProduce = true;
					return;
				}


				if (vultureAllCount >= baseVultureCount )
				{

					if (INFO.enemyRace == Races::Zerg)
					{
						if (EMB == Zerg_main_lurker)
						{
							if (S->hasResearched(TechTypes::Spider_Mines) == false)
								trainUnit = Terran_Vulture;
							else
								trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : Terran_Vulture;
						}
						else if (EMB == Zerg_main_hydra || EMB == Zerg_main_hydra_mutal || EMB == Zerg_main_queen_hydra)
						{
							trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : Terran_Vulture;
						}
						else
						{
							if (INFO.getCompletedCount(Terran_Goliath, S) >= 8)
							{
								trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : Terran_Vulture;
							}
							else
							{
								if (INFO.getCompletedCount(Terran_Armory, S) == 0)
								{
									trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
								}
								else
								{
									trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : None;

									if (trainUnit == None)
									{
										// °¡½º ºÎÁ·À¸·Î °ñ¸®¾Ñ ¸ø»ÌÀ» °æ¿ì ¹Ì³×¶ö Ã¼Å©ÇÏ°í ¹úÃÄ »ý»ê
										trainUnit = getAvailableGas() >= Terran_Goliath.gasPrice() ? None : hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
									}
								}
							}
						}

						if (baseTankCount > tankCount)
						{
							if (baseGoliathCount > goliathCount
								&& (EMB == Zerg_main_mutal || EMB == Zerg_main_hydra_mutal))
								trainUnit = Terran_Goliath;
							else
								trainUnit = Terran_Siege_Tank_Tank_Mode;
						}
						else
						{
							if (INFO.getCompletedCount(Terran_Armory, S) && baseGoliathCount > goliathCount)
								trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath :
								(getAvailableGas() >= Terran_Goliath.gasPrice() ? None :
								hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None);
						}

						// °¡½º ¼¼ÀÌºêÇÏ±â À§ÇØ ¹úÃÄ »ý»ê
						if (saveGas)
							trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
					}
					else if (INFO.enemyRace == Races::Terran)
					{
						if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
						{
							if (INFO.getCompletedCount(Terran_Armory, S) > 0 && baseGoliathCount > goliathCount && baseTankCount <= tankCount)
							{
								trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : None;
							}
							else
							{
								trainUnit = hasEnoughResources(Terran_Siege_Tank_Tank_Mode) ? Terran_Siege_Tank_Tank_Mode : None;
							}
						}
						else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
						{
							if (SM.getNeedTank())
							{
								trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : Terran_Vulture;

								// Tank ¸¸µé Gas´Â ÀÖÀ¸³ª MineralÀÌ ºÎÁ·ÇÑ °æ¿ì Tank »ý»êÀ» À§ÇØ ÀÌÇÏ Factory¿¡¼­ Vulture »ý»êÀ» Áß´ÜÇÑ´Ù.
								if (trainUnit == Terran_Siege_Tank_Tank_Mode && getAvailableMinerals() < Terran_Siege_Tank_Tank_Mode.mineralPrice())
									return;
							}
							else
							{
								trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;;
							}
						}
					}
					else if (INFO.enemyRace == Races::Protoss)
					{
						if (baseTankCount > tankCount)
						{
							trainUnit = Terran_Siege_Tank_Tank_Mode;
							if (SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller)
							{
								if (INFO.getCompletedCount(Terran_Marine,S)<3)
									trainUnit = None;
							}
						}
						else
						{
							if (getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice())
							{
								trainUnit = Terran_Siege_Tank_Tank_Mode;
							}
							else if (getAvailableMinerals() > 250 )
								trainUnit = Terran_Vulture;
				
							else
							{
								trainUnit = None;
							}
							
						}
					}
				}


				if (trainUnit == Terran_Siege_Tank_Tank_Mode && maxTankCount <= INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S))
					trainUnit = Terran_Goliath;

				f->setState(new FactoryTrainState());
				f->action(trainUnit);
				addReserveResources(trainUnit);
				continue;
			}
			else if (state == "Train")
			{
				// º´·Â »ý»ê Á¾·áµÇ¸é Idle·Î ¹Ù²ñ
				f->action();
			}
			else if (state == "BuildAddon")
			{
				f->action();
			}
			else
			{

			}

			addonIndex++;

		}

	}

	// AddOn 
	for (auto f : factoryList)
	{
		string state = f->getState();

		if (state == "BuildAddon")
		{
			currentFrameSetAddOnCount++;
		}
	}

	for (auto f : factoryList)
	{
		if (f->unit()->getAddon() == nullptr)
		{
			string state = f->getState();

			if (state == "New" && f->isComplete())
			{
				f->setState(new FactoryIdleState());
			}


			if (state == "LiftAndLand")
			{
				if (f->unit()->isLifted() || !f->unit()->canLift())
				{
					f->action(moveFirstFactoryPos);
					continue;
				}
				else
				{
					f->setState(new FactoryIdleState());
					cout << "to idle f->unit()->getTilePosition() : " << f->unit()->getTilePosition() << endl;
				}
			}


			if (state == "Idle")
			{
				UnitType trainUnit = Terran_Vulture;

				if (!hasEnoughResources(trainUnit))
				{
					waitToProduce = true;
					return;
				}

				if (vultureAllCount >= baseVultureCount )
				{
					if (machineShopCount + currentFrameSetAddOnCount < mashineShopCount)
					{
						bool needMachineShop = true;


						if (INFO.enemyRace == Races::Zerg)
						{
							if (EIB <= Zerg_9_Balup || EMB == Zerg_main_zergling)
							{
								if (INFO.getCompletedCount(Terran_Vulture, S) < baseVultureCount && INFO.getDestroyedCount(Zerg_Zergling, E) < 10)
									needMachineShop = false;
							}
						}
						else if (INFO.enemyRace == Races::Protoss)
						{
							if (EIB == Toss_1g_forward
								|| EIB == Toss_2g_zealot
								|| EIB == Toss_2g_forward)
							{
								if (INFO.getCompletedCount(Terran_Vulture, S) < baseVultureCount && INFO.getAllCount(Protoss_Cybernetics_Core, E) == 0)
									needMachineShop = false;
							}
						}

						if (f->unit()->getAddon() == nullptr && hasEnoughResources(Terran_Machine_Shop) && needMachineShop)
						{
							if (!bw->canBuildHere(f->unit()->getTilePosition(), Terran_Machine_Shop, f->unit())
								&& !bw->getUnitsInRectangle((Position)(f->unit()->getTilePosition() + TilePosition(4, 1)), (Position)(f->unit()->getTilePosition() + TilePosition(6, 3)), Filter::IsBuilding).empty())
							{
								if (isFirstFactory())
								{
									if (f->getState() != "LiftAndLand")
									{
										findAndSaveFirstFactoryPos(f->unit());
										f->setState(new FactoryLiftAndLandState());
										f->action(moveFirstFactoryPos);
									}

									continue;
								}
							}
							else
							{
								f->setState(new FactoryBuildAddonState());
								currentFrameSetAddOnCount++;
								f->action();
								addReserveResources(Terran_Machine_Shop);
								continue;
							}
						}
					}


					if (INFO.enemyRace == Races::Zerg)
					{
						if (INFO.getCompletedCount(Terran_Armory, S) == 0)
						{
							trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
						}
						else
						{
							if (EMB == Zerg_main_queen_hydra)
							{
								if (goliathCount <= baseGoliathCount)
									trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : Terran_Vulture;
								else
									trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
							}
							else
							{
								trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : None;

								if (trainUnit == None)
								{
									// °¡½º ºÎÁ·À¸·Î °ñ¸®¾Ñ ¸ø»ÌÀ» °æ¿ì ¹Ì³×¶ö Ã¼Å©ÇÏ°í ¹úÃÄ »ý»ê
									trainUnit = getAvailableGas() >= Terran_Goliath.gasPrice() ? None : hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
								}
							}
						}

						// °¡½º ¼¼ÀÌºêÇÏ±â À§ÇØ ¹úÃÄ »ý»ê
						if (saveGas)
							trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
					}
					else if (INFO.enemyRace == Races::Terran)
					{
						if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
						{
							if (S->hasResearched(TechTypes::Spider_Mines))
							{

								if (baseVultureCount > vultureCount)
								{
									trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
								}
								else
								{
									if (getAvailableGas() < 600)
										//if (getAvailableGas() < 200)
										trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
									else
										trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
								}
							}
							else
							{
								trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : None;
							}

							if (saveGas)
								trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
						}
						else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
						{
							if (!SM.getNeedTank())
							{
								trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
							}
							else
							{
								trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
							}
						}
					}
					else
					{

						if (goliathCount < baseGoliathCount && INFO.getCompletedCount(Terran_Armory, S) > 0)
						{
							trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : None;


							if (trainUnit == None)
								trainUnit = getAvailableGas() >= Terran_Goliath.gasPrice() ? None : hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
						}
						else
						{

							trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
						}
					}
				}

				if (INFO.enemyRace == Races::Terran) {

					if (trainUnit == Terran_Vulture && maxVultureCount <= INFO.getAllCount(Terran_Vulture, S)
						|| (S->hasResearched(TechTypes::Spider_Mines) && S->isUpgrading(UpgradeTypes::Ion_Thrusters) && maxVultureCount >= tankCount + goliathCount))
						continue;

					else if (trainUnit == Terran_Goliath && maxGoliathCount <= INFO.getAllCount(Terran_Goliath, S)
						&& S->getUpgradeLevel(UpgradeTypes::Charon_Boosters))
						continue;
				}

				f->setState(new FactoryTrainState());
				f->action(trainUnit);
				addReserveResources(trainUnit);
				continue;
			}
			else if (state == "Train")
			{

				f->action();
			}
			else if (state == "BuildAddon")
			{
				f->action();
			}
			else
			{

			}
		}
	}
}

void TrainManager::starportTraining()
{
	uList starportList = INFO.getBuildings(Terran_Starport, S);
	uList controlTowerList = INFO.getBuildings(Terran_Control_Tower, S);
	int wraithCount = INFO.getAllCount(Terran_Wraith, S);
	int dropshipCount = INFO.getAllCount(Terran_Dropship, S);
	int vesselCount = INFO.getAllCount(Terran_Science_Vessel, S);
	int ValkyrieCount = INFO.getAllCount(Terran_Valkyrie, S);
	int maxVesselCount = 0;
	int baseWraithCount = 0;
	if (SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller)
		baseWraithCount = 1;
	int baseValkyrieCount = 0;
	int en_CarrierCount = INFO.getAllCount(Protoss_Carrier, E);
	int en_wraithCount = INFO.getAllCount(Terran_Wraith, E);

	int maxWraithCount = 0;
	int maxValkyrieCount = 0;
	int maxDropshipCount = 0;
	int needConrolTowerCount = 0;
	



	
	int en_valkyrieCount = INFO.getAllCount(Terran_Valkyrie, E);
	if (starportList.empty())
		return;
//¿ÆÑ§´¬ÊýÁ¿ÉèÖÃ=======================================================
	if (INFO.enemyRace == Races::Zerg)
	{
		maxVesselCount = 3;
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		if (SM.getMyBuild() == MyBuildTypes::Protoss_TemplarKiller && TIME<8.4*24*60)
				maxVesselCount = 1;

		else if (SM.getMyBuild() == MyBuildTypes::Protoss_CarrierKiller && EMB==Toss_fast_carrier)
			maxVesselCount = 0;


		else maxVesselCount = 2;

	}
	else
	{
		maxVesselCount = 1;
	}
//ÉèÖÃ=======================================================
	if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
		{
			needConrolTowerCount = 1;
			maxDropshipCount = 3; //Dropship TEST
		}
		else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
		{
			if (INFO.getCompletedCount(Terran_Valkyrie, E) > 0 || INFO.getCompletedCount(Terran_Goliath, E) > 2)
			{
				baseWraithCount = 2;
				baseValkyrieCount = 2;
				maxValkyrieCount =2;
				maxWraithCount =5;
				needConrolTowerCount = 1;
			}
			else
			{
				// wraith »ý»ê Threshold
				baseWraithCount = 1;
				baseValkyrieCount = 0;
				maxValkyrieCount = 2;
				// wraith ÃÖ´ë »ý»ê
				maxWraithCount = 3;
				// ÇÊ¿äÇÑ ÄÁÆ®·ÑÅ¸¿ö
				needConrolTowerCount = (en_wraithCount > 0) ? 1 : 0;
			}
		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		
  maxWraithCount = 0;
  needConrolTowerCount = 1;	
  baseWraithCount = 0;
		if (SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller)
			baseWraithCount = 1;
		
		   if (en_CarrierCount >= 2)
			   baseValkyrieCount = 2;

		   if (SM.getMyBuild() == MyBuildTypes::Protoss_CarrierKiller && (EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier))
		   {
			   if (en_CarrierCount<2)
				   baseValkyrieCount = 2;
			   else baseValkyrieCount = 3;
		   }

	}

	else
	{
		baseWraithCount = 1;
		needConrolTowerCount = 1;
	}

	if (SM.getMainStrategy() == Eliminate) {
		baseWraithCount = baseWraithCount > 1 ? baseWraithCount : 1;
		maxWraithCount = maxWraithCount > 3 ? maxWraithCount : 3;
	}

	for (auto s : starportList)
	{
		string state = s->getState();

		if (state == "New" && s->isComplete()) {
			s->setState(new StarportIdleState());
		}

		// ÃÖ¼Ò ·¹ÀÌ½º »ý»ê ÈÄ ¸Ó½Å¼¥ Ãß°¡
		if (state == "Idle")
		{
			//if (wraithCount >= maxWraithCount && vesselCount >= maxVesselCount)
			//	continue;

			UnitType trainUnit = UnitTypes::None;

			// ±âº» ·¹ÀÌ½º ¼ö´Â ¸ÂÃá´Ù.
			if (wraithCount < baseWraithCount)
			{
				trainUnit = Terran_Wraith;
			}
			else
			{
				if ((int)controlTowerList.size() < needConrolTowerCount)
				{
					// ÄÁÆ®·ÑÅ¸¿ö Ãß°¡
					if (s->unit()->getAddon() == nullptr && hasEnoughResources(Terran_Control_Tower))
					{
						s->setState(new StarportBuildAddonState());
						s->action();
					}

					continue;
				}

				if (INFO.enemyRace == Races::Terran)
				{
					if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
					{
						if (dropshipCount < maxDropshipCount)
							trainUnit = hasEnoughResources(Terran_Dropship) ? Terran_Dropship : None;
						else
						{
							if (vesselCount < maxVesselCount)
								trainUnit = Terran_Science_Vessel;
						}
					}
				}
				else if (INFO.enemyRace == Races::Protoss)
				{
					if (SM.getMyBuild() == MyBuildTypes::Protoss_CannonKiller &&SM.getMainStrategy()!=AttackAll &&dropshipCount <2)
					trainUnit = hasEnoughResources(Terran_Dropship) ? Terran_Dropship : None;
                     
					else if (vesselCount < maxVesselCount) 
					{
						if (INFO.getCompletedCount(Terran_Science_Facility, S) == 0
								|| INFO.getCompletedCount(Terran_Starport, S) == 0
								|| INFO.getCompletedCount(Terran_Control_Tower, S) == 0)
						{
							continue;
						}

						trainUnit = Terran_Science_Vessel;
					}
					else
					{
						if ((ValkyrieCount < baseValkyrieCount))
						{
							if (INFO.getCompletedCount(Terran_Science_Facility, S) == 0
								|| INFO.getCompletedCount(Terran_Starport, S) == 0
								|| INFO.getCompletedCount(Terran_Control_Tower, S) == 0
								)
							{
								continue;
							}
							trainUnit = Terran_Valkyrie;
						}
					}
				}
				else if(INFO.enemyRace == Races::Zerg)
				{

					if (vesselCount < maxVesselCount) 
					{
						if (INFO.getCompletedCount(Terran_Science_Facility, S) == 0
							|| INFO.getCompletedCount(Terran_Starport, S) == 0
							|| INFO.getCompletedCount(Terran_Control_Tower, S) == 0)
						{
							continue;
						}

						trainUnit = Terran_Science_Vessel;
					}


					else if (wraithCount < 3 )
					{
						trainUnit = hasEnoughResources(Terran_Wraith) ? Terran_Wraith : None;
					}
				}
			}

			// ÀÚ¿ø ºÎÁ·
			if (trainUnit == Terran_Science_Vessel && nextVessleTime < TIME)
			{
				if (!hasEnoughResources(trainUnit) && trainUnit.gasPrice() > getAvailableGas())
				{
					saveGas = true;
					continue;
				}
			}

			// »ÌÀ» À¯´Ö ¾øÀ½
			if (trainUnit == UnitTypes::None)
				continue;

			s->setState(new StarportTrainState());
			s->action(trainUnit);
			addReserveResources(trainUnit);

			if (trainUnit == Terran_Science_Vessel && saveGas)
			{
				saveGas = false;
				nextVessleTime = TIME + (24 * 60 * 2); // º£½½ Àç»ý»êÀº 2ºÐ ÈÄºÎÅÍ
				cout << "## º£½½ Àç»ý»êÀº 2ºÐµÚ¿¡.." << nextVessleTime << endl;
			}

			continue;
		}
		else if (state == "Train")
		{
			// º´·Â »ý»ê Á¾·áµÇ¸é Idle·Î ¹Ù²ñ
			s->action();
		}
		else if (state == "BuildAddon")
		{
			s->action();
		}
		else
		{

		}
	}
}

int TrainManager::getMaxScvNeedCount()
{
	uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);
	int maxScvNeedCount = 0;

	for (auto c : commandCenterList)
	{
		
		maxScvNeedCount += ScvManager::Instance().getDepotMineralSize(c->unit()) * 2;
		maxScvNeedCount += 3; // for Refinery
		maxScvNeedCount += 1; // for Spare(Build, Scout, Repair)

		// Command°¡ ¿Ï¼ºµÇÁö ¾ÊÀº°ÍÀÌ ÀÖ´Ù¸é ¿Ï¼º ½Ã RebalancingÀ» À§ÇØ 10°³ margineÀ» Ãß°¡
		if (!c->isComplete())
		{
			maxScvNeedCount += 10;
			continue;
		}
	}

	return min(maxScvNeedCount, 60);
}
/*
Position TrainManager::getBarrackEnemyChokePosition()
{
	Position targetPos = Positions::None;

	if (INFO.getSecondChokePoint(E) == nullptr)
		targetPos = INFO.getMainBaseLocation(E)->Center();
	else
	{
		const ChokePoint *secondChokePoint = INFO.getSecondChokePoint(E);

		int dist1 = INFO.getMainBaseLocation(E)->Center().getApproxDistance((Position)secondChokePoint->Pos(BWEM::ChokePoint::end1));
		int dist2 = INFO.getMainBaseLocation(E)->Center().getApproxDistance((Position)secondChokePoint->Pos(BWEM::ChokePoint::end2));

		targetPos = dist1 > dist2 ? (Position)secondChokePoint->Pos(BWEM::ChokePoint::end1) : (Position)secondChokePoint->Pos(BWEM::ChokePoint::end2);

		Position tempPos = getCirclePosFromPosByDegree(targetPos, (Position)secondChokePoint->Center(), 180);

		if (!tempPos.isValid())
			tempPos.makeValid();

		if (targetPos.getApproxDistance(tempPos) > 3 * TILE_SIZE)
			targetPos = getMiddlePositionByDist(targetPos, tempPos, 3 * TILE_SIZE);
		else
			targetPos = tempPos;

		if (!targetPos.isValid())
			targetPos.makeValid();
	}

	return targetPos;
}

void TrainManager::setBarrackLiftState(UnitInfo *b)
{
	// °ø°Ý¹ÞÀ¸¸é
	if (b->unit()->isUnderAttack() || b->getVeryFrontEnemyUnit() != nullptr)
	{
		b->setState(new BarrackLiftAndMoveState());

		if (b->getVeryFrontEnemyUnit() != nullptr)
		{
			int safeDist = b->getVeryFrontEnemyUnit()->getType().airWeapon().maxRange() > TILE_SIZE ? b->getVeryFrontEnemyUnit()->getType().airWeapon().maxRange() / TILE_SIZE : TILE_SIZE;
			b->action(getBackPostion(b->pos(), b->getVeryFrontEnemyUnit()->getPosition(), safeDist + 1));
		}
		else
		{
			moveBackPostion(b, b->pos(), 2);
		}
	}
	else if (INFO.getSecondChokePosition(S).isValid() && (b->hp() * 100) / b->type().maxHitPoints() < 33)
	{
		b->setState(new BarrackLiftAndMoveState());
		b->action(INFO.getSecondChokePosition(S));
	}
	else if (TIME % 24 == 0)
	{
		if (INFO.enemyRace == Races::Terran)
		{
			if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion)
			{
				if (b->pos().getApproxDistance(INFO.getMainBaseLocation(E)->getPosition()) > 10)
				{
					b->setState(new BarrackLiftAndMoveState());
					// Àû ¸ÞÀÎº£ÀÌ½º µÎ¹øÂ° ÃÊÅ©Æ÷ÀÎÆ®(³ÎÀÌ¶ó¸é ¸ÞÀÎº£ÀÌ½º)·Î Á¤ÂûÇÏ·¯ °¨
					b->action(getBarrackEnemyChokePosition());
				}
			}
			else
			{
				b->setState(new BarrackLiftAndMoveState());

				uList tankList = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);

				// ÅÊÅ©ÀüÃ¼ÀÇ Æò±Õ Æ÷Áö¼Ç¿¡¼­ ÅÊÅ© ÀüÃ¼ Æò±Õ ¹æÇâ 4Å¸ÀÏ ¾ÕÀ¸·Î ÀÌµ¿
				if (!tankList.empty())
				{
					b->action(get1stUnitFrontPosition(tankList));
					// ÃßÈÄ ÅÊÅ©¿¡ Àü¼±Çü¼º ·ÎÁ÷ÀÌ µé¾î°¡¸é ÀÌ ÇÔ¼ö°¡ ÁÁÀ½
					//b->action(getAvgTankFrontPosition(tankList));
				}
				else
					b->action(getBarrackEnemyChokePosition());
			}

		}
		else if (INFO.enemyRace == Races::Protoss && SM.getMainStrategy() != WaitToBase && SM.getMainStrategy() != WaitToFirstExpansion)
		{
			b->setState(new BarrackLiftAndMoveState());
			uList tankList = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);

			// ¼±µÎ ÅÊÅ© Æ÷Áö¼Ç¿¡¼­ ÅÊÅ© ¹æÇâ 4Å¸ÀÏ ¾ÕÀ¸·Î ÀÌµ¿
			if (!tankList.empty())
			{
				b->action(get1stUnitFrontPosition(tankList));
			}
			else if (INFO.getSecondChokePosition(S).isValid())
				b->action(INFO.getSecondChokePosition(S));
		}
		else
		{
			if (INFO.getSecondChokePosition(S).isValid() && b->pos().getApproxDistance(INFO.getSecondChokePosition(S)) > 10)
			{
				b->setState(new BarrackLiftAndMoveState());
				b->action(INFO.getSecondChokePosition(S));
			}
		}

	}
}

void TrainManager::actionBarrackLiftAndMoveState(UnitInfo *b)
{

	if (b->unit()->isUnderAttack() || b->getVeryFrontEnemyUnit() != nullptr)
	{
		if (b->getVeryFrontEnemyUnit() != nullptr)
		{
			int safeDist = b->getVeryFrontEnemyUnit()->getType().airWeapon().maxRange() > TILE_SIZE ? b->getVeryFrontEnemyUnit()->getType().airWeapon().maxRange() / TILE_SIZE : TILE_SIZE;
			b->action(getBackPostion(b->pos(), b->getVeryFrontEnemyUnit()->getPosition(), safeDist + 1));
		}
		else
		{
			moveBackPostion(b, b->pos(), 2);
		}
	}
	else if ((b->hp() * 100) / b->type().maxHitPoints() < 33)
	{
		b->action(INFO.getSecondChokePosition(S));
	}
	// °Ç¹° Æ¯Á¤ Æ÷ÀÎÆ®±îÁö ÀÌµ¿
	else if (TIME % 48 == 0)
	{
		if (INFO.enemyRace == Races::Terran)
		{
			if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion)
			{
				// Àû ¸ÞÀÎº£ÀÌ½º Á¤ÂûÇÏ·¯ °¨
				b->action(getBarrackEnemyChokePosition());
			}
			else
			{
				uList tankList = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);

				// ¼±µÎ ÅÊÅ© Æ÷Áö¼Ç¿¡¼­ ÅÊÅ© ¹æÇâ 4Å¸ÀÏ ¾ÕÀ¸·Î ÀÌµ¿
				if (!tankList.empty())
				{
					b->action(get1stUnitFrontPosition(tankList));
					// ÃßÈÄ ÅÊÅ©¿¡ Àü¼±Çü¼º ·ÎÁ÷ÀÌ µé¾î°¡¸é ÀÌ ÇÔ¼ö°¡ ÁÁÀ½
					//b->action(getAvgTankFrontPosition(tankList));
				}
				else
					b->action(getBarrackEnemyChokePosition());
			}

		}
		else if (INFO.enemyRace == Races::Protoss && SM.getMainStrategy() != WaitToBase && SM.getMainStrategy() != WaitToFirstExpansion)
		{
			uList tankList = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);
			uList vultureList = INFO.getUnits(Terran_Vulture, S);
			uList goliathList = INFO.getUnits(Terran_Goliath, S);

			// ¼±µÎ Æ÷Áö¼Ç ¹æÇâ 4Å¸ÀÏ ¾ÕÀ¸·Î ÀÌµ¿
			if (!tankList.empty())
				b->action(get1stUnitFrontPosition(tankList));
			else if (!vultureList.empty())
				b->action(get1stUnitFrontPosition(vultureList));
			else if (!goliathList.empty())
				b->action(get1stUnitFrontPosition(goliathList));
			else
				b->action(INFO.getSecondChokePosition(S));
		}
		else
			b->action(INFO.getSecondChokePosition(S));
	}
}


Position TrainManager::get1stUnitFrontPosition(uList unitList)
{
	int dist = INT_MAX;
	UnitInfo *firstUnit = nullptr;

	for (auto t : unitList)
	{
		int tempDist = INT_MAX;
		theMap.GetPath(t->pos(), SM.getMainAttackPosition(), &tempDist);

		if (tempDist >= 0 && dist > tempDist)
		{
			firstUnit = t;
			dist = tempDist;
		}
	}

	return targetPos;
}
*/
bool TrainManager::isFirstFactory()
{
	if (INFO.getTypeBuildingsInRadius(Terran_Factory, S, INFO.getMainBaseLocation(S)->getPosition(), 0, true, true).size() <= 1)
		return true;
	else
		return false;
}

bool TrainManager::isFirstBarrack()
{
	if (INFO.getTypeBuildingsInRadius(Terran_Factory, S, INFO.getMainBaseLocation(S)->getPosition(), 0, true, true).size()<= 1)
		return true;
	else
		return false;
}

void TrainManager::findAndSaveFirstFactoryPos(Unit factory)
{
	moveFirstFactoryPos = ReserveBuilding::Instance().getReservedPosition(Terran_Factory, factory, Terran_Machine_Shop);

	ReserveBuilding::Instance().assignTiles(moveFirstFactoryPos, Terran_Factory);
}

bool TrainManager::isTimeToMoveCommandCenter(UnitInfo *c)
{
	// Ä¿¸Çµå ¼¾ÅÍ hp°¡ 50% ÀÌ»óÀÌ¸é ¹öÆ¾´Ù
	if (c->hp() < c->type().maxHitPoints() * 50 / 100)
	{
		// 50% ÀÌÇÏ¶óµµ ´çÀå ³»°¡ ¸Â°í ÀÖÁö ¾Ê´Ù¸é ¹öÆ¾´Ù
		if (c->getVeryFrontEnemyUnit() != nullptr || c->unit()->isUnderAttack())
			return true;
	}

	return false;
}

void TrainManager::setBarricadeBarrack(uList &bList)
{
	if (SM.getMainStrategy() != AttackAll && INFO.getFirstExpansionLocation(S))
	{
		for (auto b : bList) {
			if (b->getState() == "Idle") 
			{
				b->setState(new BarrackBarricadeState());
				b->action();
				break;
			}
		}
	}
	else
	{
		for (auto b : bList) {
			if (b->getState() == "Barricade") {
				b->unit()->lift();
				b->setState(new BarrackIdleState());
				break;
			}
		}
	}

}

bool TrainManager::isSafeComsatPosition(UnitInfo *depot)
{
	// º»Áø, ¾Õ¸¶´ç Á¦¿Ü ³ª¸ÓÁö À§Ä¡ÀÇ Ä¿¸Çµå¼¾ÅÍ¿¡ ÄÄ»û Ãß°¡ ½Ã
	// ÇØ´ç Áö¿ªÀÌ ¾ÈÀüÇÑÁö ÆÇ´Ü ÈÄ ÄÄ»û Ãß°¡
	// (ÁÖ·Î ÅÊÅ©¶§¹®¿¡ ÄÄ»ûÀÌ °è¼Ó ÆÄ±«µÇ´Â °ÍÀ» ¹æÁöÇÏ±â À§ÇØ Ãß°¡)

	if (isSameArea(depot->pos(), (Position)MYBASE))
		return true;

	if (INFO.getFirstExpansionLocation(S) && isSameArea(depot->pos(), INFO.getFirstExpansionLocation(S)->getPosition()))
		return true;

	// ÄÄ»û Center À§Ä¡
	Position basePosition = (Position)(depot->unit()->getTilePosition() + TilePosition(5, 2));
	uList enemyList = INFO.getUnitsInRadius(E, basePosition, 15 * TILE_SIZE, true, true, true, true);

	// ÄÄ»û À§Ä¡°¡ À§ÇèÇÏ¸é false
	if (getDamageAtPosition(basePosition, Terran_Comsat_Station, enemyList, false))
		return false;

	return true;
}

bool TrainManager::needStopTrainToFactory()
{
	bool stopTrain = false;
	int initialCombatUnitCreateCount = INFO.getAllCount(Terran_Vulture, S) + INFO.getDestroyedCount(Terran_Vulture, S)
									   + INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) + INFO.getDestroyedCount(Terran_Siege_Tank_Tank_Mode, S);

	// ÃÊ¹Ý ÆÑÅä¸® °ø°Ý À¯´Ö 2±â »ý»êÇÒ ¶§ ±îÁö ÆÑÅä¸® ÃÖ´ëÇÑ ½¬Áö ¾Êµµ·Ï ¸¶¸° »ý»ê ½¬´Â ·ÎÁ÷ Ãß°¡
	if (initialCombatUnitCreateCount < 2)
	{
		for (auto f : INFO.getBuildings(Terran_Factory, S))
		{
			if (!f->isComplete())
				continue;

			// addon ÀÌ ÀÖ´Â °æ¿ì
			if (f->unit()->getAddon()) {
				if (getAvailableMinerals() <= 150)
				{
					if (f->unit()->getAddon()->isCompleted()) {
						// º´·ÂÀ» ¾È»Ì°í ÀÖ°Å³ª »ý»ê °ð ¿Ï·á µÇ´Â °æ¿ì º´·Â »ý»ê °¡´É
						if (f->unit()->getTrainingQueue().empty() || f->unit()->isTraining() && f->unit()->getRemainingTrainTime() <= 130)
							stopTrain = true;
					}
					// addon ÀÌ °ð ¿Ï¼ºµÇ´Â °æ¿ì º´·Â »ý»ê °¡´É
					else if (!f->unit()->getAddon()->isCompleted() && f->unit()->getAddon()->getRemainingBuildTime() <= 130)
						stopTrain = true;
				}
			}
			// addon ÀÌ ¾ø´Â °æ¿ì
			else {
				if (getAvailableMinerals() <= 75)
				{
					// »ý»êÁßÀÌ°Å³ª ¾÷±×·¹ÀÌµå ÁßÀÌ ¾Æ´Ñ °æ¿ì
					if (f->unit()->getTrainingQueue().empty())
						stopTrain = true;
					// »ý»ê ¿Ï·á Àü
					else if (f->unit()->isTraining() && f->unit()->getRemainingTrainTime() <= 130)
						stopTrain = true;
				}
			}
		}
	}

	return stopTrain;
}

bool TrainManager::needStopTrainToBarracks()
{
	bool stopTrain = false;

	if ((TIME < 24 * 60 * 5 && (EIB == Terran_4scv || EIB == Toss_4probes || EIB == Zerg_4_Drone_Real)
		&& INFO.getCompletedCount(Terran_Barracks, S) && INFO.getAllCount(Terran_Marine, S) < 1 && INFO.getAllCount(Terran_SCV, S) >= 2) || ((SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller) && INFO.getAllCount(Terran_SCV, S) >= 6) && !INFO.getAllCount(Terran_Barracks, S))
	{
		// cout << "@@@ ¹è·° º´·ÂÁ» »ý»êÇØ¾ßÇØ¼­ ÀÏ²Û »ý»ê ½¯°Ô ¤Ð" << endl;
		stopTrain = true;
	}

	return stopTrain;
}

Position TrainManager::getBarricadePosition()
{
	if (barricadePosition == Positions::None) {

		Position eBase = (Position)INFO.getMainBaseLocation(E)->Center();
		Position secondChoke = (Position)INFO.getSecondChokePoint(S)->Pos(INFO.getSecondChokePoint(S)->end2);

		if (eBase.getApproxDistance(secondChoke) >
				eBase.getApproxDistance((Position)INFO.getSecondChokePoint(S)->Pos(INFO.getSecondChokePoint(S)->end1))) {
			secondChoke = (Position)INFO.getSecondChokePoint(S)->Pos(INFO.getSecondChokePoint(S)->end1);
		}

		// ¾Õ¸¶´ç°ú ¼¼ÄÁ ÃÊÅ©Æ÷ÀÎÆ®°¡ ¸Õ °æ¿ì ( ex : µ¥½ºÆ¼³×ÀÌ¼Ç)
		const pair<const Area *, const Area *> &areas = INFO.getSecondChokePoint(S)->GetAreas();
		const vector<ChokePoint> &secondCP = areas.first->ChokePoints(areas.second);
		WalkPosition pos = WalkPositions::Origin;

		for (auto &cp : secondCP) {
			pos += cp.Center();
		}

		if (INFO.getFirstExpansionLocation(S)->getPosition().getApproxDistance((Position)INFO.getSecondChokePoint(S)->Center()) > 340)
			pos = (pos * 2 + (WalkPosition)INFO.getFirstExpansionLocation(S)->getPosition() * 2) / (secondCP.size() * 2 + 2);
		else
			pos = (WalkPosition)secondChoke;

		barricadePosition = (Position)pos;
	}

	return barricadePosition;

}

void TrainManager::setBarricadePosition(Position p)
{
	barricadePosition = p;
}