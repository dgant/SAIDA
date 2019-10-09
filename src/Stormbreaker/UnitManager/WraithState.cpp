#include "WraithState.h"
#include "../InformationManager.h"
#include "../UnitManager/TankManager.h"

using namespace MyBot;

bool defaultAction(Unit me)
{
	vector<Unit> enemiesTargetMe = INFO.getUnitInfo(me, S)->getEnemiesTargetMe();
	Unit enemy = nullptr;
	Unit closestTarget = nullptr;

	
	enemy = UnitUtil::GetClosestEnemyTargetingMe(me, enemiesTargetMe);


	int range = max(me->getType().airWeapon().maxRange(), me->getType().groundWeapon().maxRange());
	closestTarget = me->getClosestUnit(BWAPI::Filter::IsEnemy, range);

	if (enemy != nullptr)	
	{
		Position dir = me->getPosition() - enemy->getPosition();
		Position pos = getCirclePosFromPosByDegree(me->getPosition(), dir, 15);
		goWithoutDamage(me, pos, 1);
		return true;
	}
	else if (closestTarget != nullptr && closestTarget->getType() == UnitTypes::Zerg_Overlord)	
	{
	
		CommandUtil::attackUnit(me, closestTarget);

		return true;
	}

	return false;
}


State *WraithScoutState::action(Position targetPos)
{
	if (unit == nullptr || unit->exists() == false)	return nullptr;

	if (!defaultAction(unit))
		goWithoutDamage(unit, targetPos, 1);

	return nullptr;
}


State *WraithIdleState::action()
{


	if (!INFO.getUnitInfo(unit, S)->getEnemiesTargetMe().empty() && unit->canCloak() && unit->getEnergy() > 30)
	{
		unit->cloak();
	}

	if (isBeingRepaired(unit)) return nullptr;

	Position movePosition = Positions::None;



	Base *enemyFirstExpansion = INFO.getBaseLocation(INFO.getFirstExpansionLocation(E)->getTilePosition());

	if (enemyFirstExpansion->GetEnemyAirDefenseBuildingCount() || enemyFirstExpansion->GetEnemyAirDefenseUnitCount())
	{
		movePosition = (Position(INFO.getSecondChokePoint(S)->Center()) + INFO.getFirstExpansionLocation(S)->getPosition()) / 2;//INFO.getFirstChokePosition(S);
	}
	else
	{
		movePosition = enemyFirstExpansion->getPosition();
	}

	if (unit->getDistance(movePosition) > 8 * TILE_SIZE)
	{
	
		goWithoutDamage(unit, movePosition, 1);

	}

	return nullptr;
}

State *WraithAttackWraithState::action()
{
	if (tU != nullptr)
	{
	
		action(tU);
	}

	return nullptr;
}

State *WraithAttackWraithState::action(Unit targetUnit)
{
	

	if (targetUnit == nullptr) return nullptr;

	tU = targetUnit;

	
	for (auto eWraith : INFO.getTypeUnitsInRadius(Terran_Wraith, E, unit->getPosition(), TILE_SIZE * 8))
	{
		targetUnit = eWraith->unit();
		break;
	}

	
	if (!INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, unit->getPosition(), TILE_SIZE * 10).empty())
	{
		
		Position movePosition = INFO.getFirstChokePosition(S);
		CommandUtil::move(unit, movePosition);
		tU = nullptr;

		return new WraithIdleState();
	}

	
	if (targetUnit->isCloaked() && INFO.getAvailableScanCount() < 1)
	{
		
		tU = nullptr;

		return new WraithIdleState();
	}

	
	if (targetUnit->getDistance(unit) <= targetUnit->getType().sightRange() && !unit->isCloaked())
	{
		unit->cloak();
	}
	else if (targetUnit->getDistance(unit) > targetUnit->getType().sightRange() && unit->isCloaked())
	{
		unit->decloak();
	}

	if (targetUnit->getTarget() == unit || targetUnit->getOrderTarget() == unit) 
	{
		CommandUtil::attackUnit(unit, targetUnit);
	}
	else 
	{
		
		if (unit->getDistance(targetUnit) > unit->getType().airWeapon().maxRange())
		{
			if (unit->getTarget() != targetUnit)
			{
				CommandUtil::attackUnit(unit, targetUnit);
			}

			return nullptr;
		}

		if (unit->getAirWeaponCooldown() == 0)
		{
			unit->attack(targetUnit);
		}

		Position movePosition = targetUnit->getPosition() + Position((int)(targetUnit->getVelocityX() * 6), (int)(targetUnit->getVelocityY() * 6));

		unit->move(movePosition);
		
	}

	return nullptr;
}

