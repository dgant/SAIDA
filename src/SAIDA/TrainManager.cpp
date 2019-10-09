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
	// 현재 프레임을 기준으로 BuildQueue 에 추가되어 예약된 자원과 병력 생산에 사용된 자원을 제외한 사용 가능한 자원과 비교
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

	// 9 Drone 壇뚤세핀
	if (INFO.enemyRace == Races::Zerg && EIB <= Zerg_9_Drone &&
				INFO.getTypeUnitsInRadius(Zerg_Zergling, E, INFO.getMainBaseLocation(S)->getPosition(), 10 * TILE_SIZE).size())
		{
			return;
	}

	// 초반 일꾼러쉬 상대로 마린 1기 생산하기 위해 일꾼 생산 중단
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

	// 공중에 떠있는 커맨드가 있는가
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

		// 우리 본진 커맨드센터가 아니라면 위기상황 판단하여 띄운다
		if (state != "LiftAndMove") 
		{
			// 현재 베이스를 유지할 상황이 안된다면
			if (c->isComplete() && isTimeToMoveCommandCenter(c))
			{
				// 내가 있던 위치가 추가 멀티 지역이었으면 내 추가 멀티 리스트에서 정보 삭제
				Position pos = c->pos();

				auto targetBase = find_if(INFO.getAdditionalExpansions().begin(), INFO.getAdditionalExpansions().end(), [pos](Base * base) {
					return pos == base->getPosition();
				});

				if (targetBase != INFO.getAdditionalExpansions().end())
					INFO.removeAdditionalExpansionData(c->unit());

				const Base *b = getEscapeBase(c);

				if (b) {
					// addon건설중이거나 유닛 훈련중이면 취소한다
					if (c->unit()->canCancelAddon())
						c->unit()->cancelAddon();

					for (word i = 0, size = c->unit()->getTrainingQueue().size(); i < size; i++) {
						if (c->unit()->canCancelTrain())
							c->unit()->cancelTrain();
					}

					// 안전한 곳으로 날라간다
					c->setState(new CommandCenterLiftAndMoveState(getEscapeBase(c)->getTilePosition()));
					state = c->getState();
				}
			}
		}

		if (state == "New" || state == "Idle")
		{
			// 앞마당 언덕에서 지은 커맨드인지 판단
			if (c->isComplete() && c->unit()->getDistance(MYBASE) > 5 * TILE_SIZE && isSameArea(c->pos(), MYBASE)
					&& INFO.getCompletedCount(Terran_Command_Center, S) == 2) {


				if (INFO.getFirstExpansionLocation(S) && INFO.isTimeToMoveCommandCenterToFirstExpansion()) {
					c->setState(new CommandCenterLiftAndMoveState(INFO.getFirstExpansionLocation(S)->getTilePosition()));
					c->action();
				}
				else // 일단 띄운다.
				{
					if (!c->getLift())
						c->unit()->lift();
				}

				continue;
			}

			UnitType trainUnit = Terran_SCV;

			// 자원 부족
			if (!hasEnoughResources(trainUnit))
				continue;

			// 적이 내 본진 근처에 있고 Factory가 안돌고 있으면 SCV를 뽑지 않음...
			if (waitToProduce && INFO.enemyInMyArea().size())
				continue;

			// 컴샛스테이션 지을수 있으면 상태변경
			else if (INFO.getCompletedCount(Terran_Academy, S) >= 1 && c->unit()->getAddon() == nullptr && c->unit()->canBuildAddon()
					 && hasEnoughResources(Terran_Comsat_Station) && isSafeComsatPosition(c)) {
				c->setState(new CommandCenterBuildAddonState());
				addReserveResources(Terran_Comsat_Station);
			}
			// SCV 만들 수 있으면 뽑기
			else if (curScvCnt <= maxScvNeedCount && c->unit()->canTrain())
			{
				if (INFO.getCompletedCount(Terran_Command_Center, S) > 2
						&& ScvManager::Instance().depotHasEnoughMineralWorkers(c->unit())) // 앞마당 이상일 때
					continue;

				if (!SM.checkTurretFirst()) {
					c->setState(new CommandCenterTrainState(1));
					addReserveResources(trainUnit);
				}
			}
			// 커맨드센터 근처에 있는 자원이 다 떨어지면 커맨드센터 옮긴다.
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

					// 멀티 할 수 있는 위치 있으면
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
			//========================묏낍뻘청쉔접폅윱돨헙워========================
			if (INFO.getCompletedCount(Terran_Factory, S) == 0)
			{
				basemarineCount = 3;
			}
			//============================묏낍쉔접폅윱돨헙워=========================
			if (INFO.getCompletedCount(Terran_Factory, S) != 0)
			{
				//샙筠�絹威늄㉰�
				if (INFO.getCompletedCount(Terran_Machine_Shop, S) == 0)
				{
					basemarineCount = 4;
				}
				//샙筠�絹苑㉰�
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
			// 벌쳐를 최소 2마리 ~ 4마리 까지 적 병력에 맞춰서 뽑는다.
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
		
			baseTankCount = 3;//4譴옹角샘뇟
		
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

			// 배틀크루저 나왔을 때 골리앗 조금 더 추가 생산
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
										// 가스 부족으로 골리앗 못뽑을 경우 미네랄 체크하고 벌쳐 생산
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

						// 가스 세이브하기 위해 벌쳐 생산
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

								// Tank 만들 Gas는 있으나 Mineral이 부족한 경우 Tank 생산을 위해 이하 Factory에서 Vulture 생산을 중단한다.
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
				// 병력 생산 종료되면 Idle로 바뀜
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
									// 가스 부족으로 골리앗 못뽑을 경우 미네랄 체크하고 벌쳐 생산
									trainUnit = getAvailableGas() >= Terran_Goliath.gasPrice() ? None : hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
								}
							}
						}

						// 가스 세이브하기 위해 벌쳐 생산
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
//옰欺눋鑒좆�阮�=======================================================
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
//�阮�=======================================================
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
				// wraith 생산 Threshold
				baseWraithCount = 1;
				baseValkyrieCount = 0;
				maxValkyrieCount = 2;
				// wraith 최대 생산
				maxWraithCount = 3;
				// 필요한 컨트롤타워
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

		// 최소 레이스 생산 후 머신샵 추가
		if (state == "Idle")
		{
			//if (wraithCount >= maxWraithCount && vesselCount >= maxVesselCount)
			//	continue;

			UnitType trainUnit = UnitTypes::None;

			// 기본 레이스 수는 맞춘다.
			if (wraithCount < baseWraithCount)
			{
				trainUnit = Terran_Wraith;
			}
			else
			{
				if ((int)controlTowerList.size() < needConrolTowerCount)
				{
					// 컨트롤타워 추가
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

			// 자원 부족
			if (trainUnit == Terran_Science_Vessel && nextVessleTime < TIME)
			{
				if (!hasEnoughResources(trainUnit) && trainUnit.gasPrice() > getAvailableGas())
				{
					saveGas = true;
					continue;
				}
			}

			// 뽑을 유닛 없음
			if (trainUnit == UnitTypes::None)
				continue;

			s->setState(new StarportTrainState());
			s->action(trainUnit);
			addReserveResources(trainUnit);

			if (trainUnit == Terran_Science_Vessel && saveGas)
			{
				saveGas = false;
				nextVessleTime = TIME + (24 * 60 * 2); // 베슬 재생산은 2분 후부터
				cout << "## 베슬 재생산은 2분뒤에.." << nextVessleTime << endl;
			}

			continue;
		}
		else if (state == "Train")
		{
			// 병력 생산 종료되면 Idle로 바뀜
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

		// Command가 완성되지 않은것이 있다면 완성 시 Rebalancing을 위해 10개 margine을 추가
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
	// 공격받으면
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
					// 적 메인베이스 두번째 초크포인트(널이라면 메인베이스)로 정찰하러 감
					b->action(getBarrackEnemyChokePosition());
				}
			}
			else
			{
				b->setState(new BarrackLiftAndMoveState());

				uList tankList = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);

				// 탱크전체의 평균 포지션에서 탱크 전체 평균 방향 4타일 앞으로 이동
				if (!tankList.empty())
				{
					b->action(get1stUnitFrontPosition(tankList));
					// 추후 탱크에 전선형성 로직이 들어가면 이 함수가 좋음
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

			// 선두 탱크 포지션에서 탱크 방향 4타일 앞으로 이동
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
	// 건물 특정 포인트까지 이동
	else if (TIME % 48 == 0)
	{
		if (INFO.enemyRace == Races::Terran)
		{
			if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion)
			{
				// 적 메인베이스 정찰하러 감
				b->action(getBarrackEnemyChokePosition());
			}
			else
			{
				uList tankList = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);

				// 선두 탱크 포지션에서 탱크 방향 4타일 앞으로 이동
				if (!tankList.empty())
				{
					b->action(get1stUnitFrontPosition(tankList));
					// 추후 탱크에 전선형성 로직이 들어가면 이 함수가 좋음
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

			// 선두 포지션 방향 4타일 앞으로 이동
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
	// 커맨드 센터 hp가 50% 이상이면 버틴다
	if (c->hp() < c->type().maxHitPoints() * 50 / 100)
	{
		// 50% 이하라도 당장 내가 맞고 있지 않다면 버틴다
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
	// 본진, 앞마당 제외 나머지 위치의 커맨드센터에 컴샛 추가 시
	// 해당 지역이 안전한지 판단 후 컴샛 추가
	// (주로 탱크때문에 컴샛이 계속 파괴되는 것을 방지하기 위해 추가)

	if (isSameArea(depot->pos(), (Position)MYBASE))
		return true;

	if (INFO.getFirstExpansionLocation(S) && isSameArea(depot->pos(), INFO.getFirstExpansionLocation(S)->getPosition()))
		return true;

	// 컴샛 Center 위치
	Position basePosition = (Position)(depot->unit()->getTilePosition() + TilePosition(5, 2));
	uList enemyList = INFO.getUnitsInRadius(E, basePosition, 15 * TILE_SIZE, true, true, true, true);

	// 컴샛 위치가 위험하면 false
	if (getDamageAtPosition(basePosition, Terran_Comsat_Station, enemyList, false))
		return false;

	return true;
}

bool TrainManager::needStopTrainToFactory()
{
	bool stopTrain = false;
	int initialCombatUnitCreateCount = INFO.getAllCount(Terran_Vulture, S) + INFO.getDestroyedCount(Terran_Vulture, S)
									   + INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) + INFO.getDestroyedCount(Terran_Siege_Tank_Tank_Mode, S);

	// 초반 팩토리 공격 유닛 2기 생산할 때 까지 팩토리 최대한 쉬지 않도록 마린 생산 쉬는 로직 추가
	if (initialCombatUnitCreateCount < 2)
	{
		for (auto f : INFO.getBuildings(Terran_Factory, S))
		{
			if (!f->isComplete())
				continue;

			// addon 이 있는 경우
			if (f->unit()->getAddon()) {
				if (getAvailableMinerals() <= 150)
				{
					if (f->unit()->getAddon()->isCompleted()) {
						// 병력을 안뽑고 있거나 생산 곧 완료 되는 경우 병력 생산 가능
						if (f->unit()->getTrainingQueue().empty() || f->unit()->isTraining() && f->unit()->getRemainingTrainTime() <= 130)
							stopTrain = true;
					}
					// addon 이 곧 완성되는 경우 병력 생산 가능
					else if (!f->unit()->getAddon()->isCompleted() && f->unit()->getAddon()->getRemainingBuildTime() <= 130)
						stopTrain = true;
				}
			}
			// addon 이 없는 경우
			else {
				if (getAvailableMinerals() <= 75)
				{
					// 생산중이거나 업그레이드 중이 아닌 경우
					if (f->unit()->getTrainingQueue().empty())
						stopTrain = true;
					// 생산 완료 전
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
		// cout << "@@@ 배럭 병력좀 생산해야해서 일꾼 생산 쉴게 ㅠ" << endl;
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

		// 앞마당과 세컨 초크포인트가 먼 경우 ( ex : 데스티네이션)
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