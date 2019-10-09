#include "WraithState.h"
#include "../InformationManager.h"
#include "../UnitManager/TankManager.h"

using namespace MyBot;

bool defaultAction(Unit me)
{
	vector<Unit> enemiesTargetMe = INFO.getUnitInfo(me, S)->getEnemiesTargetMe();
	Unit enemy = nullptr;
	Unit closestTarget = nullptr;

	// ���� �� �����ϳ�
	enemy = UnitUtil::GetClosestEnemyTargetingMe(me, enemiesTargetMe);

	//���� ����� ���ݰ����� ��
	int range = max(me->getType().airWeapon().maxRange(), me->getType().groundWeapon().maxRange());
	closestTarget = me->getClosestUnit(BWAPI::Filter::IsEnemy, range);

	if (enemy != nullptr)	// ���� ������ ȸ��
	{
		Position dir = me->getPosition() - enemy->getPosition();
		Position pos = getCirclePosFromPosByDegree(me->getPosition(), dir, 15);
		goWithoutDamage(me, pos, 1);
		return true;
	}
	else if (closestTarget != nullptr && closestTarget->getType() == UnitTypes::Zerg_Overlord)	// �����ε� �߽߰� ����
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
		//cout << "Idle���� Move ���" << movePosition << endl;
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

	//�ֺ��� �ٸ� Wraith�� ������ Ÿ�� ����� �ٲ��ݴϴ�
	for (auto eWraith : INFO.getTypeUnitsInRadius(Terran_Wraith, E, unit->getPosition(), TILE_SIZE * 8))
	{
		targetUnit = eWraith->unit();
		break;
	}

	//�ͷ��� ������ ��������
	if (!INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, unit->getPosition(), TILE_SIZE * 10).empty())
	{
		//�ϴ� Idle������ �� ���� ù��° ��ũ����Ʈ�� ��������..
		Position movePosition = INFO.getFirstChokePosition(S);
		CommandUtil::move(unit, movePosition);
		tU = nullptr;

		return new WraithIdleState();
	}

	//��밡 Ŭ��ŷ �ߴµ� ��ĵ ������ ���� ����
	if (targetUnit->isCloaked() && INFO.getAvailableScanCount() < 1)
	{
		//�ϴ� Idle������ �� ���� ù��° ��ũ����Ʈ�� ��������..
		//Position movePosition = INFO.getFirstChokePosition(S);
		//CommandUtil::move(unit, movePosition);
		tU = nullptr;

		return new WraithIdleState();
	}

	//Ŭ��ŷ ó��
	if (targetUnit->getDistance(unit) <= targetUnit->getType().sightRange() && !unit->isCloaked())
	{
		unit->cloak();
	}
	else if (targetUnit->getDistance(unit) > targetUnit->getType().sightRange() && unit->isCloaked())
	{
		unit->decloak();
	}

	if (targetUnit->getTarget() == unit || targetUnit->getOrderTarget() == unit) //��밡 ���� Ÿ������ ��� �ִ� ���
	{
		CommandUtil::attackUnit(unit, targetUnit);
	}
	else //��밡 ����ġ�� �ִ� ���
	{
		////������ START////
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
		/////������ END///////
	}

	return nullptr;
}

