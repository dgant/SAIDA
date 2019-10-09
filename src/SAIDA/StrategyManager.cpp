#include "StrategyManager.h"
#include "UnitManager\TankManager.h"
#include "UnitManager\GoliathManager.h"
#include "TrainManager.h"

#define PERIOD 24 * 5                
#define BASE_WINNING_RATE 65.00      					

using namespace MyBot;

StrategyManager &StrategyManager::Instance()
{
	static StrategyManager instance;
	return instance;
}

StrategyManager::StrategyManager()
{

	string mapName = bw->mapFileName();
	scanForcheckMulti = false;
	mainStrategy = WaitToBase;
	myBuild = MyBuildTypes::Basic;

	searchPoint = 0;

	int find_size_x = (theMap.Size().x * 32) / 20;
	int find_size_y = (theMap.Size().y * 32) / 20;

	for (int i = 0; i < 20; i++)
	{
		for (int j = 0; j < 20; j++)
		{
			map400[searchPoint].x = find_size_x / 2 + (i * find_size_x);
			map400[searchPoint].y = find_size_y / 2 + (j * find_size_y);
			searchPoint++;
		}
	}

	random_shuffle(&map400[0], &map400[399]);

	searchPoint = 0;

	mainAttackPosition = INFO.getMainBaseLocation(E)->Center();

	lastUsingScanTime = 0;
	needTank = false;
	scanForAttackAll = false;

}

void StrategyManager::onStart()
{
}

void StrategyManager::onEnd(bool isWinner)
{
}

void StrategyManager::update()
{
	try {
		executeSupplyManagement();
	}
	catch (...) {
	}

	try {
		BasicBuildStrategy::Instance().executeBasicBuildStrategy();
	}
	catch (...) {
	}

	try {
		makeMainStrategy();
	}

	catch (...) {
	}

	try {
		if (mainStrategy == Eliminate)
			searchForEliminate();
	}
	catch (...) {
	}

	try {
		checkUpgrade();
	}
	
	catch (...) {
	}

	try {
		checkUsingScan();
	}
	catch (...) {
	}

	try {
		setAttackPosition();
	}
	
	catch (...) {
	}

	setMyBuild();

	checkNeedAttackMulti();

	decideDropship();

	checkSecondExpansion();

	checkThirdExpansion();

	checkKeepExpansion();

	checkAdvanceWait();

	checkNeedDefenceWithScv();

	if (!isExistEnemyMine)
	{
		if (!INFO.getUnits(Terran_Vulture_Spider_Mine, E).empty())
			isExistEnemyMine = true;
	}

	
}

void StrategyManager::checkEnemyMultiBase()
{
	int scanThreshold = 4;

	if (INFO.enemyRace == Races::Protoss) {
		scanThreshold = 2;

		if (EMB == Toss_arbiter || EMB == Toss_arbiter_carrier || EMB == Toss_fast_carrier
				|| EMB == Toss_dark || EMB == Toss_dark_drop) {
			scanThreshold = 4;
		}
	}

	if (INFO.getAvailableScanCount() >= scanThreshold)
	{
		int rank = 6;
		Base *bestBase = nullptr;

		
		for (auto base : INFO.getBaseLocations())
		{
			if (base->GetOccupiedInfo() != emptyBase)
				continue;

			
			if (base->GetLastVisitedTime() + (24 * 60 * 2) > TIME) continue;

			if (base->GetExpectedEnemyMultiRank() <= 6 && base->GetExpectedEnemyMultiRank() < rank)
			{
				rank = base->GetExpectedEnemyMultiRank();
				bestBase = base;
			}
		}

		if (bestBase != nullptr)
			ComsatStationManager::Instance().useScan(bestBase->Center());
	}
}

/*
bool strategyManager::enemyIsProtoss()
{
for each (Player* u in Broodwar->getPlayers())
{
if (u->isEnemy(Broodwar->self()))
{
if (u->getRace().getID() == Races::Protoss.getID())
{
return true;
}
}
}
return false;
}

bool strategyManager::enemyIsZerg()
{
for each (Player* u in Broodwar->getPlayers())
{
if (u->isEnemy(Broodwar->self()))
{
if (u->getRace().getID() == Races::Zerg.getID())
{
return true;
}
}
}
return false;
}

*/
void StrategyManager::checkNeedAttackMulti()
{
	if (multiBreakDeathPosition != Positions::Unknown)
	{
		

		if (INFO.enemyRace == Races::Terran)
		{
			int eVulture = INFO.getTypeUnitsInRadius(Terran_Vulture, E, multiBreakDeathPosition, 10 * TILE_SIZE, true).size();
			int eTank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, multiBreakDeathPosition, 15 * TILE_SIZE, true).size();
			int eWraith = INFO.getTypeUnitsInRadius(Terran_Wraith, E, multiBreakDeathPosition, 10 * TILE_SIZE, true).size();
			int eGoliath = INFO.getTypeUnitsInRadius(Terran_Goliath, E, multiBreakDeathPosition, 10 * TILE_SIZE, true).size();

			int enemyPoint = eTank * 10 + eVulture * 3 + eGoliath * 5 + eWraith * 3;


			if (enemyPoint >= 30) {
				needAttackMulti = false;
				return;
			}

			multiBreakDeathPosition = Positions::Unknown;
		}
		else if (INFO.enemyRace == Races::Protoss) 
		{

		}
		else 
		{

		}
	}

	if (INFO.enemyRace == Races::Terran)
		return;

	if (TIME % 24 != 0) {
		return;
	}

	if (INFO.enemyRace == Races::Zerg) {
		
		if (secondAttackPosition == Positions::Unknown || !secondAttackPosition.isValid())
		{
			needAttackMulti = false;
			return;
		}

		UnitInfo *firstTank = TM.getFrontTankFromPos(secondAttackPosition);
		UnitInfo *firstGol = GM.getFrontGoliathFromPos(secondAttackPosition);

		if (firstTank != nullptr)
		{
			if (isSameArea(firstTank->pos(), secondAttackPosition)) {
				needAttackMulti = true;
				return;
			}
		}

		if (firstGol != nullptr)
		{
			if (isSameArea(firstGol->pos(), secondAttackPosition)) {
				needAttackMulti = true;
				return;
			}
		}

		needAttackMulti = false;
	}
	/*

	bool strategyManager::enemyIsTerran()
	{
	for each (Player* u in Broodwar->getPlayers())
	{
	if (u->isEnemy(Broodwar->self()))
	{
	if (u->getRace().getID() == Races::Terran.getID())
	{
	return true;
	}
	}
	}
	return false;
	}
	*/
	else if (INFO.enemyRace == Races::Protoss) {
	
		if (secondAttackPosition == Positions::Unknown || !secondAttackPosition.isValid())
		{
			needAttackMulti = false;
			return;
		}

		
		if (!INFO.getFirstExpansionLocation(E)) {
			needAttackMulti = false;
			return;
		}

		Position firstExpansionE = INFO.getFirstExpansionLocation(E)->Center();
		UnitInfo *firstTank = TM.getFrontTankFromPos(firstExpansionE);

		
		if (firstTank && isSameArea(firstTank->pos(), firstExpansionE)
				&& INFO.getTypeBuildingsInArea(Protoss_Nexus, E, firstExpansionE, true).empty()) {
			
			word tankThreshold = (EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier) ? 2 : 4;

			if (INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, firstTank->pos(), 15 * TILE_SIZE).size() > tankThreshold) {
				needAttackMulti = true;
			}
		}
	}

	return;
}

