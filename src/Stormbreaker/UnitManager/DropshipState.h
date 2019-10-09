#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"

namespace MyBot
{
	class DropshipIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
		virtual State *action() override;
	};

	class DropshipBoardingState : public State
	{
	public:
		virtual string getName() override {
			return "Boarding";
		}
		virtual State *action() override;
	private:
		int waitBoardTIME = 0;
		int preRemainSpace = Terran_Dropship.spaceProvided();
	};

	class DropshipGoState : public State
	{
	public:
		DropshipGoState(Position taregetPosition, bool r) {
			reverse = r;
			dropTarget = taregetPosition;
		}

		virtual string getName() override {
			return "Go";
		}
		virtual State *action() override;

	private:
		int direction = 1;
		Position dropTarget;
		vector<Position> myRoute;
		bool reverse;
	};

	class DropshipBackState : public State
	{
	public:
		DropshipBackState(Position taregetPosition, vector<Position> &route) {
			dropTarget = taregetPosition;
			myRoute = route;
		}

		virtual string getName() override {
			return "Back";
		}
		virtual State *action() override;
	private:
		Position dropTarget;
		vector<Position> myRoute;
	};
}