State *WraithKillScvState::action(Position targetPosition)
{
	uList enemyWraithList = INFO.getUnits(Terran_Wraith, E);

	
	if (!enemyWraithList.empty())
	{
		return new WraithIdleState();
	}

	if (targetPosition == Positions::Unknown)
	{
		return new WraithIdleState();
	}

	if (!INFO.getUnitInfo(unit, S)->getEnemiesTargetMe().empty())
	{
		if (unit->isCloaked())
		{
			return new WraithIdleState();
		}
		else
		{
			if (unit->canCloak() && unit->getEnergy() > 40 && unit->getHitPoints() > 50)
			{
				unit->cloak();
			}
			else
			{
				return new WraithIdleState();
			}
		}
	}

	
	if (unit->getPosition().getDistance(targetPosition) > 10 * TILE_SIZE)
	{
		
		goWithoutDamage(unit, targetPosition, 1);
		return nullptr;
	}

	
	UnitInfo *closestBunker = INFO.getClosestTypeUnit(E, unit->getPosition(), Terran_Bunker, 10 * TILE_SIZE, false);

	
	if (closestBunker != nullptr && unit->getDistance(closestBunker->pos()) < 8 * TILE_SIZE)
	{
		if (!unit->isCloaked())
		{
			
			moveBackPostion(INFO.getUnitInfo(unit, S), closestBunker->pos(), 1 * TILE_SIZE);
			
			return nullptr;
		}

		else if ((unit->isCloaked() && unit->getEnergy() < 10) && (TIME % 24 * 2 == 0))
		{
		
			moveBackPostion(INFO.getUnitInfo(unit, S), closestBunker->pos(), 6 * TILE_SIZE);
			return nullptr;
		}
	}

	
	uList enemyWorkerList = INFO.getTypeUnitsInRadius(INFO.getWorkerType(), E, targetPosition, TILE_SIZE * 12, false);
	uList enemyMedicList = INFO.getTypeUnitsInRadius(Terran_Medic, E, targetPosition, TILE_SIZE * 8, false);
	uList turretList = INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, targetPosition, TILE_SIZE * 12, true, true);
	Unit targetUnit = nullptr;
	int targetId = INT_MAX;

	
	for (auto turret : turretList)
	{
		if (turret->isComplete())
		{
			return new WraithIdleState();
		}
		else if (turret->hp() > 150)
		{
			return new WraithIdleState();
		}
		else
		{
			targetUnit = turret->unit()->getBuildUnit();
		}
	}

	
	if (targetUnit != nullptr)
	{
		CommandUtil::attackUnit(unit, targetUnit);
		return nullptr;
	}

	
	uList enemyBuildingList = INFO.getBuildingsInRadius(E, unit->getPosition(), 10 * TILE_SIZE, true, false, true);
	bool isExistMarineInBunker = false;

	for (auto eb : enemyBuildingList)
	{
		if (!eb->isComplete())
		{
			targetUnit = eb->unit()->getBuildUnit();
		}
		else if (eb->type() == Terran_Bunker && eb->getMarinesInBunker())
		{
			isExistMarineInBunker = true;
		}
	}

	if (isExistMarineInBunker)
	{
		
		if (unit->isCloaked())
		{
			if (unit->isUnderAttack())
				return new WraithIdleState();
		}
		else
		{
			if (unit->isUnderAttack())
			{
				if (unit->canCloak() && unit->getEnergy() > 40 && unit->getHitPoints() > 50)
				{
					unit->cloak();
				}
				else
				{
					return new WraithIdleState();
				}
			}
		}
	}


	if (targetUnit != nullptr)
	{
		CommandUtil::attackUnit(unit, targetUnit);
		return nullptr;
	}

	if (enemyWorkerList.empty())
		return new WraithIdleState();

	bool needCloak = false;


	{
		if (!enemyMedicList.empty())
		{
			for (auto medic : enemyMedicList) 
			{
				if (medic->id() < targetId)
				{
					targetUnit = medic->unit();
					targetId = medic->id();
				}
			}
		}
		else
		{
			for (auto worker : enemyWorkerList)
			{
				
				if (!unit->isCloaked() && closestBunker != nullptr && worker->unit()->getDistance(closestBunker->pos()) < 5 * TILE_SIZE)
				{
					continue;
				}

				if (worker->id() > targetId)
				{
					targetUnit = worker->unit();
					targetId = worker->id();
				}
			}

			
			if (targetUnit == nullptr)
			{
				for (auto worker : enemyWorkerList) 
				{
					if (worker->id() < targetId)
					{
						targetUnit = worker->unit();
						targetId = worker->id();
					}
				}

				if (targetUnit != nullptr)
					needCloak = true;
			}
		}

		if (targetUnit != nullptr)
		{
		
			if (needCloak)
			{
				
				if (!unit->isCloaked())
				{
					if (unit->canCloak() && unit->getEnergy() > 40)
					{
						unit->cloak();
					}
				}
				else
				{
				

					if (unit->getEnergy() > 10)
					{
					
						CommandUtil::attackUnit(unit, targetUnit);

						if (unit->getDistance(targetUnit) > 4 * TILE_SIZE)
							CommandUtil::move(unit, (unit->getPosition() + targetUnit->getPosition()) / 2);
					}
				}
			}
			else
			{
				CommandUtil::attackUnit(unit, targetUnit);

				if (unit->getDistance(targetUnit) > 4 * TILE_SIZE)
					CommandUtil::move(unit, (unit->getPosition() + targetUnit->getPosition()) / 2);
			}
		}
	}

	return nullptr;
}

