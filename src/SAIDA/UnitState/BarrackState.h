#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"
#include "../TrainManager.h"
#include "../UnitManager/TankManager.h"
#include "../BuildingStrategy/SelfBuildingPlaceFinder.h"

namespace MyBot
{
	class BarrackIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
	};

	class BarrackTrainState : public State
	{
	public:
		virtual string getName() override {
			return "Train";
		}
		virtual State *action() override;
		virtual State *action(UnitType unitType) override;
	};

	class BarrackBarricadeState : public State
	{

	public:
		virtual string getName() override {
			return "Barricade";
		}
		virtual State *action() override;
	};

	class BarrackLiftAndMoveState : public State
	{
	public:
		virtual string getName() override {
			return "LiftAndMove";
		}
		virtual State *action() override;
	private:
		bool efeChecked = false;
		bool emChecked = false;
		int direction = 1;
	};

	class BarrackNeedRepairState : public State
	{
	public:
		virtual string getName() override {
			return "NeedRepair";
		}
		virtual State *action() override;
	};
}