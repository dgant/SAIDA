#include "WraithState.h"
#include "../InformationManager.h"
#include "../UnitManager/TankManager.h"

using namespace MyBot;

bool defaultAction(Unit me)
{
	vector<Unit> enemiesTargetMe = INFO.getUnitInfo(me, S)->getEnemiesTargetMe();
	Unit enemy = nullptr;
	Unit closestTarget = nullptr;

	// 누가 날 공격하나
	enemy = UnitUtil::GetClosestEnemyTargetingMe(me, enemiesTargetMe);

	//가장 가까운 공격가능한 적
	int range = max(me->getType().airWeapon().maxRange(), me->getType().groundWeapon().maxRange());
	closestTarget = me->getClosestUnit(BWAPI::Filter::IsEnemy, range);

	if (enemy != nullptr)	// 공격 받으면 회피
	{
		Position dir = me->getPosition() - enemy->getPosition();
		Position pos = getCirclePosFromPosByDegree(me->getPosition(), dir, 15);
		goWithoutDamage(me, pos, 1);
		return true;
	}
	else if (closestTarget != nullptr && closestTarget->getType() == UnitTypes::Zerg_Overlord)	// 오버로드 발견시 공격
	{
		//me->rightClick(closestTarget);
		CommandUtil::attackUnit(me, closestTarget);

		return true;
	}

	return false;
}

//////////////////////////////////// Scout ///////////////////////////////////////////////////
State *WraithScoutState::action(Position targetPos)
{
	if (unit == nullptr || unit->exists() == false)	return nullptr;

	if (!defaultAction(unit))
		goWithoutDamage(unit, targetPos, 1);

	return nullptr;
}

//////////////////////////////////// Idle ///////////////////////////////////////////////////
State *WraithIdleState::action()
{
	/*if (unit->isCloaked() && !unit->isUnderAttack())
	{
		unit->decloak();
	}*/

	if (!INFO.getUnitInfo(unit, S)->getEnemiesTargetMe().empty() && unit->canCloak() && unit->getEnergy() > 30)
	{
		unit->cloak();
	}

	if (isBeingRepaired(unit)) return nullptr;

	Position movePosition = Positions::None;

	//movePosition = (INFO.getFirstExpansionLocation(S)->Center() + (Position)INFO.getSecondChokePoint(S)->Center()) / 2;

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
		//cout << "Idle상태 Move 명령" << movePosition << endl;
		//CommandUtil::move(unit, movePosition);
		goWithoutDamage(unit, movePosition, 1);

	}

	return nullptr;
}
//////////////////////////////////// AttackWraith ///////////////////////////////////////////////////
State *WraithAttackWraithState::action()
{
	if (tU != nullptr)
	{
		//cout << "action tU" << endl;
		action(tU);
	}

	return nullptr;
}

State *WraithAttackWraithState::action(Unit targetUnit)
{
	//cout << "attack wraith state" << endl;

	if (targetUnit == nullptr) return nullptr;

	tU = targetUnit;

	//주변에 다른 Wraith가 있으면 타겟 대상을 바꿔줍니다
	for (auto eWraith : INFO.getTypeUnitsInRadius(Terran_Wraith, E, unit->getPosition(), TILE_SIZE * 8))
	{
		targetUnit = eWraith->unit();
		break;
	}

	//터렛이 있으면 도망가기
	if (!INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, unit->getPosition(), TILE_SIZE * 10).empty())
	{
		//일단 Idle상태일 때 가는 첫번째 초크포인트로 도망가기..
		Position movePosition = INFO.getFirstChokePosition(S);
		CommandUtil::move(unit, movePosition);
		tU = nullptr;

		return new WraithIdleState();
	}

	//상대가 클로킹 했는데 스캔 없으면 공격 해제
	if (targetUnit->isCloaked() && INFO.getAvailableScanCount() < 1)
	{
		//일단 Idle상태일 때 가는 첫번째 초크포인트로 도망가기..
		//Position movePosition = INFO.getFirstChokePosition(S);
		//CommandUtil::move(unit, movePosition);
		tU = nullptr;

		return new WraithIdleState();
	}

	//클로킹 처리
	if (targetUnit->getDistance(unit) <= targetUnit->getType().sightRange() && !unit->isCloaked())
	{
		unit->cloak();
	}
	else if (targetUnit->getDistance(unit) > targetUnit->getType().sightRange() && unit->isCloaked())
	{
		unit->decloak();
	}

	if (targetUnit->getTarget() == unit || targetUnit->getOrderTarget() == unit) //상대가 나를 타겟으로 잡고 있는 경우
	{
		CommandUtil::attackUnit(unit, targetUnit);
	}
	else //상대가 도망치고 있는 경우
	{
		////무빙샷 START////
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
		/////무빙샷 END///////
	}

	return nullptr;
}

