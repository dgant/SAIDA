#pragma once

#include "../UnitManager/VultureState.h"
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
		int needWaiting; 
		bool needCheck;
		int direction;
		int waitingTime;
		word preAmountOfEnemy;

		vector<Position> defenceMinePos;
		word mine_idx;

		bool needFight;

		
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