void StrategyManager::makeMainStrategy()
{
	
	if (TIME % 24 != 0) {
		return;
	}

	
	if (mainStrategy == Eliminate && INFO.getOccupiedBaseLocations(E).empty())
	{
		return;
	}

	int vultureCount = INFO.getCompletedCount(Terran_Vulture, S);
	int tankCount = INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S);
	int goliathCount = INFO.getCompletedCount(Terran_Goliath, S);
	int enemyVultureCount = INFO.getCompletedCount(Terran_Vulture, E);
	int enemyTankCount = INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, E);
	int enemyGoliathCount = INFO.getCompletedCount(Terran_Goliath, E);

	
	if (needTank == false)
	{
		if (ESM.getEnemyMainBuild() == Terran_3fac)
		{
			needTank = true;
		}

		if (TIME < 24 * 60 * 6
				&& (enemyTankCount > 0 || enemyGoliathCount > 1))
		{
			needTank = true;
		}
		else if (TIME >= 24 * 60 * 6 && TIME < 24 * 60 * 8
				 && ((enemyTankCount > 3 && INFO.hasRearched(TechTypes::Tank_Siege_Mode)) || enemyGoliathCount > 4))
		{
			needTank = true;
		}
		else if (TIME > 24 * 60 * 8)
		{
			needTank = true;
		}
	}

	
	uMap myUnits = INFO.getUnits(S);
	int myAttackUnitSupply = 0;

	for (auto &myUnit : myUnits)
	{
		if (!myUnit.second->isComplete())
			continue;

		if (myUnit.second->type().isWorker())
			continue;

		if (myUnit.second->type() == Terran_Marine)
			continue;

		myAttackUnitSupply += myUnit.second->type().supplyRequired();
	}

	
	uMap enemyUnits = INFO.getUnits(E);
	int enemyAttackUnitSupply = 0;

	for (auto &enemyUnit : enemyUnits)
	{
		if (!enemyUnit.second->isComplete())
			continue;

		if (enemyUnit.second->type().isWorker())
			continue;

	
		if (enemyUnit.second->type() == Protoss_Carrier)
			continue;

		
		if (enemyUnit.second->type() == Zerg_Queen)
			continue;

		enemyAttackUnitSupply += enemyUnit.second->type().supplyRequired();
	}

	if (mainStrategy == AttackAll)
	{
		if (INFO.enemyRace == Races::Zerg)
		{
			if (myAttackUnitSupply < enemyAttackUnitSupply * 1.0 + 4)
			{
				mainStrategy = BackAll;
				scanForAttackAll = false;
				printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
				return;
			}
		}
		else if (INFO.enemyRace == Races::Terran)
		{
			if (myBuild == MyBuildTypes::Terran_TankGoliath)
			{
				waitLinePosition = Positions::Unknown;
				drawLinePosition = Positions::Unknown;

				if (S->supplyUsed() < 320 || (tankCount < enemyTankCount * 1.2 && S->supplyUsed() < 380))
				{
					mainStrategy = WaitLine;
					scanForAttackAll = false;
					printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
					return;
				}
			}
			else if (myBuild == MyBuildTypes::Terran_VultureTankWraith)
			{
				if (myAttackUnitSupply < enemyAttackUnitSupply)
				{
					mainStrategy = BackAll;
					scanForAttackAll = false;
					printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
					return;
				}

				if (needTank)
				{
					if (enemyTankCount > 2 && !S->hasResearched(TechTypes::Tank_Siege_Mode) && tankCount == 0)
					{
						mainStrategy = BackAll;
						scanForAttackAll = false;
						printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
						return;
					}
				}
			}
		}
		else
		{

			if (ESM.getEnemyMainBuild() == Toss_fast_carrier || ESM.getEnemyMainBuild() == Toss_arbiter_carrier) {

				if (finalAttack) {
					if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) < 2)
						finalAttack = false;
					else
						return;
				}

				if (SM.getMainAttackPosition() != Positions::Unknown) {
					UnitInfo *frontTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

					if (frontTank && frontTank->pos().getApproxDistance(SM.getMainAttackPosition()) < 30 * TILE_SIZE) {
						uList interceptor = INFO.getUnits(Protoss_Interceptor, E);
						word interceptorCntInAttackArea = 0;

						for (auto i : interceptor) {
							if (i->getLastSeenPosition().getApproxDistance(SM.getMainAttackPosition()) <= 50 * TILE_SIZE) {
								interceptorCntInAttackArea++;
							}
						}

						uList goliath = INFO.getTypeUnitsInRadius(Terran_Goliath, S, frontTank->pos(), 20 * TILE_SIZE);

						if (interceptorCntInAttackArea > goliath.size() * 4) {
							mainStrategy = BackAll;
							scanForAttackAll = false;
							printf("[BackAll] near Protoss_Interceptor.size() = %d, near goliath.size()  = %d\n", interceptorCntInAttackArea, goliath.size());
							return;
						}
					}
				}

				if (myAttackUnitSupply < enemyAttackUnitSupply && myAttackUnitSupply < 45 && tankCount < 4)
				{
					mainStrategy = BackAll;
					scanForAttackAll = false;
					printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
					return;
				}

			}
			else {
			
				if (!INFO.getUnits(Protoss_Arbiter, E).empty()) {
					if (INFO.getCompletedCount(Terran_Science_Vessel, S) == 0 && INFO.getAvailableScanCount() < 1) {
						mainStrategy = BackAll;
						scanForAttackAll = false;
						return;
					}
				}

				if (myAttackUnitSupply < enemyAttackUnitSupply && myAttackUnitSupply < 45 && tankCount < 4)
				{
					mainStrategy = BackAll;
					scanForAttackAll = false;
					printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
					return;
				}

			}

		}
	}
	else if (mainStrategy == DrawLine)
	{
		if (S->supplyUsed() >= 380 && (INFO.getOccupiedBaseLocations(E).size() < 2 || INFO.getAllCount(Terran_Command_Center, E) < 2))
		{
			mainStrategy = AttackAll;
			return;
		}

		if (INFO.enemyRace == Races::Terran)
		{
			UnitInfo *firstTank = TM.getFrontTankFromPos(mainAttackPosition);

			if (firstTank == nullptr)
			{
				
				waitLinePosition = INFO.getSecondChokePosition(S);

				drawLinePosition = Positions::Unknown;
				mainStrategy = WaitLine;

				return;
			}

		

			if (needWait(firstTank))
			{
				mainStrategy = WaitLine;
				drawLinePosition = Positions::Unknown;
				return;
			}

		
			if (INFO.getAvailableScanCount() < 1 && INFO.getUnits(Spell_Scanner_Sweep, S).empty()
					&& (isExistEnemyMine || (INFO.getAllCount(Terran_Wraith, E) + INFO.getDestroyedCount(Terran_Wraith, E))))
			{
				drawLinePosition = Positions::Unknown;
				mainStrategy = WaitLine;

				return;
			}

			
			if (TIME > nextScanTime || drawLinePosition.getApproxDistance(firstTank->pos()) <= 3 * TILE_SIZE)
			{
				setDrawLinePosition();

			
				if (drawLinePosition == Positions::Unknown || !drawLinePosition.isValid())
				{
					drawLinePosition = Positions::Unknown;
					mainStrategy = WaitLine;

					return;
				}

				
				if (drawLinePosition.getApproxDistance(firstTank->pos()) > 9 * TILE_SIZE || !bw->isVisible((TilePosition)drawLinePosition))
				{
					int myUnitCountInDrawLine = INFO.getUnitsInRadius(S, drawLinePosition, 10 * TILE_SIZE, true).size();
					int enemyUnitCountInDrawLine = INFO.getUnitsInRadius(E, drawLinePosition, 10 * TILE_SIZE, true).size();

					if (myUnitCountInDrawLine <= enemyUnitCountInDrawLine)
					{
						ComsatStationManager::Instance().useScan(drawLinePosition);
						nextScanTime = TIME + 25 * 24;
					}
				}
			}

		
			uList enemyMine = INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, E, firstTank->pos(), Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange(), true);

			if (!enemyMine.empty())
			{
				Position scanPosition = getAvgPosition(enemyMine);

				if (scanPosition.isValid())
				{
					ComsatStationManager::Instance().useScan(drawLinePosition);
				}
			}

			if (!moreUnits(firstTank))
			{
				drawLinePosition = Positions::Unknown;
				mainStrategy = WaitLine;
			}

			return;
		}
	}
	else if (mainStrategy == WaitLine)
	{
		if (S->supplyUsed() >= 380 && (INFO.getOccupiedBaseLocations(E).size() < 2 || INFO.getAllCount(Terran_Command_Center, E) < 2))
		{
			mainStrategy = AttackAll;
			return;
		}

		if (INFO.enemyRace == Races::Terran)
		{
			UnitInfo *firstTank = TM.getFrontTankFromPos(mainAttackPosition);

			if (firstTank != nullptr)
			{
				int distanceToFirstTank = 0;
				int distanceToSecondChokePoint = 0;

				theMap.GetPath(MYBASE, firstTank->pos(), &distanceToFirstTank);
				theMap.GetPath(MYBASE, INFO.getSecondChokePosition(S), &distanceToSecondChokePoint);

				if (distanceToFirstTank < distanceToSecondChokePoint)
					waitLinePosition = INFO.getSecondChokePosition(S);
				else
					waitLinePosition = firstTank->pos();
			}
			else
			{
				
			
				waitLinePosition = INFO.getSecondChokePosition(S);

				return;
			}

			
			if (nextDrawTime == 0)
			{
				nextDrawTime = TIME + 20 * 24;
				return;
			}

			if (nextDrawTime > 0 && nextDrawTime > TIME)
			{
				return;
			}

			
			nextDrawTime = 0;
			drawLinePosition = Positions::Unknown;

			if (needWait(firstTank))
			{
				waitLinePosition = firstTank->pos();
				return;
			}

			if (tankCount < 3 || goliathCount < 2
					|| (INFO.getAvailableScanCount() < 2
						&& (isExistEnemyMine || (INFO.getAllCount(Terran_Wraith, E) + INFO.getDestroyedCount(Terran_Wraith, E))))
			   )
			{
				return;
			}

			setDrawLinePosition();

			
			if (drawLinePosition == Positions::Unknown || !drawLinePosition.isValid())
			{
				drawLinePosition = Positions::Unknown;

				return;
			}

		
			if (drawLinePosition.getApproxDistance(firstTank->pos()) > 9 * TILE_SIZE || !bw->isVisible((TilePosition)drawLinePosition))
			{
				int myUnitCountInDrawLine = INFO.getUnitsInRadius(S, drawLinePosition, 10 * TILE_SIZE, true).size();
				int enemyUnitCountInDrawLine = INFO.getUnitsInRadius(E, drawLinePosition, 10 * TILE_SIZE, true).size();

				if (myUnitCountInDrawLine <= enemyUnitCountInDrawLine)
				{
					ComsatStationManager::Instance().useScan(drawLinePosition);
				}
			}

			if (moreUnits(firstTank))
			{
				mainStrategy = DrawLine;
				return;
			}

		
			drawLinePosition = Positions::Unknown;
		}

		return;
	}
	else
	{
		if (S->supplyUsed() >= 380 && (INFO.getOccupiedBaseLocations(E).size() < 2 || INFO.getAllCount(Terran_Command_Center, E) < 2))
		{
			mainStrategy = AttackAll;
			return;
		}

		if (INFO.enemyRace == Races::Zerg)
		{
			if (myAttackUnitSupply >= 12 * 2 && S->hasResearched(TechTypes::Tank_Siege_Mode) && tankCount > 2 &&
					myAttackUnitSupply >= enemyAttackUnitSupply * 1.5 + 8)
			{
				UnitInfo *armory = INFO.getClosestTypeUnit(S, MYBASE, Terran_Armory);

				if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Plating) || S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons)
						|| (armory != nullptr && armory->unit()->getRemainingUpgradeTime() > 0 && armory->unit()->getRemainingUpgradeTime() < 24 * 30))
				{
					mainStrategy = AttackAll;

					if (scanForAttackAll == false && INFO.getAvailableScanCount() > 0)
					{
						ComsatStationManager::Instance().useScan(INFO.getSecondChokePosition(E));
						scanForAttackAll = true;
					}

					return;
				}
			}
		}
		else if (INFO.enemyRace == Races::Terran)
		{
			if (myBuild == MyBuildTypes::Terran_TankGoliath)
			{
				if (S->hasResearched(TechTypes::Tank_Siege_Mode))
				{
					
					UnitInfo *closestTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

					if (closestTank != nullptr)
					{
						waitLinePosition = closestTank->pos();
						drawLinePosition = Positions::Unknown;
						mainStrategy = WaitLine;

						return;
					}

					
				}
			}
			else if (myBuild == MyBuildTypes::Terran_VultureTankWraith)
			{
				if (myAttackUnitSupply < 3 * 2)
					return;

				if (needTank)
				{
					if (!S->hasResearched(TechTypes::Tank_Siege_Mode) && tankCount < enemyTankCount)
					{
						return;
					}
				}

				if (myAttackUnitSupply >= (int)(enemyAttackUnitSupply * 1.3) + 2)
				{
					mainStrategy = AttackAll;

					if (scanForAttackAll == false && INFO.getAvailableScanCount() > 0)
					{
						ComsatStationManager::Instance().useScan(INFO.getSecondChokePosition(E));
						scanForAttackAll = true;
					}

					return;
				}
			}
		}
		else 
		{

			if (ESM.getEnemyMainBuild() == Toss_fast_carrier || ESM.getEnemyMainBuild() == Toss_arbiter_carrier)
			{
				
				int threshold = 3;

				if (INFO.getCompletedCount(Protoss_Carrier, E) <= 3)
					threshold = 2;

				int sizeDiff = INFO.getCompletedCount(Terran_Goliath, S) - INFO.getCompletedCount(Protoss_Carrier, E) * threshold;

				if (S->hasResearched(TechTypes::Tank_Siege_Mode) && tankCount >= 4
						&& sizeDiff > 0 && S->getUpgradeLevel(UpgradeTypes::Charon_Boosters))
				{
					bool canAttack = false;

					if (SM.getMainAttackPosition() != Positions::Unknown) {

						uList carrier = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, SM.getMainAttackPosition(), 30 * TILE_SIZE);

						
						if (carrier.size() > 4) {

							UnitInfo *frontTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

							if (frontTank) {

								if (frontTank->pos().getApproxDistance(SM.getMainAttackPosition()) < 20 * TILE_SIZE) {
									uList goliath = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, frontTank->pos(), 20 * TILE_SIZE);

									if (goliath.size() >= carrier.size() * 4)
										canAttack = true;
								}
							}
						}
						else
							canAttack = true;
					}

					if (canAttack) {
						mainStrategy = AttackAll;

						if (scanForAttackAll == false && INFO.getAvailableScanCount() > 0)
						{
							ComsatStationManager::Instance().useScan(INFO.getSecondChokePosition(E));
							scanForAttackAll = true;
							return;
						}
					}

				}

				
				if (INFO.getCompletedCount(Terran_Goliath, S) <= 4 && INFO.getCompletedCount(Protoss_Carrier, E) >= 8) {
					if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) + INFO.getCompletedCount(Terran_Vulture, S) >= 10) {
						mainStrategy = AttackAll;
						finalAttack = true;
						return;
					}
				}

			}
			else
			{
				double rushThreshhold = 1.3;

				if (EMB == Toss_2base_zealot_dra) {
					if (scanForcheckMulti == false && INFO.getAvailableScanCount() > 0)
					{
						if (INFO.getSecondExpansionLocation(E) && !bw->isVisible((TilePosition)INFO.getSecondExpansionLocation(E)->Center())) {
							ComsatStationManager::Instance().useScan(INFO.getSecondExpansionLocation(E)->Center());
							scanForcheckMulti = true;
						}
					}

					rushThreshhold = 1.5;
				}
				else if (EMB == Toss_mass_production) {
					rushThreshhold = 1.2;
				}

				
				if (tankCount >= 6 && vultureCount >= 3 && S->hasResearched(TechTypes::Tank_Siege_Mode) && myAttackUnitSupply >= (int)(enemyAttackUnitSupply * rushThreshhold))
				{
					
					if (!INFO.getUnits(Protoss_Arbiter, E).empty()) {
						if (INFO.getAllCount(Terran_Science_Vessel, S) == 0 && INFO.getAvailableScanCount() < 5) {
							return;
						}
					}

					
					if (!INFO.getUnits(Protoss_Dark_Templar, E).empty()) {
						if (INFO.getAllCount(Terran_Science_Vessel, S) == 0 && INFO.getAvailableScanCount() < 2) {
							return;
						}
					}

					UnitInfo *armory = INFO.getClosestTypeUnit(S, MYBASE, Terran_Armory);

					
					if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) ||
							(armory != nullptr && armory->unit()->getRemainingUpgradeTime() > 0 && armory->unit()->getRemainingUpgradeTime() < 24 * 30))
					{
						mainStrategy = AttackAll;

						if (scanForAttackAll == false && INFO.getAvailableScanCount() > 0)
						{
							ComsatStationManager::Instance().useScan(INFO.getSecondChokePosition(E));
							scanForAttackAll = true;
						}

						return;
					}
				}
			}
		}

		uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

		for (auto c : commandCenterList)
		{
			if (INFO.getFirstExpansionLocation(S) != nullptr && c->pos().getApproxDistance(INFO.getFirstExpansionLocation(S)->getPosition()) < 6 * TILE_SIZE)
			{
				mainStrategy = WaitToFirstExpansion;
				return;
			}
		}

		mainStrategy = WaitToBase;
	}
}



