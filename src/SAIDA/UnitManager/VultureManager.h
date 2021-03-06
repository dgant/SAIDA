#pragma once

#include "../UnitState/VultureState.h"
#include "TankManager.h"
#include "../StrategyManager.h"
#include "../InformationManager.h"

#define VM VultureManager::Instance()

namespace MyBot
{
	class VultureKiting : public UListSet
	{
	public:
		void action();
		void actionVsT();
		void initParam()
		{
			direction = 1;
			needWaiting = false;
			needCheck = false;
			waitingTime = 0;
			needFight = false;
			preAmountOfEnemy = 0;

			target = INFO.getMainBaseLocation(E)->Center();

			mine_idx = 0;
		}
		void setTarget(Position p) {
			target = p;
		}
		int isWaiting() {
			return needWaiting;
		}
		void clearSet();
		bool canFight(Position);
		bool needBack(Position);
		bool setDefenceMine(UnitInfo *);
		bool setGuardMine(UnitInfo *);
	private:
		Position target;
		int needWaiting; // 0 : no Wait, 1 : defence Build, 2 : range Unit
		bool needCheck;
		int direction;
		int waitingTime;
		word preAmountOfEnemy;

		vector<Position> defenceMinePos;
		word mine_idx;

		bool needFight;

		// 테란전
		int preStage = WaitToBase;
		int changedTime = 0;
	};

	class VultureManager
	{
		VultureManager();

	public:
		static VultureManager &Instance();
		void update();
		bool moveAsideForCarrierDefence(UnitInfo *v);
		void onUnitDestroyed(Unit u);

		void setVultureDefence(uList &vList);
		void setScoutVulture(uList &vList);
		void checkDiveVulture();
		void checkSpiderMine(uList &vList);
		bool needKeepMulti();
		Position checkForwardPylon();

		bool diveDone = false;
		word needScoutCnt = 0;
		bool scoutDone = false;
		int lastScoutTime = 0;

		// 전진 Pylon의 위치를 찾기 위함.
		// Unknown 은 초기화 상태 , 발견되서 파괴되었으면 Positions::None으로 변경된다.
		Position forwardBuildingPosition = Positions::Unknown;
		bool checkedForwardPylon = false;

		UnitInfo *getFrontVultureFromPos(Position pos);
		bool needPcontrol = false;
		bool getNeedPcon() {
			return needPcontrol;
		}
	private:

		UListSet vultureDefenceSet;
		VultureKiting kitingSet;
		VultureKiting kitingSecondSet;
		VultureKiting keepMultiSet;

		map<Unit, Unit> removeMineMap;
	};
}