State *WraithKillScvState::action(Position targetPosition)
{
	uList enemyWraithList = INFO.getUnits(Terran_Wraith, E);

	// 적 레이스가 있을 경우 Idle 로 바꾼 후 AttackWraith 가 되도록
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

	// targetPosition 까지 이동 시 안전하게 돌아서 이동
	if (unit->getPosition().getDistance(targetPosition) > 10 * TILE_SIZE)
	{
		//printf("----------GoWithoutDamage\n");
		goWithoutDamage(unit, targetPosition, 1);
		return nullptr;
	}

	// targetPosition 내에서 가까이 있는 벙커까지 가지 않도록 (클로킹 안되어 있을 때)
	UnitInfo *closestBunker = INFO.getClosestTypeUnit(E, unit->getPosition(), Terran_Bunker, 10 * TILE_SIZE, false);

	// 클로킹 안되어 있거나 클로킹했지만 마나가 작을 때 벙커 뒤로
	if (closestBunker != nullptr && unit->getDistance(closestBunker->pos()) < 8 * TILE_SIZE)
	{
		if (!unit->isCloaked())
		{
			//printf("[%d] 뒤로 도망가야해 (%d, %d)\n", unit->getID(), unit->getPosition().x / TILE_SIZE, unit->getPosition().y / TILE_SIZE);
			moveBackPostion(INFO.getUnitInfo(unit, S), closestBunker->pos(), 1 * TILE_SIZE);
			//CommandUtil::backMove(unit, getBackPostion(unit->getPosition(), closestBunker->pos(), 2));
			return nullptr;
		}

		else if ((unit->isCloaked() && unit->getEnergy() < 10) && (TIME % 24 * 2 == 0))
		{
			//printf("[%d] 뒤로 도망가야해\n", unit->getID());
			moveBackPostion(INFO.getUnitInfo(unit, S), closestBunker->pos(), 6 * TILE_SIZE);
			return nullptr;
		}
	}

	// 일꾼 강제어택
	uList enemyWorkerList = INFO.getTypeUnitsInRadius(INFO.getWorkerType(), E, targetPosition, TILE_SIZE * 12, false);
	uList enemyMedicList = INFO.getTypeUnitsInRadius(Terran_Medic, E, targetPosition, TILE_SIZE * 8, false);
	uList turretList = INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, targetPosition, TILE_SIZE * 12, true, true);
	Unit targetUnit = nullptr;
	int targetId = INT_MAX;

	// 터렛 건설중인 일꾼 우선 공격
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

	// 터렛 건설중인 일꾼 공격
	if (targetUnit != nullptr)
	{
		CommandUtil::attackUnit(unit, targetUnit);
		return nullptr;
	}

	// 벙커에 마린이 있으면 도망, 벙커 건설중인 일꾼 공격
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
		// 클로킹 하고 있는데 공격을 받고 있다면 ㅌㅌ
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

	// 건물 건설중인 일꾼 공격
	if (targetUnit != nullptr)
	{
		CommandUtil::attackUnit(unit, targetUnit);
		return nullptr;
	}

	//해당 지역에 적 일꾼이 없는 경우 Idle로 State 해제
	if (enemyWorkerList.empty())
		return new WraithIdleState();

	bool needCloak = false;

	//	if (unit->getTarget() == nullptr) //타겟 유닛이 없는 경우
	{
		if (!enemyMedicList.empty()) //메딕이 있으면 메딕부터 처리
		{
			for (auto medic : enemyMedicList) //ID값이 가장 작은 유닛을 우선순위로 공격함
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
				// 클로킹 안되어 있을 때 벙커 가까이 있는 SCV 는 제외
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

			// 벙커 근처 scv 밖에 남지 않은 경우 클로킹 요청
			if (targetUnit == nullptr)
			{
				for (auto worker : enemyWorkerList) //ID값이 가장 작은 유닛을 우선순위로 공격함
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
			// 클로킹이 필요
			if (needCloak)
			{
				// 클로킹 안되어 있으면 클로킹
				if (!unit->isCloaked())
				{
					if (unit->canCloak() && unit->getEnergy() > 40)
					{
						unit->cloak();
					}
				}
				else
				{
					// 마나 없으면 ㅌㅌ
					//if (unit->getEnergy() < 10)
					//	return new WraithIdleState();

					if (unit->getEnergy() > 10)
					{
						// 클로킹 되어 있으면 공격
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
	else // DangerUnit 없음.
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

		// 떠있지 않은것은 타겟팅 하지 않는다.
		if (enemyBarrack != nullptr && !enemyBarrack->getLift())
			enemyBarrack = nullptr;

		UnitInfo *enemyEngineering = INFO.getClosestTypeUnit(E, w->pos(), Terran_Engineering_Bay);

		// 떠있지 않은것은 타겟팅 하지 않는다.
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

	/*
	if (unit->getHitPoints() < 50)
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

	UnitInfo *fTank = nullptr;

	//fTank = TankManager::Instance().getCloseTankFromPosition(INFO.getMainBaseLocation(E)->Center());
	fTank = TankManager::Instance().getCloseTankFromPosition(SM.getMainAttackPosition());

	if (fTank != nullptr && unit->getDistance(fTank->unit()) > 3 * TILE_SIZE)
	{
		GoWithoutDamage(unit, fTank->pos());
	}
	*/
	return nullptr;
}
