#include "ScvManager.h"
#include "../EnemyStrategyManager.h"
#include "TankManager.h"
#include "MarineManager.h"
#include "../TrainManager.h"
#include <time.h>

//#include "../UnitInfo.h"

using namespace MyBot;

ScvManager::ScvManager()
{
	refineryScvCnt = 3;
	repairScvCnt = 1;
	MaxRepairScvCnt = 10; // DefenceBunker를 제외하고 일반 Repair Scv의 Max Count이다.
	scanMyBaseUnit = nullptr;
	totalScoutUnitDefenceScvCount = 0;
	totalEarlyRushDefenceScvCount = 0;
	MineralStateScvCount = 0;
	GasStateScvCount = 0;
	RepairStateScvCount = 0;
	memset(time, 0, sizeof(time));
}

ScvManager &ScvManager::Instance()
{
	static ScvManager instance;
	return instance;
}

void ScvManager::update()
{
	if (TIME % FR != 0)	{
		return;
	}

	CheckDepot(); // Command Center의 Lift / Land에 따른 Check
	CheckRefineryScv(); // Refinery에 붙는 scv에 대한 Check
	CheckRepairScv(); // 건물에 Repair를 붙이기 위한 Check
	CheckBunkerDefence();
	//CheckFirstChokeDefence();
	CheckCombatSet();
	CheckWorkerDefence();
	CheckEnemyScoutUnit();
	CheckEarlyRush();
	CheckNightingale();

	int factoryCnt = 4;

	/*if (INFO.enemyRace == Races::Zerg)
	{
		factoryCnt = 5;
	}*/
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

	// Idle로 먼저 변경해주면서 Frame Delay를 없앤다
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
		else if (state == "Combat")// || state == "WorkerCombat")
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

	////////////////////////////
	combatSet.action();
	nightingaleSet.action();
	firstChokeDefenceSet.action();
	bunkerDefenceSet.action();
	removeBlockingSet.action(checkBase);
}

