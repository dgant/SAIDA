#pragma once

#include "../UnitManager\MarineManager.h"
#include "../InformationManager.h"
#include "../UnitState\MedicState.h"
#include "../Common.h"
#include "../StrategyManager.h"
//using namespace std;
namespace MyBot
{
	class MedicTreatmentSet : public UListSet
	{
	private:
	public:
		void init();
		void action();
	};
	class MedicManager
	{
	private:
		MedicManager();
		~MedicManager() {};

		int MaxRepairMedicCnt;
		int RepairStateMedicCount;
		int IdleStateMedicCount;

	public:
		static MedicManager &Instance();
		void update();
		void CheckTreatment();
		void CheckRepairMedic();

		void				beforeRemoveState(UnitInfo *Medic);
		void				setMedicIdleState(Unit);

		Unit                setStateToMedic(UnitInfo *Medic, State *state);

		//Unit				chooseRepairMedicforMedic(Unit unit, int maxRange = INT_MAX);
		UnitInfo			*chooseMedicForMedic(Position position, int avoidMedicID = 0, int maxRange = INT_MAX);
		UnitInfo             *chooseMedicClosestTo(Position position, int maxRange);


		int				getRepairMedicCount()
		{
			return RepairStateMedicCount;
		}
		MedicTreatmentSet treatmentSet;
		clock_t time[18];
	};
}