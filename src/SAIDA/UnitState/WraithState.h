#pragma once

#include "../Common.h"
#include "../UnitData.h"

#include "State.h"

namespace MyBot
{
	class WraithIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
		virtual State *action() override;
	};
	class WraithKitingState : public State
	{
	public:
		virtual string getName() override {
			return "Kiting";
		}
	};
	class WraithScoutState : public State
	{
	public:
		virtual string getName() override {
			return "Scout";
		}
		virtual State *action(Position targetPosition) override;
	};

	class WraithAttackWraithState : public State
	{
	public:
		virtual string getName() override {
			return "AttackWraith";
		}
		virtual State *action() override;
		virtual State *action(Unit targetUnit) override;
	private:
		Position tP = Positions::Invalid;
		Unit tU = nullptr;
	};

	class WraithKillScvState : public State
	{
	public:
		virtual string getName() override {
			return "KillScv";
		}
		virtual State *action(Position targetPosition) override;
	};

	class WraithFollowTankState : public State
	{
	public:
		virtual string getName() override {
			return "FollowTank";
		}
		virtual State *action() override;
	};

	class WraithRetreatState : public State
	{
	public:
		virtual string getName() override {
			return "Retreat";
		}
		virtual State *action() override;
	};
	class WraithCloakState : public State
	{
	public:
		virtual string getName() override {
			return "Cloak";
		}
		virtual State *action() override;
	};

	class WraithBattleAssistState : public State
	{
	public:
		virtual string getName() override {
			return "BattleAssist";
		}
		virtual State *action() override;
	};
}