void StrategyManager::executeSupplyManagement()
{
	
	if (TIME % 24 != 0) {
		return;
	}


	int onBuildingSupplyCount = 0;

	onBuildingSupplyCount += CM.getConstructionQueueItemCount(Terran_Supply_Depot) * Terran_Supply_Depot.supplyProvided();

	
	uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

	for (auto c : commandCenterList)
	{
		if (!c->isComplete() && c->hp() > 875)
		{
			onBuildingSupplyCount += c->type().supplyProvided();
		}
	}

	
	if (S->supplyUsed() + onBuildingSupplyCount >= 400)
	{
		return;
	}

	
	if (S->supplyTotal() < 400)
	{
		
		int supplyMargin = 0;

		int numberOfCommandCenter = getTrainBuildingCount(Terran_Command_Center);
		int numberOfBarracks = getTrainBuildingCount(Terran_Barracks);
		int numberOfFactory = getTrainBuildingCount(Terran_Factory);
		int numberofStarport = getTrainBuildingCount(Terran_Starport);

		supplyMargin = (numberOfBarracks * 1 + numberOfCommandCenter * 1 + numberOfFactory * 2 + numberofStarport * 3) * 2;

		double velocity = 0;
		int supplyCount = INFO.getAllCount(Terran_Supply_Depot, S);
		int remaingMineral = S->minerals();

		if (supplyCount == 0)
		{
			velocity = 2;
		}
		else if (supplyCount == 1)
		{
			if (ESM.getEnemyInitialBuild() <= Zerg_9_Drone)
			{
				velocity = 1.0;

				if (BasicBuildStrategy::Instance().cancelSupplyDepot)
				{
					if (INFO.getAllCount(Terran_Bunker, S) < 1)
					{
						velocity = 0.5;
					}
				}
			}
			else
			{
				velocity = 2.0;
			}
		}
		else if (supplyCount == 2)
			velocity = 1.0;
		else if (supplyCount == 3)
			velocity = 1.3;
		else if (supplyCount >= 4)
			velocity = 1.5;

		supplyMargin = (int)(supplyMargin * velocity);

		
		int currentSupplyShortage = S->supplyUsed() + supplyMargin - S->supplyTotal();

		
		if (currentSupplyShortage + S->supplyTotal() > 400) {
			currentSupplyShortage = currentSupplyShortage + S->supplyTotal() - 400;
		}

		
		if (currentSupplyShortage > 0) {
			
			if (currentSupplyShortage > onBuildingSupplyCount) {
				bool needBarricade = false;

				for (auto b : INFO.getBuildings(Terran_Barracks, S))
				{
					if (b->getState() == "Barricade")
					{
						needBarricade = true;
					}
				}

				for (auto b : INFO.getBuildings(Terran_Engineering_Bay, S))
				{
					if (b->getState() == "Barricade")
					{
						needBarricade = true;
					}
				}

				
				{
					
					if (currentSupplyShortage > 2 * Terran_Supply_Depot.supplyProvided()) {
						int neededTobuildByForce = currentSupplyShortage / Terran_Supply_Depot.supplyProvided();

						for (int i = 0; i < min(neededTobuildByForce, remaingMineral / 100); i++) {
							if (CM.getConstructionQueueItemCount(Terran_Factory)
									+ BM.buildQueue.getItemCount(Terran_Factory) < neededTobuildByForce)
							{
								BM.buildQueue.queueAsHighestPriority(Terran_Supply_Depot);
							}
						}
					}
					else {
						if (supplyCount <= 4)
						{
							
							bool isToEnqueue = true;

							if (!BM.buildQueue.isEmpty())
							{
								if (BM.buildQueue.existItem(Terran_Supply_Depot))
								{
									isToEnqueue = false;
								}
							}

							if (isToEnqueue) {
								BM.buildQueue.queueAsHighestPriority(Terran_Supply_Depot, true);
							}
						}
						else
						{
							
							bool isToEnqueue = true;

							if (!BM.buildQueue.isEmpty()) {
								BuildOrderItem currentItem = BM.buildQueue.getHighestPriorityItem();

								if (currentItem.metaType.isUnit() && currentItem.metaType.getUnitType() == Terran_Supply_Depot) {
									isToEnqueue = false;
								}
							}

							if (isToEnqueue) {
								
								BM.buildQueue.queueAsHighestPriority(Terran_Supply_Depot, true);
							}
						}
					}
				}
			}
		}
	}
}

void StrategyManager::searchForEliminate()
{
	if (TIME % 24 != 0) {
		return;
	}

	Position targetPosition = Positions::None;

	uList enemys = INFO.getBuildingsInRadius(E, Positions::Origin, 0, true, true, true);

	bool isAir = false;

	for (auto eu : enemys)
	{
		if (eu->pos() != Positions::Unknown)
		{
			targetPosition = eu->pos();
			isAir = eu->unit()->isFlying();
			break;
		}
	}

	
	if (isAir && INFO.getAllCount(Terran_Wraith, S) == 0)
	{
		if (S->supplyUsed() > 390) 
		{
			for (auto u : INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S))
			{
				UnitInfo *closestVulture = INFO.getClosestTypeUnit(S, u->pos(), Terran_Vulture);

				if (closestVulture != nullptr)
					CommandUtil::attackUnit(u->unit(), closestVulture->unit());
			}
		}

		for (auto star : INFO.getTypeBuildingsInRadius(Terran_Starport, S))
		{
			if (star->isComplete())
			{
				if (!star->unit()->isTraining() && Broodwar->canMake(Terran_Wraith, star->unit()))
				{
					star->unit()->train(Terran_Wraith);
				}
			}
		}
	}

	for (auto u : INFO.getUnitsInRadius(S, Positions::Origin, 0, true, true, false))
	{
		if (u->getState() != "Eliminate" && u->getState() != "Defence") {
			
			if (INFO.enemyRace != Races::Protoss || u->unit()->getType() != Terran_Goliath) {
				u->setState(new EliminateState());
			}
		}

		if (u->unit()->isSieged())
			u->unit()->unsiege();
	}

	if (targetPosition == Positions::None) {

		for (auto u : INFO.getUnitsInRadius(S, Positions::Origin, 0, true, true, false))
			searchAllMap(u->unit());

	}
	else {

		for (auto u : INFO.getUnitsInRadius(S, Positions::Origin, 0, true, true, false)) {

			if (isAir) {
				if (u->unit()->getType() == Terran_Goliath || u->unit()->getType() == Terran_Wraith)
					u->unit()->attack(targetPosition);
				else
					searchAllMap(u->unit());
			}
			else
				u->unit()->attack(targetPosition);

		}

	}

}

void StrategyManager::searchAllMap(Unit u) {

	Position targetPosition = Positions::None;
	UnitCommand command = u->getLastCommand();
	bool enemyRemoved = false;

	
	if (command.getTargetPosition().isValid() && bw->isVisible((TilePosition)command.getTargetPosition())) {
		if (INFO.getUnitsInRadius(E, command.getTargetPosition(), 10 * TILE_SIZE, true, true, true, true).empty() &&
				INFO.getBuildingsInRadius(E, command.getTargetPosition(), 10 * TILE_SIZE, true, true, true).empty())
			enemyRemoved = true;
	}

	if (u->isIdle() || enemyRemoved) {

		if (searchPoint < 400)
		{
			targetPosition = map400[searchPoint];
			searchPoint++;
		}
		else
		{
			searchPoint = 0;
		}

		u->attack(targetPosition);
	}
}

