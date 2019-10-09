#include "SoldierManager.h"
#include "../HostileManager.h"
#include "TankManager.h"
#include "MarineManager.h"
#include "../TrainManager.h"
#include <time.h>



using namespace MyBot;

SoldierManager::SoldierManager()
{
	refineryScvCnt = 3;
	repairScvCnt = 1;
	MaxRepairScvCnt = 10; 
	scanMyBaseUnit = nullptr;
	totalScoutUnitDefenceScvCount = 0;
	totalEarlyRushDefenceScvCount = 0;
	MineralStateScvCount = 0;
	GasStateScvCount = 0;
	RepairStateScvCount = 0;
	memset(time, 0, sizeof(time));
}

SoldierManager &SoldierManager::Instance()
{
	static SoldierManager instance;
	return instance;
}

void SoldierManager::update()
{
	if (TIME % FR != 0)	{
		return;
	}

	CheckDepot(); 
	CheckRefineryScv(); 
	CheckRepairScv(); 
	CheckBunkerDefence();
	
	CheckCombatSet();
	CheckWorkerDefence();
	CheckEnemyScoutUnit();
	CheckEarlyRush();
	CheckNightingale();

	int factoryCnt = 4;

	
	if (INFO.enemyRace == Races::Protoss)
	{
		factoryCnt = 6;
	}

	if (checkBase == Positions::None)
	{
		checkBase = MYBASE;
	}

	if (blockingRemoved && !checkBaseUpdated && INFO.getSecondExpansionLocation(S) != nullptr)
	{
		checkBase = INFO.getSecondExpansionLocation(S)->Center();
		checkBaseUpdated = true;
		blockingRemoved = false;
	}

	if (!blockingRemoved  && INFO.getAllCount(Terran_Factory, S) >= factoryCnt)
	{
		CheckRemoveBlocking(checkBase);
	}

	uList ScvList = INFO.getUnits(Terran_SCV, S);

	
	for (auto scv : ScvList) {
		if (scv->getState() == "New")
			setStateToSCV(scv, new ScvIdleState());
	}

	for (auto scv : ScvList)
	{
		string state = scv->getState();

		if (state == "Idle")
		{
			setMineralScv(scv);
		}
		else if (state == "BunkerDefence") {

			Unit bunker = MM.getBunker();

			if (bunker == nullptr || scv->hp() < 35)
				setMineralScv(scv);
		}
		else if (state == "Combat")
		{
			if (scv->hp() < 35) {
				if (INFO.enemyRace == Races::Protoss) {
					UnitInfo *closestUnit = INFO.getClosestUnit(E, scv->pos(), GroundCombatKind, 3 * TILE_SIZE, true);

					if (!closestUnit)
						setMineralScv(scv);
				}
				else if (INFO.enemyRace == Races::Zerg && (ESM.getEnemyInitialBuild() <= Zerg_5_Drone || ESM.getEnemyInitialBuild() == Zerg_9_Hat)) {
					if (INFO.getCompletedCount(Terran_SCV, S) > 6)
						setMineralScv(scv);
				}
				else
					setMineralScv(scv);
			}
		}
		else if (state == "Repair")
		{
			if ( ( ( (ESM.getEnemyInitialBuild() == Toss_4probes || ESM.getEnemyInitialBuild() == Terran_4scv || ESM.getEnemyInitialBuild() == Zerg_4_Drone_Real || ESM.getEnemyInitialBuild() <= Zerg_9_Balup || ESM.getEnemyInitialBuild() == Terran_bunker_rush || ESM.getEnemyInitialBuild() == Toss_cannon_rush || ESM.getEnemyInitialBuild() == Terran_2b_forward)
					 && ESM.getEnemyMainBuild() == UnknownMainBuild)
					|| ESM.getEnemyMainBuild() == Zerg_main_zergling) )
			{

				if (scv->unit()->getLastCommand().getType() == UnitCommandTypes::Repair)
				{
					if (scv->unit()->getLastCommand().getTarget() != nullptr && scv->unit()->getLastCommand().getTarget()->exists() && scv->unit()->getLastCommand().getTarget()->getType() == Terran_SCV)
					{
						UnitInfo *targetScv = INFO.getUnitInfo(scv->unit()->getLastCommand().getTarget(), S);

						if (targetScv->getState() != "Mineral")
						{
							setMineralScv(scv);
							continue;
						}
					}
				}

				UnitInfo *closestUnit = INFO.getClosestUnit(E, scv->pos(), GroundUnitKind, 3 * TILE_SIZE);

				if (closestUnit || scv->unit()->isUnderAttack())
				{
					setMineralScv(scv);
				}
			}
		}

		if (state != "Combat" && state != "Nightingale" && state != "RemoveBlocking" && state != "FirstChokeDefence" && state != "BunkerDefence")
		{
			scv->action();
		}
	}


	combatSet.action();
	nightingaleSet.action();
	firstChokeDefenceSet.action();
	bunkerDefenceSet.action();
	removeBlockingSet.action(checkBase);
}

void	SoldierManager::setMineralScv(UnitInfo *scv, Unit center)
{
	
	if (center == nullptr)
		center = getNearestDepot(scv);

	if (center == nullptr)
	{
		
		if (!scv->unit()->isGatheringMinerals())
		{
			Unit tempMineral = getTemporaryMineral(scv->pos());

			if (tempMineral != nullptr) {
				setStateToSCV(scv, new ScvIdleState());
				CommandUtil::gather(scv->unit(), tempMineral);
			}
		}
	}
	else
	{
		
		Unit mineral = getBestMineral(center);

		if (mineral != nullptr)
		{
		
			setStateToSCV(scv, new ScvMineralState(mineral));

			
			if (scv->unit()->canReturnCargo())
				scv->unit()->returnCargo();
			else
				CommandUtil::gather(scv->unit(), mineral);
		}
		else
		{
			if (!scv->unit()->isGatheringMinerals())
			{
				Unit tempMineral = getTemporaryMineral(scv->pos());

				if (tempMineral != nullptr) {
					setStateToSCV(scv, new ScvIdleState());
					CommandUtil::gather(scv->unit(), tempMineral);
				}
			}
		}
	}
}


Unit SoldierManager::getBestMineral(Unit depot)
{
	if (depotMineralMap[depot].size() == 0)
		return nullptr;

	for (int depth = 0; depth < 2; depth++)
	{
		for (auto m : depotMineralMap[depot])
		{
			if (getMineralScvCount(m) == depth)
				return m;
		}
	}

	return nullptr;
}


Unit SoldierManager::getNearestDepot(UnitInfo *scv)
{
	Unit depot = nullptr;
	int dist = INT_MAX;

	uList DepotList = INFO.getBuildings(Terran_Command_Center, S);

	if (DepotList.size() == 1)
		return DepotList[0]->unit();

	for (auto c : DepotList)
	{
		
		if (c->unit()->isFlying() || !c->isComplete() || depotHasEnoughMineralWorkers(c->unit()))
			continue;

		if (depotBaseMap[c->unit()] != nullptr && depotBaseMap[c->unit()]->GetDangerousAreaForWorkers())
			continue;

		int tmp_dist = -1;
		theMap.GetPath(scv->pos(), c->pos(), &tmp_dist);

		if ( tmp_dist < dist && tmp_dist > 0)
		{
			dist = tmp_dist;
			depot = c->unit();
		}
	}

	return depot;
}


Unit SoldierManager::getTemporaryMineral(Position pos)
{
	Unit depot = nullptr;
	int dist = INT_MAX;

	uList DepotList = INFO.getBuildings(Terran_Command_Center, S);

	if (DepotList.size() == 1)
		return DepotList[0]->unit()->getClosestUnit(Filter::IsMineralField);

	for (auto c : DepotList)
	{
		if (c->unit()->isFlying() || !c->isComplete())
			continue;

		if (depotMineralMap[c->unit()].size() == 0)
			continue;

		if (depotBaseMap[c->unit()] != nullptr && depotBaseMap[c->unit()]->GetDangerousAreaForWorkers())
			continue;

		int tmp_dist = -1;
		theMap.GetPath(pos, c->pos(), &tmp_dist);

		if (tmp_dist < dist && tmp_dist > 0)
		{
			dist = tmp_dist;
			depot = c->unit();
		}
	}

	if (depot != nullptr)
		return depot->getClosestUnit(Filter::IsMineralField);
	else
		return bw->getClosestUnit(pos, Filter::IsMineralField);

	return nullptr;
}


bool	SoldierManager::isDepotNear(Unit refinery)
{
	uList DepotList = INFO.getBuildings(Terran_Command_Center, S);

	for (auto center : DepotList)
	{
		if (center->isComplete() && center->getLift() == false && refinery->getDistance(center->pos()) < 10 * TILE_SIZE &&
				depotBaseMap[center->unit()] != nullptr && depotBaseMap[center->unit()]->GetDangerousAreaForWorkers() == false)
			return true;
	}

	return false;
}


void SoldierManager::initMineralsNearDepot(Unit depot)
{
	if (depotMineralMap.find(depot) == depotMineralMap.end())
		depotMineralMap[depot] = vector<Unit>();
	else
		depotMineralMap[depot].resize(0);

	vector<pair<int, Unit>> sortVect;


	Base *baselocation = INFO.getNearestBaseLocation(depot->getPosition());

	
	if (depot->getDistance(baselocation->getPosition()) > 5 * TILE_SIZE) {
		depotBaseMap[depot] = nullptr;
		return;
	}

	depotBaseMap[depot] = baselocation;
	baseSafeMap[baselocation] = true;

	for (auto m : baselocation->Minerals())
	{
		
		sortVect.push_back(pair<int, Unit>(m->Pos().getApproxDistance(depot->getPosition()), m->Unit()));
	}

	if ( sortVect.size() )
	{
		sort(sortVect.begin(), sortVect.end(), [](pair<int, Unit> a, pair<int, Unit> b) {
			return a.first < b.first;
		} );
		
		vector<pair<int, Unit>>::iterator iter;

		for (iter = sortVect.begin(); iter != sortVect.end(); iter++)
			depotMineralMap[depot].push_back(iter->second);
	}
}


bool SoldierManager::depotHasEnoughMineralWorkers(Unit depot)
{
	for (auto m : depotMineralMap[depot])
	{
		if (m->exists() && getMineralScvCount(m) < 2)
			return false;
	}

	return true;
}


int		SoldierManager::getMineralScvCount(Unit m)
{
	if (mineralScvCountMap.find(m) == mineralScvCountMap.end())
		return mineralScvCountMap[m] = 0;
	else
		return mineralScvCountMap[m];
}


int		SoldierManager::getRefineryScvCount(Unit m)
{
	if (refineryScvCountMap.find(m) == refineryScvCountMap.end())
		return refineryScvCountMap[m] = 0;
	else
		return refineryScvCountMap[m];
}


int		SoldierManager::getRepairScvCount(Unit m)
{
	if (repairScvCountMap.find(m) == repairScvCountMap.end())
		return repairScvCountMap[m] = 0;
	else
		return repairScvCountMap[m];
}


int		SoldierManager::getScoutUnitDefenceScvCountMap(Unit m)
{
	if (scoutUnitDefenceScvCountMap.find(m) == scoutUnitDefenceScvCountMap.end())
		return scoutUnitDefenceScvCountMap[m] = 0;
	else
		return scoutUnitDefenceScvCountMap[m];
}


