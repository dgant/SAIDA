#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"

namespace MyBot
{
	class CommandCenterIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
	};

	class CommandCenterTrainState : public State
	{
	public:
		CommandCenterTrainState(int cnt) {
			count = cnt;
		}
		virtual string getName() override {
			return "Train";
		}
		virtual State *action() override;
	private:
		int count;
	};

	class CommandCenterBuildAddonState : public State
	{
	public:
		virtual string getName() override {
			return "BuildAddon";
		}
		virtual State *action() override;
	};

	class CommandCenterLiftAndMoveState : public State
	{
	public:
		CommandCenterLiftAndMoveState(TilePosition p);
		virtual string getName() override {
			return "LiftAndMove";
		}
		virtual State *action() override;
		TilePosition targetPosition = TilePositions::None;
		UnitInfo *ui = nullptr;
	};

	class CommandLandingState : public State
	{
	public:
		CommandLandingState(Position p);
		virtual string getName() override {
			return "Landing";
		}
		virtual State *action() override;
	private :
		Position targetPosition;
		int nextLandTime;
	};

}