State *WraithKillScvState::action(Position targetPosition)
{
	uList enemyWraithList = INFO.getUnits(Terran_Wraith, E);

	// �� ���̽��� ���� ��� Idle �� �ٲ� �� AttackWraith �� �ǵ���
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

	// targetPosition ���� �̵� �� �����ϰ� ���Ƽ� �̵�
	if (unit->getPosition().getDistance(targetPosition) > 10 * TILE_SIZE)
	{
		//printf("----------GoWithoutDamage\n");
		goWithoutDamage(unit, targetPosition, 1);
		return nullptr;
	}

	// targetPosition ������ ������ �ִ� ��Ŀ���� ���� �ʵ��� (Ŭ��ŷ �ȵǾ� ���� ��)
	UnitInfo *closestBunker = INFO.getClosestTypeUnit(E, unit->getPosition(), Terran_Bunker, 10 * TILE_SIZE, false);

	// Ŭ��ŷ �ȵǾ� �ְų� Ŭ��ŷ������ ������ ���� �� ��Ŀ �ڷ�
	if (closestBunker != nullptr && unit->getDistance(closestBunker->pos()) < 8 * TILE_SIZE)
	{
		if (!unit->isCloaked())
		{
			//printf("[%d] �ڷ� ���������� (%d, %d)\n", unit->getID(), unit->getPosition().x / TILE_SIZE, unit->getPosition().y / TILE_SIZE);
			moveBackPostion(INFO.getUnitInfo(unit, S), closestBunker->pos(), 1 * TILE_SIZE);
			//CommandUtil::backMove(unit, getBackPostion(unit->getPosition(), closestBunker->pos(), 2));
			return nullptr;
		}

		else if ((unit->isCloaked() && unit->getEnergy() < 10) && (TIME % 24 * 2 == 0))
		{
			//printf("[%d] �ڷ� ����������\n", unit->getID());
			moveBackPostion(INFO.getUnitInfo(unit, S), closestBunker->pos(), 6 * TILE_SIZE);
			return nullptr;
		}
	}

	// �ϲ� ��������
	uList enemyWorkerList = INFO.getTypeUnitsInRadius(INFO.getWorkerType(), E, targetPosition, TILE_SIZE * 12, false);
	uList enemyMedicList = INFO.getTypeUnitsInRadius(Terran_Medic, E, targetPosition, TILE_SIZE * 8, false);
	uList turretList = INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, targetPosition, TILE_SIZE * 12, true, true);
	Unit targetUnit = nullptr;
	int targetId = INT_MAX;

	// �ͷ� �Ǽ����� �ϲ� �켱 ����
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

	// �ͷ� �Ǽ����� �ϲ� ����
	if (targetUnit != nullptr)
	{
		CommandUtil::attackUnit(unit, targetUnit);
		return nullptr;
	}

	// ��Ŀ�� ������ ������ ����, ��Ŀ �Ǽ����� �ϲ� ����
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
		// Ŭ��ŷ �ϰ� �ִµ� ������ �ް� �ִٸ� ����
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

	// �ǹ� �Ǽ����� �ϲ� ����
	if (targetUnit != nullptr)
	{
		CommandUtil::attackUnit(unit, targetUnit);
		return nullptr;
	}

	//�ش� ������ �� �ϲ��� ���� ��� Idle�� State ����
	if (enemyWorkerList.empty())
		return new WraithIdleState();

	bool needCloak = false;

	//	if (unit->getTarget() == nullptr) //Ÿ�� ������ ���� ���
	{
		if (!enemyMedicList.empty()) //�޵��� ������ �޵���� ó��
		{
			for (auto medic : enemyMedicList) //ID���� ���� ���� ������ �켱������ ������
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
				// Ŭ��ŷ �ȵǾ� ���� �� ��Ŀ ������ �ִ� SCV �� ����
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

			// ��Ŀ ��ó scv �ۿ� ���� ���� ��� Ŭ��ŷ ��û
			if (targetUnit == nullptr)
			{
				for (auto worker : enemyWorkerList) //ID���� ���� ���� ������ �켱������ ������
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
			// Ŭ��ŷ�� �ʿ�
			if (needCloak)
			{
				// Ŭ��ŷ �ȵǾ� ������ Ŭ��ŷ
				if (!unit->isCloaked())
				{
					if (unit->canCloak() && unit->getEnergy() > 40)
					{
						unit->cloak();
					}
				}
				else
				{
					// ���� ������ ����
					//if (unit->getEnergy() < 10)
					//	return new WraithIdleState();

					if (unit->getEnergy() > 10)
					{
						// Ŭ��ŷ �Ǿ� ������ ����
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
	else // DangerUnit ����.
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

		// ������ �������� Ÿ���� ���� �ʴ´�.
		if (enemyBarrack != nullptr && !enemyBarrack->getLift())
			enemyBarrack = nullptr;

		UnitInfo *enemyEngineering = INFO.getClosestTypeUnit(E, w->pos(), Terran_Engineering_Bay);

		// ������ �������� Ÿ���� ���� �ʴ´�.
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
