#include "BarrackState.h"
#include "../InformationManager.h"
#include "../UnitManager/MarineManager.h"

using namespace MyBot;

State *BarrackTrainState::action()
{
	if (!unit->isTraining())
	{
		return new BarrackIdleState();
	}
	else
	{
		return nullptr;
	}

	return nullptr;
}

State *BarrackTrainState::action(UnitType unitType)
{
	if (unitType != None)
	{
		if (!unit->isTraining() && Broodwar->canMake(unitType, unit))
		{
			unit->train(unitType);
		}
	}

	return nullptr;
}

State *BarrackLiftAndMoveState::action()
{
	if (!unit->isLifted() && unit->canLift())
	{
		unit->lift();
		return nullptr;
	}

	if (unit->getHitPoints() < Terran_Barracks.maxHitPoints() * 0.35)
		return new BarrackNeedRepairState();

	if (SM.getMainStrategy() == BackAll) {
		CommandUtil::move(unit, MYBASE);
		return nullptr;
	}

	UnitInfo *ba = INFO.getUnitInfo(unit, S);
	UnitInfo *frontTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(ba->pos(), &dangerPoint, true);

	//�ʹ� ���� ������ ���
	if (frontTank != nullptr && SM.getMainStrategy() != WaitToBase && SM.getMainStrategy() != WaitToFirstExpansion)
	{
		if ((ba->getEnemiesTargetMe().size() > 0 && dangerPoint < 4 * TILE_SIZE) || unit->isUnderAttack()) // Danger ������
		{
			if (frontTank != nullptr)
				CommandUtil::move(unit, frontTank->pos());
			else
				CommandUtil::move(unit, MYBASE);

			return nullptr;
		}
		else //������ ���
		{
			UnitInfo *closestTarget = nullptr;

			if (INFO.enemyRace == Races::Terran)
				closestTarget = INFO.getClosestTypeUnit(E, frontTank->pos(), Terran_Siege_Tank_Tank_Mode, 20 * TILE_SIZE, true, false);
			else
				closestTarget = INFO.getClosestUnit(E, frontTank->pos(), GroundCombatKind, 20 * TILE_SIZE, false, true);

			Position targetPos = (closestTarget == nullptr) ? SM.getMainAttackPosition() : closestTarget->pos();

			CommandUtil::move(unit, getDirectionDistancePosition(frontTank->pos(), targetPos, 10 * TILE_SIZE));
			return nullptr;
		}

	}
	else if (INFO.enemyRace == Races::Terran) //�׶��� �ʹ� ������ ���
	{
		// HP�� 45%�̸��̸� ����� ���·� ����(��� ������ �ִ� ��Ȳ�̱� ������ ���� �� HP ���� �� ���ƿ����� ����
		if (unit->getHitPoints() < Terran_Barracks.maxHitPoints() * 0.45)
			return new BarrackNeedRepairState();

		//�ƸӸ� ������ ���, ��Ÿ��Ʈ ������ ���, �� ��ũ ���ڰ� ������ ��� �츮 ���̽��� ���ƿ����� ó�� + wraith���� ���
		if (INFO.getCompletedCount(Terran_Armory, E) || INFO.getCompletedCount(Terran_Starport, E) || INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, E) + INFO.getDestroyedCount(Terran_Siege_Tank_Tank_Mode, E) > 0
				|| INFO.getAllCount(Terran_Wraith, E) > 0 || ( INFO.getAllCount(Terran_Marine, E) >= 3 && unit->getHitPoints() < Terran_Barracks.maxHitPoints() * 0.60) || INFO.getAllCount(Terran_Goliath, E) > 0)
		{
			if (frontTank != nullptr && frontTank->pos().isValid())
				CommandUtil::move(unit, frontTank->pos());
			else
				CommandUtil::move(unit, INFO.getSecondChokePosition(S));

			return nullptr;
		}

		////////���� ���̽��ʿ� �� �ִ� ��Ȳ���� Move ó��////////////
		Position targetPos = Positions::None;

		Base *efe = INFO.getFirstExpansionLocation(E);
		const Base *em = INFO.getMainBaseLocation(E);

		if (efe == nullptr) return nullptr;

		//1. efe�� Ŀ�ǵ弾�͸� �� �� ��� efe�� ����
		//2. efe���� ������ ��� �ִ� ��Ŀ�� ������ goWithoutDamage�� ó��
		//3. goWithoutDamage�� �� �� ���ų�, Ŀ�ǵ弾�� ��ġ �����̿� �����ϸ� em���� �̵��Ѵ�.
		//4. em�� �ӽż��� �ְ�, �� ��ġ�� �����ϸ� �� ���� �ִ´�.
		//5. �ӽż��� ������ ���丮 ��ġ�� �̵��Ѵ�.
		//(��� ��쿡�� HP�� 45% �̸����� �������� repair�� ����)

		//�ո��� Ȯ�� ���� ��� �ϵ���
		if (!efeChecked)
		{
			targetPos = efe->getPosition();

			if (!INFO.getTypeBuildingsInRadius(Terran_Command_Center, E, targetPos, 5 * TILE_SIZE, true, true).empty() //CommandCenter�� �ðų�
					|| targetPos.getApproxDistance(unit->getPosition()) < 6 * TILE_SIZE) // �װ� �ƴϴ��� �̹� �跰�� �� ��ġ�� �� ������
			{
				efeChecked = true;
			}

			if (goWithoutDamage(unit, targetPos, 1, 9 * TILE_SIZE) == false)
			{
				CommandUtil::move(unit, targetPos);
			}

			return nullptr;
		}
		else
		{
			targetPos = em->getPosition();

			for (auto f : INFO.getTypeBuildingsInRadius(Terran_Factory, E, targetPos, 15 * TILE_SIZE, true, true))
			{
				targetPos = f->pos();
				break;
			}

			for (auto ms : INFO.getTypeBuildingsInRadius(Terran_Machine_Shop, E, unit->getPosition(), 15 * TILE_SIZE, true, true))
			{
				targetPos = ms->pos();
				break;
			}

			if (emChecked && (unit->isUnderAttack() || dangerPoint < 3 * TILE_SIZE))
			{
				if (frontTank != nullptr && frontTank->pos().isValid())
					CommandUtil::move(unit, frontTank->pos());
				else
					CommandUtil::move(unit, INFO.getSecondChokePosition(S));

				/*CommandUtil::move(unit, targetPos);
				UnitInfo *me = INFO.getUnitInfo(unit, S);

				moveBackPostion(me, dangerUnit->pos(), 5 * TILE_SIZE);*/
				return nullptr;
			}

			if (goWithoutDamage(unit, targetPos, 1, 9 * TILE_SIZE) == false)
			{
				CommandUtil::move(unit, targetPos);
			}

			if (targetPos.getApproxDistance(unit->getPosition()) < 2 * TILE_SIZE)
			{
				emChecked = true;
			}

			return nullptr;
		}
	}

	return nullptr;
}