bool StrategyManager::getNeedSecondExpansion()
{
	return needSecondExpansion;
}

bool StrategyManager::getNeedThirdExpansion()
{
	return needThirdExpansion;
}

bool StrategyManager::getNeedAdvanceWait()
{
	return advanceWait;
}

void StrategyManager::setMyBuild()
{
	if (INFO.enemyRace == Races::Terran)
	{
		myBuild = MyBuildTypes::Terran_TankGoliath;
	}
	else if (INFO.enemyRace == Races::Protoss)
	{

	}
	else if (INFO.enemyRace == Races::Zerg)
	{

	}
}


void StrategyManager::blockFirstChokePoint(UnitType type)
{
	WalkPosition cpEnd1 = theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition())->ChokePoints().at(0)->Pos(BWEM::ChokePoint::end1);
	WalkPosition cpMiddle = theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition())->ChokePoints().at(0)->Pos(BWEM::ChokePoint::middle);
	WalkPosition cpEnd2 = theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition())->ChokePoints().at(0)->Pos(BWEM::ChokePoint::end2);

	Position cpEnd1Pos = Position(cpEnd1);
	Position cpMiddlePos = Position(cpMiddle);
	Position cpEnd2Pos = Position(cpEnd2);

	

	for (auto &unit : S->getUnits())
	{
		if (!unit->isCompleted()) continue;

		if (unit->getType() == type)
		{
			if (m1 == nullptr)
				m1 = unit;
			else if (m2 == nullptr && unit != m1)
				m2 = unit;
		}
	}

	if (cpEnd1Pos.getDistance(cpEnd2Pos) > 50) 
	{
		if (m1 != nullptr)
			if (m1->getDistance((cpEnd1Pos + cpMiddlePos) / 2) > 1)
			{
				m1->move((cpEnd1Pos + cpMiddlePos) / 2);
			}
			else
			{
				m1->holdPosition();
			}

		if (m2 != nullptr)
			if (m2->getDistance((cpEnd2Pos + cpMiddlePos) / 2) > 1)
			{
				m2->move((cpEnd2Pos + cpMiddlePos) / 2);
			}
			else
			{
				m2->holdPosition();
			}
	}
	else 
	{
		if (m1 != nullptr)
			if (m1->getDistance(getDefencePos(cpMiddlePos, cpEnd1Pos, INFO.getMainBaseLocation(S)->getPosition())) > 1)
			{
				m1->move(getDefencePos(cpMiddlePos, cpEnd1Pos, INFO.getMainBaseLocation(S)->getPosition()));
			}
			else
			{
				m1->holdPosition();
			}
	}
}

int StrategyManager::getTrainBuildingCount(UnitType type)
{
	uList buildings = INFO.getBuildings(type, S);
	int count = 0;

	for (auto b : buildings)
	{
		
		if (!b->isComplete() && b->hp() < b->type().maxHitPoints() * 0.6)
			continue;

		count++;
	}

	return count;
}

void StrategyManager::checkUpgrade()
{
	if (TIME % 24 != 0)
		return;

	if ((INFO.enemyRace == Races::Protoss) &&
			(ESM.getEnemyInitialBuild() == Toss_1g_dragoon || ESM.getEnemyInitialBuild() == Toss_2g_dragoon ||
			 ESM.getEnemyInitialBuild() == Toss_1g_double || ESM.getEnemyInitialBuild() == UnknownBuild))
	{
		
		if (BasicBuildStrategy::Instance().isResearchingOrResearched(TechTypes::Tank_Siege_Mode)) {
			needUpgrade = false;
			return;
		}

		
		if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0 && INFO.getCompletedCount(Terran_Machine_Shop, S) > 0 &&
				!BasicBuildStrategy::Instance().isResearchingOrResearched(TechTypes::Tank_Siege_Mode)
				&& !(EIB != Toss_cannon_rush && (EMB == UnknownMainBuild || EMB == Toss_dark) && !HasEnemyFirstExpansion()))
			needUpgrade = true;
		else
			needUpgrade = false;
	}

	return;
}

void StrategyManager::checkUsingScan()
{
	if (TIME % 24 != 0)
		return;

	if (INFO.getAvailableScanCount() < 1)
		return;

	if (INFO.getMainBaseLocation(E) != nullptr && INFO.getMainBaseLocation(E)->GetLastVisitedTime() == 0)
	{
		ComsatStationManager::Instance().useScan(INFO.getMainBaseLocation(E)->Center());
		return;
	}

	if (INFO.enemyRace == Races::Zerg)
	{

	}
	else if (INFO.enemyRace == Races::Terran)
	{

	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		if (ESM.getEnemyMainBuild() != Toss_arbiter_carrier
				&& lastUsingScanTime + 24 * 60 * 1.5 < TIME
				&& INFO.getAvailableScanCount() > 1) 
		{

			INFO.setNextScanPointOfMainBase();

			if (INFO.getMainBaseLocation(E) != nullptr && !bw->isVisible((TilePosition)INFO.getMainBaseLocation(E)->Center())) {
				ComsatStationManager::Instance().useScan((Position)INFO.getScanPositionOfMainBase());
				lastUsingScanTime = TIME;
			}

		}
	}

	checkEnemyMultiBase();
}

