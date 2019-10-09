#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"

namespace MyBot
{
	class StarportIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
	};

	class StarportBuildAddonState : public State
	{
	public:
		virtual string getName() override {
			return "BuildAddon";
		}
		virtual State *action() override;
	};

	class StarportTrainState : public State
	{
	public:
		virtual string getName() override {
			return "Train";
		}
		virtual State *action() override;
		virtual State *action(UnitType unitType) override;
	};
}