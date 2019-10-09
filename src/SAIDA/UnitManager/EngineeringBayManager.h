#pragma once

#include "../UnitManager/EngineeringBayState.h"

namespace MyBot
{
	class EngineeringBayManager
	{
		EngineeringBayManager();

	public:
		static EngineeringBayManager &Instance();
		void update();
	private :
	};
}