int		SoldierManager::getEarlyRushDefenceScvCount(Unit m)
{
	if (earlyRushDefenceScvCountMap.find(m) == earlyRushDefenceScvCountMap.end())
		return earlyRushDefenceScvCountMap[m] = 0;
	else
		return earlyRushDefenceScvCountMap[m];
}


int		SoldierManager::getAssignedScvCount(Unit depot)
{
	if (!depot->isCompleted() || depot->isFlying())
		return 0;

	int scvCnt = 0;

	for (auto m : depotMineralMap[depot])
		scvCnt += getMineralScvCount(m);

	return scvCnt;
}


int SoldierManager::getRemainingAverageMineral(Unit depot)
{
	int totalMineral = 0;

	if (depotMineralMap[depot].empty())
		return 0;

	for (auto m : depotMineralMap[depot])
	{
		totalMineral += m->getResources();
	}

	return totalMineral / depotMineralMap[depot].size();
}


void	SoldierManager::CheckRefineryScv()
{
	if (MineralStateScvCount + GasStateScvCount < 10 && INFO.getCompletedCount(Terran_SCV, S) < 15)
		setNeedCountForRefinery(0);
	else 	if (S->gas() < 88 && INFO.getAllCount(Terran_Factory, S) < 1)
		setNeedCountForRefinery(3);
	else if (INFO.getAllCount(Terran_Command_Center, S) == 1 && S->supplyUsed() < 52)
	{
		if (INFO.getAllCount(Terran_Machine_Shop, S) > 0)
			setNeedCountForRefinery(2);
		else
			setNeedCountForRefinery(1);
	}
	else
		setNeedCountForRefinery(3);

	for (auto r : INFO.getBuildings(Terran_Refinery, S))
	{
		if (!r->isComplete())
			continue;

		
		if (!isDepotNear(r->unit()))
			continue;

		
		if (getRefineryScvCount(r->unit()) < refineryScvCnt)
		{
			UnitInfo *assignedScv = chooseScvClosestTo(r->pos());

			setStateToSCV(assignedScv, new ScvGasState(r->unit()));
		}
		else if (getRefineryScvCount(r->unit()) > refineryScvCnt)
		{
			uList ScvList = INFO.getUnits(Terran_SCV, S);

			for (auto scv : ScvList)
			{
				
				if (scv->getState() == "Gas" && scv->getTarget() == r->unit() && !scv->unit()->isCarryingGas() && scv->pos().getApproxDistance(r->pos()) > 80)
				{
					setStateToSCV(scv, new ScvIdleState());

					
					if (refineryScvCountMap[r->unit()] == refineryScvCnt)
						return;
				}
			}
		}
		else {}
	}
}


void SoldierManager::CheckDepot(void) {
	for (auto c : INFO.getBuildings(Terran_Command_Center, S))
	{
		if (!c->isComplete() && !c->unit()->isUnderAttack() && (c->hp() == 1300 || c->hp() == 1301))
		{
			Base *baselocation = INFO.getNearestBaseLocation(c->pos());

			if (c->unit()->getDistance(baselocation->getPosition()) > 5 * TILE_SIZE) {
				continue;
			}

			preRebalancingNewDepot(c->unit(), baselocation->Minerals().size());
		}

		if (!c->isComplete())
			continue;

	
		if (!c->unit()->isFlying() && !c->getLift()) {
			if (depotBaseMap[c->unit()] != nullptr && depotMineralMap[c->unit()].size() != depotBaseMap[c->unit()]->Minerals().size()) {
				initMineralsNearDepot(c->unit());
				rebalancingNewDepot(c->unit());
			}
		}

		if (depotBaseMap[c->unit()] != nullptr)
		{
			if (baseSafeMap[depotBaseMap[c->unit()]] != depotBaseMap[c->unit()]->GetDangerousAreaForWorkers())
			{
				
				if (depotBaseMap[c->unit()]->GetDangerousAreaForWorkers())
				{
					if (isExistSafeBase())
						setIdleAroundDepot(c->unit());
				}
				else 
					rebalancingNewDepot(c->unit());
			}

			baseSafeMap[depotBaseMap[c->unit()]] = depotBaseMap[c->unit()]->GetDangerousAreaForWorkers();
		}
	}
}

bool SoldierManager::isExistSafeBase(void)
{
	for (auto c : INFO.getBuildings(Terran_Command_Center, S))
	{
		if (depotBaseMap[c->unit()] != nullptr && depotBaseMap[c->unit()]->GetDangerousAreaForWorkers() == false)
			return true;
	}

	return false;
}