void StrategyManager::setAttackPosition()
{
	if (TIME % 24 != 0)
		return;

	const Base *currentAttackBase = nullptr;

	if (mainAttackPosition != Positions::Unknown)
	{
		Base *nearestBase = INFO.getNearestBaseLocation(mainAttackPosition, true);

		if (nearestBase && nearestBase->GetOccupiedInfo() == enemyBase)
			currentAttackBase = nearestBase;
	}

	
	for (auto eBase : INFO.getOccupiedBaseLocations(E))
	{
		if (eBase->Center().getApproxDistance(theMap.Center()) < 10 * TILE_SIZE)
		{
			currentAttackBase = eBase;
		}
	}

	
	Position standardPos = MYBASE;
	int closestDistance = INT_MAX;
	Position closestPosition = Positions::Unknown;

	if (currentAttackBase) 
	{
		bool isExistResourceDepot = false;

		Position currentBasePos = currentAttackBase->Center();

		if (INFO.enemyRace == Races::Zerg)
		{
			if (INFO.getTypeBuildingsInRadius(Zerg_Hatchery, E, currentBasePos, 8 * TILE_SIZE).size()
					+ INFO.getTypeBuildingsInRadius(Zerg_Lair, E, currentBasePos, 8 * TILE_SIZE).size()
					+ INFO.getTypeBuildingsInRadius(Zerg_Hive, E, currentBasePos, 8 * TILE_SIZE).size() > 0)
				isExistResourceDepot = true;
		}
		else
		{
			if (INFO.getTypeBuildingsInRadius(INFO.getBasicResourceDepotBuildingType(INFO.enemyRace), E, currentBasePos, 8 * TILE_SIZE).size() > 0)
				isExistResourceDepot = true;
		}

		if (isExistResourceDepot)
		{
			mainAttackPosition = currentBasePos;
		}
		else 
		{
			if (INFO.getBuildingsInRadius(E, mainAttackPosition, 2 * TILE_SIZE, true, false, true).size() == 0)
			{
				uMap enemyBuildings = INFO.getBuildings(E);

				Position sameAreaBuilding = Positions::None;

				for (auto &enemyBuilding : enemyBuildings)
				{
					if (enemyBuilding.second->pos() == Positions::Unknown)
						continue;

					if (enemyBuilding.second->getLift())
						continue;

					if (isSameArea(enemyBuilding.second->pos(), mainAttackPosition))
					{
						mainAttackPosition = enemyBuilding.second->pos();
						sameAreaBuilding = mainAttackPosition;
						break;
					}
				}

			
				if (sameAreaBuilding == Positions::None)
				{
					for (auto &enemyBuilding : enemyBuildings)
					{
						if (enemyBuilding.second->pos() == Positions::Unknown)
							continue;

						if (enemyBuilding.second->getLift())
							continue;

						int dist = getGroundDistance(currentBasePos, enemyBuilding.second->pos());

						if (dist >= 0 && dist < 12 * TILE_SIZE)
						{
							mainAttackPosition = enemyBuilding.second->pos();
							break;
						}
					}
				}
			}
		}
		/*
		bool flyingDetector = false;
		bool flyingSquadExists = false;	    // scourge and carriers to flying squad if any, otherwise main squad
		for (const auto unit : flyingSquad.getUnits())
		{
		if (unit->getType().isDetector())
		{
		flyingDetector = true;
		}
		else if (unit->getType() == BWAPI::UnitTypes::Zerg_Mutalisk ||
		unit->getType() == BWAPI::UnitTypes::Terran_Wraith ||
		unit->getType() == BWAPI::UnitTypes::Terran_Valkyrie ||
		unit->getType() == BWAPI::UnitTypes::Terran_Battlecruiser ||
		unit->getType() == BWAPI::UnitTypes::Protoss_Corsair ||
		unit->getType() == BWAPI::UnitTypes::Protoss_Scout)
		{
		flyingSquadExists = true;
		}
		}

		*/
	
		standardPos = mainAttackPosition;
		int farDistance = 0;
		Position farPosition = Positions::Unknown;

		for (auto eBase : INFO.getOccupiedBaseLocations(E))
		{
			
			if (eBase->isIsland())
				continue;

			int tempDist = getGroundDistance(standardPos, eBase->Center());

			if (tempDist == -1)
				continue;

			if (currentAttackBase != nullptr && eBase == currentAttackBase)
				continue;

			
			if (currentAttackBase == INFO.getMainBaseLocation(E) && INFO.getFirstExpansionLocation(E) && eBase == INFO.getFirstExpansionLocation(E))
				continue;
			/*
			bool isDetector = unit->getType().isDetector();
			if (_squadData.canAssignUnitToSquad(unit, flyingSquad)
			&&
			(unit->getType() == BWAPI::UnitTypes::Zerg_Mutalisk ||
			unit->getType() == BWAPI::UnitTypes::Zerg_Scourge && flyingSquadExists ||
			unit->getType() == BWAPI::UnitTypes::Terran_Wraith ||
			unit->getType() == BWAPI::UnitTypes::Terran_Valkyrie ||
			unit->getType() == BWAPI::UnitTypes::Terran_Battlecruiser ||
			unit->getType() == BWAPI::UnitTypes::Protoss_Corsair ||
			unit->getType() == BWAPI::UnitTypes::Protoss_Scout ||
			unit->getType() == BWAPI::UnitTypes::Protoss_Carrier && flyingSquadExists ||
			isDetector && !flyingDetector && flyingSquadExists))
			{
			_squadData.assignUnitToSquad(unit, flyingSquad);
			if (isDetector)
			{
			flyingDetector = true;
			}
			*/
			
			if (INFO.getFirstExpansionLocation(E) && currentAttackBase == INFO.getFirstExpansionLocation(E) && eBase == INFO.getMainBaseLocation(E))
				continue;

			
			UnitInfo *forwardTank = TM.getFrontTankFromPos(currentAttackBase->Center());

			if (forwardTank)
			{
				int gDist = getGroundDistance(forwardTank->pos(), currentAttackBase->Center());
				int baseGDist = getGroundDistance(currentAttackBase->Center(), eBase->Center());

				if (gDist >= 0 && baseGDist >= 0 && gDist > baseGDist)
					continue;
			}

			
			if (currentAttackBase == INFO.getMainBaseLocation(E) && isSameArea(eBase->Center(), currentAttackBase->Center()))
				continue;

			if (INFO.enemyRace == Races::Zerg)
			{
				if (INFO.getTypeBuildingsInRadius(Zerg_Hatchery, E, eBase->Center(), 8 * TILE_SIZE).size()
						+ INFO.getTypeBuildingsInRadius(Zerg_Lair, E, eBase->Center(), 8 * TILE_SIZE).size()
						+ INFO.getTypeBuildingsInRadius(Zerg_Hive, E, eBase->Center(), 8 * TILE_SIZE).size() == 0)
					continue;
			}
			/*
			Squad & dropSquad = _squadData.getSquad("Drop");

			// The squad is initialized with a Hold order.
			// There are 3 phases, and in each phase the squad is given a different order:
			// Collect units (Hold); load the transport (Load); go drop (Drop).
			// If it has already been told to go, we are done.
			if (dropSquad.getSquadOrder().getType() != SquadOrderTypes::Hold &&
			dropSquad.getSquadOrder().getType() != SquadOrderTypes::Load)
			{
			return;
			}
			*/
			else
			{
				if (INFO.getTypeBuildingsInRadius(INFO.getBasicResourceDepotBuildingType(INFO.enemyRace), E, eBase->Center(), 8 * TILE_SIZE).size() == 0)
					continue;
			}

			if (tempDist > farDistance)
			{
				farDistance = tempDist;
				farPosition = eBase->Center();
			}
		}

		if (farPosition != Positions::Unknown)
			secondAttackPosition = farPosition;
		else
		{
			secondAttackPosition = Positions::Unknown;
			needAttackMulti = false;
		}
	}
	else
	{
		if (mainAttackPosition != Positions::Unknown && theMap.Center().getApproxDistance(mainAttackPosition) < 10 * TILE_SIZE) {
			mainAttackPosition = INFO.getMainBaseLocation(E)->Center();
			return;
		}
		/*
		for (const auto unit : _combatUnits)
		{
		// If the squad doesn't have a transport, try to add one.
		if (!transportUnit &&
		unit->getType().spaceProvided() > 0 && unit->isFlying() &&
		_squadData.canAssignUnitToSquad(unit, dropSquad))
		{
		_squadData.assignUnitToSquad(unit, dropSquad);
		transportUnit = unit;
		}

		// If the unit fits and is good to drop, add it to the squad.
		// Rewrite unitIsGoodToDrop() to select the units of your choice to drop.
		// Simplest to stick to units that occupy the same space in a transport, to avoid difficulties
		// like "add zealot, add dragoon, can't add another dragoon--but transport is not full, can't go".
		else if (unit->getType().spaceRequired() <= transportSpotsRemaining &&
		unitIsGoodToDrop(unit) &&
		_squadData.canAssignUnitToSquad(unit, dropSquad))
		{
		_squadData.assignUnitToSquad(unit, dropSquad);
		transportSpotsRemaining -= unit->getType().spaceRequired();
		}
		}
		*/
		secondAttackPosition = Positions::Unknown;
		needAttackMulti = false;

		standardPos = mainAttackPosition;
		closestDistance = INT_MAX;
		closestPosition = Positions::Unknown;

		

		for (auto eBase : INFO.getOccupiedBaseLocations(E))
		{
			
			if (eBase->isIsland())
				continue;

			int tempDist = getGroundDistance(standardPos, eBase->Center());

			if (closestDistance > tempDist)
			{
				closestPosition = eBase->Center();
				closestDistance = tempDist;
			}
		}

		if (closestPosition != Positions::Unknown) 
		{
			mainAttackPosition = closestPosition;
			return;
		}
		else 
		{
			uMap enemyBuildings = INFO.getBuildings(E);
			
			bool isExistEnemyBuilding = false;
			closestDistance = INT_MAX;
			Position tempBuildingPos = Positions::None;

			for (auto &enemyBuilding : enemyBuildings)
			{
				if (enemyBuilding.second->pos() == Positions::Unknown)
					continue;

				
				if (enemyBuilding.second->getLift())
					continue;

				int dist = getGroundDistance(MYBASE, enemyBuilding.second->pos());

				if (closestDistance > dist)
				{
					tempBuildingPos = enemyBuilding.second->pos();
					closestDistance = dist;
					isExistEnemyBuilding = true;
				}
			}
			/*
				// start off assuming all enemy units in region are just workers
		const int numDefendersPerEnemyUnit = 2;

		// all of the enemy units in this region
		BWAPI::Unitset enemyUnitsInRegion;
        for (const auto unit : BWAPI::Broodwar->enemy()->getUnits())
        {
            // If it's a harmless air unit, don't worry about it for base defense.
			// TODO something more sensible
            if (unit->getType() == BWAPI::UnitTypes::Zerg_Overlord ||
				unit->getType() == BWAPI::UnitTypes::Protoss_Observer ||
				unit->isLifted())  // floating terran building
            {
                continue;
            }

            if (BWTA::getRegion(BWAPI::TilePosition(unit->getPosition())) == myRegion)
            {
                enemyUnitsInRegion.insert(unit);
            }
        }*/
			if (isExistEnemyBuilding)
			{
				
				mainAttackPosition = tempBuildingPos;
				return;
			}

		
			bool completedSearch = true;

			for (auto sl : INFO.getStartLocations())
			{
				if (sl->GetLastVisitedTime() == 0)
				{
					completedSearch = false;
					break;
				}
			}

			
			if (completedSearch)
			{
				
				mainStrategy = Eliminate;
			}
			else
			{
				
				mainAttackPosition = INFO.getMainBaseLocation(E)->Center();
			}
		}
	}
}

void StrategyManager::checkAdvanceWait()
{
	if (!advanceWait)
	{
		if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 2 && INFO.getCompletedCount(Terran_Goliath, S) >= 4)
			advanceWait = true;
	}
}

void StrategyManager::setDrawLinePosition()
{
	UnitInfo *firstTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

	if (firstTank == nullptr)
	{
		drawLinePosition = waitLinePosition;
		return;
	}

	
	if (INFO.getAllCount(Terran_Command_Center, S) < 3)
	{
		Position tempLinePosition = Positions::Unknown;
		int tankToFirstWaitLineDistance = 0;

		
		tankToFirstWaitLineDistance = firstTank->pos().getApproxDistance(INFO.getFirstWaitLinePosition());

		if (tankToFirstWaitLineDistance > 12 * TILE_SIZE)
		{
			tempLinePosition = getDirectionDistancePosition(firstTank->pos(), INFO.getFirstWaitLinePosition(), 12 * TILE_SIZE);
		}
		else
		{
			tempLinePosition = INFO.getFirstWaitLinePosition();
		}

		
		if (tempLinePosition.getApproxDistance(INFO.getFirstWaitLinePosition()) < 5 * TILE_SIZE)
		{
			drawLinePosition = INFO.getFirstWaitLinePosition();
		}
		else
		{
			drawLinePosition = tempLinePosition;
		}

		int SCPToFirstWaitLineDistance = 0;
		int SCPToFirstTankDistance = 0;

		theMap.GetPath(INFO.getSecondChokePosition(S), INFO.getFirstWaitLinePosition(), &SCPToFirstWaitLineDistance);
		theMap.GetPath(INFO.getSecondChokePosition(S), firstTank->pos(), &SCPToFirstTankDistance);

		if (SCPToFirstTankDistance < SCPToFirstWaitLineDistance - 2.5 * TILE_SIZE)
		{
			
			return;
		}
		else
		{
			
		}
	}

	int tankDistance = 0;
	int firstWaitLineDistance = 0;
	Position basePosition = Positions::Unknown;

	theMap.GetPath(INFO.getFirstExpansionLocation(S)->getPosition(), firstTank->pos(), &tankDistance);
	theMap.GetPath(INFO.getFirstExpansionLocation(S)->getPosition(), INFO.getFirstWaitLinePosition(), &firstWaitLineDistance);

	if (tankDistance < firstWaitLineDistance)
		basePosition = INFO.getFirstWaitLinePosition();
	else
		basePosition = firstTank->pos();

	const CPPath &path = theMap.GetPath(firstTank->pos(), mainAttackPosition);

	if (path.empty())
	{
		
		if (basePosition.getApproxDistance(mainAttackPosition) < 15 * TILE_SIZE)
		{
			drawLinePosition = mainAttackPosition;
		}
		else
		{
			drawLinePosition = getDirectionDistancePosition(basePosition, mainAttackPosition, 15 * TILE_SIZE);
		}
	}
	else
	{
		auto c = path.begin();

		
		if (basePosition.getApproxDistance((Position)(*c)->Center()) < 6 * TILE_SIZE)
		{
			c++;

			
			if (c == path.end())
			{
				drawLinePosition = getDirectionDistancePosition(basePosition, mainAttackPosition, 15 * TILE_SIZE);
				/*			
		int numEnemyFlyingInRegion = std::count_if(enemyUnitsInRegion.begin(), enemyUnitsInRegion.end(), [](BWAPI::Unit u) { return u->isFlying(); });
		int numEnemyGroundInRegion = enemyUnitsInRegion.size() - numEnemyFlyingInRegion;
		drawLinePosition = getDirectionDistancePosition(basePosition, mainAttackPosition, 15 * TILE_SIZE);

		// assign units to the squad
		UAB_ASSERT(_squadData.squadExists(squadName.str()), "Squad should exist: %s", squadName.str().c_str());
        Squad & defenseSquad = _squadData.getSquad(squadName.str());

				*/
			}
			else
			{
				drawLinePosition = getDirectionDistancePosition(basePosition, (Position)(*c)->Center(), 15 * TILE_SIZE);
			}
		}
		else
		{
			drawLinePosition = getDirectionDistancePosition(basePosition, (Position)(*c)->Center(), 15 * TILE_SIZE);
		}

		
		if (!bw->isWalkable((WalkPosition)drawLinePosition))
		{
			for (int i = 0; i < 3; i++)
			{
				drawLinePosition = getDirectionDistancePosition(drawLinePosition, (Position)theMap.Center(), 3 * TILE_SIZE);

				if (bw->isWalkable((WalkPosition)drawLinePosition))
					break;
			}
		}
		/*
			if (theMap.GetMiniTile((WalkPosition)drawLinePosition).Altitude() < 100)
		{
			for (int i = 0; i < 5; i++)
			{
				drawLinePosition = getDirectionDistancePosition(drawLinePosition, (Position)theMap.Center(), 3 * TILE_SIZE);

				if (bw->isWalkable((WalkPosition)drawLinePosition) && theMap.GetMiniTile((WalkPosition)drawLinePosition).Altitude() > 1792)
					break;
			}
		*/
		
		if (theMap.GetMiniTile((WalkPosition)drawLinePosition).Altitude() < 100)
		{
			for (int i = 0; i < 2; i++)
			{
				drawLinePosition = getDirectionDistancePosition(drawLinePosition, (Position)theMap.Center(), 3 * TILE_SIZE);

				if (bw->isWalkable((WalkPosition)drawLinePosition) && theMap.GetMiniTile((WalkPosition)drawLinePosition).Altitude() > 100)
					break;
			}
		}
	}
}

