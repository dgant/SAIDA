#pragma once

#include "../UnitState/GoliathState.h"
#include "TankManager.h"
#include "DropshipManager.h"

#define GM GoliathManager::Instance()

namespace MyBot
{
	class vsTerranSet : public UListSet
	{
	public :
		void init();
		void action();

	private :
		int preStage = WaitToBase;
		int changedTime = 0;
	};

	class GoliathManager
	{
		GoliathManager();

	public:
		static GoliathManager &Instance();
		Position getDefaultMovePosition();
		// return 0 : nothing, 1 : wait, 2 : move back, 3 : force attack
		int getWaitOrMoveOrAttack();
		void update();

		bool removeOurTurret(UnitInfo *g);

		void setDefenceGoliath(uList &gList);
		void onUnitDestroyed(Unit u);
		void setKeepMultiGoliath();
		void setKeepMultiGoliath2();
		void setMultiBreakGoliath();
		bool enoughGoliathForDrop();
		int getUsableGoliathCnt()	{
			if (INFO.enemyRace == Races::Terran)
				return goliathTerranSet.size();
			else
				return INFO.getCompletedCount(Terran_Goliath, S) - goliathDefenceSet.size() - goliahtKeepMultiSet.size() - goliahtKeepMultiSet2.size();
		}
		void initDropshipSet() {
			dropshipSet.clear();
		}

		UnitInfo *getFrontGoliathFromPos(Position pos);

		void setStateToGoliath(UnitInfo *t, State *state);
	private :
		// getDefaultMovePosition 한번만 호출하기 위한 값
		int defaultMovePositionFrame = 0;
		int isNeedToWaitAttackFrame = 0;
		Position firstTankPosition;
		int isNeedToWaitAttack = 0;

		UListSet goliathProtectTankSet;
		UListSet goliathDefenceSet;
		UListSet goliathMultiBreakSet;
		UListSet goliahtKeepMultiSet;
		UListSet goliahtKeepMultiSet2;
		vsTerranSet goliathTerranSet;
		UListSet dropshipSet;
		void checkDropship();

		Position multiBase = Positions::Unknown;
		Position multiBase2 = Positions::Unknown;

		bool setMiddlePosition = false;
		int allAttackFrame = 0;
	};

}