void SoldierManager::CheckRepairScv(void)
{
	
	if (RepairStateScvCount > MaxRepairScvCnt)
	{
		return;
	}

	int scvCnt = INFO.getCompletedCount(Terran_SCV, S);

	for (auto &b : INFO.getBuildings(S))
	{
		if (!b.second->isComplete())
			continue;

		bool myArea = false;

		for (auto base : INFO.getOccupiedBaseLocations(S))
		{
			if (isSameArea(base->Center(), b.second->pos())) {
				myArea = true;
				break;
			}
		}

		if (!myArea)
			continue;

		
		repairScvCnt = 1;

		
		double needRepairThreshold = 0.3;

		
		if (b.second->type() == Terran_Bunker || b.second->type() == Terran_Missile_Turret ||
				(b.second->type() == Terran_Barracks && b.second->getState() == "Barricade"))
			needRepairThreshold = 1;

		bool needPreRepair = false;

		
		if (b.second->type() == Terran_Missile_Turret)
		{
			if (INFO.enemyRace == Races::Zerg)
			{
				int mutalsAroundTurret = INFO.getTypeUnitsInRadius(Zerg_Mutalisk, E, b.second->pos(), 8 * TILE_SIZE).size();
				int goliathAroundTurret = INFO.getTypeUnitsInRadius(Terran_Goliath, S, b.second->pos(), 8 * TILE_SIZE).size();

				if (mutalsAroundTurret >= 4 && goliathAroundTurret <= 4)
					needPreRepair = true;

				repairScvCnt = 1 + (mutalsAroundTurret > 2 ? mutalsAroundTurret / 2 : 1);

				if (repairScvCnt > 6) repairScvCnt = 6;
			}

			if (INFO.enemyRace == Races::Protoss)
			{
				
				if (isSameArea(MYBASE, b.second->pos()) &&
						INFO.getTypeUnitsInRadius(Protoss_Dark_Templar, E, b.second->pos(), 8 * TILE_SIZE).size())
				{
					needPreRepair = true;
					repairScvCnt = 6;
				}
			}
		}

		
		if (INFO.enemyRace == Races::Protoss)
		{
			if (b.second->type() == Terran_Comsat_Station
					&& INFO.getTypeBuildingsInArea(Terran_Missile_Turret, S, MYBASE, true).empty()) {
				
				if (isSameArea(MYBASE, b.second->pos()) &&
						INFO.getTypeUnitsInRadius(Protoss_Dark_Templar, E, b.second->pos(), 8 * TILE_SIZE).size())
				{
					needPreRepair = true;
					repairScvCnt = 4;
				}
			}
		}

		int thresholdHp = (int)(needRepairThreshold * b.second->type().maxHitPoints());

		for (int i = 0; i < scvCnt; i++)
		{
			if (getRepairScvCount(b.first) < repairScvCnt && (needPreRepair || b.second->hp() < thresholdHp)) {
				if (chooseRepairScvClosestTo(b.first, 25 * TILE_SIZE) == nullptr)
					break;
			}
			else
				break;
		}
	}

	repairScvCnt = 1;

	
	uList vessels = INFO.getUnits(Terran_Science_Vessel, S);

	for (auto v : vessels)
	{
		if (v->hp() < 110 && getRepairScvCount(v->unit()) < repairScvCnt && isInMyArea(v->pos()))
			if (chooseRepairScvClosestTo(v->unit()) == nullptr)
				return;
	}

	uList dropships = INFO.getUnits(Terran_Dropship, S);

	for (auto d : dropships)
	{
		if (d->hp() < d->type().maxHitPoints() && getRepairScvCount(d->unit()) < repairScvCnt && isInMyArea(d->pos()))
		{
			if (chooseRepairScvClosestTo(d->unit(), INT_MAX, true) == nullptr) 
				return;
		}
	}

	Base *secondMulti = INFO.getSecondExpansionLocation(S);
	Base *thirdMulti = INFO.getThirdExpansionLocation(S);

	if (secondMulti != nullptr)
	{
		uList tankSet = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, secondMulti->Center(), 15 * TILE_SIZE);
		uList golSet = INFO.getTypeUnitsInRadius(Terran_Goliath, S, secondMulti->Center(), 15 * TILE_SIZE);

		for (auto t : tankSet)
		{
			if (t->hp() < t->type().maxHitPoints() && getRepairScvCount(t->unit()) < repairScvCnt &&  isInMyArea(t->pos(), true))
			{
				if (chooseRepairScvClosestTo(t->unit(), 15 * TILE_SIZE) == nullptr)
					break;
			}
		}

		for (auto g : golSet)
		{
			if (g->hp() < g->type().maxHitPoints() && getRepairScvCount(g->unit()) < repairScvCnt &&  isInMyArea(g->pos(), true))
				if (chooseRepairScvClosestTo(g->unit(), 15 * TILE_SIZE) == nullptr)
					break;
		}
	}

	if (thirdMulti != nullptr)
	{
		uList tankSet = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, thirdMulti->Center(), 15 * TILE_SIZE);
		uList golSet = INFO.getTypeUnitsInRadius(Terran_Goliath, S, thirdMulti->Center(), 15 * TILE_SIZE);

		for (auto t : tankSet)
		{
			if (t->hp() < t->type().maxHitPoints() && getRepairScvCount(t->unit()) < repairScvCnt &&  isInMyArea(t->pos(), true))
				if (chooseRepairScvClosestTo(t->unit(), 15 * TILE_SIZE) == nullptr)
					break;
		}

		for (auto g : golSet)
		{
			if (g->hp() < g->type().maxHitPoints() && getRepairScvCount(g->unit()) < repairScvCnt &&  isInMyArea(g->pos(), true))
				if (chooseRepairScvClosestTo(g->unit(), 15 * TILE_SIZE) == nullptr)
					break;
		}
	}

	
	for (auto &b : INFO.getAdditionalExpansions())
	{
		uList tankSet = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, b->Center(), 15 * TILE_SIZE);
		uList golSet = INFO.getTypeUnitsInRadius(Terran_Goliath, S, b->Center(), 15 * TILE_SIZE);

		for (auto t : tankSet)
		{
			if (t->hp() < t->type().maxHitPoints() && getRepairScvCount(t->unit()) < repairScvCnt &&  isInMyArea(t->pos(), true))
			{
				if (chooseRepairScvClosestTo(t->unit(), 15 * TILE_SIZE) == nullptr)
					break;
			}
		}

		for (auto g : golSet)
		{
			if (g->hp() < g->type().maxHitPoints() && getRepairScvCount(g->unit()) < repairScvCnt &&  isInMyArea(g->pos(), true))
				if (chooseRepairScvClosestTo(g->unit(), 15 * TILE_SIZE) == nullptr)
					break;
		}
	}


	if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion)
	{
		if (INFO.enemyInMyArea().size() == 0)
		{
			
			for (auto &u : INFO.getUnits(S))
			{
				if (theMap.GetArea(WalkPosition(u.second->pos())) != nullptr && u.second->type().isMechanical() && !u.second->type().isWorker()) // GetArea는 nullptr 체크 필요
				{
					if (isInMyArea(u.second) && u.second->hp() < u.second->type().maxHitPoints() && getRepairScvCount(u.first) < repairScvCnt)
						if (chooseRepairScvClosestTo(u.first) == nullptr)
							return;
				}
			}
		}
	}
}
void SoldierManager::CheckBunkerDefence()
{
	int facCnt = INFO.getCompletedCount(Terran_Factory, S);

	if (facCnt == 0 || facCnt > 2 || ESM.getEnemyInitialBuild() <= Zerg_5_Drone && ESM.getEnemyMainBuild() != Zerg_main_zergling)
	{
		bunkerDefenceSet.init();
		return;
	}

	Unit mbunker = MM.getBunker();

	if (mbunker == nullptr) { 
		bunkerDefenceSet.init();
		return;
	}

	UnitInfo *bunker = INFO.getUnitInfo(mbunker, S);

	
	if (isSameArea(MYBASE, bunker->pos()) && INFO.getAllCount(Terran_Command_Center, S) > 1) {
		bunkerDefenceSet.init();
		return;
	}

	word bunkerDefenceNeedCnt = 0;
	word needRightNowCnt = 0;


	Position basePos;

	
	if (INFO.getCompletedCount(Terran_Command_Center, S) == 1 || isSameArea(bunker->pos(), MYBASE)) {
		
		basePos = MYBASE;
	}
	else {
		
		basePos = INFO.getFirstExpansionLocation(S)->Center();
	}

	int base2BunkerDist = getGroundDistance(bunker->pos(), basePos);

	double frameToGetBunker = base2BunkerDist * 1.3 / Terran_SCV.topSpeed();

	uList eUnits = INFO.getUnitsInRadius(E, Positions::Origin, 0, true, false, false, true);

	if (INFO.enemyRace == Races::Protoss) {
		int DragoonCnt = 0;
		int ZealotCnt = 0;
		int dangerousDragoonCnt = 0;
		int dangerousZealotCnt = 0;
		double zealotTopSpeed = E->topSpeed(Protoss_Zealot);
		int dragoonAttackRange = UnitUtil::GetAttackRange(Protoss_Dragoon, E, false);

		for (auto e : eUnits) {
			int passedTime = TIME - e->getLastPositionTime();

			if (passedTime > 24 * 25)
				continue;

			if (e->type() == Protoss_Dragoon) {
				if ((getGroundDistance(e->getLastSeenPosition(), bunker->pos()) - dragoonAttackRange) / Protoss_Dragoon.topSpeed() - passedTime < frameToGetBunker)
				{
					DragoonCnt++;

					if (getGroundDistance(bunker->pos(), e->vPos()) < dragoonAttackRange + base2BunkerDist + 3 * TILE_SIZE)
						dangerousDragoonCnt++;
				}
			}
			else if (e->type() == Protoss_Zealot) {
				if (getGroundDistance(e->getLastSeenPosition(), bunker->pos()) / zealotTopSpeed - passedTime < frameToGetBunker)
				{
					ZealotCnt++;

					if (getGroundDistance(bunker->pos(), e->vPos()) < base2BunkerDist + 3 * TILE_SIZE)
						dangerousZealotCnt++;
				}
			}
		}

		bunkerDefenceNeedCnt = DragoonCnt + (ZealotCnt / 2);
		needRightNowCnt = dangerousDragoonCnt + (dangerousZealotCnt * 2 / 3);
	}
	else if (INFO.enemyRace == Races::Zerg) {
		int ZerglingCnt = 0;
		int HydraCnt = 0;
		int LurkerCnt = 0;
		int dangerousZerglingCnt = 0;
		int dangerousHydraCnt = 0;
		int dangerousLurkerCnt = 0;

		double zerglingTopSpeed = E->topSpeed(Zerg_Zergling);
		double hydraTopSpeed = E->topSpeed(Zerg_Hydralisk);
		int hydraAttackRange = UnitUtil::GetAttackRange(Zerg_Hydralisk, E, false);
		int lurkerAttackRange = UnitUtil::GetAttackRange(Zerg_Lurker, E, false);

		for (auto e : eUnits) {
			int passedTime = TIME - e->getLastPositionTime();

			if (passedTime > 24 * 25)
				continue;

			if (e->type() == Zerg_Zergling) {
				if (getGroundDistance(e->getLastSeenPosition(), bunker->pos()) / zerglingTopSpeed - passedTime < frameToGetBunker)
				{
					ZerglingCnt++;

					if (getGroundDistance(bunker->pos(), e->vPos()) < base2BunkerDist + 3 * TILE_SIZE)
						dangerousZerglingCnt++;
				}
			}
			else if (e->type() == Zerg_Hydralisk) {
				if ((getGroundDistance(e->getLastSeenPosition(), bunker->pos()) - hydraAttackRange) / hydraTopSpeed - passedTime < frameToGetBunker)
				{
					HydraCnt++;

					if (getGroundDistance(bunker->pos(), e->vPos()) < hydraAttackRange + base2BunkerDist + 3 * TILE_SIZE)
						dangerousHydraCnt++;
				}
			}
			else if (e->type() == Zerg_Lurker || e->type() == Zerg_Lurker_Egg) {
				if ((getGroundDistance(e->getLastSeenPosition(), bunker->pos()) - lurkerAttackRange) / Zerg_Lurker.topSpeed() - passedTime < frameToGetBunker)
				{
					LurkerCnt++;

					if (getGroundDistance(bunker->pos(), e->vPos()) < lurkerAttackRange + base2BunkerDist + 3 * TILE_SIZE)
						dangerousLurkerCnt++;
				}
			}
		}

		bunkerDefenceNeedCnt = LurkerCnt + (ZerglingCnt / 4) + HydraCnt;
		needRightNowCnt = dangerousLurkerCnt + (dangerousZerglingCnt / 4) + dangerousHydraCnt;
	}
	else if (INFO.enemyRace == Races::Terran) {
		
		int MarineCnt = 0;
		int dangerousMarineCnt = 0;

		int marineAttackRange = UnitUtil::GetAttackRange(Terran_Marine, E, false);

		for (auto e : eUnits) {
			int passedTime = TIME - e->getLastPositionTime();

			if (passedTime > 24 * 25)
				continue;

			if (e->type() == Terran_Marine) {
				if ((getGroundDistance(e->getLastSeenPosition(), bunker->pos()) - marineAttackRange) / Terran_Marine.topSpeed() - passedTime < frameToGetBunker)
				{
					MarineCnt++;

					if (getGroundDistance(bunker->pos(), e->vPos()) < base2BunkerDist + marineAttackRange + 3 * TILE_SIZE)
						dangerousMarineCnt++;
				}
			}
		}

		bunkerDefenceNeedCnt = MarineCnt / 2;
		needRightNowCnt = dangerousMarineCnt / 2;
	}

	
	if (bunkerDefenceNeedCnt > 2)
		bunkerDefenceNeedCnt = 2;

	
	if (INFO.enemyRace == Races::Protoss && ESM.getEnemyMainBuild() == Toss_2gate_dra_push
			&& INFO.getCompletedCount(Terran_Command_Center, S) < 2)
		bunkerDefenceNeedCnt += 2;

	
	if (needRightNowCnt > 6)
		needRightNowCnt = 6;

	
	word baseCount = max(bunkerDefenceNeedCnt, needRightNowCnt);

	
	while (bunkerDefenceSet.size() < baseCount) {
		UnitInfo *closestScv = chooseScvClosestTo(bunker->pos());

		if (closestScv == nullptr)
			break;

		setStateToSCV(closestScv, new ScvBunkerDefenceState());
	}

	
	if (bunkerDefenceSet.size() > baseCount) {
		
		if (bunker->hp() * 2 >= bunker->type().maxHitPoints()) {
			uList scvs = bunkerDefenceSet.getUnits();

			for (auto s : scvs) {
				setStateToSCV(s, new ScvIdleState());

				if (bunkerDefenceSet.size() <= baseCount)
					break;
			}
		}
	}
}

void SoldierManager::CheckFirstChokeDefence()
{
	word needCnt = INFO.getFirstChokeDefenceCnt();

	if (needCnt == 0 && firstChokeDefenceSet.size()) {
		uList scvs = firstChokeDefenceSet.getUnits();

		for (auto s : scvs) {
			setStateToSCV(s, new ScvCombatState());
		}
	}
	else {
		if (needCnt > firstChokeDefenceSet.size()) {
			word assignCnt = needCnt - firstChokeDefenceSet.size();

			while (firstChokeDefenceSet.size() < assignCnt) {
				UnitInfo *scv = chooseScvClosestTo((Position)INFO.getFirstChokePoint(S)->Center());

				if (scv == nullptr) {
					break;
				}

				setStateToSCV(scv, new ScvFirstChokeDefenceState());
			}

		}
		else if (needCnt < firstChokeDefenceSet.size()) {

			for (auto scv : firstChokeDefenceSet.getUnits()) {
				setStateToSCV(scv, new ScvIdleState());
				needCnt++;

				if (needCnt == firstChokeDefenceSet.size())
					break;
			}
		}
	}
}

