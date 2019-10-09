#pragma once

#include "../InformationManager.h"
#include "../UnitState/VessleState.h"
#include "../StrategyManager.h"

namespace MyBot
{
	class VessleManager
	{
		VessleManager();
	public:
		static VessleManager &Instance();
		void update();
		void initTargetList();
		Unit choiceTarget(UnitInfo *v);
		Unit choicePosition(int num);
		Unit enemyTargetUnit;
	private:
		Unit choiceUnit;
		UListSet targetList;
	};
}