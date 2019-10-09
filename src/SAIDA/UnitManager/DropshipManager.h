#pragma once

#include "../InformationManager.h"
#include "../UnitState/DropshipState.h"

namespace MyBot
{
	class DropshipManager
	{
		DropshipManager();

	public:
		static DropshipManager &Instance();
		void update();

		bool getBoardingState() {
			return waitToBoarding;
		}

	private:
		bool waitToBoarding = false;
		bool reverse = true;
	};
}