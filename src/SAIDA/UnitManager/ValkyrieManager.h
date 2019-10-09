#pragma once

#include "../UnitState/ValkyrieState.h"
#include "../StrategyManager.h"


namespace MyBot
{
	class ValkyrieManager
	{
		ValkyrieManager();

	public:
		static ValkyrieManager &Instance();
		void update();
		void onUnitDestroyed(Unit u);
	private:
		//UnitInfo *scoutWraith = nullptr;
		Unit oneTargetUnit = nullptr;
		bool canCloak = false;
	};
}