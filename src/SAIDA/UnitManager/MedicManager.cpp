#include "../UnitManager\MedicManager.h"
#include "../UnitManager\MarineManager.h"
#include "../TrainManager.h"
#include "../InformationManager.h"

using namespace MyBot;
MedicManager::MedicManager()
{

}

MedicManager &MedicManager::Instance()
{
	static MedicManager instance;
	return instance;
}

void MedicManager::update()
{
	if (TIME < 300 || TIME % 2 != 0)
		return;

	
	uList MedicList = INFO.getUnits(Terran_Medic, S);
	uList MarineList = INFO.getUnits(Terran_Marine, S);
	uList VultureList = INFO.getUnits(Terran_Vulture, S);

	if (MedicList.empty())
		return;

//============================================================
	for (auto m : MedicList)
	{
		string state = m->getState();

		if (state == "New"&& m->isComplete())
		{
			m->setState(new MedicIdleState());
			m->action();
		}
		if (state == "Idle")
		{
			m->setState(new MedicFindPatientState());
			m->action();
		}
		if (state == "FindPatient")
		{
		
			m->action();
		}
		if (state == "Heal")
		{
		
			m->action();
		}



	}
	
}

