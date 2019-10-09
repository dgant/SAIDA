#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"

namespace MyBot
{
	class FactoryIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
	};

	class FactoryBuildAddonState : public State
	{
	public:
		virtual string getName() override {
			return "BuildAddon";
		}
		virtual State *action() override;
	};

	class FactoryTrainState : public State
	{
	public:
		virtual string getName() override {
			return "Train";
		}
		virtual State *action() override;
		virtual State *action(UnitType unitType) override;
	};

	class FactoryLiftAndLandState : public State
	{
	public:
		virtual string getName() override {
			return "LiftAndLand";
		}
		virtual State *action(TilePosition targetPos) override;
	};
}