State *BarrackNeedRepairState::action()
{
	if (unit->getHitPoints() == Terran_Barracks.maxHitPoints())
		return new BarrackLiftAndMoveState();

	UnitInfo *frontTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

	if (frontTank != nullptr)
	{
		Position movePosition = getDirectionDistancePosition(frontTank->pos(), MYBASE, 5 * TILE_SIZE);

		// �ش� ��ġ�� walkable�̸� ����. // walkable�� �ƴϸ� SCV�� ������ ����.
		if (bw->isWalkable((WalkPosition)movePosition) && getGroundDistance(frontTank->pos(), movePosition) < 10 * TILE_SIZE) {
			CommandUtil::move(unit, movePosition);
			return nullptr;
		}

		// �¿� 10���� �����鼭 Walkable�� ���� ã�Ƽ� ����.
		int angle = 10;

		for (int i = 1; i <= 9; i++)
		{
			Position newPosition = getCirclePosFromPosByDegree(frontTank->pos(), movePosition, angle * i);

			if (bw->isWalkable((WalkPosition)newPosition) && getGroundDistance(frontTank->pos(), newPosition) < 10 * TILE_SIZE) {
				CommandUtil::move(unit, newPosition);
				return nullptr;
			}

			newPosition = getCirclePosFromPosByDegree(frontTank->pos(), movePosition, angle * i * -1);

			if (bw->isWalkable((WalkPosition)newPosition) && getGroundDistance(frontTank->pos(), newPosition) < 10 * TILE_SIZE) {
				CommandUtil::move(unit, newPosition);
				return nullptr;
			}
		}
	}
	else
		CommandUtil::move(unit, MYBASE);

	return nullptr;
}

