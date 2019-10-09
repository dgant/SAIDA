#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"

namespace MyBot
{
	class ValkyrieIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
		virtual State *action() override;
	};


	class ValkyrieScoutState : public State
	{
	public:
		virtual string getName() override {
			return "Scout";
		}
		virtual State *action() override;
	private:
		Base *targetBase = nullptr;
		UnitInfo *me = nullptr;
	};

	class ValkyrieAttackWraithState : public State
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

	class ValkyrieRetreatState : public State
	{
	public:
		virtual string getName() override {
			return "Retreat";
		}
		virtual State *action() override;
	};

	class ValkyrieFollowTankState : public State
	{
	public:
		virtual string getName() override {
			return "FollowTank";
		}
		virtual State *action() override;
	};

	class ValkyrieDefenceState : public State
	{
	public:
		virtual string getName() override {
			return "Defence";
		}
		virtual State *action() override;
	};
	

}