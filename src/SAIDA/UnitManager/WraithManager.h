#pragma once

#include "../UnitState/WraithState.h"
#include "../StrategyManager.h"

namespace MyBot
{
	class WraithManager
	{
		WraithManager();

	public:
		static WraithManager &Instance();
		void update();
		Position getKillScvTargetBase();
	private:
		//UnitInfo *scoutWraith = nullptr;
		Unit oneTargetUnit = nullptr;
		bool canCloak = false;
	};
}