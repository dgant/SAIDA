#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"

#define THR_CHECK 70
#define THR_MINERAL 53
#define FR 2

namespace MyBot
{
	class ScvIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
	};

	class ScvMoveState : public State
	{
	public:
		ScvMoveState(Position p) {
			m_targetPos = p;
		}

		virtual string getName() override {
			return "Move";
		}
		virtual State *action() override {
			unit->move(m_targetPos);
			return nullptr;
		}
	};

	//zergling, zealot병력 싸울때
	class ScvCombatState : public State
	{
	public:
		virtual string getName() override {
			return "Combat";
		}
	};

	//상대 일꾼 공격 대응 Combat
	class ScvWorkerCombatState : public State
	{
	public:
		virtual string getName() override {
			return "WorkerCombat";
		}
		virtual State *action() override;
	private:
		Unit targetUnit = nullptr;
		Unit repairTarget = nullptr;
		Position watingPos = Positions::None;
		int waitingTime = 0;
	};

	class ScvCounterAttackState : public State
	{
	public:
		virtual string getName() override {
			return "CounterAttack";
		}
		virtual State *action() override;
	};

	class ScvMineralState : public State
	{
	public:
		ScvMineralState(Unit Mineral);

		virtual string getName() override {
			return "Mineral";
		}
		virtual State *action() override;

	private:
		bool pre_gathered;
		bool pre_carry;
		int skip_cnt;
		Unit healerScv;
		int repairStartTime = 0;
		bool started = false;
		int startHp = 0;
	};

	class ScvPreMineralState : public State
	{
	public:
		virtual string getName() override {
			return "PreMineral";
		}
	};

	class ScvGasState : public State
	{
	public:
		ScvGasState(Unit ref) {
			m_targetUnit = ref;
		}
		virtual string getName() override {
			return "Gas";
		}
		virtual State *action() override;
	};

	class ScvBuildState : public State
	{
	public:
		virtual string getName() override {
			return "Build";
		}
	};

	class ScvScoutState : public State
	{
	public:
		virtual string getName() override {
			return "Scout";
		}
	};

	class ScvRepairState : public State
	{
	public:
		ScvRepairState(Unit target);
		virtual string getName() override {
			return "Repair";
		}
		virtual State *action() override;
	private:
		int Threshold;
	};

	class ScvBunkerDefenceState : public State
	{
	public:
		virtual string getName() override {
			return "BunkerDefence";
		}
	};

	class ScvScoutUnitDefenceState : public State
	{
	public:
		ScvScoutUnitDefenceState(Unit eScouter);

		virtual string getName() override {
			return "ScoutUnitDefence";
		}
		virtual State *action() override;
	private:
		bool m_isEarlyScout;
	};

	class ScvScanMyBaseState : public State {
	public:
		ScvScanMyBaseState(Unit targetUnit = nullptr) {
			m_targetPos = Positions::None;
			m_targetUnit = targetUnit;
		}
		virtual string getName() override {
			return "ScanMyBase";
		}
		virtual State *action() override;
	private:
		vector<Position> searchList;
	};

	class ScvEarlyRushDefenseState : public State {
	public:
		ScvEarlyRushDefenseState(Unit targetUnit, Position pos, bool isNecessary = false) {
			m_targetUnit = targetUnit;
			m_targetPos = pos;
			m_isNecessary = isNecessary;
		}

		virtual string getName() override {
			return "EarlyRushDefense";
		}
		virtual State *action() override;
	private:
		bool m_isNecessary;
	};

	class ScvNightingaleState : public State
	{
	public:
		virtual string getName() override {
			return "Nightingale";
		}
	};

	class ScvFirstChokeDefenceState : public State
	{
	public:
		virtual string getName() override {
			return "FirstChokeDefence";
		}
	};

	class ScvRemoveBlockingState : public State
	{
	public:
		virtual string getName() override {
			return "RemoveBlocking";
		}
	};

	class ScvPrisonedState : public PrisonedState
	{
	public:
		State *action() {
			if (prisonedTime + 24 * 60 < TIME)
				return nullptr;

			return new ScvIdleState();
		}
	};
}