void SoldierManager::CheckCombatSet()
{
	uList ScvList = INFO.getUnits(Terran_SCV, S);

	if (INFO.enemyRace == Races::Terran)
	{
		if (!SM.getNeedDefenceWithScv())
		{
			if (combatSet.size())
				combatSet.init();
		}
		else
		{
			for (auto scv : INFO.getTypeUnitsInArea(Terran_SCV, S, INFO.getFirstExpansionLocation(S)->Center()))
			{
				if (scv->hp() >= 35 && scv->getState() != "Combat")
				{
					setStateToSCV(scv, new ScvCombatState());
				}
			}
		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		if (distanceToBunker == 0)
		{
			distanceToBunker = INFO.getFirstChokePosition(S).getApproxDistance(MYBASE) + INFO.getFirstChokePosition(S).getApproxDistance((Position)INFO.getSecondChokePointBunkerPosition()) + 2 * TILE_SIZE;
			return;
		}

		int DragoonCnt = 0, ZealotCnt = 0, DarkTemplerCnt = 0;

		for (auto eu : INFO.getUnitsInRadius(E, Positions::None, 0, true, false, false))
		{
			if (eu->pos() == Positions::Unknown)
				continue;

			if (getGroundDistance(MYBASE, eu->pos()) < distanceToBunker)
			{
				if (eu->type() == Protoss_Dragoon)
					DragoonCnt++;
				else if (eu->type() == Protoss_Zealot)
					ZealotCnt++;
				else if (eu->type() == Protoss_Dark_Templar && (eu->unit()->isDetected() || INFO.getAvailableScanCount() > 0))
					DarkTemplerCnt++;
			}
		}

		int VultureCnt = 0, TankCnt = 0, GoliathCnt = 0, MarineCnt = 0;
		int BunkerCnt = INFO.getCompletedCount(Terran_Bunker, S);

		
		if ((EIB == Toss_1g_forward || EIB == Toss_2g_forward) && INFO.getAllCount(Terran_Command_Center, S) < 2 && BunkerCnt)
		{
			combatSet.init();
			return;
		}

		for (auto mu : INFO.getUnitsInRadius(S, Positions::None, 0, true, false, false))
		{
			if (isInMyArea(mu))
			{
				if (mu->type() == Terran_Vulture)
					VultureCnt++;
				else if (mu->type() == Terran_Siege_Tank_Tank_Mode && mu->getState() != "FirstExpansionSecure")
					TankCnt++;
				else if (mu->type() == Terran_Goliath)
					GoliathCnt++;
				else if (mu->type() == Terran_Marine && !mu->unit()->isLoaded() && mu->getState() != "FirstChokeDefence")
					MarineCnt++;
			}
		}

		word EnemyValue = (DragoonCnt * 3) + (ZealotCnt * 2) + (DarkTemplerCnt * 3);
		word MyValue = MarineCnt + (VultureCnt * 2) + (GoliathCnt * 2) + (TankCnt * 2);

		
		if (EnemyValue - MyValue == 1)
			EnemyValue++;

		if (EnemyValue > MyValue)
		{
			UListSet scvlist;

			for (auto scv : ScvList)
			{
				if (scv->getState() == "Mineral" || scv->getState() == "Gas" || scv->getState() == "Idle")
				{
					scvlist.add(scv);
				}
			}

			uList sortedList = scvlist.getSortedUnitList(INFO.getSecondChokePosition(S));

			for (auto scv : sortedList)
			{
				if (MineralStateScvCount <= 4 || combatSet.size() >= (EnemyValue - MyValue)) break;

				if (scv->hp() >= 35)
				{
					setStateToSCV(scv, new ScvCombatState());
				}
			}

			uList sortedCombatList = combatSet.getSortedUnitList(MYBASE);

			for (auto scv : sortedCombatList)
			{
				if (combatSet.size() <= (EnemyValue - MyValue)) break;

				setStateToSCV(scv, new ScvIdleState());
			}
		}
		else
		{
			combatSet.init();
		}
	}
	else
	{
		if (INFO.getCompletedCount(Terran_Command_Center, S) > 1)
		{
			if (combatSet.size())
				combatSet.init();

			return;
		}

		if (ESM.getEnemyInitialBuild() <= Zerg_5_Drone || ESM.getEnemyInitialBuild() == Zerg_9_Hat)
		{
			uList Zerglings = getEnemyInMyYard(1600, Zerg_Zergling);

			int marineCnt = INFO.getCompletedCount(Terran_Marine, S);
			int vultureCnt = INFO.getCompletedCount(Terran_Vulture, S);

			if (Zerglings.size() == 0 || vultureCnt > 0)
			{
				combatSet.init();
				return;
			}
			else
			{
				
				if (INFO.getCompletedCount(Terran_Bunker, S) > 0 && marineCnt > 1 && (int)INFO.getTypeUnitsInArea(Zerg_Zergling, E, MYBASE).size() < marineCnt * 3)
				{
					combatSet.init();
				}
				else
				{
					setNeedCountForRefinery(0);

					
					word needScvCount = INFO.getDestroyedCount(Zerg_Zergling, E) < 6 ?
										(ScvList.size() > 4 ? ScvList.size() - 4 : ScvList.size())
										
										: Zerglings.size() > 4 ? Zerglings.size() + 4 : Zerglings.size() + 2;

					UListSet scvlist;

					for (auto scv : ScvList)
					{
						if (scv->getState() == "Mineral")
						{
							scvlist.add(scv);
						}
					}

					uList sortedList = scvlist.getSortedUnitList(INFO.getSecondChokePosition(S));

					for (auto scv : sortedList)
					{
						if (combatSet.size() >= needScvCount) break;

						if (scv->hp() >= 35)
						{
							setStateToSCV(scv, new ScvCombatState());
						}
					}

					if ((INFO.getCompletedCount(Terran_SCV, S) <= 6 || combatSet.size() < Zerglings.size()) && combatSet.size() < needScvCount) {
						for (auto scv : sortedList)
						{
							if (combatSet.size() >= needScvCount) break;

							setStateToSCV(scv, new ScvCombatState());
						}
					}
				}
			}
		}
		else if (ESM.getEnemyInitialBuild() <= Zerg_12_Pool)
		{
			uList Zerglings = getEnemyInMyYard(1600, Zerg_Zergling, false);

			
			if ((int)Zerglings.size() > INFO.getCompletedCount(Terran_Vulture, S) * 5 &&
					INFO.getUnitsInRadius(S, INFO.getFirstExpansionLocation(S)->Center(), 8 * TILE_SIZE, true, false, false).size() == 0)
			{
				UListSet scvlist;

				for (auto scv : ScvList)
				{
					if (scv->getState() == "Mineral")
					{
						scvlist.add(scv);
					}
				}

				uList sortedList = scvlist.getSortedUnitList(INFO.getSecondChokePosition(S));

				for (auto scv : sortedList)
				{
					if (MineralStateScvCount <= 4 || combatSet.size() >= Zerglings.size() + 2) break;

					if (scv->hp() >= 35)
					{
						setStateToSCV(scv, new ScvCombatState());
					}
				}

				uList sortedCombatList = combatSet.getSortedUnitList(MYBASE);

				for (auto scv : sortedCombatList)
				{
					if (combatSet.size() <= Zerglings.size() + 2) break;

					setStateToSCV(scv, new ScvIdleState());
				}
			}
			else
				combatSet.init();
		}
	}
}


void SoldierManager::setScvIdleState(Unit u)
{
	setStateToSCV(INFO.getUnitInfo(u, S), new ScvIdleState());
}

void	SoldierManager::setIdleAroundDepot(Unit depot)
{
	uList ScvList = INFO.getUnits(Terran_SCV, S);

	for (auto scv : ScvList)
	{
		if ( scv->unit()->getDistance(depot) < 10 * TILE_SIZE )
		{
			if (scv->getState() == "Idle")
				scv->unit()->stop();

			if (scv->getState() == "Mineral" || scv->getState() == "Gas") {
				setStateToSCV(scv, new ScvIdleState());
				scv->unit()->stop();
			}
		}
	}
}


Unit SoldierManager::chooseConstuctionScvClosestTo(TilePosition buildingPosition, int avoidScvID)
{
	UnitInfo *closestScv = chooseScvClosestTo((Position)buildingPosition, avoidScvID);

	return setStateToSCV(closestScv, new ScvBuildState());
}


UnitInfo *SoldierManager::chooseScouterScvClosestTo(Position p)
{
	UnitInfo *closestScv = chooseScvClosestTo(p);
	setStateToSCV(closestScv, new ScvScoutState());

	return closestScv;
}


Unit	SoldierManager::chooseRepairScvClosestTo(Unit unit, int maxRange, bool withGas)
{
	UnitInfo *closestScv = chooseScvClosestTo(unit->getPosition(), unit->getID(), maxRange, withGas);
	return setStateToSCV(closestScv, new ScvRepairState(unit));
}


UnitInfo *SoldierManager::chooseScvClosestTo(Position position, int avoidScvID, int maxRange, bool withGas) {
	
	UnitInfo *closestScv = nullptr;
	int closestdist = maxRange;

	uList ScvList = INFO.getUnits(Terran_SCV, S);

	
	for (auto scv : ScvList)
	{
		
		if (scv->id() == avoidScvID) continue;

		bool candidate = scv->unit()->isInterruptible() && (scv->getState() == "Idle" || (scv->getState() == "Mineral" && !scv->unit()->canReturnCargo() && !scv->isGatheringMinerals() && scv->unit()->canCommand()));

		if (withGas && scv->getState() == "Gas")	candidate = true;

		
		if (candidate)
		{
			
			int distance = INT_MAX;
			theMap.GetPath(scv->pos(), position, &distance, false);

			if (distance < closestdist && distance >= 0)
			{
				closestScv = scv;
				closestdist = distance;
			}
		}
	}

	return closestScv;
}


Unit	SoldierManager::chooseRepairScvforScv(Unit unit, int maxRange, bool withGas)
{
	UnitInfo *closestScv = chooseScvForScv(unit->getPosition(), unit->getID(), maxRange, withGas);
	return setStateToSCV(closestScv, new ScvRepairState(unit));
}


UnitInfo *SoldierManager::chooseScvForScv(Position position, int avoidScvID, int maxRange, bool withGas) {

	UnitInfo *weakestScv = nullptr;
	int hp = Terran_SCV.maxHitPoints();

	uList ScvList = INFO.getTypeUnitsInRadius(Terran_SCV, S, position, maxRange);

	
	for (auto scv : ScvList)
	{
		if (scv->id() == avoidScvID) continue;

		bool candidate = scv->unit()->isInterruptible() && (scv->getState() == "Idle" || (scv->getState() == "Mineral" && !scv->unit()->canReturnCargo() && !scv->isGatheringMinerals() && scv->unit()->canCommand()));

		if (withGas && scv->getState() == "Gas")	candidate = true;

		
		if (candidate)
		{
			if (scv->hp() <= hp)
			{
				weakestScv = scv;
				hp = scv->hp();
			}
		}
	}

	return weakestScv;
}


void SoldierManager::beforeRemoveState(UnitInfo *scv) {
	string stateName = scv->getState();

	if (stateName == "Mineral") {
		MineralStateScvCount--;
		mineralScvCountMap[scv->getTarget()]--;
	}
	else if (stateName == "Gas") {
		GasStateScvCount--;
		refineryScvCountMap[scv->getTarget()]--;
	}
	else if (stateName == "Repair") {
		RepairStateScvCount--;

		if (--repairScvCountMap[scv->getTarget()] == 0)
			repairScvCountMap.erase(scv->getTarget());
	}
	else if (stateName == "ScoutUnitDefence") {
		totalScoutUnitDefenceScvCount--;

		if (--scoutUnitDefenceScvCountMap[scv->getTarget()] == 0)
			scoutUnitDefenceScvCountMap.erase(scv->getTarget());
	}
	else if (stateName == "EarlyRushDefense") {
		totalEarlyRushDefenceScvCount--;

		if (--earlyRushDefenceScvCountMap[scv->getTarget()] == 0)
			earlyRushDefenceScvCountMap.erase(scv->getTarget());
	}
	else if (stateName == "ScanMyBase")
		scanMyBaseUnit = nullptr;
	else if (stateName == "BunkerDefence")
		bunkerDefenceSet.del(scv);
	else if (stateName == "Combat")
		combatSet.del(scv);
	else if (stateName == "Nightingale")
		nightingaleSet.del(scv);
	else if (stateName == "FirstChokeDefence")
		firstChokeDefenceSet.del(scv);
	else if (stateName == "RemoveBlocking")
		removeBlockingSet.del(scv);
}


Unit SoldierManager::setStateToSCV(UnitInfo *scv, State *state) {
	if (scv == nullptr) {
		delete state;
		return nullptr;
	}

	
	beforeRemoveState(scv);

	scv->setState(state);

	string stateName = scv->getState();

	
	if (stateName == "Mineral") {
		MineralStateScvCount++;
		mineralScvCountMap[scv->getTarget()]++;
	}
	else if (stateName == "Gas") {
		GasStateScvCount++;
		refineryScvCountMap[scv->getTarget()]++;
	}
	else if (stateName == "Repair") {
		RepairStateScvCount++;
		repairScvCountMap[scv->getTarget()]++;
	}
	else if (stateName == "ScoutUnitDefence") {
		totalScoutUnitDefenceScvCount++;
		scoutUnitDefenceScvCountMap[scv->getTarget()]++;
	}
	else if (stateName == "EarlyRushDefense") {
		totalEarlyRushDefenceScvCount++;
		earlyRushDefenceScvCountMap[scv->getTarget()]++;
	}
	else if (stateName == "ScanMyBase")
		scanMyBaseUnit = scv->unit();
	else if (stateName == "BunkerDefence")
		bunkerDefenceSet.add(scv);
	else if (stateName == "Combat")
		combatSet.add(scv);
	else if (stateName == "Nightingale")
		nightingaleSet.add(scv);
	else if (stateName == "FirstChokeDefence")
		firstChokeDefenceSet.add(scv);
	else if (stateName == "RemoveBlocking")
		removeBlockingSet.add(scv);

	return scv->unit();
}


void SoldierManager::rebalancingNewDepot(Unit depot)
{
	uList ScvList = INFO.getUnits(Terran_SCV, S);

	int newMineralCnt = 0;

	for (auto m : depotMineralMap[depot]) {
		if (getMineralScvCount(m) == 0)
			newMineralCnt++;
	}

	for (auto scv : ScvList)
	{
		if (scv->getState() == "PreMineral")
		{
			setMineralScv(scv, depot);
			newMineralCnt--;
		}
	}

	for (auto scv : ScvList)
	{
		if (newMineralCnt <= 0)
			return;

		if (scv->getState() == "Idle")
		{
			setMineralScv(scv, depot);
			newMineralCnt--;
		}
	}

	
	for (auto scv : ScvList)
	{
		if (newMineralCnt <= 0)
			return;

		if (scv->getState() == "Mineral" && !scv->unit()->canReturnCargo() && !scv->isGatheringMinerals())
		{
			if (mineralScvCountMap[scv->getTarget()] > 1) 
			{
				setMineralScv(scv, depot);
				newMineralCnt--;
			}
		}
	}
}

void SoldierManager::preRebalancingNewDepot(Unit depot, int cnt)
{
	Unit mineral = depot->getClosestUnit(Filter::IsMineralField, 8 * TILE_SIZE);

	if (mineral == nullptr)
		return;

	uList ScvList = INFO.getUnits(Terran_SCV, S);

	int newMineralCnt = cnt;

	
	for (auto scv : ScvList)
	{
		if (newMineralCnt == 0)
			return;

		if (scv->getState() == "Idle" || (scv->getState() == "Mineral" && !scv->unit()->canReturnCargo() && !scv->isGatheringMinerals() && mineralScvCountMap[scv->getTarget()] > 1))
		{
			setStateToSCV(scv, new ScvPreMineralState());
			scv->unit()->rightClick(mineral);
			newMineralCnt--;
		}
	}
}

void	SoldierManager::onUnitCompleted(Unit u)
{
	if (u->getType().isResourceDepot())
	{
		initMineralsNearDepot(u);
		rebalancingNewDepot(u);
	}
}

void	SoldierManager::onUnitDestroyed(Unit u)
{
	if (!u->isCompleted())
		return;

	if (u->getType().isResourceDepot())
	{
		if (depotMineralMap.find(u) != depotMineralMap.end())
			depotMineralMap.erase(u);

		if (depotBaseMap.find(u) != depotBaseMap.end())
			depotBaseMap.erase(u);

		setIdleAroundDepot(u);
	}
	else if (u->getType() == Terran_SCV)
	{
		UnitInfo *deleteScv = INFO.getUnitInfo(u, S);

		if (deleteScv == nullptr)
			return;

		beforeRemoveState(deleteScv);
	}
	else if (u->getType().isMineralField())
	{
		if (mineralScvCountMap.find(u) != mineralScvCountMap.end())
			mineralScvCountMap.erase(u);
	}
	else if (u->getType().isRefinery())
	{
		if (refineryScvCountMap.find(u) != refineryScvCountMap.end())
			refineryScvCountMap.erase(u);
	}
	else {}
}

void	SoldierManager::onUnitLifted(Unit depot)
{
	if (depot->getType() != Terran_Command_Center)
		return;

	if (depotMineralMap.find(depot) != depotMineralMap.end())
		depotMineralMap.erase(depot);

	if (depotBaseMap.find(depot) != depotBaseMap.end())
		depotBaseMap.erase(depot);

	setIdleAroundDepot(depot);
}

void	SoldierManager::onUnitLanded(Unit depot)
{
	if (depot->getType() != Terran_Command_Center)
		return;

	initMineralsNearDepot(depot);
	rebalancingNewDepot(depot);
}

void SoldierManager::CheckWorkerDefence()
{
	
	if ((ESM.getEnemyInitialBuild() == Toss_4probes || ESM.getEnemyInitialBuild() == Terran_4scv || ESM.getEnemyInitialBuild() == Zerg_4_Drone_Real) && ESM.getEnemyMainBuild() == UnknownMainBuild)
	{
		int radius = 0;

		if (INFO.enemyRace == Races::Terran)
		{
			radius = 27 * TILE_SIZE;
		}
		else
		{
			radius = 50 * TILE_SIZE;
		}

		uList eWorkers = INFO.getTypeUnitsInRadius(INFO.getWorkerType(INFO.enemyRace), E, INFO.getMainBaseLocation(S)->Center(), radius);

		int workerCombatCnt = 0;

		uList ScvList = INFO.getUnits(Terran_SCV, S);

		for (auto scv : ScvList)
		{
			if (scv->getState() == "WorkerCombat")
			{
				workerCombatCnt++;
			}
		}

		if (eWorkers.size() > 0)
		{
			
			if (MineralStateScvCount <= 1 || workerCombatCnt >= (int)eWorkers.size() + 2)	return;

			if (ScvList.size() < eWorkers.size()) return;

			for (auto scv : ScvList)
			{
				if (scv->getState() == "Mineral" )
				{
					setStateToSCV(scv, new ScvWorkerCombatState());
					workerCombatCnt++;

					
					if (MineralStateScvCount <= 1 || workerCombatCnt >= (int)eWorkers.size() + 2)
						break;
				}
			}
		}
		else
		{
			for (auto scv : ScvList)
			{
				if (scv->getState() == "WorkerCombat") {
					setStateToSCV(scv, new ScvIdleState());
				}
			}
		}

	}
}

void SoldierManager::CheckEnemyScoutUnit() {
	if (ESM.getEnemyInitialBuild() == Toss_4probes || ESM.getEnemyInitialBuild() == Terran_4scv || ESM.getEnemyInitialBuild() == Zerg_4_Drone_Real || SM.getMainStrategy() != WaitToBase)
		return;

	vector<UnitInfo *>enemyScouters = getEnemyScountInMyArea();

	if (enemyScouters.empty())
		return;

	
	if (ESM.getEnemyInitialBuild() == Toss_cannon_rush) {
		for (auto e : enemyScouters) {
			for (int i = getScoutUnitDefenceScvCountMap(e->unit()); i < 1; i++) {
			
				UnitInfo *assignedScv = chooseScvClosestTo(e->pos());

				setStateToSCV(assignedScv, new ScvScoutUnitDefenceState(e->unit()));
			}
		}

		return;
	}

	if (totalScoutUnitDefenceScvCount == 0)
	{
		
		UnitInfo *assignedScv = chooseScvClosestTo(enemyScouters.at(0)->pos());

		setStateToSCV(assignedScv, new ScvScoutUnitDefenceState(enemyScouters.at(0)->unit()));
	}
}

void SoldierManager::CheckEarlyRush() {
	
	UnitInfo *eRefinery = INFO.getUnitInfo(HostileManager::Instance().getEnemyGasRushRefinery(), E);

	
	if (eRefinery) {
		uList barracks = INFO.getBuildings(Terran_Barracks, S);

		
		if (barracks.size() == 1 && barracks.at(0)->hp() >= 500) {
			for (int i = getEarlyRushDefenceScvCount(eRefinery->unit()); i < 4; i++) {
				UnitInfo *assignedScv = chooseScvClosestTo(eRefinery->pos());

				setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(eRefinery->unit(), eRefinery->pos()));
			}
		}
	}

	
	if (INFO.enemyRace == Races::Protoss) {
		if (ESM.getEnemyInitialBuild() == UnknownBuild && ESM.getEnemyMainBuild() == UnknownMainBuild) {
		
			uList pylons = INFO.getBuildings(Protoss_Pylon, E);

			for (auto pylon : pylons) {
				if (pylon->pos() == Positions::Unknown || !pylon->isHide())
					continue;

				
				if (getGroundDistance(pylon->pos(), INFO.getMainBaseLocation(S)->Center())
						< getGroundDistance(pylon->pos(), INFO.getMainBaseLocation(E)->Center())) {
					for (int i = getEarlyRushDefenceScvCount(pylon->unit()); i < 1; i++) {
						UnitInfo *assignedScv = chooseScvClosestTo(pylon->pos());

						setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(pylon->unit(), pylon->pos()));
					}
				}
			}
		}
		else if (ESM.getEnemyInitialBuild() == Toss_cannon_rush) {
			uList buildings = getEnemyInMyYard(1600, UnitTypes::Buildings);

			uList pylons;
			uList photonCannonsGateways;
			bool isPhotonCompleted = false;

			for (auto building : buildings) {
				if (building->type() == Protoss_Pylon) {
					pylons.push_back(building);
				}
				
				else if (building->type() == Protoss_Photon_Cannon) {
					if (building->isComplete() && building->isPowered() && building->hp() > 45) {
						isPhotonCompleted = true;
						photonCannonsGateways.clear();
						pylons.clear();

						if (totalEarlyRushDefenceScvCount) {
							earlyRushDefenceScvCountMap.clear();

							uList scvs = INFO.getUnits(Terran_SCV, S);

							for (auto scv : scvs)
								if (scv->getState() == "EarlyRushDefense")
									setScvIdleState(scv->unit());
						}

						break;
					}

					photonCannonsGateways.push_back(building);
				}
				else if (building->type() == Protoss_Gateway) {
					photonCannonsGateways.push_back(building);
				}
			}

			
			if (!photonCannonsGateways.empty()) {
				for (auto photonCannonGateway : photonCannonsGateways) {
					
					int leftTime = photonCannonGateway->getRemainingBuildTime();

					int dist;

					theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), photonCannonGateway->pos(), &dist);

					
					double frameToGetThere = (double)dist / Terran_SCV.topSpeed() * 1.5;
					double shieldsPerTime = (double)photonCannonGateway->type().maxShields() / photonCannonGateway->type().buildTime();
					double HPPerTime = (double)photonCannonGateway->type().maxHitPoints() / photonCannonGateway->type().buildTime();
					double damagePerTime = (double)getDamage(Terran_SCV, photonCannonGateway->type(), S, E) / Terran_SCV.groundWeapon().damageCooldown();

					int needSCVCnt = (int)ceil((photonCannonGateway->hp() + frameToGetThere * (HPPerTime + shieldsPerTime)) / (damagePerTime * (leftTime - frameToGetThere)));

					for (int i = getEarlyRushDefenceScvCount(photonCannonGateway->unit()); i < needSCVCnt; i++) {
						UnitInfo *assignedScv = chooseScvClosestTo(photonCannonGateway->pos());

						setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(photonCannonGateway->unit(), photonCannonGateway->pos()));
					}
				}
			}
			
			else if (!pylons.empty()) {
				for (auto pylon : pylons) {
					
					int leftTime = pylon->getRemainingBuildTime();

					int dist;

					theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), pylon->pos(), &dist);

					
					double frameToGetThere = (double)dist / Terran_SCV.topSpeed();

					if (frameToGetThere < leftTime * 1.2) {
						continue;
					}
					
					else {
						for (int i = getEarlyRushDefenceScvCount(pylon->unit()); i < 1; i++) {
							UnitInfo *assignedScv = chooseScvClosestTo(pylon->pos());

							setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(pylon->unit(), pylon->pos()));
						}
					}
				}
			}
		}
		else if ((ESM.getEnemyInitialBuild() == Toss_1g_forward || ESM.getEnemyInitialBuild() == Toss_2g_forward) && ESM.getEnemyMainBuild() == UnknownMainBuild) {
			
			if (INFO.getCompletedCount(Protoss_Gateway, E) == 0) {
				uList buildings = getEnemyInMyYard(1600, UnitTypes::Buildings);

				uList gateways;

				
				if (INFO.getCompletedCount(Protoss_Zealot, E)) {
					gateways.clear();

					if (totalEarlyRushDefenceScvCount) {
						earlyRushDefenceScvCountMap.clear();

						uList scvs = INFO.getUnits(Terran_SCV, S);

						for (auto scv : scvs)
							if (scv->getState() == "EarlyRushDefense")
								setScvIdleState(scv->unit());
					}
				}
				else {
					for (auto building : buildings) {
						if (building->type() == Protoss_Gateway) {
							gateways.push_back(building);
						}
					}
				}

				
				if (!gateways.empty()) {
					for (auto gateway : gateways) {
						
						int leftTime = gateway->getCompleteTime() + Protoss_Zealot.buildTime();

						int dist;

						theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), gateway->pos(), &dist);

						
						double frameToGetThere = (double)dist / Terran_SCV.topSpeed() * 1.5;

						double shieldsPerTime = (double)gateway->type().maxShields() / gateway->type().buildTime();
						double HPPerTime = (double)gateway->type().maxHitPoints() / gateway->type().buildTime();
						double damagePerTime = (double)getDamage(Terran_SCV, gateway->type(), S, E) / Terran_SCV.groundWeapon().damageCooldown();

					
						int needSCVCnt = min((int)ceil((gateway->hp() + frameToGetThere * (HPPerTime + shieldsPerTime)) / (damagePerTime * (leftTime - frameToGetThere))), 6);

						for (int i = getEarlyRushDefenceScvCount(gateway->unit()); i <= needSCVCnt; i++) {
							UnitInfo *assignedScv = chooseScvClosestTo(gateway->pos());

							setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(gateway->unit(), gateway->pos()));
						}
					}
				}
			}

			if (INFO.getCompletedCount(Protoss_Zealot, E) <= 1 && INFO.getDestroyedCount(Protoss_Zealot, E) > 0) {
				UnitInfo *gateway = INFO.getClosestTypeUnit(E, MYBASE, Protoss_Gateway, 0, true, true);

				if (gateway) {
					if (INFO.getSecondChokePosition(S).getApproxDistance(theMap.Center()) + 10 * TILE_SIZE
							> INFO.getSecondChokePosition(S).getApproxDistance(gateway->pos())) {
						for (int i = totalEarlyRushDefenceScvCount; i < 1; i++) {
							UnitInfo *assignedScv = chooseScvClosestTo(gateway->pos());

							setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(gateway->unit(), gateway->pos()));
						}
					}
				}
			}
		}
	}
	else if (ESM.getEnemyInitialBuild() == Terran_bunker_rush) {
		uList buildings = getEnemyInMyYard(1600, UnitTypes::Buildings);

		uList bunkers;
		uList barracks;

		for (auto building : buildings) {
			if (building->type() == Terran_Bunker) {
				bunkers.push_back(building);
			}
			else if (building->type() == Terran_Barracks) {
				barracks.push_back(building);
			}
		}

		int totEarlyRushDefenceScvCount = 0;

		for (auto bunker : bunkers) {
			
			if (barracks.empty()) {
				for (int i = getEarlyRushDefenceScvCount(bunker->unit()); i < 5; i++) {
					UnitInfo *assignedScv = chooseScvClosestTo(bunker->pos());

					setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(bunker->unit(), bunker->pos()));
				}
			}
			
			else
				totEarlyRushDefenceScvCount += getEarlyRushDefenceScvCount(bunker->unit());
		}

		if (!totEarlyRushDefenceScvCount) {
			for (auto barrack : barracks) {
				int firstSomething = -1;
				int endSomething = -1;

				for (int i = 0; i < 5; i++) {
					TilePosition underBarracks = barrack->unit()->getTilePosition() + TilePosition(i, Terran_Barracks.tileHeight());

					if (!bw->isBuildable(underBarracks, true)) {
						if (firstSomething == -1)
							firstSomething = i;
					}
					else if (firstSomething >= 0) {
						endSomething = i;
						break;
					}
				}

				Position marineMove = Position(0, 0);

				if (firstSomething == 0) {
					if (endSomething == -1) {
						marineMove = Position(4 * 32, -32);
					}
					else {
						marineMove = Position(endSomething * 32, 0);
					}
				}

			
				Position topLeft = Position(barrack->unit()->getLeft(), barrack->unit()->getBottom()) + marineMove;
				Position bottomRight = topLeft + Position(Terran_Marine.width(), Terran_Marine.height());

				

				Position rightOfMarine = Position(bottomRight.x + Terran_SCV.dimensionLeft() + 2, topLeft.y + Terran_SCV.dimensionUp() + 2);
				Position bottomOfMarine = rightOfMarine + Position(-Terran_SCV.width() - 2, Terran_SCV.height() + 2);
				Position leftOfMarine = Position(bottomOfMarine.x - Terran_SCV.width() - 2, rightOfMarine.y);
				Position topOfMarine = bottomOfMarine + Position(0, rightOfMarine.y - Terran_SCV.height() - 2);

				vector<Position> surround;

				if (firstSomething == 0) {
					if (endSomething == 4) {
						surround.push_back(topOfMarine);
						surround.push_back(bottomOfMarine);
						surround.push_back(rightOfMarine);
					}
					else if (endSomething == -1) {
						surround.push_back(topOfMarine);
						surround.push_back(rightOfMarine);
					}
					else {
						surround.push_back(bottomOfMarine);
						surround.push_back(rightOfMarine);
					}
				}
				else if (firstSomething == 1) {
					surround.push_back(bottomOfMarine);
					surround.push_back(leftOfMarine);
				}
				else {
					surround.push_back(rightOfMarine);
					surround.push_back(bottomOfMarine);
					surround.push_back(leftOfMarine);
				}

				for (auto scv : INFO.getUnits(Terran_SCV, S)) {
					if (scv->getState() == "EarlyRushDefense") {

						for (auto iter = surround.begin(); iter != surround.end(); iter++) {
							if (scv->getTargetPos() == (*iter)) {
								surround.erase(iter);
								break;
							}
						}
					}
				}

				for (word i = getEarlyRushDefenceScvCount(barrack->unit()); i < 5; i++) {
					Position targetPosition = surround.size() > i ? surround[i] : barrack->pos();
					UnitInfo *assignedScv = chooseScvClosestTo(targetPosition);

					setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(barrack->unit(), targetPosition, surround.size() > i));
				}
			}
		}
	}
	else if (ESM.getEnemyInitialBuild() == Terran_1b_forward || ESM.getEnemyInitialBuild() == Terran_2b_forward) {
		uList eMarineList = getEnemyInMyYard(1600, Terran_Marine);
		uList eScv = getEnemyInMyYard(1600, Terran_SCV);
		int sMarineCnt = INFO.getCompletedCount(Terran_Marine, S);
		int sVultureCnt = INFO.getCompletedCount(Terran_Vulture, S);
		int sTankCnt = INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S);
		int sBunkerCnt = INFO.getCompletedCount(Terran_Bunker, S);

		int size = (eMarineList.size() - sMarineCnt - sVultureCnt * 3 - sTankCnt * 10 - sBunkerCnt * 30) * 2 + eScv.size();

		if (size > 0) {
			int defenseScvCnt = 0;

			for (auto scv : INFO.getTypeUnitsInRadius(Terran_SCV, S)) {
				if (scv->getState() == "EarlyRushDefense")
					defenseScvCnt++;
			}

			Unit target = eMarineList.size() ? eMarineList.front()->unit() : eScv.front()->unit();

			for (int i = defenseScvCnt; i < size; i++) {
				UnitInfo *assignedScv = chooseScvClosestTo(target->getPosition());

				setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(target, target->getPosition()));
			}
		}
	}
	else if (ESM.getEnemyInitialBuild() == Zerg_sunken_rush) {
		uList buildings = getEnemyInMyYard(1600, UnitTypes::Buildings);

		uList hatcherys;

		for (auto building : buildings) {
			if (building->type() == Zerg_Hatchery) {
				hatcherys.push_back(building);
			}
		}

		
		if (!hatcherys.empty()) {
			for (auto hatchery : hatcherys) {
				for (int i = getEarlyRushDefenceScvCount(hatchery->unit()); i < 6; i++) {
					UnitInfo *assignedScv = chooseScvClosestTo(hatchery->pos());

					setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(hatchery->unit(), hatchery->pos()));
				}
			}
		}
	}
}

