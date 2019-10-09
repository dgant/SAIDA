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

	//초반 이후 전략인 경우
	if (frontTank != nullptr && SM.getMainStrategy() != WaitToBase && SM.getMainStrategy() != WaitToFirstExpansion)
	{
		if ((ba->getEnemiesTargetMe().size() > 0 && dangerPoint < 4 * TILE_SIZE) || unit->isUnderAttack()) // Danger 상태임
		{
			if (frontTank != nullptr)
				CommandUtil::move(unit, frontTank->pos());
			else
				CommandUtil::move(unit, MYBASE);

			return nullptr;
		}
		else //안전한 경우
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
	else if (INFO.enemyRace == Races::Terran) //테란전 초반 전략인 경우
	{
		// HP가 45%미만이면 리페어 상태로 변경(상대 본진에 있는 상황이기 때문에 조금 더 HP 높을 때 돌아오도록 변경
		if (unit->getHitPoints() < Terran_Barracks.maxHitPoints() * 0.45)
			return new BarrackNeedRepairState();

		//아머리 지어진 경우, 스타포트 지어진 경우, 적 탱크 숫자가 많아진 경우 우리 베이스로 돌아오도록 처리 + wraith나온 경우
		if (INFO.getCompletedCount(Terran_Armory, E) || INFO.getCompletedCount(Terran_Starport, E) || INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, E) + INFO.getDestroyedCount(Terran_Siege_Tank_Tank_Mode, E) > 0
				|| INFO.getAllCount(Terran_Wraith, E) > 0 || ( INFO.getAllCount(Terran_Marine, E) >= 3 && unit->getHitPoints() < Terran_Barracks.maxHitPoints() * 0.60) || INFO.getAllCount(Terran_Goliath, E) > 0)
		{
			if (frontTank != nullptr && frontTank->pos().isValid())
				CommandUtil::move(unit, frontTank->pos());
			else
				CommandUtil::move(unit, INFO.getSecondChokePosition(S));

			return nullptr;
		}

		////////상대방 베이스쪽에 가 있는 상황에서 Move 처리////////////
		Position targetPos = Positions::None;

		Base *efe = INFO.getFirstExpansionLocation(E);
		const Base *em = INFO.getMainBaseLocation(E);

		if (efe == nullptr) return nullptr;

		//1. efe의 커맨드센터를 못 본 경우 efe로 간다
		//2. efe에서 마린이 들어 있는 벙커를 만나면 goWithoutDamage로 처리
		//3. goWithoutDamage로 갈 수 없거나, 커맨드센터 위치 가까이에 도착하면 em으로 이동한다.
		//4. em에 머신샵이 있고, 그 위치가 안전하면 그 위에 있는다.
		//5. 머신샵이 없으면 팩토리 위치로 이동한다.
		//(모든 경우에서 HP가 45% 미만으로 떨어지면 repair로 변경)

		//앞마당 확인 못한 경우 하도록
		if (!efeChecked)
		{
			targetPos = efe->getPosition();

			if (!INFO.getTypeBuildingsInRadius(Terran_Command_Center, E, targetPos, 5 * TILE_SIZE, true, true).empty() //CommandCenter를 봤거나
					|| targetPos.getApproxDistance(unit->getPosition()) < 6 * TILE_SIZE) // 그게 아니더라도 이미 배럭이 그 위치에 가 있으면
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

		// 해당 위치가 walkable이면 간다. // walkable이 아니면 SCV가 수리를 못함.
		if (bw->isWalkable((WalkPosition)movePosition) && getGroundDistance(frontTank->pos(), movePosition) < 10 * TILE_SIZE) {
			CommandUtil::move(unit, movePosition);
			return nullptr;
		}

		// 좌우 10도씩 돌리면서 Walkable인 곳을 찾아서 간다.
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
		// 바리케이트 위치 못구했을 때 앞마당 벙커 위치부터 firstChokePoint 까지 이동하며 위치 구하자
		// y = ax + b 이용
		Unit bunker = MM.getBunker();

		// 벙커 없을 때
		if (bunker == nullptr)
			return nullptr;

		// 앞마당 정보 없을 때
		if (INFO.getFirstChokePoint(S) == nullptr || INFO.getFirstExpansionLocation(S) == nullptr)
			return nullptr;

		// 앞마당 벙커가 아닐때
		if (!isSameArea(bunker->getPosition(), INFO.getFirstExpansionLocation(S)->getPosition()))
			return nullptr;

		// pos1 = 벙커 위치, pos2 = 내 본진과 가까운 firstChokePoint 끝점
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

				// 기울기 구할 때 분모 0일때 에러처리
				if (pos2.x - pos1.x == 0)
					return nullptr;

				bool plus = (pos2.x - pos1.x) > 0 ? true : false; // x 증가시킬지 감소 시킬지
				double a = (pos2.y - pos1.y) / (pos2.x - pos1.x); // 기울기
				double b = pos1.y - (a * pos1.x);                 // y 절편

				// 벙커 위치에서 firstChokePoint 방향으로 이동하며 배럭 위치 계산
				for (int i = 0; i < abs(TilePosition(pos2).x - TilePosition(pos1).x); i++)
				{
					int x = pos1.x + (i * (plus ? 1 : -1) * TILE_SIZE);
					int y = int((a * x) + b);

					TilePosition tempTilePosition = TilePosition(Position(x, y));

					if (tempTilePosition.isValid() && bw->canBuildHere(tempTilePosition, Terran_Barracks))
					{
						landPosition = tempTilePosition;
						//cout << "@@ 되는 자리 : " << landPosition << endl;
					}
					else
					{
						//cout << "-- 안되는 자리 : " << tempTilePosition << endl;
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

		// 위에서 구해봤는데도 landPosition 이 없으면 바리게이트 동작 안함
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
		//	// 목표지점에 도착한 경우.

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