State *BarrackBarricadeState::action()
{
	if (!unit->isLifted() && (SM.getMainStrategy() == AttackAll || EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier)) {
		unit->lift();
		return new BarrackLiftAndMoveState();
	}

	//Position landPosition = TrainManager::Instance().getBarricadePosition();
	TilePosition landPosition = TerranConstructionPlaceFinder::Instance().getBarracksPositionInSCP();

	if (landPosition == TilePositions::None || !landPosition.isValid()) {
		// �ٸ�����Ʈ ��ġ �������� �� �ո��� ��Ŀ ��ġ���� firstChokePoint ���� �̵��ϸ� ��ġ ������
		// y = ax + b �̿�
		Unit bunker = MM.getBunker();

		// ��Ŀ ���� ��
		if (bunker == nullptr)
			return nullptr;

		// �ո��� ���� ���� ��
		if (INFO.getFirstChokePoint(S) == nullptr || INFO.getFirstExpansionLocation(S) == nullptr)
			return nullptr;

		// �ո��� ��Ŀ�� �ƴҶ�
		if (!isSameArea(bunker->getPosition(), INFO.getFirstExpansionLocation(S)->getPosition()))
			return nullptr;

		// pos1 = ��Ŀ ��ġ, pos2 = �� ������ ����� firstChokePoint ����
		vector<Position >pos1List;
		vector<Position> pos2List;

		// pos1List
		Position bunkerCenter = bunker->getPosition();
		Position bunkerTopLeft = (Position)bunker->getTilePosition();

		int distanceCenter = bunkerCenter.getApproxDistance((Position)MYBASE);
		int distanceTopLeft = bunkerTopLeft.getApproxDistance((Position)MYBASE);

		if (distanceCenter < distanceTopLeft)
		{
			pos1List.push_back(bunkerCenter);
			pos1List.push_back(bunkerTopLeft);
		}
		else
		{
			pos1List.push_back(bunkerTopLeft);
			pos1List.push_back(bunkerCenter);
		}

		// pos2List
		Position firstChokePointEnd1 = INFO.getFirstChokePosition(S, BWEM::ChokePoint::end1);
		Position firstChokePointEnd2 = INFO.getFirstChokePosition(S, BWEM::ChokePoint::end2);
		Position firstChokePointMid = INFO.getFirstChokePosition(S, BWEM::ChokePoint::middle);

		int distanceEnd1 = firstChokePointEnd1.getApproxDistance((Position)MYBASE);
		int distanceEnd2 = firstChokePointEnd2.getApproxDistance((Position)MYBASE);

		if (distanceEnd1 < distanceEnd2)
		{
			pos2List.push_back(firstChokePointEnd1);
			pos2List.push_back(firstChokePointMid);
			pos2List.push_back(firstChokePointEnd2);
		}
		else
		{
			pos2List.push_back(firstChokePointEnd2);
			pos2List.push_back(firstChokePointMid);
			pos2List.push_back(firstChokePointEnd1);
		}

		for (auto pos1 : pos1List)
		{
			bool find = false;

			for (auto pos2 : pos2List)
			{
				// bw->drawLineMap((Position)pos1, (Position)pos2, Colors::Red);

				// ���� ���� �� �и� 0�϶� ����ó��
				if (pos2.x - pos1.x == 0)
					return nullptr;

				bool plus = (pos2.x - pos1.x) > 0 ? true : false; // x ������ų�� ���� ��ų��
				double a = (pos2.y - pos1.y) / (pos2.x - pos1.x); // ����
				double b = pos1.y - (a * pos1.x);                 // y ����

				// ��Ŀ ��ġ���� firstChokePoint �������� �̵��ϸ� �跰 ��ġ ���
				for (int i = 0; i < abs(TilePosition(pos2).x - TilePosition(pos1).x); i++)
				{
					int x = pos1.x + (i * (plus ? 1 : -1) * TILE_SIZE);
					int y = int((a * x) + b);

					TilePosition tempTilePosition = TilePosition(Position(x, y));

					if (tempTilePosition.isValid() && bw->canBuildHere(tempTilePosition, Terran_Barracks))
					{
						landPosition = tempTilePosition;
						//cout << "@@ �Ǵ� �ڸ� : " << landPosition << endl;
					}
					else
					{
						//cout << "-- �ȵǴ� �ڸ� : " << tempTilePosition << endl;
						continue;
					}
				}


				if (landPosition != TilePositions::None && landPosition.isValid())
				{
					find = true;
					break;
				}
			}

			if (find)
				break;
		}

		// ������ ���غôµ��� landPosition �� ������ �ٸ�����Ʈ ���� ����
		if (landPosition == TilePositions::None || !landPosition.isValid())
			return nullptr;

		//else
		//	bw->drawBoxMap((Position)landPosition, Position(landPosition + TilePosition(4, 3)), Colors::White);
	}

	if (unit->isLifted()) {
		if (!unit->canLand(false))
		{
			return nullptr;
		}

		if (unit->canLand(landPosition))
			unit->land(landPosition);

		//if (unit->getPosition().getApproxDistance(landPosition) >= 7 * TILE_SIZE) {
		//	CommandUtil::move(unit, landPosition);
		//}
		//else {
		//	// ��ǥ������ ������ ���.

		//	TilePosition realLandPos = ConstructionPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Barracks, (TilePosition)landPosition, false, landPosition);

		//	if (realLandPos != TilePositions::None && unit->canLand()) {
		//		if (theMap.GetArea(realLandPos) == theMap.GetArea((TilePosition)INFO.getSecondChokePoint(S)->Center())) {
		//			unit->land(realLandPos);
		//		}
		//	}
		//}

	}
	else {
		if (unit->getPosition().getApproxDistance((Position)landPosition) >= 4 * TILE_SIZE )
			unit->lift();
	}

	return nullptr;
}