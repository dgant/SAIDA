#pragma once

#include "Common.h"
#include "UnitData.h"
#include "InformationManager.h"
#include "StrategyManager.h"

#define ESM	HostileManager::Instance()

namespace MyBot
{


	class HostileManager
	{
		HostileManager();
		void checkEnemyInitialBuild();

		void checkEnemyMainBuild();

		void useFirstExpansion();   
		void useFirstTechBuilding(); 
		void useFirstAttackUnit();   
		bool timeBetween(int from, int to);
		void useBuildingCount();   
	
		bool setStrategyByEnemyBuildingInMyBase(uList eBuildings, InitialBuildType initialBuildType);

		void checkEnemyBuilding(); 
		void checkEnemyUnits(); 
		void checkEnemyTechNUpgrade(); 

	public:
	
		static HostileManager &Instance();
		void update();
		InitialBuildType getEnemyInitialBuild();
		MainBuildType getEnemyMainBuild();
		int getWaitTimeForDrop() {
			return waitTimeForDrop;
		}
		void setWaitTimeForDrop(int waitTimeForDrop_) {
			waitTimeForDrop = waitTimeForDrop_;
		}
		Unit getEnemyGasRushRefinery() {
			return enemyGasRushRefinery;
		}
		vector<pair<InitialBuildType, int>> getEIBHistory() {
			return eibHistory;
		}
		vector<pair<MainBuildType, int>> getEMBHistory() {
			return embHistory;
		}
	private:
		InitialBuildType enemyInitialBuild;
		MainBuildType enemyMainBuild;
		vector<pair<InitialBuildType, int>> eibHistory;
		vector<pair<MainBuildType, int>> embHistory;
		Unit enemyGasRushRefinery;

		float nexusBuildSpd_per_frmCnt = (float)(Protoss_Nexus.maxHitPoints() + Protoss_Nexus.maxShields()) / Protoss_Nexus.buildTime(); // hp per frame count
		float commandBuildSpd_per_frmCnt = (float)Terran_Command_Center.maxHitPoints() / Terran_Command_Center.buildTime(); // hp per frame count
		UnitInfo *secondNexus = nullptr;
		UnitInfo *secondCommandCenter = nullptr;

		double centerToBaseOfMarine = 0;
		double centerToBaseOfZ = 0;
		double centerToBaseOfSCV = 0;
		double centerToBaseOfProbe = 0;
		double baseToBaseOfMarine = 0;
		int timeToBuildBarrack = 0;
		int timeToBuildGateWay = 0;
		int waitTimeForDrop = 0; 

		void setEnemyMainBuild(MainBuildType st);
		void setEnemyInitialBuild(InitialBuildType ib, bool force = false);
		void clearBuildConstructQueue();
		bool isTheUnitNearMyBase(uList unitList);
		void checkDarkDropHadCome();
	};
}
