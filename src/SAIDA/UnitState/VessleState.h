#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "../InformationManager.h"
#include "State.h"

namespace MyBot {
	class VessleIdleState : public State
	{
	public:
		VessleIdleState(Position pos_) {
			pos = pos_;
		}
		virtual string getName() override {
			return "Idle";
		}
		virtual State *action() override;
	private :
		Position pos;
	};

	class VessleCruiseState : public State
	{
	public:
		virtual string getName() override {
			return "Cruise";
		}
		virtual State *action(Position pos) override;
	private:
	};

	class VessleDefenseState : public State
	{
	public:
		virtual string getName() override {
			return "DefenseBase";
		}
		virtual State *action() override;
	private:
	};

	class VessleBattleGuideState: public State
	{
	public:
		virtual string getName() override {
			return "BattleGuide";
		}
		virtual State *action(Unit targetUnit) override;
		virtual State *action(Unit targetUnit, Unit targetEnemy) override;

	private:
		//Unit target = nullptr;
	};
}