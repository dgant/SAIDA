#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"
#include "../UnitManager/MarineManager.h"
#include "../InformationManager.h"
#include "../HostileManager.h"

namespace MyBot
{
	class MarineIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
		virtual State *action() override;
	};

	class MarineMoveState : public State
	{
	public:
		virtual string getName() override {
			return "Move";
		}
	};

	class MarineAttackState : public State
	{
	public:
		MarineAttackState(Position targetPos) {
			m_targetUnit = nullptr;
			m_targetPos = targetPos;
		}
		MarineAttackState(UnitInfo *target) {
			m_targetUnit = target->unit();
			m_targetPos = target->pos();
		}
		virtual string getName() override {
			return "Attack";
		}
		virtual State *action() override;
	};

	class MarinePatrolState : public State
	{
	public:
		virtual string getName() override {
			return "Patrol";
		}
	};

	class MarineKillScouterState : public State
	{
	public:
		virtual string getName() override {
			return "KillScouter";
		}
		virtual State *action(Position lastScouterPosition) override;
	};

	class MarineKitingState : public State
	{
	public:
		virtual string getName() override {
			return "Kiting";
		}
		virtual State *action() override;
	};

	class MarineDefenceState : public State
	{
	public:
		virtual string getName() override {
			return "Defence";
		}
		virtual State *action() override;
		State *bunkerRushDefence();
		UnitInfo *getMarineDefenceTargetUnit(const Unit &unit);
		bool shuttleDefence(const Unit &unit);
		bool darkDefence(const Unit &unit, const UnitInfo *closeUnit);
	};

	class MarineZealotDefenceState : public State
	{
	public:
		virtual string getName() override {
			return "ZealotDefence";
		}
		virtual State *action() override;
	};

	class MarineFirstChokeDefenceState : public State
	{
	public:
		MarineFirstChokeDefenceState(Position p);
		virtual string getName() override {
			return "FirstChokeDefence";
		}
		virtual State *action() override;
	private:
		Position defencePosition;
	};

	class MarineGoGoGoState : public State
	{
	public:
		virtual string getName() override {
			return "GoGoGo";
		}
		virtual State *action() override;
	};
}