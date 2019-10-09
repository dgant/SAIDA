#pragma once

#include "../InformationManager.h"
#include "../UnitManager/ScvState.h"
#include "../Common.h"

namespace MyBot
{
	class ScvNightingaleSet : public UListSet
	{
	private:
		int builderId = -1;
	public:
		void init();
		void action();
		void del(UnitInfo *u) {
			if (builderId == u->id())
				builderId = -1;

			UListSet::del(u);
		}
		bool buildTurret(UnitInfo *scv, bool canBuildTurret, int &builderId, int TURRET_GAP);
	};

	class ScvBunkerDefenceSet : public UListSet
	{
	public :
		void init();
		void action();
	};

	class ScvCombatSet : public UListSet
	{
	public :
		void init();
		void action();
	};
	class FirstChokeDefenceSet : public UListSet
	{
	public:
		FirstChokeDefenceSet();
		void init();
		void action();
	private:
		Position waitingPositionAtFirstChoke = Positions::None;
	};


	class ScvRemoveBlockingSet : public UListSet
	{
	private:

	public:
		void init();
		void action(Position p);
	};


	class SoldierManager
	{
	private:
		SoldierManager();
		~SoldierManager() {};

		map<Unit, vector<Unit>> depotMineralMap;
		map<Unit, BWEM::Base *> depotBaseMap;
		map<BWEM::Base *, bool> baseSafeMap; 

		map<Unit, int> mineralScvCountMap;
		map<Unit, int> refineryScvCountMap;
		map<Unit, int> repairScvCountMap;
		map<Unit, int> scoutUnitDefenceScvCountMap;
		map<Unit, int> earlyRushDefenceScvCountMap;
		int totalScoutUnitDefenceScvCount;
		int totalEarlyRushDefenceScvCount;
		map<Unit, UnitInfo *> bunkerDefenceSCVList;

		int refineryScvCnt; 
		int repairScvCnt; 
		int MaxRepairScvCnt;
		int MineralStateScvCount;
		int GasStateScvCount;
		int RepairStateScvCount;
		Unit scanMyBaseUnit;
		bool blockingRemoved = false;
		bool checkBaseUpdated = false;
		Position checkBase = Positions::None;

	public:
		static SoldierManager &Instance();
		void update();
		void CheckRefineryScv();
		void CheckDepot();
		void CheckRepairScv();
		void CheckBunkerDefence();
		void CheckFirstChokeDefence();
		void CheckCombatSet();
		void CheckWorkerDefence();
		void CheckEnemyScoutUnit();
		void CheckEarlyRush();
		void CheckNightingale();
		void CheckRemoveBlocking(Position p);
		Unit				getBestMineral(Unit depot);
		Unit				getNearestDepot(UnitInfo *u);
		Unit				getTemporaryMineral(Position pos);
		void				setMineralScv(UnitInfo *uInfo, Unit center = nullptr);
		bool				isDepotNear(Unit refinery);
		bool				isExistSafeBase();
		void				setIdleAroundDepot(Unit depot);
		void				initMineralsNearDepot(Unit depot);
		bool				depotHasEnoughMineralWorkers(Unit depot);
		void				setScvIdleState(Unit);
		Unit				chooseConstuctionScvClosestTo(TilePosition buildingPosition, int avoidScvID = 0);
		Unit				chooseRepairScvClosestTo(Unit u, int maxRange = INT_MAX, bool withGas = false);

	
		Unit				chooseRepairScvforScv(Unit unit, int maxRange = INT_MAX, bool withGas = false);
		UnitInfo			*chooseScvForScv(Position position, int avoidScvID = 0, int maxRange = INT_MAX, bool withGas = false);

		UnitInfo			*chooseScouterScvClosestTo(Position p);
		UnitInfo			*chooseScvClosestTo(Position position, int avoidScvID = 0, int maxRange = INT_MAX, bool withGas = false);



		void				beforeRemoveState(UnitInfo *scv);
		Unit				setStateToSCV(UnitInfo *scv, State *state);
		void				rebalancingNewDepot(Unit depot);
		void				preRebalancingNewDepot(Unit depot, int cnt);

		void				onUnitCompleted(Unit worker_or_depot);
		void				onUnitDestroyed(Unit worker_or_depot);
		void				onUnitLifted(Unit depot);
		void				onUnitLanded(Unit depot);

		const int			getAllMineralScvCount() {
			return MineralStateScvCount;
		}
		const int			getAllRefineryScvCount() {
			return GasStateScvCount;
		}
		int					getMineralScvCount(Unit);
		int					getRefineryScvCount(Unit);
		int					getRepairScvCount(Unit);
		int					getScoutUnitDefenceScvCountMap(Unit);
		int					getEarlyRushDefenceScvCount(Unit);
		int					getAssignedScvCount(Unit depot);
		int					getNeedCountForRefinery(void) {
			return refineryScvCnt;
		}
		int					getDepotMineralSize(Unit depot) {
			if (depotMineralMap.find(depot) == depotMineralMap.end())
				return 0;

			return depotMineralMap[depot].size();
		}
		void				setNeedCountForRefinery(int n) {
			refineryScvCnt = n;
		}
		Unit				getScanMyBaseUnit() {
			return scanMyBaseUnit;
		}
		int                 getRemainingAverageMineral(Unit depot);
		FirstChokeDefenceSet			getFirstChokeDefenceSet() {
			return firstChokeDefenceSet;
		}
		
		map<Unit, int> &getMineralScvCountMap() {
			return mineralScvCountMap;
		}
		map<Unit, int> &getRefineryScvCountMap() {
			return refineryScvCountMap;
		}
		map<Unit, int> &getRepairScvCountMap() {
			return repairScvCountMap;
		}
		map<Unit, int> &getEarlyRushDefenceScvCountMap() {
			return earlyRushDefenceScvCountMap;
		}
		int				getRepairScvCount() {
			return RepairStateScvCount;
		}

		ScvBunkerDefenceSet bunkerDefenceSet;
		ScvCombatSet combatSet;
		ScvNightingaleSet nightingaleSet;
		FirstChokeDefenceSet firstChokeDefenceSet;
		ScvRemoveBlockingSet removeBlockingSet;

		clock_t time[18];
		int startWaitingTimeForWorkerCombat = 0;
		int distanceToBunker = 0;
	};
}