State *WraithFollowTankState::action()
{
	UnitInfo *w = INFO.getUnitInfo(unit, S);

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(w->pos(), &dangerPoint, true);

	UnitInfo *frontTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

	if (dangerPoint < 6 * TILE_SIZE)
	{
		if (frontTank != nullptr)
		{
			CommandUtil::move(unit,	getDirectionDistancePosition(frontTank->pos(), MYBASE, 6 * TILE_SIZE));
		}
		else
			CommandUtil::move(unit, MYBASE);

		return nullptr;
	}
	else 
	{
		if (w->unit()->exists() && w->unit()->getTarget() != nullptr)
		{
			if (w->unit()->getTarget()->getType() == Terran_Siege_Tank_Tank_Mode) return nullptr;
		}

		for (auto et : INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, w->pos(), 15 * TILE_SIZE, false))
		{
			w->unit()->attack(et->unit());
			break;
		}

		UnitInfo *enemyBarrack = INFO.getClosestTypeUnit(E, w->pos(), Terran_Barracks);

		
		if (enemyBarrack != nullptr && !enemyBarrack->getLift())
			enemyBarrack = nullptr;

		UnitInfo *enemyEngineering = INFO.getClosestTypeUnit(E, w->pos(), Terran_Engineering_Bay);

	
		if (enemyEngineering != nullptr && !enemyEngineering->getLift())
			enemyEngineering = nullptr;


		if (enemyBarrack == nullptr &&  enemyEngineering == nullptr)
		{
			if (frontTank != nullptr)
			{
				UnitInfo *closestTarget = INFO.getClosestUnit(E, frontTank->pos(), GroundUnitKind, 15 * TILE_SIZE, true, true);
				Position TargetPos = (closestTarget == nullptr) ? SM.getMainAttackPosition() : closestTarget->pos();

				CommandUtil::move(unit, getDirectionDistancePosition(frontTank->pos(), TargetPos, 4 * TILE_SIZE) );
			}
			else
				CommandUtil::move(unit, MYBASE);
		}
		else if (enemyBarrack != nullptr && enemyEngineering != nullptr)
		{
			if (enemyBarrack->pos().getApproxDistance(w->pos()) > enemyEngineering->pos().getApproxDistance(w->pos()))
				CommandUtil::attackUnit(unit, enemyEngineering->unit());
			else
				CommandUtil::attackUnit(unit, enemyBarrack->unit());
		}
		else if (enemyBarrack == nullptr)
		{
			CommandUtil::attackUnit(unit, enemyEngineering->unit());
		}
		else
			CommandUtil::attackUnit(unit, enemyBarrack->unit());

		if (!unit->isIdle()) return nullptr;

		for (auto eFloatingB : INFO.getBuildingsInRadius(E, w->pos(), 12 * TILE_SIZE, false, true, false))
		{
			CommandUtil::attackUnit(unit, eFloatingB->unit());
			break;
		}
	}

	
	return nullptr;
}