void	ScvManager::setMineralScv(UnitInfo *scv, Unit center)
{
	///// 在scv中查找最有效的基地。 
	if (center == nullptr)
		center = getNearestDepot(scv);

	if (center == nullptr)
	{
		//采在最近的安全中心的mineral上。 
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
		//在该中心寻找最需要的mineral。 
		Unit mineral = getBestMineral(center);

		if (mineral != nullptr)
		{
			// Mineral State로 변경하고 Mineral에 SCV Count를 증가 시킨다.
			setStateToSCV(scv, new ScvMineralState(mineral));

			// 자원 들고 있을 때 자원 반납
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

// 기능 : Depot에서 가장 필요한 Mineral을 찾는 로직
// 동작 : 가장 SCV가 적게 붙어있는 Mineral 위주로. 그중에 가장 가까운 Mineral을 고른다.
// 리턴 : 모두 2마리씩 할당되어 있으면 nullptr, 아니면 Mineral Unit
Unit ScvManager::getBestMineral(Unit depot)
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

// 기능 : 해당 Postion에서 가장 가까이 있는 Center
// 동작 : Center 들을 조회해서 유효한 Center중 가장 가까운 Center를 찾는다.
// 리턴 : Center Unit
Unit ScvManager::getNearestDepot(UnitInfo *scv)
{
	Unit depot = nullptr;
	int dist = INT_MAX;

	uList DepotList = INFO.getBuildings(Terran_Command_Center, S);

	if (DepotList.size() == 1)
		return DepotList[0]->unit();

	for (auto c : DepotList)
	{
		//공중에 떠 있거나 완료 안되거나 일꾼이 이미 가득이면 skip
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

// Position에서 가장 가까운 안전한 Mineral
Unit ScvManager::getTemporaryMineral(Position pos)
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

// 기능 : Refinery 근처에 Center가 있는지를 확인
// 동작 : Refinery 근처에 10 * 32 에 유효 센터가 있는지 확인.
// 리턴 : True or False
bool	ScvManager::isDepotNear(Unit refinery)
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

// 기능 : 해당 CommandCenter의 주변 Mineral 초기화
// 동작 : Command Center에서 5 * 32 사이에 있는 Base를 찾는다.
//		  해당 Base를 DepotMap에 저장하고 Base의 Mineral을 거리별로 Sorting해서
//		  DepotMineral Map에 저장한다.
void ScvManager::initMineralsNearDepot(Unit depot)
{
	if (depotMineralMap.find(depot) == depotMineralMap.end())
		depotMineralMap[depot] = vector<Unit>();
	else
		depotMineralMap[depot].resize(0);

	vector<pair<int, Unit>> sortVect;

	// Depot에 가장 가까운 Base Location을 찾는다.
	Base *baselocation = INFO.getNearestBaseLocation(depot->getPosition());

	// Base와 Center가 멀면 처리하지 않는다. Center를 본진에 짓고 앞마당에 내릴때를 고려
	if (depot->getDistance(baselocation->getPosition()) > 5 * TILE_SIZE) {
		depotBaseMap[depot] = nullptr;
		return;
	}

	depotBaseMap[depot] = baselocation;
	baseSafeMap[baselocation] = true;

	for (auto m : baselocation->Minerals())
	{
		//if (m->Unit()->getType() == Resource_Mineral_Field)
		sortVect.push_back(pair<int, Unit>(m->Pos().getApproxDistance(depot->getPosition()), m->Unit()));
	}

	if ( sortVect.size() )
	{
		sort(sortVect.begin(), sortVect.end(), [](pair<int, Unit> a, pair<int, Unit> b) {
			return a.first < b.first;
		} );
		// 5 4 3 2 1 로 정렬해서 추가해놓는다.
		vector<pair<int, Unit>>::iterator iter;

		for (iter = sortVect.begin(); iter != sortVect.end(); iter++)
			depotMineralMap[depot].push_back(iter->second);
	}
}

// 기능 : Command가 Mineral SCV가 충분히 있는지
// 동작 : 2마리씩 안붙어 있으면 false
// 리턴 : true / false
bool ScvManager::depotHasEnoughMineralWorkers(Unit depot)
{
	for (auto m : depotMineralMap[depot])
	{
		if (m->exists() && getMineralScvCount(m) < 2)
			return false;
	}

	return true;
}

// 기능 : Mineral에 붙어있는 SCV 갯수
// 동작 : map이 만들어져 있지 않으면 생성
// 리턴 : 해당 map의 value(Scv 갯수)
int		ScvManager::getMineralScvCount(Unit m)
{
	if (mineralScvCountMap.find(m) == mineralScvCountMap.end())
		return mineralScvCountMap[m] = 0;
	else
		return mineralScvCountMap[m];
}

// 기능 : Refinery에 붙어있는 SCV 갯수
int		ScvManager::getRefineryScvCount(Unit m)
{
	if (refineryScvCountMap.find(m) == refineryScvCountMap.end())
		return refineryScvCountMap[m] = 0;
	else
		return refineryScvCountMap[m];
}

// 기능 : Repair 되고 있는 Unit에 붙어있는 SCV 갯수
int		ScvManager::getRepairScvCount(Unit m)
{
	if (repairScvCountMap.find(m) == repairScvCountMap.end())
		return repairScvCountMap[m] = 0;
	else
		return repairScvCountMap[m];
}

// 기능 : 초반 방어타워 러시 건물에 붙어있는 SCV 갯수
int		ScvManager::getScoutUnitDefenceScvCountMap(Unit m)
{
	if (scoutUnitDefenceScvCountMap.find(m) == scoutUnitDefenceScvCountMap.end())
		return scoutUnitDefenceScvCountMap[m] = 0;
	else
		return scoutUnitDefenceScvCountMap[m];
}

// 기능 : 초반 방어타워 러시 건물에 붙어있는 SCV 갯수
int		ScvManager::getEarlyRushDefenceScvCount(Unit m)
{
	if (earlyRushDefenceScvCountMap.find(m) == earlyRushDefenceScvCountMap.end())
		return earlyRushDefenceScvCountMap[m] = 0;
	else
		return earlyRushDefenceScvCountMap[m];
}

// 기능 : Command Center의 미네랄에 붙어 있는 총 SCV 갯수
int		ScvManager::getAssignedScvCount(Unit depot)
{
	if (!depot->isCompleted() || depot->isFlying())
		return 0;

	int scvCnt = 0;

	for (auto m : depotMineralMap[depot])
		scvCnt += getMineralScvCount(m);

	return scvCnt;
}

// 기능 : 해당 디팟에 있는 미네랄의 평균값을 리턴
int ScvManager::getRemainingAverageMineral(Unit depot)
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

// 기능 : Refinery 필요 갯수에 맞게 SCV를 Refinery에 보냄.
// 동작 : Mineral 또는 Idle SCV중 가장 가까운 SCV를 필요 갯수만큼 Refinery에 보낸다.
//		  SCV 필요 갯수보다 큰 경우 SCV를 뺀다.
void	ScvManager::CheckRefineryScv()
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

		// 주변에 center 없으면 skip
		if (!isDepotNear(r->unit()))
			continue;

		// SCV 부족한 경우
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
				// 가스 채취중이며 Gas를 들고 있지 않은 경우에는 Idle로 변경 이때 가스 들어가기 직전 SCV는 명령수행 불가하므로 거리가 어느정도 멀어진 SCV를 골라야 한다.
				if (scv->getState() == "Gas" && scv->getTarget() == r->unit() && !scv->unit()->isCarryingGas() && scv->pos().getApproxDistance(r->pos()) > 80)
				{
					setStateToSCV(scv, new ScvIdleState());

					// 다 뽑았으면 return;
					if (refineryScvCountMap[r->unit()] == refineryScvCnt)
						return;
				}
			}
		}
		else {}
	}
}

// 기능 : Command Center들을 Check해서 Initializing을 수행
// 동작 : Command Center가 Lift 되었다가 Land 되는 순간 Initialzing / Rebalancing필요
//        Command Ceter의 Mineral 갯수가 이전과 다른경우 Initialzing / Rebalancing 수행
//		  (확장 건물 지어질 때 시야 문제로 Mineral Map을 제대로 구성하지 못하는 케이스를 보완 함.)
void ScvManager::CheckDepot(void) {
	for (auto c : INFO.getBuildings(Terran_Command_Center, S))
	{
		if (!c->isComplete() && !c->unit()->isUnderAttack() && (c->hp() == 1300 || c->hp() == 1301))
		{
			Base *baselocation = INFO.getNearestBaseLocation(c->pos());

			// Base와 Center가 멀면 처리하지 않는다. Center를 본진에 짓고 앞마당에 내릴때를 고려
			if (c->unit()->getDistance(baselocation->getPosition()) > 5 * TILE_SIZE) {
				continue;
			}

			preRebalancingNewDepot(c->unit(), baselocation->Minerals().size());
		}

		if (!c->isComplete())
			continue;

		// 계속 내려앉아 있는 상태라면 Mineral 갯수를 비교해서 변동 사항이 있으면 InitMineral & Rebalancing을 다시 한다.
		// 이는 멀티를 했을 경우 시야 부족으로 미네랄이 가려질때 비정상적으로 Map에 저장되는 것을 보완하기 위함이다.
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
				// Dangerous로 변경 됨.
				if (depotBaseMap[c->unit()]->GetDangerousAreaForWorkers())
				{
					if (isExistSafeBase())
						setIdleAroundDepot(c->unit());
				}
				else // Danger -> no Danger로 변경
					rebalancingNewDepot(c->unit());
			}

			baseSafeMap[depotBaseMap[c->unit()]] = depotBaseMap[c->unit()]->GetDangerousAreaForWorkers();
		}
	}
}

