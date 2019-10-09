#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "../InformationManager.h"
#include "../UnitState\State.h"

namespace MyBot
{
	class MedicIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
		virtual State *action() override;
	};
	class MedicFindPatientState : public State
	{
	public:
		virtual string getName() override {
			return "FindPatient";
		}
		virtual State *action() override;
	};
	class MedicHealState : public State
	{
	public:
		virtual string getName() override {
			return " Heal";
		}
		virtual State *action() override;
	};
	class MedicRetreatState : public State
	{
	public:
		virtual string getName() override {
			return " Retreat";
		}
		virtual State *action() override;
	};
}