bool StrategyManager::needWait(UnitInfo *firstTank)
{
	
	if (firstTank == nullptr)
		return false;

	bool needWait = false;


	if (INFO.getAllCount(Terran_Command_Center, S) < 3)
	{
		int SCPToFirstWaitLineDistance = 0;
		int SCPToFirstTankDistance = 0;

		theMap.GetPath(INFO.getSecondChokePosition(S), INFO.getFirstWaitLinePosition(), &SCPToFirstWaitLineDistance);
		theMap.GetPath(INFO.getSecondChokePosition(S), firstTank->pos(), &SCPToFirstTankDistance);

	
		if (SCPToFirstTankDistance > SCPToFirstWaitLineDistance + 8 * TILE_SIZE)
			return true;
		else
			return false;
	}

	
	if ((INFO.getCompletedCount(Terran_Factory, S) + INFO.getDestroyedCount(Terran_Factory, S)) < 6
			&& (INFO.getCompletedCount(Terran_Dropship, S) + INFO.getDestroyedCount(Terran_Dropship, S)) < 3)
	{
		Position mainBase = Positions::Unknown;
		Position mySCP = INFO.getSecondChokePosition(S);
		Position enemySCP = INFO.getSecondChokePosition(E);
		Position center = theMap.Center();
		Position firstWaitLine = INFO.getFirstWaitLinePosition();
		UnitInfo *firstTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

		if (INFO.getMainBaseLocation(S) != nullptr)
			mainBase = INFO.getMainBaseLocation(S)->getPosition();

		if (firstTank != nullptr && mainBase != Positions::Unknown && firstWaitLine != Positions::Unknown && mySCP.isValid() && enemySCP.isValid())
		{
			int distanceFirstTank = 0;
			int distanceFirstWaitLine = 0;
			bool leftAndRight = false;

			if (abs(mySCP.x - enemySCP.x) > abs(mySCP.y - enemySCP.y))
				leftAndRight = true;

			theMap.GetPath(mainBase, firstTank->pos(), &distanceFirstTank);
			theMap.GetPath(mainBase, firstWaitLine, &distanceFirstWaitLine);

			if (!centerIsMyArea)
			{
				centerIsMyArea = (leftAndRight ? abs(center.x - firstTank->pos().x) < 8 * TILE_SIZE
								  : abs(center.y - firstTank->pos().y) < 8 * TILE_SIZE)
								 && distanceFirstTank > distanceFirstWaitLine + 6 * TILE_SIZE;
			}
			else
			{
				if (distanceFirstTank <= distanceFirstWaitLine + 6 * TILE_SIZE)
					centerIsMyArea = false;
			}
		}
		/*
		if (INFO.getAllCount(Terran_Command_Center, S) < 3)
	{
		int SCPToFirstWaitLineDistance = 0;
		int SCPToFirstTankDistance = 0;

		theMap.GetPath(INFO.getSecondChokePosition(S), INFO.getFirstWaitLinePosition(), &SCPToFirstWaitLineDistance);
		theMap.GetPath(INFO.getSecondChokePosition(S), firstTank->pos(), &SCPToFirstTankDistance);

	
		if (SCPToFirstTankDistance > SCPToFirstWaitLineDistance + 8 * TILE_SIZE)
			return true;
		//else

		else
		{
		if (distanceFirstTank <= distanceFirstWaitLine + 6 * TILE_SIZE)
		centerIsMyArea = false;
		}
			return false;
	}
		
		*/

		if (!centerIsMyArea)
			return false;
		else
			return true;
	}

	int distance = 0;
	const CPPath &path = theMap.GetPath(firstTank->pos(), INFO.getFirstExpansionLocation(E)->getPosition(), &distance);

	if (path.empty())
	{
		
		if (distance < 25 * TILE_SIZE)
		{
			needWait = true;
			
		}
	}
	else if (path.size() == 1)
	{
		
		auto chokePoint = path.begin();

		theMap.GetPath(firstTank->pos(), (Position)(*chokePoint)->Center(), &distance);

		if (distance < 25 * TILE_SIZE)
		{
			needWait = true;
			
		}

	}
	else if (path.size() == 2)
	{
		
		const ChokePoint *secondChokePoint = nullptr;
		const ChokePoint *thirdChokePoint = nullptr;

		if ((*path.begin())->Center().getApproxDistance((WalkPosition)INFO.getFirstExpansionLocation(E)->getPosition())
				< (*path.rbegin())->Center().getApproxDistance((WalkPosition)INFO.getFirstExpansionLocation(E)->getPosition()))
		{
			secondChokePoint = (*path.begin());
			thirdChokePoint = (*path.rbegin());
			/*
			Position start = (Position)(*thirdChokePoint->Geometry().begin());
			Position end = (Position)(*thirdChokePoint->Geometry().rbegin());
			*/
		}
		else
		{
			secondChokePoint = (*path.rbegin());
			thirdChokePoint = (*path.begin());
		}

		

		if (Position(secondChokePoint->Center()).getApproxDistance((Position)thirdChokePoint->Center()) < 20 * TILE_SIZE)
		{
			Position start = (Position)(*thirdChokePoint->Geometry().begin());
			Position end = (Position)(*thirdChokePoint->Geometry().rbegin());

			if (start.getApproxDistance(end) < 4 * TILE_SIZE)
			{
				theMap.GetPath(firstTank->pos(), (Position)thirdChokePoint->Center(), &distance);

				if (distance < 13 * TILE_SIZE)
				{
					needWait = true;
					
				}
			}
		}
	}
	else if (path.size() == 3)
	{
		const ChokePoint *closestChokePoint = nullptr;

		closestChokePoint = (*path.begin());

		if (INFO.getSecondChokePoint(E) != nullptr
				&& Position(closestChokePoint->Center()).getApproxDistance(INFO.getSecondChokePosition(E)) < 30 * TILE_SIZE)
		{
			

			Position start = (Position)(*closestChokePoint->Geometry().begin());
			Position end = (Position)(*closestChokePoint->Geometry().rbegin());
			/*
					Position start = (Position)(*thirdChokePoint->Geometry().begin());
			Position end = (Position)(*thirdChokePoint->Geometry().rbegin());
			*/

			if (start.getApproxDistance(end) < 4 * TILE_SIZE)
			{
				theMap.GetPath(firstTank->pos(), (Position)closestChokePoint->Center(), &distance);

				if (distance < 13 * TILE_SIZE)
				{
					needWait = true;
					
				}
			}
		}
	}
	else
	{
		
	}

	
	if (needWait)
	{
		if (INFO.getOccupiedBaseLocations(E).size() > 2)
		{
			surround = true;

			if (secondAttackPosition != Positions::Unknown && multiBreakDeathPosition == Positions::Unknown)
			{
				needAttackMulti = true;
			}
			else
			{
				needAttackMulti = false;
			}

			return true;
		}
	}
	else
	{
		surround = false;
	}

	needAttackMulti = false;

	return false;
}
/*
void StrategyManager::assignWorkersToUnassignedBuildings()
{
// for each building that doesn't have a builder, assign one
for (Building & b : _buildings)
{
if (b.status != BuildingStatus::Unassigned)
{
continue;
}

// BWAPI::Broodwar->printf("Assigning Worker To: %s", b.type.getName().c_str());

BWAPI::TilePosition testLocation = getBuildingLocation(b);
if (!testLocation.isValid())
{
continue;
}

b.finalPosition = testLocation;

// grab the worker unit from WorkerManager which is closest to this final position
b.builderUnit = WorkerManager::Instance().getBuilder(b);
if (!b.builderUnit || !b.builderUnit->exists())
{
continue;
}

// reserve this building's space
BuildingPlacer::Instance().reserveTiles(b.finalPosition,b.type.tileWidth(),b.type.tileHeight());

b.status = BuildingStatus::Assigned;
// BWAPI::Broodwar->printf("assigned and placed building %s", b.type.getName().c_str());
}
}
*/
bool StrategyManager::moreUnits(UnitInfo *firstTank)
{
	int myVultureCountInLine = 0;
	int myTankCountInLine = 0;
	int myGoliathCountInLine = 0;
	/*

	int enemyVultureCountInLine = INFO.getTypeUnitsInRadius(Terran_Vulture, E, drawLinePosition, 30 * TILE_SIZE, true).size();
	int enemyTankCountInLine = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, drawLinePosition, 30 * TILE_SIZE, true).size();
	int enemyGoliathCountInLine = INFO.getTypeUnitsInRadius(Terran_Goliath, E, drawLinePosition, 30 * TILE_SIZE, true).size();
	int enemyWraithCountInLine = INFO.getTypeUnitsInRadius(Terran_Wraith, E, drawLinePosition, 30 * TILE_SIZE, true).size();
	*/
	for (auto g : INFO.getTypeUnitsInRadius(Terran_Goliath, S, waitLinePosition, 30 * TILE_SIZE, true))
		myVultureCountInLine++;

	
	for (auto t : INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, waitLinePosition, 30 * TILE_SIZE, true))
		if (t->getState() == "SiegeLineState")
			myTankCountInLine++;

	for (auto g : INFO.getTypeUnitsInRadius(Terran_Goliath, S, waitLinePosition, 30 * TILE_SIZE, true))
		if (g->getState() == "VsTerran" || g->getState() == "ProtectTank")
			myGoliathCountInLine++;

	int enemyVultureCountInLine = INFO.getTypeUnitsInRadius(Terran_Vulture, E, drawLinePosition, 30 * TILE_SIZE, true).size();
	int enemyTankCountInLine = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, drawLinePosition, 30 * TILE_SIZE, true).size();
	int enemyGoliathCountInLine = INFO.getTypeUnitsInRadius(Terran_Goliath, E, drawLinePosition, 30 * TILE_SIZE, true).size();
	int enemyWraithCountInLine = INFO.getTypeUnitsInRadius(Terran_Wraith, E, drawLinePosition, 30 * TILE_SIZE, true).size();

	
	if (enemyWraithCountInLine && !myGoliathCountInLine)
	{

		return false;
	}

	
	double higherUpgrade = 1.0;

	if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) >= 2 && E->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 2)
		higherUpgrade = 1.2;

	
	if ((int)(myTankCountInLine * higherUpgrade) < (int)(enemyTankCountInLine * 1.3))
	{
		if ((myTankCountInLine * higherUpgrade) + (myGoliathCountInLine * 0.5) + (myVultureCountInLine * 0.3)
				< (enemyTankCountInLine * 1.3) + (enemyVultureCountInLine * 0.3) + (enemyGoliathCountInLine * 0.5))
		{
			

			return false;
		}
	}
	else
	{
		
		if (myTankCountInLine < 3)
		{

			return false;
		}
	}


	return true;
}

