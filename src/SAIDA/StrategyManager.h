#pragma once

#include "Common.h"

#include "UnitData.h"
#include "BuildOrderQueue.h"
#include "InformationManager.h"
#include "BuildManager.h"
#include "ConstructionManager.h"
#include "ScoutManager.h"
#include "EnemyStrategyManager.h"
#include "BasicBuildStrategy.h"
#include "UnitManager\ScvManager.h"
#include "UnitManager\ComsatStationManager.h"

#define SM StrategyManager::Instance()
#define ATTACKPOS SM.getMainAttackPosition()
#define ML_ON_OFF false      // 머신러닝 적용 여부

namespace MyBot
{
	/// 상황을 판단하여, 정찰, 빌드, 공격, 방어 등을 수행하도록 총괄 지휘를 하는 class<br>
	/// InformationManager 에 있는 정보들로부터 상황을 판단하고, <br>
	/// BuildManager 의 buildQueue에 빌드 (건물 건설 / 유닛 훈련 / 테크 리서치 / 업그레이드) 명령을 입력합니다.<br>
	/// 정찰, 빌드, 공격, 방어 등을 수행하는 코드가 들어가는 class
	class StrategyManager
	{
		StrategyManager();

		void executeSupplyManagement();
		void makeMainStrategy();
		bool useML();
		void checkNeedAttackMulti();
		void checkEnemyMultiBase();

	public:
		/// static singleton 객체를 리턴합니다
		static StrategyManager 	&Instance();

		/// 경기가 시작될 때 일회적으로 전략 초기 세팅 관련 로직을 실행합니다
		void onStart();

		///  경기가 종료될 때 일회적으로 전략 결과 정리 관련 로직을 실행합니다
		void onEnd(bool isWinner);

		/// 경기 진행 중 매 프레임마다 경기 전략 관련 로직을 실행합니다
		void update();
		void searchForEliminate();

		void searchAllMap(Unit u);

		bool decideToBuildWhenDeadLock(UnitType *);
		// 현재 Main Strategy
		MainStrategyType getMainStrategy() {
			return mainStrategy;
		}

		// 내 빌드
		MyBuildType getMyBuild();

		bool getNeedUpgrade();
		bool getNeedTank();

		Position getMainAttackPosition();
		Position getSecondAttackPosition();
		Position getDrawLinePosition();
		Position getWaitLinePosition();

		bool getNeedAttackMulti() {
			return needAttackMulti;
		}

		void decideDropship();
		bool getDropshipMode() {
			return dropshipMode;
		}

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

		// 추가 멀티 먹어야 하나
		bool needAdditionalExpansion();
	private:
		// 내 빌드 설정
		void setMyBuild();

		void blockFirstChokePoint(UnitType type);

		// 서플 증설 시 사용할 건물 count
		int getTrainBuildingCount(UnitType type);

		// 초반에 유닛뽑기보다 업그레이드를 먼저 해줘야 하는 상황을 설정해 주기 위함.
		void checkUpgrade();

		// 스캔 사용(적 빌드 유추 등)
		void checkUsingScan();
		int lastUsingScanTime;

		MainStrategyType mainStrategy;
		MyBuildType myBuild;

		Unit m1, m2;

		int searchPoint;

		bool needUpgrade = false;
		// vsT 탱크가 필요
		bool needTank;

		bool scanForAttackAll;

		bool scanForcheckMulti;

		Position map400[400]; // Search Point

		bool dropshipMode = false;

		// 공격 위치 설정
		void setAttackPosition();
		Position mainAttackPosition = Positions::Unknown;
		Position secondAttackPosition = Positions::Unknown;
		bool needAttackMulti = false;

		// 초반 앞마당 앞에 조금 더 전진해서 있어야 하는지 체크
		void checkAdvanceWait();
		bool advanceWait = false;

		// 선긋기
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

		// 2번째 멀티 먹어야 할 타이밍
		bool needSecondExpansion = false;
		void checkSecondExpansion();

		Position keepMultiDeathPosition = Positions::Unknown;
		Position multiBreakDeathPosition = Positions::Unknown;

		// 3번째 멀티 먹어야 할 타이밍
		bool needThirdExpansion = false;
		void checkThirdExpansion();

		void checkKeepExpansion();
		// 멀티 지켜야 하는지 - 지상
		bool needKeepMultiSecond = false;
		bool needKeepMultiThird = false;
		bool needMineSecond = false;
		bool needMineThird = false;
		// 멀티 지켜야 하는지 - 공중
		bool needKeepMultiSecondAir = false;
		bool needKeepMultiThirdAir = false;

		// 멀티지역 마인 제거
		void killMine(Base *base);
		// SCV랑 테란전 초반 막기
		bool needDefenceWithScv = false;

		// 승률 예측 모델이 값을 보내기 시작했는지
		bool sendStart = false;

		bool isCircuitBreakers = false;

		bool finalAttack = false;
	};
}
