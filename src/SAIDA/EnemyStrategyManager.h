#pragma once

#include "Common.h"
#include "UnitData.h"
#include "InformationManager.h"
#include "StrategyManager.h"

#define ESM	EnemyStrategyManager::Instance()

namespace MyBot
{

	/// 상대 빌드를 파악하는 class
	/// InformationManager 에 있는 상대 유닛 정보들로부터 상대 빌드 판단
	class EnemyStrategyManager
	{
		EnemyStrategyManager();
		/// 적 초기 빌드 유추
		void checkEnemyInitialBuild();

		// 적 메인(중반) 빌드 유추
		void checkEnemyMainBuild();

		void useFirstExpansion();    // 초반 앞마당 멀티 먹는 타이밍 이용해서 적 빌드 유추
		void useFirstTechBuilding(); // 첫 병력 생산 관련 건물 타이밍 및 개수로 유추
		void useFirstAttackUnit();   // 첫 공격 유닛이 보이는 타이밍으로 유추
		bool timeBetween(int from, int to);
		void useBuildingCount();     // 건물 개수로 유추
		// 내 본진에 적의 eBuildings 가 있는지 체크하고 있으면 initialBuildType 전략 세팅 (return set 여부)
		bool setStrategyByEnemyBuildingInMyBase(uList eBuildings, InitialBuildType initialBuildType);

		void checkEnemyBuilding(); // 적 건물을 통한 전략 유추
		void checkEnemyUnits(); // 적 유닛을 통한 전략 유추
		void checkEnemyTechNUpgrade(); // 적 테크와 업그레이드 상황 판단을 통한 유추

	public:
		/// static singleton 객체를 리턴합니다
		static EnemyStrategyManager &Instance();
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
		int waitTimeForDrop = 0; // 0 아직 판단되지 않음. -1 : 왔음, -2 막음

		void setEnemyMainBuild(MainBuildType st);
		void setEnemyInitialBuild(InitialBuildType ib, bool force = false);
		void clearBuildConstructQueue();
		bool isTheUnitNearMyBase(uList unitList);
		void checkDarkDropHadCome();
	};
}