void SoldierManager::CheckNightingale()
{
	if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) == 0)
		return;

	word ScvCntThreshold = (INFO.enemyRace == Races::Terran) ?  3 : 2;

	if (SM.getMainStrategy() != WaitToBase && SM.getMainStrategy() != WaitToFirstExpansion)
	{
		while (nightingaleSet.size() < ScvCntThreshold) {
			UnitInfo *assignedScv = chooseScvClosestTo(INFO.getSecondChokePosition(S));

			if (assignedScv == nullptr)
				break;

			setStateToSCV(assignedScv, new ScvNightingaleState());
		}
	}
	else if (nightingaleSet.size() > 0)
		nightingaleSet.init();
}

void SoldierManager::CheckRemoveBlocking(Position p)
{
	const Area *a = theMap.GetArea((TilePosition)p);

	if (a == nullptr) return;

	bool exists = false;

	for (auto cp : a->ChokePoints())
	{
		if (cp->BlockingNeutral() != nullptr && cp->BlockingNeutral()->IsMineral())
		{
			exists = true;

			

			if (removeBlockingSet.isEmpty())
			{
				UnitInfo *assingedScv = chooseScvClosestTo(cp->BlockingNeutral()->Pos());

				if (assingedScv == nullptr)
					break;

				setStateToSCV(assingedScv, new ScvRemoveBlockingState());
				break;
			}
		}
	}

	if (!exists)
	{
		blockingRemoved = true;
		removeBlockingSet.init();
	}
	else
	{
		blockingRemoved = false;
	}

}

