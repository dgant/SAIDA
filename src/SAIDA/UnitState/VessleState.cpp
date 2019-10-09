#include "VessleState.h"

using namespace MyBot;

State *VessleIdleState::action()
{
	unit->move(pos);
	return nullptr;
}

State *VessleCruiseState::action(Position position)
{
	unit->move(position);
	return nullptr;
}

State *VessleBattleGuideState::action(Unit targetUnit)
{
	return nullptr;
}

State *VessleBattleGuideState::action(Unit targetUnit, Unit targetEnemy)
{
	Race er = INFO.enemyRace;

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(unit->getPosition(), &dangerPoint, true);

	
	if (dangerUnit != nullptr && dangerUnit->type() == Zerg_Scourge && dangerUnit->pos().getApproxDistance(unit->getPosition()) < 4 * TILE_SIZE)
	{
		moveBackPostion(INFO.getUnitInfo(unit, S), dangerUnit->pos(), 8 * TILE_SIZE);
		return nullptr;
	}

	if (unit->getHitPoints() < 110)
	{
		uList nightingaleScvs = INFO.getTypeUnitsInRadius(Terran_SCV, S, unit->getPosition(), TILE_SIZE * 30);

		for (auto scv : nightingaleScvs)
		{
			if (scv->getState() == "Nightingale")
			{
				unit->move(scv->pos());
				return nullptr;
			}
		}
	}

	if (targetUnit == nullptr || !targetUnit->exists()) 
	{
		if (er == Races::Zerg && targetEnemy != nullptr && targetEnemy->exists() && S->hasResearched(TechTypes::Irradiate) && unit->getEnergy() >= 75)
		{
			unit->useTech(TechTypes::Irradiate, targetEnemy);
			return nullptr;
		}
		else if (er == Races::Protoss && targetEnemy != nullptr  && targetEnemy->exists() && S->hasResearched(TechTypes::EMP_Shockwave) && unit->getEnergy() >= 100)
		{
			unit->useTech(TechTypes::EMP_Shockwave, INFO.getUnitInfo(targetEnemy, E)->vPos());
			return nullptr;
		}

		Position pos_ = INFO.getMainBaseLocation(S)->getPosition();

		if (pos_.isValid())
		{
			unit->move(pos_);
		}

		return nullptr;
	}


	if (er == Races::Zerg)
	{
		if (unit->getSpellCooldown() == 0 && unit->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Unit && targetEnemy != nullptr && targetEnemy->exists())
		{
			unit->useTech(TechTypes::Irradiate, targetEnemy);
			return nullptr;
		}

		UnitInfo *closestLurker = INFO.getClosestTypeUnit(E, targetUnit->getPosition(), Zerg_Lurker, 20 * TILE_SIZE, true, false, false);


		if (unit->getHitPoints() >= unit->getInitialHitPoints() * 0.7) 
		{
			if (targetEnemy != nullptr && targetEnemy->exists() && S->hasResearched(TechTypes::Irradiate) && unit->getEnergy() >= 75)
			{
			
				unit->useTech(TechTypes::Irradiate, targetEnemy);
				return nullptr;
			}
		}

		if (dangerUnit != nullptr && dangerPoint < 4 * TILE_SIZE) 
		{
			if (dangerUnit->type() == Zerg_Scourge)
			{
				moveBackPostion(INFO.getUnitInfo(unit, S), dangerUnit->pos(), 8 * TILE_SIZE);
				return nullptr;
			}

			if (dangerUnit->type().isFlyer())
			{
				UnitInfo *closestG = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Goliath, 30 * TILE_SIZE);

				if (closestG != nullptr)
				{
					Position p = getAvgPosition(INFO.getTypeUnitsInRadius(Terran_Goliath, S, unit->getPosition(), 30 * TILE_SIZE));

					if (p.isValid())
					{
						unit->move(p);
					}
					else
					{
						unit->move(MYBASE);
					}
				}
				else
				{
					unit->move(MYBASE);
				}
			}
			else
			{
				Position p = getAvgPosition(INFO.getTypeUnitsInRadius(targetUnit->getType(), S));

				if (p.isValid())
				{
					unit->move(p);
				}
				else
				{
					unit->move(MYBASE);
				}

			}

			return nullptr;
		}
		else 
		{
			if (targetEnemy != nullptr && targetEnemy->exists() && S->hasResearched(TechTypes::Irradiate) && unit->getEnergy() >= 75)
			{
				
				unit->useTech(TechTypes::Irradiate, targetEnemy);
				return nullptr;
			}

			if (closestLurker != nullptr &&  INFO.getTypeUnitsInRadius(Terran_Science_Vessel, S, closestLurker->pos(), 2 * TILE_SIZE).size() < 1)
			{
				unit->move(closestLurker->pos());
				return nullptr;
			}
			else if (unit->getDistance(targetUnit) > 2 * TILE_SIZE)
			{
				UnitInfo *cu = INFO.getClosestUnit(E, unit->getPosition());
				Position pos = Positions::None;

				if (cu != nullptr && cu->pos().isValid())
				{
					pos = getDirectionDistancePosition(targetUnit->getPosition(), cu->pos(), TILE_SIZE * 5);
				}
				else if (SM.getMainAttackPosition() != Positions::Unknown)
				{
					pos = getDirectionDistancePosition(targetUnit->getPosition(), SM.getMainAttackPosition(), TILE_SIZE * 5);
				}
				else
				{
					pos = targetUnit->getPosition();
				}

				if (pos.isValid())
				{
					unit->move(pos);
				}

				return nullptr;
			}

		}
	}
	else if (er == Races::Protoss)
	{
		if (unit->getSpellCooldown() == 0 && unit->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position && targetEnemy != nullptr && targetEnemy->exists())
		{
		
			unit->useTech(TechTypes::EMP_Shockwave, INFO.getUnitInfo(targetEnemy, E)->vPos());
			return nullptr;
		}




		UnitInfo *closestArbiter = INFO.getClosestTypeUnit(E, targetUnit->getPosition(), Protoss_Arbiter, 20 * TILE_SIZE);

	

		int targetUnitId = 0;
		int targetEnemyId = 0;

		if (targetUnit != nullptr)
		{
			targetUnitId = targetUnit->getID();
		}

		if (targetEnemy != nullptr && targetEnemy->exists())
		{
			targetEnemyId = targetEnemy->getID();
		}


		if (unit->getHitPoints() >= unit->getInitialHitPoints() * 0.7) 
		{
			if (targetEnemy != nullptr && targetEnemy->exists() && S->hasResearched(TechTypes::EMP_Shockwave) && unit->getEnergy() >= 100)
			{
				if (unit->getSpellCooldown() == 0 && unit->getDistance(targetEnemy) < 8 * TILE_SIZE)
				{
	
					unit->useTech(TechTypes::EMP_Shockwave, INFO.getUnitInfo(targetEnemy, E)->vPos());
				}
				else
				{
					CommandUtil::move(unit, targetEnemy->getPosition());
				}

				return nullptr;
			}
		}

		if (dangerUnit != nullptr && dangerPoint < 4 * TILE_SIZE) 
		{
			if (dangerUnit->type().isFlyer())
			{
				UnitInfo *closestG = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Goliath, 30 * TILE_SIZE);

				if (closestG != nullptr)
				{
					Position p = getAvgPosition(INFO.getTypeUnitsInRadius(Terran_Goliath, S, unit->getPosition(), 30 * TILE_SIZE));

					if (p.isValid())
					{
						unit->move(p);
					}
					else
					{
						unit->move(MYBASE);
					}
				}
				else
				{
					unit->move(MYBASE);
				}
			}
			else
			{
				Position p = getAvgPosition(INFO.getTypeUnitsInRadius(targetUnit->getType(), S));

				if (p.isValid())
				{
					unit->move(p);
				}
				else
				{
					unit->move(MYBASE);
				}

			}

			return nullptr;
		}
		else 
		{
			if (targetEnemy != nullptr && targetEnemy->exists() && S->hasResearched(TechTypes::EMP_Shockwave) && unit->getEnergy() >= 100)
			{
				if (unit->getSpellCooldown() == 0 && unit->getDistance(targetEnemy) < 10 * TILE_SIZE)
				{
			
					unit->useTech(TechTypes::EMP_Shockwave, INFO.getUnitInfo(targetEnemy, E)->vPos());
				}
				else
				{
					CommandUtil::move(unit, targetEnemy->getPosition());
				}

				return nullptr;
			}

			if (S->hasResearched(TechTypes::EMP_Shockwave) && unit->getEnergy() > 100 && closestArbiter != nullptr && INFO.getTypeUnitsInRadius(Terran_Science_Vessel, S, closestArbiter->pos(), 5 * TILE_SIZE).size() < 1)
			{
			
				unit->move(closestArbiter->pos());
				return nullptr;
			}
			else if (unit->getDistance(targetUnit) > 2 * TILE_SIZE)
			{
				UnitInfo *cu = INFO.getClosestUnit(E, unit->getPosition());

				Position pos = Positions::None;

				if (cu != nullptr && cu->pos().isValid())
				{
					pos = getDirectionDistancePosition(targetUnit->getPosition(), cu->pos(), TILE_SIZE * 5);
				}
				else if (SM.getMainAttackPosition() != Positions::Unknown)
				{
					pos = getDirectionDistancePosition(targetUnit->getPosition(), SM.getMainAttackPosition(), TILE_SIZE * 5);
				}
				else
				{
					pos = targetUnit->getPosition();
				}

				if (pos.isValid())
				{
					unit->move(pos);
				}

				return nullptr;
			}
		}
	}
	else
	{
		if (unit->getSpellCooldown() == 0 && unit->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Unit && targetEnemy != nullptr && targetEnemy->exists())
		{
			unit->useTech(TechTypes::Defensive_Matrix, targetEnemy);
			return nullptr;
		}



		if (unit->getHitPoints() >= unit->getInitialHitPoints() * 0.5) 
		{
			if (targetEnemy != nullptr && targetEnemy->exists() && S->hasResearched(TechTypes::Defensive_Matrix) && unit->getEnergy() >= 100)
			{
				if (unit->getSpellCooldown() == 0)
				{
					unit->useTech(TechTypes::Defensive_Matrix, targetEnemy);
				}

				return nullptr;
			}
		}

		if (dangerUnit != nullptr && dangerPoint < 5 * TILE_SIZE) 
		{
			if (dangerUnit->type().isFlyer())
			{
				UnitInfo *closestG = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Goliath, 30 * TILE_SIZE);

				if (closestG != nullptr)
				{
					Position p = getAvgPosition(INFO.getTypeUnitsInRadius(Terran_Goliath, S, unit->getPosition(), 30 * TILE_SIZE));

					if (p.isValid())
					{
						unit->move(p);
					}
					else
					{
						unit->move(MYBASE);
					}
				}
				else
				{
					unit->move(MYBASE);
				}
			}
			else
			{
				Position p = getAvgPosition(INFO.getTypeUnitsInRadius(targetUnit->getType(), S));

				if (p.isValid())
				{
					unit->move(p);
				}
				else
				{
					unit->move(MYBASE);
				}

			}

			return nullptr;
		}
		else 
		{
			if (targetEnemy != nullptr && targetEnemy->exists() && S->hasResearched(TechTypes::Defensive_Matrix) && unit->getEnergy() >= 100)
			{
				if (unit->getSpellCooldown() == 0)
				{
					unit->useTech(TechTypes::Defensive_Matrix, targetEnemy);
				}

				return nullptr;
			}

			if (unit->getDistance(targetUnit) > 2 * TILE_SIZE)
			{
				UnitInfo *cu = INFO.getClosestUnit(E, unit->getPosition());

				Position pos = Positions::None;

				if (cu != nullptr && cu->pos().isValid())
				{
					pos = getDirectionDistancePosition(targetUnit->getPosition(), cu->pos(), TILE_SIZE * 4);
				}
				else if (SM.getMainAttackPosition() != Positions::Unknown)
				{
					pos = getDirectionDistancePosition(targetUnit->getPosition(), SM.getMainAttackPosition(), TILE_SIZE * 4);
				}
				else
				{
					pos = targetUnit->getPosition();
				}

				if (pos.isValid())
				{
					unit->move(pos);
				}

				return nullptr;
			}
		}
	}

	if (er != Races::Terran)
	{
		if (unit->getEnergy() >= 150 && unit->getSpellCooldown() == 0)
		{
			if (targetUnit->isUnderAttack() && !targetUnit->isDefenseMatrixed())
			{
				unit->useTech(TechTypes::Defensive_Matrix, targetUnit);
			}
		}
	}

	return nullptr;
}

State *VessleDefenseState::action()
{
	UnitInfo *ab = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Arbiter, 60 * TILE_SIZE);

	if (ab != nullptr && S->hasResearched(TechTypes::EMP_Shockwave) && unit->getEnergy() >= 100)
	{
		unit->useTech(TechTypes::EMP_Shockwave, ab->vPos());
		return nullptr;
	}
	else
	{
		Position p = getAvgPosition(INFO.getTypeBuildingsInArea(Terran_Supply_Depot, S, MYBASE));

		if (p.isValid())
		{
			unit->move(p);
		}
	}

	return nullptr;
}
