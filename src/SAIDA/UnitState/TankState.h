#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"
#include "../UnitManager/TankManager.h"

namespace MyBot
{
	class TankIdleState : public State
	{
	public:
		TankIdleState() {
			idlePos = Positions::Unknown;
		}

		TankIdleState(Position pos) {
			idlePos = pos;
		}

		virtual string getName() override {
			return "Idle";
		}
		virtual State *action() override;
	};

	class TankAttackMoveState : public State
	{
	public:
		virtual string getName() override {
			return "AttackMove";
		}
		virtual State *action() override;
	};

	class TankDefenceState : public State
	{
	private:
		int lastSiegeModeFrame = 0;
	public:
		virtual string getName() override {
			return "Defence";
		}
		virtual State *action() override;
	};

	class TankBaseDefenceState : public State
	{
	private:
		int lastSiegeModeFrame = 0;
	public:
		virtual string getName() override {
			return "BaseDefence";
		}
		virtual State *action() override;
	};

	class TankBackMove : public State {
	private:
		Position targetPos;
	public:
		TankBackMove(Position pos) {
			targetPos = pos;
		}
		virtual string getName() override {
			return "BackMove";
		}
		virtual State *action() override;
	};

	class TankGatheringState : public State {
	public:
		virtual string getName() override {
			return "Gathering";
		}
	};

	class TankSiegeAllState : public State {
	public:
		virtual string getName() override {
			return "SiegeAllState";
		}
	};

	class TankSiegeMovingState : public State {
	public:
		virtual string getName() override {
			return "SiegeMovingState";
		}
	};

	class TankSiegeLineState : public State {
	private:
	public:
		virtual string getName() override {
			return "SiegeLineState";
		}
		virtual State *action() override;
	};

	class TankFightState : public State {
	private :
	public:
		virtual string getName() override {
			return "Fight";
		}
		virtual State *action() override;
	};

	// 탱크가 서로를 엄호하면서 앞마당에 커맨드를 내릴 때 까지 보호한다.
	class TankFirstExpansionSecureState : public State {
	private:
	public:
		virtual string getName() override {
			return "FirstExpansionSecure";
		}
	};

	class TankExpandState : public State {
	private:
	public:
		virtual string getName() override {
			return "ExpandState";
		}
	};

	class TankMultiBreakState : public State {
	private:
	public:
		virtual string getName() override {
			return "MultiBreak";
		}

		virtual State *action() override;
	};

	class TankKeepMultiState : public State {
	public:
		TankKeepMultiState() {
			m_targetPos = Positions::None;
		}
		TankKeepMultiState(Position m) {
			m_targetPos = getDirectionDistancePosition(m, theMap.Center(), 5 * TILE_SIZE);
		}
		virtual string getName() override {
			return "KeepMulti";
		}

		virtual State *action() override;
	};

	class TankDropshipState : public State
	{
	public:
		TankDropshipState(Unit t) {
			m_targetUnit = t;
		}
		virtual string getName() override {
			return "Dropship";
		}
		virtual State *action() override;

	private:
		bool firstBoard = false;
		Position targetPosition = Positions::Unknown;
		int timeToClear = 0;
	};

	class TankProtectAdditionalMultiState : public State {
	public:
		TankProtectAdditionalMultiState() {
			m_targetBase = nullptr;
		}
		TankProtectAdditionalMultiState(Base *b) {
			m_targetBase = b;
		}
		virtual string getName() override {
			return "ProtectAdditionalMulti";
		}

		virtual State *action() override;
	};
}