void ScvCombatSet::init()
{
	for (auto scv : getUnits())
		scv->setState(new ScvIdleState());

	clear();
}

void ScvCombatSet::action()
{
	if (size() == 0)
		return;

	if (INFO.enemyRace == Races::Terran)
	{
		for (auto s : getUnits())
		{
			vector<UnitType> types = { Terran_Siege_Tank_Tank_Mode, Terran_Goliath };
			
			UnitInfo *closestGolTank = INFO.getClosestTypeUnit(E, s->pos(), types, 0, true);

			if (closestGolTank)
			{
				if (closestGolTank->isHide())
					s->unit()->move(closestGolTank->pos());
				else
				{
					s->unit()->attack(closestGolTank->unit());
					s->unit()->move(closestGolTank->vPos());
				}

				continue;
			}
		}
	}
	else	if (INFO.enemyRace == Races::Protoss)
	{
		for (auto s : getUnits())
		{
			int damage = 0;

			for (auto enemy : s->getEnemiesTargetMe()) {

				if (enemy->isInWeaponRange(s->unit()) && enemy->isAttacking()) {
					damage += getDamage(enemy, s->unit());

				
					if (enemy->getType() == Protoss_Zealot)
						damage += getDamage(enemy, s->unit());
				}
			}

			if (s->unit()->getHitPoints() - damage > 35) {
				UnitInfo *closest = INFO.getClosestUnit(E, s->pos(), GroundCombatKind, 0, true);

				if (closest) {
					s->unit()->attack(closest->unit());
					s->unit()->move(closest->vPos());
				}
			}
			else {
				UnitInfo *closest = INFO.getClosestUnit(E, s->pos(), GroundCombatKind, 0, true);

				if (closest) {
				
					moveBackPostion(s, closest->vPos(), 3 * TILE_SIZE);
				}
			}

		}
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		Position defencePos = Positions::None;
		Position myBase = INFO.getMainBaseLocation(S)->Center();

		UnitInfo *closestZergling = INFO.getClosestTypeUnit(E, myBase, Zerg_Zergling, 30 * TILE_SIZE, false, true);

		const ChokePoint *cp = theMap.GetPath(myBase, INFO.getSecondChokePosition(S)).at(0);

		if (closestZergling != nullptr
				&& theMap.GetArea((WalkPosition)myBase) == theMap.GetArea((WalkPosition)closestZergling->pos())
				
				&& Position(cp->Center()).getApproxDistance(closestZergling->pos()) > 2 * TILE_SIZE)
		{
			
			for (auto s : getUnits())
			{
				UnitInfo *closest = INFO.getClosestUnit(E, s->pos(), GroundCombatKind, 0, true);
				UnitInfo *weakest = getGroundWeakTargetInRange(s, true);
				Unit target = nullptr;

				if (closest != nullptr && !closest->unit()->exists()) closest = nullptr;

				if (weakest != nullptr && !weakest->unit()->exists()) weakest = nullptr;

				if (closest != nullptr)
				{
					target = closest->unit();

					if (weakest) {
						if (s->unit()->isInWeaponRange(weakest->unit())) {
							target = weakest->unit();
							CommandUtil::attackUnit(s->unit(), target, true);
							continue;
						}
					}

					CommandUtil::attackUnit(s->unit(), target, !s->unit()->isInWeaponRange(target));
				}
			}
		}
		else
		{
			vector<Position> defencePos;

			for (auto pos : getRoundPositions((Position)cp->Center(), 6 * TILE_SIZE, 30))
			{
				if (pos.isValid() && myBase.isValid()) {
					const Area *a1 = theMap.GetArea((WalkPosition)pos);
					const Area *a2 = theMap.GetArea((WalkPosition)myBase);

					if (bw->isWalkable((WalkPosition)pos) && a1 != nullptr && a2 != nullptr && a1 == a2 &&
							INFO.getBuildingsInRadius(S, pos, TILE_SIZE, true, false).size() == 0)
					{
						defencePos.push_back(pos);
					}
				}
			}

			if (defencePos.size() == 0)
			{
				for (auto s : getUnits())
				{
					UnitInfo *closest = INFO.getClosestUnit(E, s->pos(), GroundCombatKind, 15 * TILE_SIZE, true);

					if (closest != nullptr) {
						s->unit()->attack(closest->unit());
						s->unit()->move(closest->vPos());
					}
				}
			}
			else
			{
				for (word i = 0; i < defencePos.size(); i++)
					bw->drawCircleMap(defencePos[i], 2, Colors::Red, true);

				int idx = 0;

				for (auto s : getUnits())
				{
					s->unit()->move(defencePos[idx++]);

					if (idx == defencePos.size())	idx = 0;
				}
			}
		}
	}

	return;
}

