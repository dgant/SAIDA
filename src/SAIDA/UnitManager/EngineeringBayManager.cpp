#include "EngineeringBayManager.h"
#include "../InformationManager.h"

using namespace MyBot;

EngineeringBayManager::EngineeringBayManager()
{

}

EngineeringBayManager &EngineeringBayManager::Instance()
{
	static EngineeringBayManager instance;
	return instance;
}

void EngineeringBayManager::update()
{
	if ( TIME % 4 != 0)
		return;

	uList engineeringBayList = INFO.getBuildings(Terran_Engineering_Bay, S);

	if (engineeringBayList.empty()) return;

	for (auto b : engineeringBayList)
	{
		string state = b->getState();

	
		if (state == "New" && b->isComplete()) {
			b->setState(new EngineeringBayIdleState());
		}

		
		if (INFO.enemyRace == Races::Protoss && SM.getMainStrategy() != AttackAll &&
				(INFO.isTimeToMoveCommandCenterToFirstExpansion() || INFO.getAltitudeDifference() <= 0) &&
				SelfBuildingPlaceFinder::Instance().getEngineeringBayPositionInSCP().isValid())
		{
			b->setState(new EngineeringBayBarricadeState());
			b->action();
			continue;
		}

		if (state == "Idle" && INFO.getFirstExpansionLocation(S))
		{
			b->setState(new EngineeringBayLiftAndMoveState());
		}
		else if (state == "LiftAndMove")
		{
			b->action();
		}
		else if (state == "Barricade")
		{
			b->action();
		}
		else if (state == "NeedRepair")
		{
			b->action();
		}
	}
}