void StrategyManager::checkSecondExpansion()
{
	if (keepMultiDeathPosition != Positions::Unknown)
		bw->drawCircleMap(keepMultiDeathPosition, 15 * TILE_SIZE, Colors::Cyan);

	if (TIME % 24 != 0)
		return;

	
	if (INFO.getAllCount(Terran_Command_Center, S) < 2)
		return;

	if (!S->hasResearched(TechTypes::Tank_Siege_Mode))
		return;

	if (INFO.getSecondExpansionLocation(S) == nullptr)
		return;

	if (!INFO.getFirstWaitLinePosition().isValid())
		return;

	int myUnitCount = INFO.getUnitsInRadius(S, INFO.getFirstWaitLinePosition(), 20 * TILE_SIZE, true, true).size();
	int enemyUnitCount = INFO.getUnitsInRadius(E, INFO.getFirstWaitLinePosition(), 20 * TILE_SIZE, true, true).size();

	
	if (enemyUnitCount > 0 && myUnitCount <= enemyUnitCount)
		return;

	
	if (INFO.enemyRace == Races::Terran)
	{
		if (keepMultiDeathPosition != Positions::Unknown)
		{
			int eVulture = INFO.getTypeUnitsInRadius(Terran_Vulture, E, keepMultiDeathPosition, 10 * TILE_SIZE, true).size();
			int eTank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, keepMultiDeathPosition, 15 * TILE_SIZE, true).size();
			int eWraith = INFO.getTypeUnitsInRadius(Terran_Wraith, E, keepMultiDeathPosition, 10 * TILE_SIZE, true).size();
			int eGoliath = INFO.getTypeUnitsInRadius(Terran_Goliath, E, keepMultiDeathPosition, 10 * TILE_SIZE, true).size();

			int enemyPoint = eTank * 10 + eVulture * 3 + eGoliath * 5 + eWraith * 3;

			if (enemyPoint >= 30) {
				needSecondExpansion = false;
				return;
			}

			keepMultiDeathPosition = Positions::Unknown;
		}

		if (needSecondExpansion == false && S->minerals() > 600 && INFO.getAllCount(Terran_Factory, S) >= 4
				&& GM.getUsableGoliathCnt() > 3 && TM.getUsableTankCnt() > 5)
		{
			needSecondExpansion = true;
		}
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		if (needSecondExpansion == false && S->minerals() > 600
				&& (INFO.getCompletedCount(Terran_Goliath, S) + INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S)) > 10)
		{
			needSecondExpansion = true;
		}
	}
	else
	{
		if (ESM.getEnemyMainBuild() == Toss_fast_carrier)
		{
			if (needSecondExpansion == false && S->minerals() > 600 && INFO.getCompletedCount(Terran_Factory, S) >= 4
					&& INFO.getCompletedCount(Terran_Goliath, S) > 4)
			{
				needSecondExpansion = true;
			}
		}
	}

	if (needSecondExpansion == false && INFO.getMainBaseLocation(S) != nullptr
			&& INFO.getAverageMineral(INFO.getMainBaseLocation(S)->getPosition()) < 450)
	{
		if (INFO.enemyRace == Races::Protoss && SM.getMainStrategy() != AttackAll)
			return;

		needSecondExpansion = true;
	}


	if (needSecondExpansion)
	{
		killMine(INFO.getSecondExpansionLocation(S));
	}
}
/*
void StrategyManager::setRepairWorker(BWAPI::Unit worker, BWAPI::Unit unitToRepair)
{
workerData.setWorkerJob(worker, WorkerData::Repair, unitToRepair);
}

void WorkerManager::stopRepairing(BWAPI::Unit worker)
{
workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
}

void WorkerManager::handleGasWorkers()
{
for (const auto unit : BWAPI::Broodwar->self()->getUnits())
{
// if that unit is a refinery
if (unit->getType().isRefinery() && unit->isCompleted())
{
// Don't collect gas if gas collection is off, or if the resource depot is missing.
if (_collectGas && refineryHasDepot(unit))
{
// Gather gas: If too few are assigned, add more.
int numAssigned = workerData.getNumAssignedWorkers(unit);
for (int i = 0; i < (Config::Macro::WorkersPerRefinery - numAssigned); ++i)
{
BWAPI::Unit gasWorker = getGasWorker(unit);
if (gasWorker)
{
workerData.setWorkerJob(gasWorker, WorkerData::Gas, unit);
}
else
{
return;    // won't find any more, either for this refinery or others
}
}
}
else
{
// Don't gather gas: If any workers are assigned, take them off.
std::set<BWAPI::Unit> gasWorkers;
workerData.getGasWorkers(gasWorkers);
for (const auto gasWorker : gasWorkers)
{
if (gasWorker->getOrder() != BWAPI::Orders::HarvestGas)    // not inside the refinery
{
workerData.setWorkerJob(gasWorker, WorkerData::Idle, nullptr);
// An idle worker carrying gas will become a ReturnCargo worker,
// so gas will not be lost needlessly.
}
}
}
}
}
}

// Is the refinery near a resource depot that it can deliver gas to?
bool WorkerManager::refineryHasDepot(BWAPI::Unit refinery)
{
// Iterate through units, not bases, because even if the main hatchery is destroyed
// (so the base is considered gone), a macro hatchery may be close enough.
// TODO could iterate through bases (from InfoMan) instead of units
for (const auto unit : BWAPI::Broodwar->self()->getUnits())
{
if (unit->getType().isResourceDepot() &&
(unit->isCompleted() || unit->getType() == BWAPI::UnitTypes::Zerg_Lair || unit->getType() == BWAPI::UnitTypes::Zerg_Hive) &&
unit->getDistance(refinery) < 400)
{
return true;
}
}

return false;
}

void StrategyManager::handleIdleWorkers()
{
for (const auto worker : workerData.getWorkers())
{
UAB_ASSERT(worker != nullptr, "Worker was null");

if (workerData.getWorkerJob(worker) == WorkerData::Idle)
{
if (worker->isCarryingMinerals() || worker->isCarryingGas())
{
// It's carrying something, set it to hand in its cargo.
setReturnCargoWorker(worker);         // only happens if there's a resource depot
}
else {
// Otherwise send it to mine minerals.
setMineralWorker(worker);             // only happens if there's a resource depot
}
}
}
}


*/
void StrategyManager::checkThirdExpansion()
{
	if (TIME % 24 != 0)
		return;

	
	if (INFO.getCompletedCount(Terran_Command_Center, S) < 3)
		return;

	if (INFO.getThirdExpansionLocation(S) == nullptr)
		return;

	if (!INFO.getThirdExpansionLocation(S)->Center().isValid())
		return;

	if (INFO.enemyRace == Races::Terran)
	{
		int myUnitCountInFirstWaitLine = INFO.getUnitsInRadius(S, INFO.getFirstWaitLinePosition(), 20 * TILE_SIZE, true, true).size();
		int enemyUnitCountInFirstWaitLine = INFO.getUnitsInRadius(E, INFO.getFirstWaitLinePosition(), 20 * TILE_SIZE, true, true).size();

		
		if (enemyUnitCountInFirstWaitLine > 0 && myUnitCountInFirstWaitLine <= enemyUnitCountInFirstWaitLine)
			return;

		UnitInfo *firstTank = TM.getFrontTankFromPos(getMainAttackPosition());
		int firstTankDistanceFromEnemySCP = 0;

		if (firstTank != nullptr && INFO.getSecondChokePosition(E).isValid())
		{
			firstTankDistanceFromEnemySCP = getGroundDistance(firstTank->pos(), INFO.getSecondChokePosition(E));
		}

		
		if (needThirdExpansion == false && S->minerals() > 600
				&& (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) >= 2 || centerIsMyArea || surround
					|| firstTankDistanceFromEnemySCP < 30 * TILE_SIZE)
				&& GM.getUsableGoliathCnt() > 4 && TM.getUsableTankCnt() > 7)
		{

			needThirdExpansion = true;
		}
		/*
		int myUnitCountInFirstWaitLine = INFO.getUnitsInRadius(S, INFO.getFirstWaitLinePosition(), 20 * TILE_SIZE, true, true).size();
		int enemyUnitCountInFirstWaitLine = INFO.getUnitsInRadius(E, INFO.getFirstWaitLinePosition(), 20 * TILE_SIZE, true, true).size();


		if (enemyUnitCountInFirstWaitLine > 0 && myUnitCountInFirstWaitLine <= enemyUnitCountInFirstWaitLine)
		return;

		UnitInfo *firstTank = TM.getFrontTankFromPos(getMainAttackPosition());
		int firstTankDistanceFromEnemySCP = 0;

		if (firstTank != nullptr && INFO.getSecondChokePosition(E).isValid())
		{
		firstTankDistanceFromEnemySCP = getGroundDistance(firstTank->pos(), INFO.getSecondChokePosition(E));
		}

		*/
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		if (needThirdExpansion == false && S->minerals() > 600
				&& (INFO.getCompletedCount(Terran_Vulture, S)
					+ INFO.getCompletedCount(Terran_Goliath, S)
					+ INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S)) > 20)
		{
			needThirdExpansion = true;
		}
	}
	else
	{
		if (needThirdExpansion == false && INFO.getCompletedCount(Terran_Command_Center, S) >= 3)
		{
			needThirdExpansion = true;
		}
	}

	if (needSecondExpansion == false && INFO.getFirstExpansionLocation(S) != nullptr
			&& INFO.getAverageMineral(INFO.getFirstExpansionLocation(S)->getPosition()) < 450)
	{
		needThirdExpansion = true;
	}
	/*
	if (needThirdExpansion == false && INFO.getCompletedCount(Terran_Command_Center, S) >= 3)
	{
	needThirdExpansion = true;
	}
	}

	if (needSecondExpansion == false && INFO.getFirstExpansionLocation(S) != nullptr
	&& INFO.getAverageMineral(INFO.getFirstExpansionLocation(S)->getPosition()) < 450)
	{
	needThirdExpansion = true;
	}
	*/

	if (needThirdExpansion)
	{
		killMine(INFO.getThirdExpansionLocation(S));
	}
}

