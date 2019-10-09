#pragma once

#include "../UnitState/WraithState.h"
#include "../StrategyManager.h"

namespace MyBot
{
class WraithKiting: public UListSet
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

	private:
		Position target;
		int needWaiting; // 0 : no Wait, 1 : defence Build, 2 : range Unit
		bool needCheck;
		int direction;
		int waitingTime;
		word preAmountOfEnemy;

		vector<Position> defenceMinePos;
		bool needFight;
		// Å×¶õÀü
		int preStage = WaitToBase;
		int changedTime = 0;
	};
	class WraithManager
	{
		WraithManager();

	public:
		static WraithManager &Instance();
		void update();
		void onUnitDestroyed(Unit u);
		Position getKillScvTargetBase();
	private:
		//UnitInfo *scoutWraith = nullptr;
		Unit oneTargetUnit = nullptr;
		bool canCloak = false;
		WraithKiting kitingSet;
		WraithKiting kitingSecondSet;
		WraithKiting keepMultiSet;
	};
}