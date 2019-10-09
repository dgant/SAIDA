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

		// 새로 만들어진 엔베라면 Idle 로 설정
		if (state == "New" && b->isComplete()) {
			b->setState(new EngineeringBayIdleState());
		}

		//바리케이트 상태로 변경
		if (INFO.enemyRace == Races::Protoss && SM.getMainStrategy() != AttackAll &&
				(INFO.isTimeToMoveCommandCenterToFirstExpansion() || INFO.getAltitudeDifference() <= 0) &&
				TerranConstructionPlaceFinder::Instance().getEngineeringBayPositionInSCP().isValid())
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