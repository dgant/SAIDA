#include "ComsatStationState.h"

using namespace MyBot;

State *ComsatStationIdleState::action()
{
	if (unit->getEnergy() < TechTypes::Scanner_Sweep.energyCost())
		return nullptr;

	if (unit->getSpellCooldown() > 0)
		return nullptr;

	
	return new ComsatStationActivationState();
}

State *ComsatStationActivationState::action()
{
	return nullptr;
}

State *ComsatStationScanState::action(Position targetPosition)
{
	unit->useTech(TechTypes::Scanner_Sweep, targetPosition);

	return new ComsatStationIdleState();
}