#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"

namespace MyBot
{
	class ComsatStationIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
		virtual State *action() override;
	};

	class ComsatStationActivationState : public State
	{
	public:
		virtual string getName() override {
			return "Activation";
		}
		virtual State *action() override;
	};

	class ComsatStationScanState : public State
	{
	public:
		virtual string getName() override {
			return "Scan";
		}
		virtual State *action(Position targetPosition) override;
	};
}