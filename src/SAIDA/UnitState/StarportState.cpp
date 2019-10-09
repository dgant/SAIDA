#include "StarportState.h"
#include "../InformationManager.h"

using namespace MyBot;

State *StarportTrainState::action()
{
	if (!unit->isTraining())
	{
		return new StarportIdleState();
	}
	else
	{
		return nullptr;
	}

	return nullptr;
}

State *StarportTrainState::action(UnitType unitType)
{
	if (unitType != None)
	{
		if (INFO.enemyRace == Races::Terran && unitType != Terran_Dropship && S->supplyUsed() + unitType.supplyRequired() > 388 + (INFO.getAllCount(Terran_Dropship, S) * 4))
			return nullptr;

		if (!unit->isTraining() && Broodwar->canMake(unitType, unit))
		{
			unit->train(unitType);
		}
	}

	return nullptr;
}

State *StarportBuildAddonState::action()
{
	if (unit->getAddon() && !unit->isTraining())
	{
		return new StarportIdleState();
	}

	if (unit->canBuildAddon())
	{
		unit->buildAddon(Terran_Control_Tower);
	}

	return nullptr;
}
