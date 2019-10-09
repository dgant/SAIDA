#include "MedicState.h"

#include "..\UnitManager/MarineManager.h"
#include "../InformationManager.h"
#include "../StrategyManager.h"
#include "../EnemyStrategyManager.h"
using namespace MyBot;

State *MedicIdleState::action()
{
	CommandUtil::move(unit, INFO.getSecondChokePosition(S, ChokePoint::end1));
	return nullptr;
}

State *MedicRetreatState::action()
{
	uList Medic = INFO.getTypeUnitsInRadius(Terran_Medic, S, unit->getPosition(), 5 * TILE_SIZE);
	//if (Medic.size() >= 2)
//	{
	//	return new MedicHealState;
	//}
	 if (unit->getDistance(MYBASE) > 25 * TILE_SIZE)
		{
			CommandUtil::move(unit, MYBASE);
			return nullptr;
		}


		return nullptr;

	}

State *MedicHealState::action()
{
	UnitInfo *m = INFO.getUnitInfo(unit, S);
	uList  AroundMarine = INFO.getTypeUnitsInRadius(Terran_Marine, S, m->pos(),3 * TILE_SIZE);
	UnitInfo *ClosestMarine = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Marine, 3 * TILE_SIZE);
	
	for (auto m : AroundMarine)
	{
		if (m->hp() < 35)
		{
			CommandUtil::move(unit, m->pos());
			break;
		}
	}
	return nullptr;
  }
	
State *MedicFindPatientState::action()
{
	UnitInfo *m = INFO.getUnitInfo(unit, S);
	Position targetPosition = SM.getMainAttackPosition();
	UnitInfo *frontMarine = MM.getFrontMarineFromPos(targetPosition);
	UnitInfo *ClosestMarine = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Marine, 15 * TILE_SIZE);
	uList    AroundMarine = INFO.getTypeUnitsInRadius(Terran_Marine, S, m->pos(), 3 * TILE_SIZE);

	
	if (ClosestMarine != nullptr)
	{
		if (getGroundDistance(ClosestMarine->pos(), unit->getPosition()) > 3 * TILE_SIZE)
		{
		
		CommandUtil::move(unit, ClosestMarine->pos());
		return nullptr;
		}
	}
	else if (ClosestMarine == nullptr && frontMarine != nullptr)
	{
		if (getGroundDistance(frontMarine->pos(), unit->getPosition()) > 3 * TILE_SIZE)
		{
			CommandUtil::move(unit, frontMarine->pos());
			return nullptr;
		}
	}
	else if (ClosestMarine == nullptr && frontMarine == nullptr)
	{
		CommandUtil::move(unit, MYBASE);
		return nullptr;
	}
	return nullptr;
}
