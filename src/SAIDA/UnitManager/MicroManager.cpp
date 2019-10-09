#include "MicroManager.h"

using namespace Micro;

MicroManager::MicroManager() 
{
}

void MicroManager::setUnits(const BWAPI::Unitset & u) 
{ 
	_units = u; 
}
void  update()
{

}
BWAPI::Position MicroManager::calcCenter() const
{
    if (_units.empty())
    {
		bool  DrawSquadInfo = true;
		if (DrawSquadInfo)
        {
            BWAPI::Broodwar->printf("calcCenter() called on empty squad");
        }
        return BWAPI::Position(0,0);
    }

	BWAPI::Position accum(0,0);
	for (const auto unit : _units)
	{
		accum += unit->getPosition();
	}
	return BWAPI::Position(accum.x / _units.size(), accum.y / _units.size());
}

void MicroManager::execute()
{
	// Nothing to do if we have no units.
	if (_units.empty())
	{
		return;
	}

	//order = inputOrder;             // remember our order
	drawOrderText();

	// If we have no combat order (attack or defend), we're done.

	// Discover enemies within the region of interest.
	BWAPI::Unitset nearbyEnemies;

	// Always include enemies in the radius of the order.

	// For some orders, add enemies which are near our units, for different versions of "near"
	executeMicro(nearbyEnemies);
}

const BWAPI::Unitset & MicroManager::getUnits() const
{ 
    return _units; 
}

// Unused but potentially useful.
bool MicroManager::containsType(BWAPI::UnitType type) const
{
	for (const auto unit : _units)
	{
		if (unit->getType() == type)
		{
			return true;
		}
	}
	return false;
}

void MicroManager::regroup(const BWAPI::Position & regroupPosition) const
{
	for (const auto unit : _units)
	{
		// 1. A broodling should never retreat, but attack as long as it lives.
		// 2. If none of its kind has died yet, a dark templar or lurker should not retreat.
		// 3. A ground unit next to an enemy sieged tank should not move away.
		// TODO 4. A unit in stay-home mode should stay home, not "regroup" away from home.
		// TODO 5. A unit whose retreat path is blocked by enemies should do something else, at least attack-move.
		if (buildScarabOrInterceptor(unit))
		{
			// We're done for this frame.
		}
		else if (unit->getType() == BWAPI::UnitTypes::Zerg_Broodling ||
			unit->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar && BWAPI::Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Protoss_Dark_Templar) == 0 ||
			unit->getType() == BWAPI::UnitTypes::Zerg_Lurker && BWAPI::Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Lurker) == 0 ||
			(BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran &&
			!unit->isFlying() &&
			 BWAPI::Broodwar->getClosestUnit(unit->getPosition(),
				BWAPI::Filter::IsEnemy && BWAPI::Filter::GetType == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode,
				64)))
		{
		}
		else if (unit->getDistance(regroupPosition) > 96)   // air distance, which can be unhelpful sometimes
		{
			if (!mobilizeUnit(unit))
			{
			}
		}
		else
		{
			// We have retreated to a good position.
			if (!immobilizeUnit(unit))
			{
			}
		}
	}
}

// Return true if we started to build a new scarab or interceptor.
bool MicroManager::buildScarabOrInterceptor(BWAPI::Unit u) const
{
	if (u->getType() == BWAPI::UnitTypes::Protoss_Reaver)
	{
		if (!u->isTraining() && u->canTrain(BWAPI::UnitTypes::Protoss_Scarab))
		{
			return u->train(BWAPI::UnitTypes::Protoss_Scarab);
		}
	}
	else if (u->getType() == BWAPI::UnitTypes::Protoss_Carrier)
	{
		if (!u->isTraining() && u->canTrain(BWAPI::UnitTypes::Protoss_Interceptor))
		{
			return u->train(BWAPI::UnitTypes::Protoss_Interceptor);
		}
	}

	return false;
}

bool MicroManager::unitNearEnemy(BWAPI::Unit unit)
{
	assert(unit);

	BWAPI::Unitset enemyNear;

	return enemyNear.size() > 0;
}

// returns true if position:
// a) is walkable
// b) doesn't have buildings on it
// c) isn't blocked by an enemy unit that can attack ground
// NOTE Unused code, a candidate for throwing out.
bool MicroManager::checkPositionWalkable(BWAPI::Position pos) 
{
	// get x and y from the position
	int x(pos.x), y(pos.y);

	// If it's not walkable, throw it out.
	if (!BWAPI::Broodwar->isWalkable(x / 8, y / 8))
	{
		return false;
	}

	// for each of those units, if it's a building or an attacking enemy unit we don't want to go there
	for (const auto unit : BWAPI::Broodwar->getUnitsOnTile(x/32, y/32)) 
	{
		if	(unit->getType().isBuilding() ||
			unit->getType().isResourceContainer() ) 
		{		
			return false;
		}
	}

	// otherwise it's okay
	return true;
}

bool MicroManager::unitNearChokepoint(BWAPI::Unit unit) const
{
	return false;
}

// Mobilize the unit if it is immobile: A sieged tank or a burrowed zerg unit.
// Return whether any action was taken.
bool MicroManager::mobilizeUnit(BWAPI::Unit unit) const
{
	if (unit->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode && unit->canUnsiege())
	{
		return unit->unsiege();
	}
	if (unit->isBurrowed() && unit->canUnburrow() &&
		!unit->isIrradiated() &&
		(double(unit->getType().maxHitPoints()) / double(unit->getHitPoints()) < 6.25))  // very weak units stay burrowed
	{
		return unit->unburrow();
	}
	return false;
}

// Immobilixe the unit: Siege a tank, burrow a lurker. Otherwise do nothing.
// Return whether any action was taken.
bool MicroManager::immobilizeUnit(BWAPI::Unit unit) const
{
	if (unit->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode && unit->canSiege())
	{
		return unit->siege();
	}
	if (unit->canBurrow() &&
		(unit->getType() == BWAPI::UnitTypes::Zerg_Lurker || unit->isIrradiated()))
	{
		return unit->burrow();
	}
	return false;
}

// Sometimes a unit on ground attack-move freezes in place.
// Luckily it's easy to recognize, though units may be on PlayerGuard for other reasons.
// Return whether any action was taken.
bool MicroManager::unstickStuckUnit(BWAPI::Unit unit)
{
	if (!unit->isMoving() && !unit->getType().isFlyer() && !unit->isBurrowed() &&
		unit->getOrder() == BWAPI::Orders::PlayerGuard &&
		BWAPI::Broodwar->getFrameCount() % 4 == 0)
	{
		return true;
	}

	return false;
}

void MicroManager::drawOrderText() 
{
	bool DrawUnitTargetInfo = false;
	if (DrawUnitTargetInfo)
    {
		for (const auto unit : _units)
		{
			BWAPI::Broodwar->drawTextMap(unit->getPosition().x, unit->getPosition().y, "%s","Construction");
		}
	}
}