bool StrategyManager::needAdditionalExpansion()
{
	if (INFO.getOccupiedBaseLocations(S).size() < 4)
		return false;

	
	if (TIME % (24 * 10) != 0)
		return false;

	Base *firstMulti = INFO.getFirstMulti(S);
	Base *firstGasMulti = INFO.getFirstMulti(S, true);

	
	if (firstGasMulti == nullptr && firstMulti == nullptr)
		return false;

	/*
	if (TIME % (24 * 60) != 0)
		return false;

	if (INFO.enemyRace == Races::Terran)
	{
		if (surround || centerIsMyArea)
		{
			
			if (TIME % (24 * 60 * 2) != 0)
				return true;
		}
		else
		{
			
			if (TIME % (24 * 60 * 3) != 0)
				return true;
		}
	}
	else
	{
		
		if (INFO.getActivationMineralBaseCount() < 4)
		{
			
			if (TIME % (24 * 60 * 2 * 2) != 0)
				return true;
		}
	}
	
	*/
	for (int i = 1; i < 3; i++)
	{
		Base *multi = INFO.getFirstMulti(S, false, false, i);

		if (multi == nullptr)
			return false;

		killMine(multi);
	}

	if (TIME % (24 * 60) != 0)
		return false;

	if (INFO.enemyRace == Races::Terran)
	{
		if (surround || centerIsMyArea)
		{
			
			if (TIME % (24 * 60 * 2) != 0)
				return true;
		}
		else
		{
			
			if (TIME % (24 * 60 * 3) != 0)
				return true;
		}
	}
	else
	{
		
		if (INFO.getActivationMineralBaseCount() < 4)
		{
			
			if (TIME % (24 * 60 * 2 * 2) != 0)
				return true;
		}
	}

	return false;
}

void StrategyManager::checkKeepExpansion()
{
	needKeepMultiSecond = false;
	needKeepMultiThird = false;

	needKeepMultiSecondAir = false;
	needKeepMultiThirdAir = false;

	needMineSecond = false;
	needMineThird = false;

	if (INFO.getSecondExpansionLocation(S) && INFO.getSecondExpansionLocation(S)->GetOccupiedInfo() == myBase) // 내 베이스
	{
		if (INFO.getUnitsInRadius(E, INFO.getSecondExpansionLocation(S)->Center(), 12 * TILE_SIZE, true, false, true, true).size())
			needKeepMultiSecond = true;

		if (INFO.getUnitsInRadius(E, INFO.getSecondExpansionLocation(S)->Center(), 12 * TILE_SIZE, false, true, true, true).size())
			needKeepMultiSecondAir = true;

		if (INFO.getSecondExpansionLocation(S)->GetMineCount() < 8)
			needMineSecond = true;

		if (INFO.getSecondExpansionLocation(S)->GetMineCount() > 11)
			needMineSecond = false;
	}

	if (INFO.getThirdExpansionLocation(S) && INFO.getThirdExpansionLocation(S)->GetOccupiedInfo() == myBase) // 내 베이스
	{
		if (INFO.getUnitsInRadius(E, INFO.getThirdExpansionLocation(S)->Center(), 12 * TILE_SIZE, true, false, true, true).size())
			needKeepMultiThird = true;

		if (INFO.getUnitsInRadius(E, INFO.getThirdExpansionLocation(S)->Center(), 12 * TILE_SIZE, false, true, true, true).size())
			needKeepMultiThirdAir = true;

		if (INFO.getThirdExpansionLocation(S)->GetMineCount() < 8)
			needMineThird = true;

		if (INFO.getThirdExpansionLocation(S)->GetMineCount() > 11)
			needMineThird = false;
	}
}

void StrategyManager::killMine(Base *base)
{
	if (base == nullptr)
		return;

	uList mine = INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, base->getPosition(), 5 * TILE_SIZE);

	if (mine.empty())
		return;

	for (auto m : mine)
	{
		Position newPos = m->pos();
	
		Position c = (Position)base->getTilePosition();
		Position cr = c + (Position)Terran_Command_Center.tileSize();

		if (newPos.x >= c.x - 16 && newPos.y >= c.y - 16
				&& newPos.x <= cr.x + 16 && newPos.y <= cr.y + 16)
		{
			if (m->getKillMe() != MustKill && m->getKillMe() != MustKillAssigned)
			{
				m->setKillMe(MustKill);
				continue;
			}
		}

		
		Position cs = Position(cr.x, c.y + 32);
		Position csr = cs + (Position)Terran_Comsat_Station.tileSize();

		if (newPos.x >= cs.x - 16 && newPos.y >= cs.y - 16
				&& newPos.x <= csr.x + 16 && newPos.y <= csr.y + 16)
		{
			if (m->getKillMe() != MustKill && m->getKillMe() != MustKillAssigned)
			{
				m->setKillMe(MustKill);
				continue;
			}
		}
	}
}


bool MyBot::StrategyManager::decideToBuildWhenDeadLock(UnitType *)
{
	return true;
}

MyBuildType StrategyManager::getMyBuild()
{
	return myBuild;
}

bool StrategyManager::getNeedUpgrade()
{
	return needUpgrade;
}

bool StrategyManager::getNeedTank()
{
	return needTank;
}

Position StrategyManager::getMainAttackPosition()
{
	return mainAttackPosition;
}

Position StrategyManager::getSecondAttackPosition()
{
	return secondAttackPosition;
}

Position StrategyManager::getDrawLinePosition()
{
	return drawLinePosition;
}

Position StrategyManager::getWaitLinePosition()
{
	return waitLinePosition;
}

void StrategyManager::decideDropship()
{
	if (INFO.enemyRace == Races::Terran)
	{
		

		if (INFO.getCompletedCount(Terran_Dropship, S) >= 3 && TM.enoughTankForDrop() && GM.enoughGoliathForDrop()) 
		{
			dropshipMode = true;
		}
		else
		{
			dropshipMode = false;
		}
	}
}

bool StrategyManager::checkTurretFirst() {

	
	if (INFO.enemyRace == Races::Protoss)
		if (TIME < 5 * 24 * 60) {
			if (CM.existConstructionQueueItem(Terran_Missile_Turret) ||
					BM.buildQueue.existItem(Terran_Missile_Turret))

				
				if (TrainManager::Instance().getAvailableMinerals() < 60) {
					return true;
				}
		}

	return false;

}


void StrategyManager::checkNeedDefenceWithScv()
{
	needDefenceWithScv = false;

	if (S->hasResearched(TechTypes::Tank_Siege_Mode) || INFO.hasRearched(TechTypes::Spider_Mines))
		return;

	vector<UnitType> types = { Terran_Siege_Tank_Tank_Mode, Terran_Goliath };
	UnitInfo *myFrontUnit = INFO.getClosestTypeUnit(S, mainAttackPosition, types, 0, false, true);

	if (myFrontUnit == nullptr)
		return;

	
	if (!isSameArea(myFrontUnit->pos(), MYBASE))
	{
		if (INFO.getClosestTypeUnit(E, myFrontUnit->pos(), Terran_Siege_Tank_Tank_Mode, 15 * TILE_SIZE, true) == nullptr)
			return;

		if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) + INFO.getCompletedCount(Terran_Goliath, S) <=
				INFO.getCompletedCount(Terran_Vulture, E) / 2 + INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, E) + INFO.getCompletedCount(Terran_Goliath, E))
			needDefenceWithScv = true;
	}
}
void StrategyManager::onUnitShow(BWAPI::Unit unit)
{
	/*
	UAB_ASSERT(unit && unit->exists(), "bad unit");

	// add the depot if it exists
	if (unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
	workerData.addDepot(unit);
	}

	// if something morphs into a worker, add it
	if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self() && unit->getHitPoints() > 0)
	{
	workerData.addWorker(unit);
	}
	*/

}
/*
void StrategyManager::rebalanceWorkers()
{
for (const auto worker : workerData.getWorkers())
{
UAB_ASSERT(worker != nullptr, "Worker was null");

if (!workerData.getWorkerJob(worker) == WorkerData::Minerals)
{
continue;
}

BWAPI::Unit depot = workerData.getWorkerDepot(worker);

if (depot && workerData.depotIsFull(depot))
{
workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
}
else if (!depot)
{
workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
}
}
}
*/


void StrategyManager::onUnitDestroy(BWAPI::Unit unit)
{
	/*
	UAB_ASSERT(unit != nullptr, "Unit was null");

	if (unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
	workerData.removeDepot(unit);
	}

	if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
	workerData.workerDestroyed(unit);
	}

	if (unit->getType() == BWAPI::UnitTypes::Resource_Mineral_Field)
	{
	rebalanceWorkers();
	}
	*/
	
}

void StrategyManager::drawResourceDebugInfo()
{
	/*
	if (!Config::Debug::DrawResourceInfo)
	{
	return;
	}

	for (const auto worker : workerData.getWorkers())
	{
	//	UAB_ASSERT(worker != nullptr, "Worker was null");

	char job = workerData.getJobCode(worker);

	BWAPI::Position pos = worker->getTargetPosition();

	BWAPI::Broodwar->drawTextMap(worker->getPosition().x, worker->getPosition().y - 5, "\x07%c", job);
	BWAPI::Broodwar->drawTextMap(worker->getPosition().x, worker->getPosition().y + 5, "\x03%s", worker->getOrder().getName().c_str());

	BWAPI::Broodwar->drawLineMap(worker->getPosition().x, worker->getPosition().y, pos.x, pos.y, BWAPI::Colors::Cyan);

	//	BWAPI::Unit depot = workerData.getWorkerDepot(worker);
	if (depot)
	{
	BWAPI::Broodwar->drawLineMap(worker->getPosition().x, worker->getPosition().y, depot->getPosition().x, depot->getPosition().y, BWAPI::Colors::Orange);
	}
	}
	*/

}

void StrategyManager::drawWorkerInformation(int x, int y)
{
	BWAPI::Broodwar->drawTextScreen(x, y + 20, "\x04 UnitID");
	BWAPI::Broodwar->drawTextScreen(x + 50, y + 20, "\x04 State");

	int yspace = 0;
}

bool StrategyManager::isFree(BWAPI::Unit worker)
{
}

