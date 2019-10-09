#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "../UnitManager/VultureManager.h"
#include "State.h"

namespace MyBot
{
	class VultureIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
		virtual State *action() override;
	};

	class VultureScoutState : public State
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

	class VultureKitingState : public State
	{
	public:
		virtual string getName() override {
			return "Kiting";
		}
	};

	class VultureKitingMultiState : public State
	{
	public:
		virtual string getName() override {
			return "KitingMulti";
		}
	};

	class VultureKeepMultiState : public State
	{
	public:
		virtual string getName() override {
			return "KeepMulti";
		}
	};

	class VultureDiveState : public State
	{
	public:
		VultureDiveState() {}
		~VultureDiveState() {}

		virtual string getName() override {
			return "Diving";
		}
		virtual State *action() override;

	private:
		int direction = 1;
	};

	class VultureDefenceState : public State
	{
	public:
		VultureDefenceState() {}
		~VultureDefenceState() {}

		virtual string getName() override {
			return "Defence";
		}
		virtual State *action() override;
	};

	class VultureRemoveMineState : public State
	{
	public :
		VultureRemoveMineState(Unit t) {
			m_targetUnit = t;
		}
		~VultureRemoveMineState() {}

		virtual string getName() override {
			return "RemoveMine";
		}
		virtual State *action() override;
	};

	class VultureKillWorkerFirstState : public State
	{
	public:
		VultureKillWorkerFirstState() {}
		~VultureKillWorkerFirstState() {}

		virtual string getName() override {
			return "KillWorkerFirst";
		}
		virtual State *action() override;
	private:
		bool checkBase = false;
	};
}