#pragma once

#include "../UnitState/EngineeringBayState.h"

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