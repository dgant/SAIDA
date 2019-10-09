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

	//Scourge가 너무 가까이 있으면 무조건 도망가는 로직 추가
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

	if (targetUnit == nullptr || !targetUnit->exists()) //우리유닛 다 죽고 없음...
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


		if (unit->getHitPoints() >= unit->getInitialHitPoints() * 0.7) //HP가 많으면 좀 위험해도 그냥 사용
		{
			if (targetEnemy != nullptr && targetEnemy->exists() && S->hasResearched(TechTypes::Irradiate) && unit->getEnergy() >= 75)
			{
				//cout << unit->getID() << ": Irradiate 사용 : " << targetEnemy->getID() << endl;
				unit->useTech(TechTypes::Irradiate, targetEnemy);
				return nullptr;
			}
		}

		if (dangerUnit != nullptr && dangerPoint < 4 * TILE_SIZE) //위험에 처한 경우
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
		else //안전한 경우
		{
			if (targetEnemy != nullptr && targetEnemy->exists() && S->hasResearched(TechTypes::Irradiate) && unit->getEnergy() >= 75)
			{
				//cout << unit->getID() << ": (2)Irradiate 사용 : " << targetEnemy->getID() << endl;
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
			//cout << "지금 막 쓸려는 참이라구" << endl;
			unit->useTech(TechTypes::EMP_Shockwave, INFO.getUnitInfo(targetEnemy, E)->vPos());
			return nullptr;
		}


		/*if (!unit->canUseTech(TechTypes::EMP_Shockwave, nullptr, true, true, true, false))
		{
			cout << "state : emp 진행중!!" << endl;
			return nullptr;
		}*/

		UnitInfo *closestArbiter = INFO.getClosestTypeUnit(E, targetUnit->getPosition(), Protoss_Arbiter, 20 * TILE_SIZE);

		/*int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(unit->getPosition(), &dangerPoint, true);*/

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

		bw->drawTextMap(unit->getPosition() + Position(0, 20), "danger : %d, Unit:%d, Enemy: %d", dangerPoint, targetUnitId, targetEnemyId);

		if (unit->getHitPoints() >= unit->getInitialHitPoints() * 0.7) //HP가 많으면 좀 위험해도 그냥 사용
		{
			if (targetEnemy != nullptr && targetEnemy->exists() && S->hasResearched(TechTypes::EMP_Shockwave) && unit->getEnergy() >= 100)
			{
				if (unit->getSpellCooldown() == 0 && unit->getDistance(targetEnemy) < 8 * TILE_SIZE)
				{
					//cout << unit->getID() << ": EMP사용!! : " << targetEnemy->getID() << endl;
					//target = targetEnemy;
					unit->useTech(TechTypes::EMP_Shockwave, INFO.getUnitInfo(targetEnemy, E)->vPos());
				}
				else
				{
					CommandUtil::move(unit, targetEnemy->getPosition());
				}

				return nullptr;
			}
		}

		if (dangerUnit != nullptr && dangerPoint < 4 * TILE_SIZE) //위험에 처한 경우
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
		else //안전한 경우
		{
			if (targetEnemy != nullptr && targetEnemy->exists() && S->hasResearched(TechTypes::EMP_Shockwave) && unit->getEnergy() >= 100)
			{
				if (unit->getSpellCooldown() == 0 && unit->getDistance(targetEnemy) < 10 * TILE_SIZE)
				{
					//cout << unit->getID() << ": HP 낮은상태 EMP사용 : " << targetEnemy->getID() << endl;
					//target = targetEnemy;
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
				//cout << "Arbitor 가까이가기 " << closestArbiter->id() << endl;
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
	else///////////////////////////////////////terran인 경우
	{
		if (unit->getSpellCooldown() == 0 && unit->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Unit && targetEnemy != nullptr && targetEnemy->exists())
		{
			unit->useTech(TechTypes::Defensive_Matrix, targetEnemy);
			return nullptr;
		}

		/*int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(unit->getPosition(), &dangerPoint, true);*/

		if (unit->getHitPoints() >= unit->getInitialHitPoints() * 0.5) //HP가 많으면 좀 위험해도 그냥 사용
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

		if (dangerUnit != nullptr && dangerPoint < 5 * TILE_SIZE) //위험에 처한 경우
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
		else //안전한 경우
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