void ScvNightingaleSet::init()
{
	for (auto scv : getUnits())
		scv->setState(new ScvIdleState());

	clear();

	builderId = -1;
}

void ScvNightingaleSet::action()
{
	if (!size())
		return;

	UnitInfo *fTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

	if (fTank == nullptr)
	{
		init();
		return;
	}

	bool canBuildTurret = builderId == -1 
						 
						  && TrainManager::Instance().getAvailableMinerals() >= Terran_Missile_Turret.mineralPrice() + 300
						 
						  && INFO.getSecondChokePosition(S) != Positions::None
						
						  && (SM.getMainStrategy() == WaitLine
							  
							  || (INFO.enemyRace == Races::Protoss && SM.getMainStrategy() == AttackAll && INFO.getAllCount(Protoss_Carrier, E) + INFO.getAllCount(Protoss_Arbiter, E) > 0));

	
	int TURRET_GAP = INFO.getAllCount(Protoss_Carrier, E) > 0 ? 3 : 6;

	for (auto scv : getUnits())
	{
		if (isStuckOrUnderSpell(scv))
			continue;

		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(scv->pos(), &dangerPoint, false);

		if (dangerUnit && dangerPoint < 3 * TILE_SIZE)
		{
			Position movePosition = getDirectionDistancePosition(fTank->pos(), MYBASE, 3 * TILE_SIZE);

			CommandUtil::move(scv->unit(), MYBASE);

			continue;
		}

		if (scv->unit()->isRepairing()) {
			continue;
		}

		if (builderId == scv->id() || scv->unit()->isConstructing()) {
			if (builderId != -1 && scv->unit()->getBuildUnit()) {
				
				if (INFO.getAllCount(Protoss_Carrier, E) > 0 && scv->unit()->canCancelConstruction())
					builderId = -1;
				
				else if (scv->unit()->getBuildUnit()->getHitPoints() >= 0.95 * scv->unit()->getBuildUnit()->getType().maxHitPoints())
					builderId = -1;
			}
			
			else if (builderId == scv->id() && !scv->unit()->isConstructing()) {
				UnitCommand currentCommand(scv->unit()->getLastCommand());

				if (currentCommand.getType() == UnitCommandTypes::Build && currentCommand.getTargetTilePosition().isValid() && bw->canBuildHere(currentCommand.getTargetTilePosition(), Terran_Missile_Turret, scv->unit())) {
					scv->unit()->move((Position)currentCommand.getTargetTilePosition() + (Position)Terran_Missile_Turret.tileSize() / 2);
					scv->unit()->build(Terran_Missile_Turret, currentCommand.getTargetTilePosition());
				}
				else
					builderId = -1;
			}

			continue;
		}

		if (scv->unit()->getDistance(fTank->pos()) > TILE_SIZE * 5)
		{
			CommandUtil::move(scv->unit(), fTank->pos());
			continue;
		}

		if (buildTurret(scv, canBuildTurret, builderId, TURRET_GAP)) {
			canBuildTurret = false;
			continue;
		}

		bool doAction = false;

		uList vessles = INFO.getTypeUnitsInRadius(Terran_Science_Vessel, S, scv->pos(), TILE_SIZE * 10);

		for (auto v : vessles)
		{
			if (v->hp() < v->type().maxHitPoints())
			{
				CommandUtil::repair(scv->unit(), v->unit());
				doAction = true;

				break;
			}
		}

		if (doAction)
			continue;

		uList barracks = INFO.getTypeBuildingsInRadius(Terran_Barracks, S, scv->pos(), TILE_SIZE * 10);

		for (auto b : barracks)
		{
			if (b->getState() == "NeedRepair")
			{
				CommandUtil::repair(scv->unit(), b->unit());
				doAction = true;

				break;
			}
		}

		if (doAction)
			continue;

		uList tanks = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, scv->pos(), TILE_SIZE * 10);

		for (auto t : tanks)
		{
			if (t->hp() < t->type().maxHitPoints() && t->getState() != "Dropship" && t->getState() != "KeepMulti" && t->getState() != "MultiBreak")
			{
				CommandUtil::repair(scv->unit(), t->unit());
				doAction = true;

				break;
			}
		}

		if (doAction)
			continue;

		uList goliath = INFO.getTypeUnitsInRadius(Terran_Goliath, S, scv->pos(), TILE_SIZE * 10);

		for (auto g : goliath)
		{
			if (g->hp() < g->type().maxHitPoints() && g->getState() != "Dropship" && g->getState() != "KeepMulti" && g->getState() != "MultiBreak")
			{
				CommandUtil::repair(scv->unit(), g->unit());
				doAction = true;

				break;
			}
		}

		if (doAction)
			continue;

		uList scvs = INFO.getTypeUnitsInRadius(Terran_SCV, S, scv->pos(), TILE_SIZE * 10);

		for (auto s : scvs)
		{
			if (s->id() != scv->id() && s->hp() < s->type().maxHitPoints() && scv->getState() == "Nightingale")
			{
				CommandUtil::repair(scv->unit(), s->unit());
				doAction = true;

				break;
			}
		}

		if (doAction)
			continue;

		if (checkZeroAltitueAroundMe(scv->pos(), 6)) {
			CommandUtil::move(scv->unit(), SM.getMainAttackPosition());
		}
	}
}

bool ScvNightingaleSet::buildTurret(UnitInfo *scv, bool canBuildTurret, int &builderId, int TURRET_GAP) {

	uList turrets = INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, S);

	for (auto t : turrets) {
		if (!t->isComplete() && !CM.existConstructionQueueItem(Terran_Missile_Turret, t->unit()->getTilePosition(), 0)) {
			bool isConstructing = false;
			uList scvs = INFO.getTypeUnitsInRadius(Terran_SCV, S, t->pos(), TILE_SIZE);

			for (auto s : scvs) {
				if ((s->unit()->getLastCommand().getType() == UnitCommandTypes::Right_Click_Unit && s->unit()->getLastCommand().getTarget() == t->unit())
						|| (s->unit()->getLastCommand().getType() == UnitCommandTypes::Build && s->unit()->getLastCommand().getTargetTilePosition() == t->unit()->getTilePosition())) {
					isConstructing = true;
					break;
				}
			}

			if (!isConstructing) {
				CommandUtil::rightClick(scv->unit(), t->unit());
				return true;
			}
		}
	}

	
	if (canBuildTurret) {
		
		TilePosition turretPos = TilePositions::None;
		bool next = false;

		do {
			turretPos = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Missile_Turret, (TilePosition)scv->pos(), next);
			next = true;
		} while (turretPos != TilePositions::None && INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, S, (Position)turretPos, TURRET_GAP * TILE_SIZE).size() > 0);

		if (!turretPos.isValid())
			return false;

	
		if (isSameArea((Position)turretPos, INFO.getMainBaseLocation(S)->Center()) || (INFO.getFirstExpansionLocation(S) && isSameArea((Position)turretPos, INFO.getFirstExpansionLocation(S)->Center())))
			return false;

		
		if (turretPos.getApproxDistance((TilePosition)INFO.getSecondChokePosition(S)) > 10) {
			scv->unit()->move((Position)turretPos + (Position)Terran_Missile_Turret.tileSize() / 2);
			scv->unit()->build(Terran_Missile_Turret, turretPos);
			builderId = scv->id();
			return true;
		}
	}

	return false;
}