bool ScvManager::isExistSafeBase(void)
{
	for (auto c : INFO.getBuildings(Terran_Command_Center, S))
	{
		if (depotBaseMap[c->unit()] != nullptr && depotBaseMap[c->unit()]->GetDangerousAreaForWorkers() == false)
			return true;
	}

	return false;
}
// 기능 : 모든 건물들 중 Hp가 고갈된 건물에 Repair 유닛을 지정해준다.
// TODO : 현재는 Building에만 국한되어 있으나 추후에 Unit으로 확장 필요
void ScvManager::CheckRepairScv(void)
{
	// 할당 가능 Repair Count 초과
	if (RepairStateScvCount > MaxRepairScvCnt)
	{
		cout << "할당가능 Repair Count 초과" << endl;
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

		// 기본 1마리 SCV
		repairScvCnt = 1;

		// 기본 30% 이하일때만
		double needRepairThreshold = 0.3;

		// Bunker랑 Turret은 100%
		if (b.second->type() == Terran_Bunker || b.second->type() == Terran_Missile_Turret ||
				(b.second->type() == Terran_Barracks && b.second->getState() == "Barricade"))
			needRepairThreshold = 1;

		bool needPreRepair = false;

		// 터렛은 주변 무탈수에 맞게 Repair Scv를 늘린다.
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
				// 내 본진에 있는 Turret이고 주변에 Dark가 있어
				if (isSameArea(MYBASE, b.second->pos()) &&
						INFO.getTypeUnitsInRadius(Protoss_Dark_Templar, E, b.second->pos(), 8 * TILE_SIZE).size())
				{
					needPreRepair = true;
					repairScvCnt = 6;
				}
			}
		}

		// 터렛이 없고 컴셋만 있는 경우
		if (INFO.enemyRace == Races::Protoss)
		{
			if (b.second->type() == Terran_Comsat_Station
					&& INFO.getTypeBuildingsInArea(Terran_Missile_Turret, S, MYBASE, true).empty()) {
				// 내 본진에 있는 Comsat이고 주변에 Dark가 있어
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

	// Dropship과 Vessle은 항상 repair 한다.
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
			if (chooseRepairScvClosestTo(d->unit(), INT_MAX, true) == nullptr) // Dropship은 Gas 일꾼도 repair 로 전환할 수 있다.
				return;
		}
	}

	//KeepMulti 유닛은 수리한다
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

	//Additional multi 수리 추가
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

	// 그 외 유닛은 초반에만 수리한다.
	if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion)
	{
		if (INFO.enemyInMyArea().size() == 0)
		{
			// 내 완성된 Command Center와 같은 Area에 있는 Repair 가능 유닛은 수리하도록 한다.
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
void ScvManager::CheckBunkerDefence()
{
	int facCnt = INFO.getCompletedCount(Terran_Factory, S);

	if (facCnt == 0 || facCnt > 2 || ESM.getEnemyInitialBuild() <= Zerg_5_Drone && ESM.getEnemyMainBuild() != Zerg_main_zergling)
	{
		bunkerDefenceSet.init();
		return;
	}

	Unit mbunker = MM.getBunker();

	if (mbunker == nullptr) { // 벙커 1개일 경우만 생각한다.
		bunkerDefenceSet.init();
		return;
	}

	UnitInfo *bunker = INFO.getUnitInfo(mbunker, S);

	// 벙커가 본진에 있는데 확장을 건설 중인 경우는 필요 없음.
	if (isSameArea(MYBASE, bunker->pos()) && INFO.getAllCount(Terran_Command_Center, S) > 1) {
		bunkerDefenceSet.init();
		return;
	}

	word bunkerDefenceNeedCnt = 0;
	word needRightNowCnt = 0;

	// SCV 가 벙커까지 도달하는 시간 구하기
	Position basePos;

	// 벙커가 본진에 있거나 확장을 안 지은 경우
	if (INFO.getCompletedCount(Terran_Command_Center, S) == 1 || isSameArea(bunker->pos(), MYBASE)) {
		// 본진에서 벙커까지의 거리
		basePos = MYBASE;
	}
	else {
		// 확장에서 벙커까지의 거리
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
		// 테란은 기본적으로 벙커 디팬스를 하지 않지만, 적 마린이 많이 몰려올 경우에는 수리를 해줄 필요가 있다.
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

	// 멀리서 오는 경우에도 2마리 이상 붙여놓지 않는다.
	if (bunkerDefenceNeedCnt > 2)
		bunkerDefenceNeedCnt = 2;

	// 2게이트 사업 드라 푸시일땐 앞마당 완성 안되어 있을 때 미리미리 나가있자
	if (INFO.enemyRace == Races::Protoss && ESM.getEnemyMainBuild() == Toss_2gate_dra_push
			&& INFO.getCompletedCount(Terran_Command_Center, S) < 2)
		bunkerDefenceNeedCnt += 2;

	// 최대 수리 scv 수는 6마리를 넘지 않는다.
	if (needRightNowCnt > 6)
		needRightNowCnt = 6;

	// 필요한 scv 수.
	word baseCount = max(bunkerDefenceNeedCnt, needRightNowCnt);
	if (SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller)
		baseCount = 2;


	// 디팬스가 적으면 늘인다.
	while (bunkerDefenceSet.size() < baseCount) {
		UnitInfo *closestScv = chooseScvClosestTo(bunker->pos());

		if (closestScv == nullptr)
			break;

		setStateToSCV(closestScv, new ScvBunkerDefenceState());
	}

	// 디팬스가 많으면 줄인다.
	if (bunkerDefenceSet.size() > baseCount) {
		// 벙커 hp 가 50% 이상일때만 줄인다.
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

void ScvManager::CheckFirstChokeDefence()
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

void ScvManager::CheckCombatSet()
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

		// 전진 게이트일때 벙커 있으면 정리될때까지 Combat 하지 않는다.
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

		// 마린하고 scv하고 따로 놀기때문에 SCV가 너무 쉽게 죽는다.
		// 최소 2마리는 유지시키자
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
	else// if (INFO.enemyRace == Races::Zerg)
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
				// 벙커완성되어 있고 마린 2마리 있을 때에 Init 한다.
				if (INFO.getCompletedCount(Terran_Bunker, S) > 0 && marineCnt > 1 && (int)INFO.getTypeUnitsInArea(Zerg_Zergling, E, MYBASE).size() < marineCnt * 3)
				{
					combatSet.init();
				}
				else
				{
					setNeedCountForRefinery(0);

					// 6마리보다 적게 죽은 경우 최초 방어라 판단하고 동시에 출발시키기 위해 4마리 빼고 전부를 combat으로 할당해준다.
					word needScvCount = INFO.getDestroyedCount(Zerg_Zergling, E) < 6 ?
										(ScvList.size() > 4 ? ScvList.size() - 4 : ScvList.size())
										// 6마리 이상 죽은 경우 적의 저글링 숫자를 보고 맞춰서 scv 방어를 해준다.
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

			// 저글링 수가 너무 많으면 SCV도 싸운다.
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

// 기능 : 해당 SCV 를 Idle로 변경한다.
// 동작 : Construction Manager 나 Scout Manager에서 해당 job을 완료하고 Idle로 변경할 때 사용하는 API
//		  UnitData에서 해당 UnitInfo를 찾아서 Idle로 변경한다.
void ScvManager::setScvIdleState(Unit u)
{
	setStateToSCV(INFO.getUnitInfo(u, S), new ScvIdleState());
}

// 기능 : Command Center가 Destroy 되거나 Lift 되었을 때 주변의 모든 자원 캐는 SCV를 Idle로 변경한다.
void	ScvManager::setIdleAroundDepot(Unit depot)
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

// 기능 : 해당 Tile Position에서 건설 SCV를 Select 한다.
// 동작 : avoid Scv ID는 할당하지 않는다.
Unit ScvManager::chooseConstuctionScvClosestTo(TilePosition buildingPosition, int avoidScvID)
{
	UnitInfo *closestScv = chooseScvClosestTo((Position)buildingPosition, avoidScvID);

	return setStateToSCV(closestScv, new ScvBuildState());
}

// 기능 : 해당 Position에서 가장 가까운 Scout SCV를 Select 한다.
UnitInfo *ScvManager::chooseScouterScvClosestTo(Position p)
{
	UnitInfo *closestScv = chooseScvClosestTo(p);
	setStateToSCV(closestScv, new ScvScoutState());

	return closestScv;
}

// 기능 : 해당 Unit에서 가장 가까운 Repair SCV를 Select 한다.
Unit	ScvManager::chooseRepairScvClosestTo(Unit unit, int maxRange, bool withGas)
{
	UnitInfo *closestScv = chooseScvClosestTo(unit->getPosition(), unit->getID(), maxRange, withGas);
	return setStateToSCV(closestScv, new ScvRepairState(unit));
}

// 기능 : 해당 Position에서 가장 가까운 SCV를 Select 한다. avoid Scv ID는 Select 하지 않는다.
UnitInfo *ScvManager::chooseScvClosestTo(Position position, int avoidScvID, int maxRange, bool withGas) {
	// variables to hold the closest worker of each type to the building
	UnitInfo *closestScv = nullptr;
	int closestdist = maxRange;

	uList ScvList = INFO.getUnits(Terran_SCV, S);

	// look through each worker that had moved there first
	for (auto scv : ScvList)
	{
		// worker 가 2개 이상이면, avoidWorkerID 는 피한다
		if (scv->id() == avoidScvID) continue;

		bool candidate = scv->unit()->isInterruptible() && (scv->getState() == "Idle" || (scv->getState() == "Mineral" && !scv->unit()->canReturnCargo() && !scv->isGatheringMinerals() && scv->unit()->canCommand()));

		if (withGas && scv->getState() == "Gas")	candidate = true;

		// Move / Idle Worker
		if (candidate)
		{
			// if it is a new closest distance, set the pointer
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

// 기능 : 해당 Unit에서 가장 가까운 Repair SCV를 Select 한다.
Unit	ScvManager::chooseRepairScvforScv(Unit unit, int maxRange, bool withGas)
{
	UnitInfo *closestScv = chooseScvForScv(unit->getPosition(), unit->getID(), maxRange, withGas);
	return setStateToSCV(closestScv, new ScvRepairState(unit));
}

// 기능 : 해당 Position에서 HP가 가장 낮은 SCV를 Select 한다. avoid Scv ID는 Select 하지 않는다.
UnitInfo *ScvManager::chooseScvForScv(Position position, int avoidScvID, int maxRange, bool withGas) {
	// variables to hold the closest worker of each type to the building
	UnitInfo *weakestScv = nullptr;
	int hp = Terran_SCV.maxHitPoints();

	uList ScvList = INFO.getTypeUnitsInRadius(Terran_SCV, S, position, maxRange);

	// look through each worker that had moved there first
	for (auto scv : ScvList)
	{
		//avoidWorkerID 는 피한다
		if (scv->id() == avoidScvID) continue;

		bool candidate = scv->unit()->isInterruptible() && (scv->getState() == "Idle" || (scv->getState() == "Mineral" && !scv->unit()->canReturnCargo() && !scv->isGatheringMinerals() && scv->unit()->canCommand()));

		if (withGas && scv->getState() == "Gas")	candidate = true;

		// Move / Idle Worker
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

// setState 전처리 작업.
void ScvManager::beforeRemoveState(UnitInfo *scv) {
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

// 기능 : scv 에게 state 를 할당한다.
Unit ScvManager::setStateToSCV(UnitInfo *scv, State *state) {
	if (scv == nullptr) {
		delete state;
		return nullptr;
	}

	// 할당 전 스테이트
	beforeRemoveState(scv);

	scv->setState(state);

	string stateName = scv->getState();

	// 할당 후 스테이트
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

// 기능 : Center가 새로 건설되었거나 새 지점으로 Land 했을 때 일꾼을 재분배 한다.
// 동작 : 해당 Center의 Mineral 중에 SCV가 할당되지 않은 Mineral 갯수만큼 보충한다.
//		  보충 기준은 1) Idle 2) Mineral 2덩이 캐고 있는 SCV
void ScvManager::rebalancingNewDepot(Unit depot)
{
	uList ScvList = INFO.getUnits(Terran_SCV, S);

	int newMineralCnt = 0;

	for (auto m : depotMineralMap[depot]) {
		if (getMineralScvCount(m) == 0)
			newMineralCnt++;
	}

	// PreMineral 부터 보낸다.
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

	// Mineral에 2개 붙어있는 Scv들을 부터 보낸다.
	for (auto scv : ScvList)
	{
		if (newMineralCnt <= 0)
			return;

		if (scv->getState() == "Mineral" && !scv->unit()->canReturnCargo() && !scv->isGatheringMinerals())
		{
			if (mineralScvCountMap[scv->getTarget()] > 1) // Mineral에 2개 이상 붙어있는 경우
			{
				setMineralScv(scv, depot);
				newMineralCnt--;
			}
		}
	}
}

void ScvManager::preRebalancingNewDepot(Unit depot, int cnt)
{
	Unit mineral = depot->getClosestUnit(Filter::IsMineralField, 8 * TILE_SIZE);

	if (mineral == nullptr)
		return;

	uList ScvList = INFO.getUnits(Terran_SCV, S);

	int newMineralCnt = cnt;

	// Mineral에 2개 붙어있는 Scv들을 부터 보낸다.
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

void	ScvManager::onUnitCompleted(Unit u)
{
	if (u->getType().isResourceDepot())
	{
		initMineralsNearDepot(u);
		rebalancingNewDepot(u);
	}
}

void	ScvManager::onUnitDestroyed(Unit u)
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

void	ScvManager::onUnitLifted(Unit depot)
{
	if (depot->getType() != Terran_Command_Center)
		return;

	if (depotMineralMap.find(depot) != depotMineralMap.end())
		depotMineralMap.erase(depot);

	if (depotBaseMap.find(depot) != depotBaseMap.end())
		depotBaseMap.erase(depot);

	setIdleAroundDepot(depot);
}

void	ScvManager::onUnitLanded(Unit depot)
{
	if (depot->getType() != Terran_Command_Center)
		return;

	initMineralsNearDepot(depot);
	rebalancingNewDepot(depot);
}

void ScvManager::CheckWorkerDefence()
{
	// 농봉
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

		// 적 일꾼이 2마리 이상 본진 근처 (일꾼 러쉬 대비)
		if (eWorkers.size() > 0)
		{
			// 미네랄 일꾼은 최소 1마리 이상 유지
			if (MineralStateScvCount <= 1 || workerCombatCnt >= (int)eWorkers.size() + 2)	return;

			if (ScvList.size() < eWorkers.size()) return;

			for (auto scv : ScvList)
			{
				if (scv->getState() == "Mineral" )//&& scv->hp() >= 35)
				{
					setStateToSCV(scv, new ScvWorkerCombatState());
					workerCombatCnt++;

					// 미네랄 일꾼은 최소 1마리 이상 & 적 일꾼 수 + 2 마리만 지정
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

void ScvManager::CheckEnemyScoutUnit() {
	if (ESM.getEnemyInitialBuild() == Toss_4probes || ESM.getEnemyInitialBuild() == Terran_4scv || ESM.getEnemyInitialBuild() == Zerg_4_Drone_Real || SM.getMainStrategy() != WaitToBase)
		return;

	vector<UnitInfo *>enemyScouters = getEnemyScountInMyArea();

	if (enemyScouters.empty())
		return;

	// 캐논 러시인 경우 일꾼당 한마리씩 무조건 따라다닌다.
	if (ESM.getEnemyInitialBuild() == Toss_cannon_rush) {
		for (auto e : enemyScouters) {
			for (int i = getScoutUnitDefenceScvCountMap(e->unit()); i < 1; i++) {
				// 그 외에는 일꾼 한마리만 붙는다. (기존로직에서 마린이 있어도 일단 붙도록 수정됨.)
				UnitInfo *assignedScv = chooseScvClosestTo(e->pos());

				setStateToSCV(assignedScv, new ScvScoutUnitDefenceState(e->unit()));
			}
		}

		return;
	}

	if (totalScoutUnitDefenceScvCount == 0)
	{
		// 그 외에는 일꾼 한마리만 붙는다. (기존로직에서 마린이 있어도 일단 붙도록 수정됨.)
		UnitInfo *assignedScv = chooseScvClosestTo(enemyScouters.at(0)->pos());

		setStateToSCV(assignedScv, new ScvScoutUnitDefenceState(enemyScouters.at(0)->unit()));
	}
}

void ScvManager::CheckEarlyRush() {
	// 가스 러시 대응 전략
	UnitInfo *eRefinery = INFO.getUnitInfo(EnemyStrategyManager::Instance().getEnemyGasRushRefinery(), E);

	// 내 기지 안에 가스를 지은 경우
	if (eRefinery) {
		uList barracks = INFO.getBuildings(Terran_Barracks, S);

		// 배럭 hp 가 절반 차면 scv 를 붙여준다.
		if (barracks.size() == 1 && barracks.at(0)->hp() >= 500) {
			for (int i = getEarlyRushDefenceScvCount(eRefinery->unit()); i < 4; i++) {
				UnitInfo *assignedScv = chooseScvClosestTo(eRefinery->pos());

				setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(eRefinery->unit(), eRefinery->pos()));
			}
		}
	}

	// 초반러시 인 경우만 대응
	if (INFO.enemyRace == Races::Protoss) {
		if (ESM.getEnemyInitialBuild() == UnknownBuild && ESM.getEnemyMainBuild() == UnknownMainBuild) {
			// 포토러시 or 전진게이트 파악 전 정찰
			uList pylons = INFO.getBuildings(Protoss_Pylon, E);

			for (auto pylon : pylons) {
				if (pylon->pos() == Positions::Unknown || !pylon->isHide())
					continue;

				// 내 기지에 더 가깝게 파일런을 지은 경우 항상 시야 내에 있도록 한다.
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
				// 완성된 캐논이 있으면 일꾼들 더 이상 가지 않음. 기존 일꾼들도 철수.
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

			// 건설중인 포토캐논이나 게이트웨이가 있으면, 본진과의 거리, scv 이동속도, 캐논의 빌드 속도, 캐논의 에너지를 고려해서 SCV 할당
			if (!photonCannonsGateways.empty()) {
				for (auto photonCannonGateway : photonCannonsGateways) {
					// 포토가 완성될때까지 남은 시간.
					int leftTime = photonCannonGateway->getRemainingBuildTime();

					int dist;

					theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), photonCannonGateway->pos(), &dist);

					// 도달하는데 걸리는 시간.
					double frameToGetThere = (double)dist / Terran_SCV.topSpeed() * 1.5;
					double shieldsPerTime = (double)photonCannonGateway->type().maxShields() / photonCannonGateway->type().buildTime();
					double HPPerTime = (double)photonCannonGateway->type().maxHitPoints() / photonCannonGateway->type().buildTime();
					double damagePerTime = (double)getDamage(Terran_SCV, photonCannonGateway->type(), S, E) / Terran_SCV.groundWeapon().damageCooldown();

					// 목적지 도달시점 hp : (현재 hp + 증가량 * 이동시간)
					// 완성 될때까지 줄 수 있는 데미지 : (프레임별 데미지 * (남은 빌드시간 - 이동시간))
					// 목적지 도달시점 hp / (프레임별 데미지 * (남은 빌드시간 - 이동시간)) <= 마리수
					int needSCVCnt = (int)ceil((photonCannonGateway->hp() + frameToGetThere * (HPPerTime + shieldsPerTime)) / (damagePerTime * (leftTime - frameToGetThere)));

					for (int i = getEarlyRushDefenceScvCount(photonCannonGateway->unit()); i < needSCVCnt; i++) {
						UnitInfo *assignedScv = chooseScvClosestTo(photonCannonGateway->pos());

						setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(photonCannonGateway->unit(), photonCannonGateway->pos()));
					}
				}
			}
			// 포토는 없고 파일런만 있는 경우
			else if (!pylons.empty()) {
				for (auto pylon : pylons) {
					// 포토가 완성될때까지 남은 시간.
					int leftTime = pylon->getRemainingBuildTime();

					int dist;

					theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), pylon->pos(), &dist);

					// 도달하는데 걸리는 시간.
					double frameToGetThere = (double)dist / Terran_SCV.topSpeed();

					// 시간이 충분히 남았으면 대기
					if (frameToGetThere < leftTime * 1.2) {
						continue;
					}
					// 시간이 얼마 안남았으면 방어하러 이동
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
			// 게이트웨이가 완성되지 않은 경우 scv가 가서 부순다.
			if (INFO.getCompletedCount(Protoss_Gateway, E) == 0) {
				uList buildings = getEnemyInMyYard(1600, UnitTypes::Buildings);

				uList gateways;

				// 완성된 질럿 있으면 철수
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

				// 건설중인 게이트웨이가 있으면, 본진과의 거리, scv 이동속도, 캐논의 빌드 속도, 캐논의 에너지를 고려해서 SCV 할당
				if (!gateways.empty()) {
					for (auto gateway : gateways) {
						// 완성될때까지 남은 시간.
						int leftTime = gateway->getCompleteTime() + Protoss_Zealot.buildTime();

						int dist;

						theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), gateway->pos(), &dist);

						// 도달하는데 걸리는 시간.
						double frameToGetThere = (double)dist / Terran_SCV.topSpeed() * 1.5;

						double shieldsPerTime = (double)gateway->type().maxShields() / gateway->type().buildTime();
						double HPPerTime = (double)gateway->type().maxHitPoints() / gateway->type().buildTime();
						double damagePerTime = (double)getDamage(Terran_SCV, gateway->type(), S, E) / Terran_SCV.groundWeapon().damageCooldown();

						// 목적지 도달시점 hp : (현재 hp + 증가량 * 이동시간)
						// 완성 될때까지 줄 수 있는 데미지 : (프레임별 데미지 * (남은 빌드시간 - 이동시간))
						// 목적지 도달시점 hp / (프레임별 데미지 * (남은 빌드시간 - 이동시간)) <= 마리수
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
			// 배럭이 없으면 벙커방향으로 이동.
			if (barracks.empty()) {
				for (int i = getEarlyRushDefenceScvCount(bunker->unit()); i < 5; i++) {
					UnitInfo *assignedScv = chooseScvClosestTo(bunker->pos());

					setStateToSCV(assignedScv, new ScvEarlyRushDefenseState(bunker->unit(), bunker->pos()));
				}
			}
			// 배럭이 있는데 ScvEarlyRushDefenseState 상태인 SCV 가 있으면 대기.
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

				// 마린이 완성되면 생기는 위치
				Position topLeft = Position(barrack->unit()->getLeft(), barrack->unit()->getBottom()) + marineMove;
				Position bottomRight = topLeft + Position(Terran_Marine.width(), Terran_Marine.height());

				//bw->drawBoxMap(topLeft, bottomRight, Colors::Blue);

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

		// 건설중인 해처리 있으면 해처리 공격 (해쳐리당 5마리)
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

void ScvManager::CheckNightingale()
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

void ScvManager::CheckRemoveBlocking(Position p)
{
	const Area *a = theMap.GetArea((TilePosition)p);

	if (a == nullptr) return;

	bool exists = false;

	for (auto cp : a->ChokePoints())
	{
		if (cp->BlockingNeutral() != nullptr && cp->BlockingNeutral()->IsMineral())
		{
			exists = true;

			//cout << cp->BlockingNeutral()->Pos().x / 32 << "," << cp->BlockingNeutral()->Pos().y / 32 << endl;

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
			// 그냥 20 타일을 봐바
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

					// 질럿이면 양손이라 한번더 친다.
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
					// 여기서 moveBack을 통해서 적과 멀어진후, (3 타일 이상)
					// scvManager를 통해서 mineral state로 바꿔준다.
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
				//	&& INFO.getFirstChokePosition(S).getApproxDistance(closestZergling->pos()) > 2 * TILE_SIZE)
				&& Position(cp->Center()).getApproxDistance(closestZergling->pos()) > 2 * TILE_SIZE)
		{
			//공격로직
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

	bool canBuildTurret = builderId == -1 // 건설중이지 않고
						  // 터렛지을 돈이 있고
						  && TrainManager::Instance().getAvailableMinerals() >= Terran_Missile_Turret.mineralPrice() + 300
						  // validation 용
						  && INFO.getSecondChokePosition(S) != Positions::None
						  // 테란 WaitLine 이거나
						  && (SM.getMainStrategy() == WaitLine
							  // 토스 AttackAll 인데 적이 캐리어나 아비터가 있을때
							  || (INFO.enemyRace == Races::Protoss && SM.getMainStrategy() == AttackAll && INFO.getAllCount(Protoss_Carrier, E) + INFO.getAllCount(Protoss_Arbiter, E) > 0));

	// 캐리어가 있는 경우, 터렛을 더 촘촘히 짓는다.
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
				// 적이 캐리어인 경우 건물 짓기 시작하면 새로 하나 더 짓기 시작
				if (INFO.getAllCount(Protoss_Carrier, E) > 0 && scv->unit()->canCancelConstruction())
					builderId = -1;
				// 그 외에는 건물이 95% 이상 지어지면 새로 하나 더 짓기 시작
				else if (scv->unit()->getBuildUnit()->getHitPoints() >= 0.95 * scv->unit()->getBuildUnit()->getType().maxHitPoints())
					builderId = -1;
			}
			// 건물을 짓기 전까지는 계속 명령을 내려준다.
			else if (builderId == scv->id() && !scv->unit()->isConstructing()) {
				UnitCommand currentCommand(scv->unit()->getLastCommand());

				if (currentCommand.getType() == UnitCommandTypes::Build && currentCommand.getTargetTilePosition().isValid() && bw->canBuildHere(currentCommand.getTargetTilePosition(), Terran_Missile_Turret, scv->unit())) {
					cout << "다시 명령 내려준다." << currentCommand.getTargetTilePosition() << endl;
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
	// 건설중이던 터렛이 있으면 건설하기
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

	// 터렛 지을 수 있으면 터렛 짓기.
	if (canBuildTurret) {
		// 6 타일 내에 터렛이 없으면 짓기.
		TilePosition turretPos = TilePositions::None;
		bool next = false;

		do {
			turretPos = ConstructionPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Missile_Turret, (TilePosition)scv->pos(), next);
			next = true;
		} while (turretPos != TilePositions::None && INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, S, (Position)turretPos, TURRET_GAP * TILE_SIZE).size() > 0);

		if (!turretPos.isValid())
			return false;

		// 본진과 앞마당에는 짓지 않는다.
		if (isSameArea((Position)turretPos, INFO.getMainBaseLocation(S)->Center()) || (INFO.getFirstExpansionLocation(S) && isSameArea((Position)turretPos, INFO.getFirstExpansionLocation(S)->Center())))
			return false;

		// 너무 시작지점에는 짓지 않는다.
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

				// 언덕 시즈와 너무 떨어지지 않는다.
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

			// 입구 수비
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
								// 다른 SCV를 수리할 필요가 없으면 공격
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
						// 다른 SCV를 수리할 필요가 없으면 공격
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

	// Bunker가 없는 경우는 이 함수 위에서 선처리 함.
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

	Position safePos = Positions::Origin;// , safeSt = Positions::Origin, safeEn = Positions::Origin;
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
				//bw->drawCircleMap(safeT, 50, Colors::Blue);
			}

			//Position bunkerSt = (Position)bunker->unit()->getTilePosition();
			//Position bunkerEn = bunkerSt + (Position)Terran_Bunker.tileSize();
			//bw->drawBoxMap(bunkerSt, bunkerEn, Colors::Cyan, false);


			//Position gap = safePos - bunker->pos();
			//bw->drawCircleMap(safePos, 50, Colors::Blue);
			//bw->drawBoxMap(bunkerSt + gap, bunkerEn + gap, Colors::Red, false);

			//safeSt = bunkerSt + gap;
			//safeEn = bunkerEn + gap;
		}
	}

	for (auto s : getUnits())
	{
		// 아직 완성이 안되서 꼭 지켜야 하는 상황
		if (!bunker->isComplete()) {
			UnitInfo *closeUnit = INFO.getClosestUnit(E, s->pos(), GroundUnitKind, 50 * TILE_SIZE);

			if (closeUnit != nullptr) {
				// 내가 벙커로부터 너무 멀어지면 돌아온다.
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
					//if (safeSt.x < s->pos().x && safeSt.y < s->pos().y
					//		&& safeEn.x > s->pos().x && safeEn.y > s->pos().y) // 안전 지역이다.
					if (s->pos().getApproxDistance(safePos) < 50) // 안전 지역이다.
					{
						//bw->drawCircleMap(s->pos(), 5, Colors::Red, true);
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
					if (s->pos().getApproxDistance(safeT) < 50) // 안전 지역이다.
					{
						//bw->drawCircleMap(s->pos(), 5, Colors::Red, true);
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
							//if (safeSt.x < s->pos().x && safeSt.y < s->pos().y
							//		&& safeEn.x > s->pos().x && safeEn.y > s->pos().y) // 안전 지역이다.
							if (s->pos().getApproxDistance(safePos) < 50)
							{
								//bw->drawCircleMap(s->pos(), 5, Colors::Red, true);
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

					// if we've already told this unit to move to this position, ignore this command
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
