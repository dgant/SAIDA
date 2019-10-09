#pragma once

#include "Common.h"

#include "UnitData.h"
#include "BuildingStrategy/BuildOrderManager.h"
#include "InformationManager.h"
#include "BuildingStrategy/BuildManager.h"
#include "BuildingStrategy/ConstructionManager.h"
#include "ExploreManager.h"
#include "HostileManager.h"
#include "BuildingStrategy/BuildStrategy.h"
#include "UnitManager\SoldierManager.h"
#include "UnitManager\ComsatStationManager.h"

#define SM StrategyManager::Instance()
/*
#define SELF_INDEX 0
#define ENEMY_INDEX 1
*/
#define STTACKPOS SM.getSecondAttackPosition()
/*
*/
#define NeedUpgrad SM. getNeedUpgrade()
/*
bool StrategyManager::getNeedTank()
{
return needTank;
}
*/
#define ISNeeTank SM. getNeedTank()
#define ATTACKPOS SM.getMainAttackPosition()

namespace MyBot
{
	
	class StrategyManager
	{
		StrategyManager();

		void executeSupplyManagement();
		void makeMainStrategy();
		void checkNeedAttackMulti();
		void checkEnemyMultiBase();

	public:
		
		static StrategyManager 	&Instance();

		void onStart();

		void onUnitShow(BWAPI::Unit unit);
		void onUnitDestroy(BWAPI::Unit unit);
		void drawWorkerInformation(int x, int y);
		bool isFree(BWAPI::Unit worker);
		void drawResourceDebugInfo();
		void onEnd(bool isWinner);

	
		void update();
		void searchForEliminate();

		void searchAllMap(Unit u);
		bool decideToBuildWhenDeadLock(UnitType *);
		
		MainStrategyType getMainStrategy() {
			return mainStrategy;
		}
		/*
		BWAPI::TilePosition _predictedTilePosition;
		BWAPI::Unit         _assignedWorkerForThisBuilding;
		*/
		BWAPI::Unitset workers;
		BWAPI::Unitset depots;

		std::map<BWAPI::Unit, enum WorkerJob>	workerJobMap;           // worker -> job
		std::map<BWAPI::Unit, BWAPI::Unit>		workerDepotMap;         // worker -> resource depot (hatchery)
		std::map<BWAPI::Unit, BWAPI::Unit>		workerRefineryMap;      // worker -> refinery
		std::map<BWAPI::Unit, BWAPI::Unit>		workerRepairMap;        // worker -> unit to repair
		/*
				bool                _haveLocationForThisBuilding;
		int					_delayBuildingPredictionUntilFrame;
		int					_lastCreateFrame;
		
		*/
	//	std::map<BWAPI::Unit, WorkerMoveData>	workerMoveMap;          // worker -> location
		std::map<BWAPI::Unit, BWAPI::UnitType>	workerBuildingTypeMap;  // worker -> building type

		std::map<BWAPI::Unit, int>				depotWorkerCount;       // mineral workers per depot
		std::map<BWAPI::Unit, int>				refineryWorkerCount;    // gas workers per refinery

		std::map<BWAPI::Unit, int>				workersOnMineralPatch;  // workers per mineral patch
		std::map<BWAPI::Unit, BWAPI::Unit>		workerMineralAssignment;// worker -> mineral patch
		
		MyBuildType getMyBuild();

		bool getNeedUpgrade();
		bool getNeedTank();

		Position getMainAttackPosition();
		Position getSecondAttackPosition();
		Position getDrawLinePosition();
		Position getWaitLinePosition();
		/*
		bool being_terran_choke = false;
		BWAPI::TilePosition anti_terran_choke_pos = BWAPI::TilePositions::None;

		bool ignore_scout_worker = false;

		*/
		bool getNeedAttackMulti() {
			return needAttackMulti;
		}

		void decideDropship();
		bool getDropshipMode() {
			return dropshipMode;
		}
		bool being_terran_choke = false;
		BWAPI::TilePosition anti_terran_choke_pos = BWAPI::TilePositions::None;

		bool ignore_scout_worker = false;

		void updateCurrentState(BuildOrderQueue &queue);

		/*
				bool getNeedKeepSecondExpansion(UnitType uType) {
			if (uType == Terran_Goliath)
				return needKeepMultiSecond || needKeepMultiSecondAir;
			else if (uType == Terran_Vulture)
				return needKeepMultiSecond || needMineSecond;
			else
				return needKeepMultiSecond;
		}
		
		*/
		bool beingMarineRushed();
		bool beingZealotRushed();
		bool beingZerglingRushed();


		bool checkTurretFirst();

		bool getNeedSecondExpansion();
		bool getNeedThirdExpansion();

		bool getNeedKeepSecondExpansion(UnitType uType) {
			if (uType == Terran_Goliath)
				return needKeepMultiSecond || needKeepMultiSecondAir;
			else if (uType == Terran_Vulture)
				return needKeepMultiSecond || needMineSecond;
			else
				return needKeepMultiSecond;
		}
		bool getNeedKeepThirdExpansion(UnitType uType) {
			if (uType == Terran_Goliath)
				return needKeepMultiThird || needKeepMultiThirdAir;
			else if (uType == Terran_Vulture)
				return needKeepMultiThird || needMineThird;
			else
				return needKeepMultiThird;
		}

		bool getNeedAdvanceWait();
		void setMultiDeathPos(Position pos) {
			keepMultiDeathPosition = pos;
		}

		void setMultiBreakDeathPos(Position pos) {
			multiBreakDeathPosition = pos;
		}

		void checkNeedDefenceWithScv();
		bool getNeedDefenceWithScv() {
			return needDefenceWithScv;
		}

		
		bool needAdditionalExpansion();
	private:
		
		void setMyBuild();

		void blockFirstChokePoint(UnitType type);
		int getTrainBuildingCount(UnitType type);

		
		void checkUpgrade();

		void checkUsingScan();
		int lastUsingScanTime;

		MainStrategyType mainStrategy;
		MyBuildType myBuild;

		Unit m1, m2;

		int searchPoint;

		bool needUpgrade = false;
		
		bool needTank;

		bool scanForAttackAll;

		bool scanForcheckMulti;

		Position map400[400]; 

		bool dropshipMode = false;

		void setAttackPosition();
		Position mainAttackPosition = Positions::Unknown;
		Position secondAttackPosition = Positions::Unknown;
		bool needAttackMulti = false;

		
		void checkAdvanceWait();
		bool advanceWait = false;

		
		Position drawLinePosition = Positions::Unknown;
		Position waitLinePosition = Positions::Unknown;
		void setDrawLinePosition();
		bool needWait(UnitInfo *firstTank);
		bool moreUnits(UnitInfo *firstTank);
		int nextScanTime = 0;
		int nextDrawTime = -1;
		bool isExistEnemyMine = false;
		bool centerIsMyArea = false;
		bool surround = false;

		
		bool needSecondExpansion = false;
		void checkSecondExpansion();

		Position keepMultiDeathPosition = Positions::Unknown;
		Position multiBreakDeathPosition = Positions::Unknown;

		
		bool needThirdExpansion = false;
		void checkThirdExpansion();

		void checkKeepExpansion();
		
		bool needKeepMultiSecond = false;
		bool needKeepMultiThird = false;
		bool needMineSecond = false;
		bool needMineThird = false;
		
		bool needKeepMultiSecondAir = false;
		bool needKeepMultiThirdAir = false;

		
		void killMine(Base *base);
		
		bool needDefenceWithScv = false;

		
		bool sendStart = false;


		bool finalAttack = false;
	};
}
