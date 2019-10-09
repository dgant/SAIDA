#include "FactoryState.h"
#include "../InformationManager.h"

using namespace MyBot;

State *FactoryTrainState::action()
{
	if (!unit->isTraining())
	{
		return new FactoryIdleState();
	}
	else
	{
		return nullptr;
	}

	return nullptr;
}

State *FactoryTrainState::action(UnitType unitType)
{
	if (unitType != None)
	{
		if (INFO.enemyRace == Races::Terran && S->supplyUsed() + unitType.supplyRequired() > 388 + (INFO.getAllCount(Terran_Dropship, S) * 4))
			return nullptr;

		if (!unit->isTraining() && Broodwar->canMake(unitType, unit))
		{
			unit->train(unitType);
		}
	}

	return nullptr;
}

State *FactoryBuildAddonState::action()
{
	if (unit->getAddon() && !unit->isTraining())
	{
		return new FactoryIdleState();
	}

	if (unit->canBuildAddon())
	{
		unit->buildAddon(Terran_Machine_Shop);
	}

	return nullptr;
}

State *FactoryLiftAndLandState::action(TilePosition targetPos)
{
	if (unit->getTilePosition() != targetPos)
	{
		if (!unit->isLifted() && unit->canLift())
			unit->lift();
		else
			unit->land(targetPos);
	}
	else
	{
		if (unit->canLand())
		{
			bool t = unit->land(targetPos);
			cout << "can land " << targetPos << t << endl;
		}
		else
		{
			cout << "can't land " << endl;
			Broodwar->drawCircleMap(unit->getPosition(), 50, Colors::Green, true);
		}
	}


	return nullptr;
}