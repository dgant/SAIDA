#pragma once

#include "../UnitManager/WraithState.h"
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
		
		Unit oneTargetUnit = nullptr;
		bool canCloak = false;
	};
}