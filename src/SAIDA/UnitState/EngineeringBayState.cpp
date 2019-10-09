#include "EngineeringBayState.h"
#include "../InformationManager.h"

using namespace MyBot;

EngineeringBayLiftAndMoveState::EngineeringBayLiftAndMoveState()
{
}

State *EngineeringBayLiftAndMoveState::action()
{
	//Lift���ϰ� ������ Lift
	if (!unit->isLifted() && unit->canLift())
	{
		unit->lift();
	}

	//HP 35%�����̸� Repair��
	if (unit->getHitPoints() < Terran_Engineering_Bay.maxHitPoints() * 0.35)
		return new EngineeringBayNeedRepairState();

	//////////Target��ġ �����ؼ� �̵�/////////////
	/*
	1. �ո��� �̳׶� ��� ������(base.getEdgemineral�̿�)���� Ŀ�ǵ� �ݴ� �������� �̵��� ������ ��������
	2. �� ������ ���� �� ���� ������ �̵�
	3. �ʹ� �־����� Ŀ�ǵ� ���� �������� �̵�
	4. Under Attack�̸� �ͷ� ������ ���ϱ�/ �ͷ� ������ Ŀ�ǵ� ������ /�װ͵� ������ �׳� �������� �̵�
	5. HP 35% ������ �������� Repair�� ����
	*/

	Base *base = INFO.getFirstExpansionLocation(S);
	Position targetPos = Positions::None;
	Position mineralAvgPos = Positions::None;

	if (base == nullptr) return nullptr;

	if (unit->isUnderAttack())
	{
		UnitInfo *mt = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Missile_Turret, 24 * TILE_SIZE);
		UnitInfo *com = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Command_Center, 12 * TILE_SIZE);

		if (mt != nullptr)
		{
			targetPos = mt->pos();
		}
		else if (com != nullptr)
		{
			targetPos = com->pos();
		}
		else
		{
			targetPos = MYBASE;
		}

		CommandUtil::move(unit, targetPos);
		return nullptr;
	}

	if (!firstMoved)
	{
		Unit firstM = base->getEdgeMinerals().first;
		Unit secondM = base->getEdgeMinerals().second;

		if (firstM != nullptr && secondM != nullptr)
		{
			mineralAvgPos = (firstM->getInitialPosition() + secondM->getInitialPosition()) / 2;
		}

		if (mineralAvgPos.isValid())
		{
			targetPos = getDirectionDistancePosition(base->Center(), mineralAvgPos, 10 * TILE_SIZE);

			if (!targetPos.isValid())
			{
				targetPos = mineralAvgPos;
			}
		}

		if (targetPos.isValid())
		{
			if (targetPos.getApproxDistance(unit->getPosition()) < 2 * TILE_SIZE)
			{
				firstMoved = true;
			}

			CommandUtil::move(unit, targetPos);
			return nullptr;
		}

	}
	else
	{
		UnitInfo *cu = INFO.getClosestUnit(E, unit->getPosition(), AllKind, 15 * TILE_SIZE, false, true, false, false);

		//Base���� �ʹ� �ָ� �������� ���̽� ������ �̵�
		if (base->Center().getApproxDistance(unit->getPosition()) > 12 * TILE_SIZE)
		{
			targetPos = getDirectionDistancePosition(unit->getPosition(), base->Center(), TILE_SIZE * 2);
		}
		//�׷��� ������ ����� �� ������ �̵� (��, ���� base���� �Ÿ��� ������ base���� �Ÿ����� �� ��쿡�� �������� �̵�
		else if (cu != nullptr && cu->pos().isValid() && cu->pos().getApproxDistance(base->Center()) > unit->getPosition().getApproxDistance(base->Center()))
		{
			targetPos = getDirectionDistancePosition(unit->getPosition(), cu->pos(), TILE_SIZE * 3);
		}

		if (targetPos.isValid())
		{
			CommandUtil::move(unit, targetPos);
		}
	}

	return nullptr;
}

State *EngineeringBayBarricadeState::action()
{
	if (!unit->isLifted() && (SM.getMainStrategy() == AttackAll || EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier)) {
		unit->lift();
		return new EngineeringBayLiftAndMoveState();
	}

	//Position landPosition = TrainManager::Instance().getBarricadePosition();
	TilePosition landPosition = TerranConstructionPlaceFinder::Instance().getEngineeringBayPositionInSCP();

	if (landPosition == TilePositions::None || !landPosition.isValid()) {
		//		printf("(%d, %d)\n", landPosition.x, landPosition.y);
		return nullptr;
	}

	UnitInfo *ui = INFO.getUnitInfo(unit, S);

	if (ui->getVeryFrontEnemyUnit() != nullptr)
	{
		moveBackPostion(ui, ui->getVeryFrontEnemyUnit()->getPosition(), 5 * TILE_SIZE);
		return nullptr;
	}

	if (unit->isLifted()) {
		if (!unit->canLand(false))
		{
			return nullptr;
		}

		if (unit->canLand(landPosition))
			unit->land(landPosition);
	}
	else {
		if (unit->getPosition().getApproxDistance((Position)landPosition) >= 4 * TILE_SIZE)
			unit->lift();
	}

	return nullptr;
}

State *EngineeringBayNeedRepairState::action()
{
	if (unit->getHitPoints() == Terran_Engineering_Bay.maxHitPoints())
		return new EngineeringBayLiftAndMoveState();

	Position movePosition = Positions::Unknown;
	UnitInfo *closestTurret = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Missile_Turret, 20 * TILE_SIZE);

	if (closestTurret != nullptr)
	{
		movePosition = closestTurret->pos();
	}
	else
	{
		UnitInfo *closestCommandCenter = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Command_Center, 20 * TILE_SIZE);

		if (closestCommandCenter != nullptr)
		{
			movePosition = closestCommandCenter->pos();
		}
		else
		{
			movePosition = MYBASE;
		}
	}

	if (movePosition.isValid())
		CommandUtil::move(unit, movePosition);

	return nullptr;
}
