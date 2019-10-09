#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"

namespace MyBot
{
	class EngineeringBayIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
	};


	class EngineeringBayLiftAndMoveState : public State
	{
	public:
		EngineeringBayLiftAndMoveState();
		virtual string getName() override {
			return "LiftAndMove";
		}
		virtual State *action() override;
		
	private:
		bool firstMoved = false;
	
	};

	class EngineeringBayBarricadeState : public State
	{

	public:
		virtual string getName() override {
			return "Barricade";
		}
		virtual State *action() override;
	};

	class EngineeringBayNeedRepairState : public State
	{
	public:
		virtual string getName() override {
			return "NeedRepair";
		}
		virtual State *action() override;
	};
}