#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"

namespace MyBot
{
	class GoliathIdleState : public State
	{
	public:
		GoliathIdleState() {
			m_targetPos = Positions::None;
		}
		GoliathIdleState(Position pos) {
			m_targetPos = pos.isValid() ? pos : Positions::None;
		}
		virtual string getName() override {
			return "Idle";
		}
		virtual State *action() override;
	};

	class GoliathAttackState : public State
	{
	public:
		GoliathAttackState() {
			m_targetPos = Positions::None;
		}
		GoliathAttackState(Position pos) {
			m_targetPos = pos;
		}
		virtual string getName() override {
			return "Attack";
		}
		virtual State *action() override;
	};

	class GoliathMoveState : public State
	{
	public:
		GoliathMoveState(Position pos) {
			m_targetPos = pos;
		}
		virtual string getName() override {
			return "Move";
		}
		virtual State *action() override;
	};

	class GoliathDefenseState : public State
	{
	public:
		virtual string getName() override {
			return "Defense";
		}
		virtual State *action() override;
	};

	class GoliathProtossAttackState : public State
	{
	public:
		virtual string getName() override {
			return "ProtossAttack";
		}
		virtual State *action() override;
	};

	class GoliathCarrierAttackState : public State
	{
	public:
		virtual string getName() override {
			return "CarrierAttack";
		}

		virtual State *action() override;
	};

	class GoliathCarrierDefenceState : public State
	{
	public:
		virtual string getName() override {
			return "CarrierDefence";
		}

		virtual State *action() override;

	};

	class GoliathTerranState : public State
	{
	public:
		virtual string getName() override {
			return "VsTerran";
		}
	};

	class GoliathProtectTankState : public State
	{
	public:
		virtual string getName() override {
			return "ProtectTank";
		}
		virtual State *action() override;
	};

	class GoliathFightState : public State
	{
	public:
		virtual string getName() override {
			return "Fight";
		}
		virtual State *action() override;
	};

	class GoliathKeepMultiState : public State
	{
	public:
		GoliathKeepMultiState() {
			m_targetPos = Positions::None;
			basePos = Positions::None;
		}

		GoliathKeepMultiState(Position m) {
			m_targetPos = m;
			basePos = m;
		}
		virtual string getName() override {
			return "KeepMulti";
		}
		virtual State *action() override;

	private :
		bool checkFlag = false;
		Position basePos;
	};

	class GoliathMultiBreakState : public State {
	private:
	public:
		virtual string getName() override {
			return "MultiBreak";
		}

		virtual State *action() override;
	};

	class GoliathDropshipState : public State
	{
	public:
		GoliathDropshipState(Unit t) {
			m_targetUnit = t;
		}

		virtual string getName() override {
			return "Dropship";
		}
		virtual State *action() override;

	private :
		bool firstBoard = false;
		Position targetPosition = Positions::Unknown;
		int timeToClear = 0;
	};
}