MyBot::FirstChokeDefenceSet::FirstChokeDefenceSet()
{
	if (waitingPositionAtFirstChoke == Positions::None) {
		waitingPositionAtFirstChoke = INFO.getWaitingPositionAtFirstChoke(4, 6);
	}
}

void FirstChokeDefenceSet::init()
{
	for (auto scv : getUnits())
		scv->setState(new ScvIdleState());

	clear();
}

void FirstChokeDefenceSet::action()
{
	bool moveToFirstExpansionFlag = INFO.isTimeToMoveCommandCenterToFirstExpansion();

	Position waitingPosition = (waitingPositionAtFirstChoke + (Position)INFO.getFirstChokePoint(S)->Center() * 2) / 3;

	for (auto scv : getUnits()) {
		if (moveToFirstExpansionFlag) {

			UnitInfo *closeUnit = INFO.getClosestUnit(E, scv->pos(), TypeKind::GroundCombatKind, 20 * TILE_SIZE, false);
			UnitInfo *closeTank = INFO.getClosestTypeUnit(S, scv->pos(), Terran_Siege_Tank_Tank_Mode, 0, false, true);

			if (closeUnit == nullptr) {
				if (INFO.getFirstExpansionLocation(S)) {
					uList tList = INFO.getTypeBuildingsInArea(Terran_Missile_Turret, S, INFO.getFirstExpansionLocation(S)->Center());

					if (!tList.empty()) {
						CommandUtil::attackMove(scv->unit(), tList.at(0)->pos());
					}
					else {
						if (closeTank) {
							if (closeTank->pos().getApproxDistance(scv->pos()) > 3 * TILE_SIZE)
								CommandUtil::attackMove(scv->unit(), closeTank->pos());
							else
								CommandUtil::attackMove(scv->unit(), INFO.getSecondAverageChokePosition(S));
						}
						else
							CommandUtil::attackMove(scv->unit(), INFO.getSecondAverageChokePosition(S));
					}
				}

				continue;
			}

			TankFirstExpansionSecureSet tankSet = TM.getTankFirstExpansionSecureSet();
			uList tanks = tankSet.getSortedUnitList(scv->pos());

			if (!tanks.empty()) {

				UnitInfo *fisrtTank = tanks.at(0);

				
				if (fisrtTank != nullptr) {

					if (scv->unit()->getDistance(fisrtTank->pos()) < 5 * TILE_SIZE) {
						CommandUtil::attackUnit(scv->unit(), closeUnit->unit());
					}
					else {
						CommandUtil::attackMove(scv->unit(), fisrtTank->pos());
					}
				}

			}
		}
		else {

			uList enemy = INFO.getUnitsInRadius(E, (Position)INFO.getFirstChokePoint(S)->Center(), 10 * TILE_SIZE, true, false, false, false);

			
			if (enemy.empty()) {
				if (scv->unit()->getDistance(waitingPosition) > 24)
					CommandUtil::move(scv->unit(), waitingPosition, true);
			}
			else {

				if (scv->unit()->getDistance(waitingPosition) > 32) {
					CommandUtil::move(scv->unit(), waitingPosition, true);
				}
				else {

					if (getUnits().size() > 1) {
						for (auto anotherScv : getUnits()) {
							if (anotherScv->unit() == scv->unit())
								continue;

							if (anotherScv->unit()->getHitPoints() < 60) {
								CommandUtil::repair(scv->unit(), anotherScv->unit(), true);
							}
							else {
								
								UnitInfo *e = INFO.getClosestUnit(E, scv->pos(), TypeKind::GroundCombatKind, (int)(3.5 * TILE_SIZE), false, false);

								if (e != nullptr)
									CommandUtil::attackUnit(scv->unit(), e->unit());
								else
									CommandUtil::hold(scv->unit());
							}

							break;
						}
					}
					else {
						
						UnitInfo *e = INFO.getClosestUnit(E, scv->pos(), TypeKind::GroundCombatKind, (int)(3.5 * TILE_SIZE), false, false);

						if (e != nullptr)
							CommandUtil::attackUnit(scv->unit(), e->unit());
						else
							CommandUtil::hold(scv->unit());

					}
				}
			}
		}
	}

	return;
}

void ScvBunkerDefenceSet::init()
{
	for (auto scv : getUnits())
		scv->setState(new ScvIdleState());

	clear();
}

void ScvBunkerDefenceSet::action()
{
	if (size() == 0) return;

	
	Unit mbunker = MM.getBunker();

	if (mbunker == nullptr)
		return;

	UnitInfo *bunker = INFO.getUnitInfo(mbunker, S);
	int bunkerAttacker = bunker->getEnemiesTargetMe().size();

	UnitInfo *barrack = INFO.getClosestTypeUnit(S, MYBASE, Terran_Barracks);
	UnitInfo *enbe = INFO.getClosestTypeUnit(S, MYBASE, Terran_Engineering_Bay);

	uList turrets = INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, S, bunker->pos(), 5 * TILE_SIZE, false);
	Unit needRepairTurret = nullptr;

	for (auto t : turrets)
	{
		if (t->hp() < Terran_Missile_Turret.maxHitPoints())
		{
			needRepairTurret = t->unit();
			continue;
		}
	}

	Position safePos = Positions::Origin;
	Position safeT = Positions::Origin;

	if (INFO.enemyRace == Races::Zerg)
	{
		UnitInfo *closestLurker = INFO.getClosestTypeUnit(E, bunker->pos(), Zerg_Lurker, 8 * TILE_SIZE, true, false, false);

		if (closestLurker)
		{
			safePos = getDirectionDistancePosition(bunker->pos(), closestLurker->pos(), -50);

			if (needRepairTurret)
			{
				safeT = getDirectionDistancePosition(needRepairTurret->getPosition(), closestLurker->pos(), -50);
				
			}

			
		}
	}

	for (auto s : getUnits())
	{
		
		if (!bunker->isComplete()) {
			UnitInfo *closeUnit = INFO.getClosestUnit(E, s->pos(), GroundUnitKind, 50 * TILE_SIZE);

			if (closeUnit != nullptr) {
				
				if (s->pos().getApproxDistance(bunker->pos()) > 10 * TILE_SIZE) {
					CommandUtil::move(s->unit(), bunker->pos());
				}
				else {
					CommandUtil::attackUnit(s->unit(), closeUnit->unit());
				}
			}
			else {
				CommandUtil::move(s->unit(), bunker->pos());
			}

			continue;
		}

		if (!isInMyArea(s->pos()) && s->pos().getApproxDistance(bunker->pos()) >= 3 * TILE_SIZE) {
			if (safePos != Positions::Origin)
				CommandUtil::move(s->unit(), safePos);
			else
				CommandUtil::move(s->unit(), bunker->pos());
		}
		else {
			if (bunker->hp() < Terran_Bunker.maxHitPoints() && bunkerAttacker > 0) {
				if (safePos != Positions::Origin)
				{
					if (s->pos().getApproxDistance(safePos) < 50)
					{
						
						CommandUtil::repair(s->unit(), bunker->unit());
					}
					else
						CommandUtil::move(s->unit(), safePos, true);
				}
				else
					CommandUtil::repair(s->unit(), bunker->unit());

				bunkerAttacker--;
			}
			else if (needRepairTurret != nullptr) {

				if (safeT != Positions::Origin)
				{
					if (s->pos().getApproxDistance(safeT) < 50)
					{
						
						CommandUtil::repair(s->unit(), needRepairTurret);
					}
					else
						CommandUtil::move(s->unit(), safeT, true);
				}
				else
					CommandUtil::repair(s->unit(), needRepairTurret);
			}
			else {
				UnitInfo *closeUnit = INFO.getClosestUnit(E, s->pos(), GroundUnitKind, 6 * TILE_SIZE);

				if (closeUnit != nullptr) {
					int guardThreshold = closeUnit->type().groundWeapon().maxRange() >= 2 * TILE_SIZE ? 3 : 1;

					if (s->pos().getApproxDistance(closeUnit->pos()) <= guardThreshold * TILE_SIZE) {
						s->unit()->attack(closeUnit->unit());
						s->unit()->move(closeUnit->vPos());
					}
				}
				else {
					if (bunker->hp() < Terran_Bunker.maxHitPoints())
					{
						if (safePos != Positions::Origin)
						{
							
							if (s->pos().getApproxDistance(safePos) < 50)
							{
								
								CommandUtil::repair(s->unit(), bunker->unit());
							}
							else
								CommandUtil::move(s->unit(), safePos, true);
						}
						else
							CommandUtil::repair(s->unit(), bunker->unit());
					}
					else
					{
						if (barrack != nullptr && barrack->getState() == "Barricade" && barrack->hp() < Terran_Barracks.maxHitPoints())
							CommandUtil::repair(s->unit(), barrack->unit());
						else if (enbe != nullptr && enbe->getState() == "Barricade" && enbe->hp() < Terran_Engineering_Bay.maxHitPoints())
							CommandUtil::repair(s->unit(), enbe->unit());
						else
						{
							if (safePos != Positions::Origin)
								CommandUtil::move(s->unit(), safePos, true);
							else
								CommandUtil::move(s->unit(), bunker->pos());
						}
					}
				}
			}
		}
	}
}

void ScvRemoveBlockingSet::init()
{
	for (auto scv : getUnits())
		scv->setState(new ScvIdleState());

	clear();
}

void ScvRemoveBlockingSet::action(Position p)
{
	const Area *a = theMap.GetArea((TilePosition)p);

	if (a == nullptr) return;

	for (auto s : getUnits())
	{
		for (auto cp : a->ChokePoints())
		{
			if (cp->BlockingNeutral() != nullptr)
			{
				if (!cp->BlockingNeutral()->Unit()->isVisible())
				{
					CommandUtil::move(s->unit(), (Position)cp->BlockingNeutral()->Pos());
				}
				else
				{
					UnitCommand currentCommand(s->unit()->getLastCommand());

					
					if ((currentCommand.getType() == UnitCommandTypes::Gather) && (currentCommand.getTarget() == cp->BlockingNeutral()->Unit()))
					{
						return;
					}

					s->unit()->gather(cp->BlockingNeutral()->Unit());

				}

			}
		}
	}
}
