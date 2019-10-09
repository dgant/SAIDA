#pragma once

#include "../Common.h"

using namespace std;

namespace MyBot
{
	class State
	{
	public:
		Unit unit;
		virtual string getName() = 0;
		virtual State *action() {
			return nullptr;
		}
		virtual State *action(Position targetPosition) {
			unit->move(targetPosition);
			return nullptr;
		}
		virtual State *action(TilePosition targetTilePosition) {
			return nullptr;
		}
		virtual State *action(Unit targetUnit) {
			unit->rightClick(targetUnit);
			return nullptr;
		}
		virtual State *action(Unit targetUnit, Unit targetUnit2) {
			return nullptr;
		}
		virtual State *action(BWEM::Base *target) {
			return nullptr;
		}
		virtual State *action(UnitType unitType) {
			return nullptr;
		}
		Unit getTargetUnit() {
			return m_targetUnit;
		}
		Position getTargetPos() {
			return m_targetPos;
		}
		Position getIdlePos() {
			return idlePos;
		}
	protected:
		// Target¿« Target (Mineral / Refinery / Repair Unit)
		Position m_targetPos = Positions::None;
		Unit m_targetUnit = nullptr;
		TilePosition m_targetTilePos = TilePositions::None;
		Base *m_targetBase = nullptr;
		// For Tank
		Position idlePos = Positions::None;

		bool attackCarrierOrInterceptors(Unit u);

	};

	class InitialNewState : public State
	{
	public:
		virtual string getName() override {
			return "New";
		}
	};

	class EliminateState : public State
	{
	public:
		virtual string getName() override {
			return "Eliminate";
		}
	};

	class PrisonedState : public State
	{
	public:
		PrisonedState() {
			prisonedTime = TIME;
		}
		virtual string getName() override {
			return "Prisoned";
		}
		int prisonedTime;
	};
}
