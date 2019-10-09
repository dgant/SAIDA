#include "GoliathState.h"
#include "../UnitManager/GoliathManager.h"
#include "../InformationManager.h"

using namespace MyBot;

State *GoliathIdleState::action()
{
	Position waitPos;

	if (INFO.getFirstExpansionLocation(S) && INFO.getFirstExpansionLocation(S)->GetOccupiedInfo() == myBase)
		waitPos = INFO.getSecondAverageChokePosition(S);
	else
	{
		waitPos = getDirectionDistancePosition(INFO.getFirstChokePosition(S), MYBASE, 3 * TILE_SIZE);
	}

	if (unit->getPosition().getApproxDistance(waitPos) > 5 * TILE_SIZE)
		CommandUtil::move(unit, waitPos, true);

	return nullptr;
}

State *attack(Unit unit, Position targetPosition) {
	UnitInfo *me = INFO.getUnitInfo(unit, S);

	UnitInfo *closestUnit = nullptr;
	Unit closestBuilding = nullptr;
	Unit closestCombatUnit = nullptr;
	UnitInfo *closestHiddenCombatUnit = nullptr;
	UnitInfo *closestDefenseBuilding = nullptr;
	Unit closestUncompletedDefenseBuilding = nullptr;
	Unit closestBurrowLurker = nullptr;
	Unit closestUnBurrowLurker = nullptr;

	// closest 를 구하기 위해서 사용함. 실제 거리가 아니기 때문에 사용하지 말것!
	int closestUnitDist = INT_MAX;
	int closestBuildingDist = INT_MAX;
	int closestCombatUnitDist = INT_MAX;
	int closestHiddenCombatUnitDist = INT_MAX;
	int closestDefenseBuildingDist = INT_MAX;
	int closestUncompletedDefenseBuildingDist = INT_MAX;
	int closestBurrowLurkerDist = INT_MAX;
	int closestUnBurrowLurkerDist = INT_MAX;

	double enemySupply = 0;

	for (auto u : INFO.getUnitsInRadius(E, unit->getPosition(), 16 * TILE_SIZE, true, true, true, true)) {
		// 무적인 유닛은 제외. fog 안에 있으면 제외. 라바는 제외.
		if (u->type().isInvincible() || (u->isHide() && !((u->type() == Zerg_Lurker || u->type() == Terran_Vulture_Spider_Mine) && u->pos() != Positions::Unknown)) || u->type() == Zerg_Larva)
			continue;

		// 다른 area 에 있고 사정거리 밖이면 무시
		if (!isSameArea(u->pos(), unit->getPosition())) {
			if (getAttackDistance(unit, u->type(), u->pos()) > UnitUtil::GetAttackRange(unit, u->type().isFlyer()) + 3 * TILE_SIZE
					&& (!u->type().groundWeapon().targetsGround()
						|| getAttackDistance(u->type(), u->pos(), unit) > UnitUtil::GetAttackRange(u->type(), E, false) + 3 * TILE_SIZE))
				continue;
		}

		Position newPos = unit->getPosition() - u->pos();

		int dist = newPos.x * newPos.x + newPos.y * newPos.y;

		// closestUnit
		if (closestUnitDist > dist) {
			closestUnitDist = dist;
			closestUnit = u;
		}

		if ((u->type().groundWeapon().targetsGround() && !u->type().isWorker()) || u->type().isSpellcaster()) {
			if (getAttackDistance(u->type(), u->pos(), unit) <= UnitUtil::GetAttackRange(u->type(), E, false) + 4 * TILE_SIZE)
				enemySupply += u->type().supplyRequired() * (double)u->hp() / u->type().maxHitPoints();

			// closestGroundCombatUnit
			if (closestCombatUnitDist > dist && !u->isHide() && u->unit()->isDetected()) {
				closestCombatUnitDist = dist;
				closestCombatUnit = u->unit();
			}

			if (closestHiddenCombatUnitDist > dist && (u->isHide() || !u->unit()->isDetected())) {
				closestHiddenCombatUnitDist = dist;
				closestHiddenCombatUnit = u;
			}

			if (u->type() == Zerg_Lurker && closestBurrowLurkerDist > dist && !u->isHide() && u->unit()->isDetected() && u->isBurrowed()) {
				closestBurrowLurkerDist = dist;
				closestBurrowLurker = u->unit();
			}

			if (u->type() == Zerg_Lurker && closestUnBurrowLurkerDist > dist && !u->isHide() && u->unit()->isDetected() && !u->isBurrowed()) {
				closestUnBurrowLurkerDist = dist;
				closestUnBurrowLurker = u->unit();
			}
		}
	}

	for (auto u : INFO.getBuildingsInRadius(E, unit->getPosition(), 16 * TILE_SIZE, true, true, true)) {
		// 다른 area 에 있고 사정거리 밖이면 무시
		if (!isSameArea(u->pos(), unit->getPosition())) {
			if (getAttackDistance(unit, u->type(), u->pos()) > UnitUtil::GetAttackRange(unit, u->type().isFlyer()) + 3 * TILE_SIZE
					&& (!u->type().groundWeapon().targetsGround()
						|| getAttackDistance(u->type(), u->pos(), unit) > UnitUtil::GetAttackRange(u->type(), E, false) + 3 * TILE_SIZE))
				continue;
		}

		Position newPos = unit->getPosition() - u->pos();

		int dist = newPos.x * newPos.x + newPos.y * newPos.y;

		// closestBuilding (hp 가 100 보다 적은 성큰 변태중인 건물은 제외)
		if (closestBuildingDist > dist && !(u->type().groundWeapon().targetsGround() && (!u->isComplete() || u->isMorphing()) && u->hp() <= 100)) {
			closestBuildingDist = dist;
			closestBuilding = u->unit();
		}

		if (u->type().groundWeapon().targetsGround()) {
			// closestGroundCombatUnit
			if (u->isComplete() && !u->isMorphing()) {
				if (getAttackDistance(u->unit(), unit) <= UnitUtil::GetAttackRange(u->unit(), false) + 5 * TILE_SIZE)
					enemySupply += 4 * u->hp() / u->type().maxHitPoints();

				if (closestDefenseBuildingDist > dist) {
					closestDefenseBuildingDist = dist;
					closestDefenseBuilding = u;
				}
			}
			else if (u->hp() > 100) {
				if (closestUncompletedDefenseBuildingDist > dist) {
					closestUncompletedDefenseBuildingDist = dist;
					closestUncompletedDefenseBuilding = u->unit();
				}
			}
		}
	}

	// 탱크 앞으로 가도록 뒤로 빠져주기.
	if (GoliathManager::Instance().getWaitOrMoveOrAttack() == 2) {
		if (unit->isSelected())
			cout << "탱크 앞으로 가도록 뒤로 빠지자! " << endl;

		// 탱크보다 내가 앞에 있는 경우
		if (TM.frontTankOfNotDefenceTank != nullptr && getGroundDistance(TM.frontTankOfNotDefenceTank->pos(), SM.getMainAttackPosition())
				> getGroundDistance(unit->getPosition(), SM.getMainAttackPosition())) {
			if (closestDefenseBuilding) {
				int distance = getGroundDistance(unit->getPosition(), closestDefenseBuilding->pos());

				if (distance < Zerg_Sunken_Colony.groundWeapon().maxRange() + 7 * TILE_SIZE)
					CommandUtil::backMove(unit, closestUnit ? closestUnit->unit() : closestDefenseBuilding->unit(), true);
				else
					CommandUtil::goliathHold(unit, closestUnit ? closestUnit->unit() : closestBuilding);
			}
			else {
				CPPath path = theMap.GetPath(SM.getMainAttackPosition(), INFO.getMainBaseLocation(S)->Center());

				if (!path.empty()) {
					int distance = getGroundDistance(unit->getPosition(), (Position)path.at(0)->Center());

					if (distance < Zerg_Sunken_Colony.groundWeapon().maxRange() + 10 * TILE_SIZE)
						moveBackPostion(me, (Position)path.at(0)->Center(), 2 * TILE_SIZE);
					else
						CommandUtil::goliathHold(unit, closestUnit ? closestUnit->unit() : closestBuilding);
				}
			}
		}

		return nullptr;
	}

	// 유닛이 하나도 없고
	if (closestUnit == nullptr && closestBuilding == nullptr) {
		if (unit->isSelected())
			cout << "유닛이 없음 : " << targetPosition << endl;

		// 목적지에 도착한 경우 IDLE
		if (unit->getPosition().getApproxDistance(targetPosition) < 4 * TILE_SIZE)
			return new GoliathIdleState();
		// 목적지가 아닌 경우 attackMove
		else {
			// static building 이 내 근처에 있는 경우 부순다.
			for (auto &s : theMap.StaticBuildings()) {
				if (s->Unit()->exists() && s->Blocking() && unit->isInWeaponRange(s->Unit()) && INFO.getAltitudeDifference(s->Unit()->getTilePosition(), unit->getTilePosition()) == 0) {
					CommandUtil::attackUnit(unit, s->Unit());

					return nullptr;
				}
			}

			if (unit->isSelected())
				cout << " 목적지로 이동 : " << targetPosition << endl;

			CommandUtil::attackMove(unit, targetPosition);

			return nullptr;
		}
	}

	// 적 유닛에 비해 내 유닛이 너무 적으면
	double selfSupply = 0;

	for (auto u : INFO.getUnitsInRadius(S, unit->getPosition(), 5 * TILE_SIZE, true, false, false)) {
		if (u->unit()->canAttack() && !u->type().isBuilding() && u->type() != Terran_Vulture_Spider_Mine) {
			selfSupply += u->type().supplyRequired() * (u->hp() / (double)u->type().maxHitPoints());
		}
	}

	if (selfSupply <= 30 && selfSupply < enemySupply) {
		if (unit->isSelected())
			cout << selfSupply << " 밖에 없음." << enemySupply << endl;

		if (closestCombatUnit)
			CommandUtil::holdControll(unit, closestCombatUnit, getBackPostion(me, closestCombatUnit->getPosition(), 2 * TILE_SIZE, true));
		else
			moveBackPostion(me, closestUnit == nullptr ? closestBuilding->getPosition() : closestUnit->pos(), 2 * TILE_SIZE);

		return nullptr;
	}
	// 성큰이 너무 많으면 대기
	else if (GoliathManager::Instance().getWaitOrMoveOrAttack() == 1 && closestDefenseBuilding) {
		if (unit->isSelected())
			cout << "성큰 많으니까 일단 기다려볼까? " << endl;

		UnitInfo *sunken = closestDefenseBuilding;
		int distance = getAttackDistance(sunken->type(), sunken->pos(), unit);

		// 너무 가까우면 뒤로 빠진다.
		if (distance < Zerg_Sunken_Colony.groundWeapon().maxRange() + 16) {
			moveBackPostion(me, closestBuilding->getPosition(), 2 * TILE_SIZE);
			return nullptr;
		}
		// 적당한 위치에서 대기
		else if (distance < Zerg_Sunken_Colony.groundWeapon().maxRange() + 2 * TILE_SIZE) {
			CommandUtil::goliathHold(unit, closestUnit ? closestUnit->unit() : closestBuilding);
			return nullptr;
		}

		if (unit->isSelected())
			cout << "거리 " << distance << " ";
	}

	// 공격유닛이 방어타워, 안보이는 럴커와 함께 있는 경우
	// 공격중인 유닛 체력이 적은 경우 죽인다.
	// 공격중인 유닛 체력이 많은 경우 빠진다.
	if (closestCombatUnit != nullptr || closestHiddenCombatUnit != nullptr) {
		// Burrow한 럴커가 사거리 내에 있는 경우 일점사한다.
		if (closestBurrowLurker != nullptr && unit->isInWeaponRange(closestBurrowLurker)) {
			if (unit->isSelected())
				cout << "버로우 한 럴커가 사거리 내에 있는 경우 일점사 id : " << closestBurrowLurker->getID() << endl;

			CommandUtil::attackUnit(unit, closestBurrowLurker);

			return nullptr;
		}
		// 안보이는 럴커가 가장 가까이에 있는 경우.
		else if (closestHiddenCombatUnit != nullptr) {
			// 스캔이 없거나 있어도 우리 병력이 적은 경우
			if (ComsatStationManager::Instance().getAvailableScanCount() == 0
					|| INFO.getTypeUnitsInRadius(Terran_Goliath, S, closestHiddenCombatUnit->pos(), Zerg_Lurker.groundWeapon().maxRange() + 5 * TILE_SIZE).size() < 3) {
				int attackDIst = getAttackDistance(closestHiddenCombatUnit->type(), closestHiddenCombatUnit->pos(), unit);

				// 사거리 내에 있는 경우 사거리 밖으로 이동한다.
				if (attackDIst <= closestHiddenCombatUnit->type().groundWeapon().maxRange()) {
					if (unit->isSelected())
						cout << "안보이는 럴커 사거리 이내 도망가자~ " << endl;

					CommandUtil::backMove(unit, closestHiddenCombatUnit->unit());

					return nullptr;
				}
				else {
					// 2 Tile 정도 여유가 있으면 대기
					if (attackDIst <= closestHiddenCombatUnit->type().groundWeapon().maxRange() + 2 * TILE_SIZE) {
						if (unit->isSelected())
							cout << "안보이는 럴커 사거리 밖 가까이 가지말자~ " << endl;

						CommandUtil::goliathHold(unit, closestUnit ? closestUnit->unit() : closestBuilding);

						return nullptr;
					}
				}
			}
			// 스캔이 있고 골리앗이 4마리 이상인 경우
			else {
				if (unit->isSelected())
					cout << "스캔 있으니 가는거야! " << endl;

				CommandUtil::attackMove(unit, closestHiddenCombatUnit->pos());
				return nullptr;
			}
		}

		// UnBurrow한 럴커가 사거리 내에 있는 경우 일점사한다.
		if (closestUnBurrowLurker != nullptr && unit->isInWeaponRange(closestUnBurrowLurker)) {
			if (unit->isSelected())
				cout << "언버로우 한 럴커가 사거리 내에 있는 경우 일점사 id : " << closestUnBurrowLurker->getID();

			CommandUtil::attackUnit(unit, closestUnBurrowLurker);
		}
		else if (closestCombatUnit != nullptr) {
			int attackingUnitCnt = 0;

			int myUnitCnt = INFO.getUnitsInRadius(S, unit->getPosition(), 3 * TILE_SIZE, true, false).size();

			if (unit->isSelected())
				cout << "내 주변에 우리 유닛 : " << myUnitCnt << " ";

			// 주변에 우리편이 별로 없는 경우 (backmove 가능)
			if (myUnitCnt <= 3) {
				// 방어타워가 같이 있는 경우 범위 밖에서 싸운다.
				if (closestDefenseBuilding != nullptr && !closestDefenseBuilding->isHide() && closestDefenseBuilding->unit()->isInWeaponRange(unit)) {
					if (unit->isSelected())
						cout << "나를 타게팅하는 유닛이 성큰인 경우 성큰 사거리로부터 벗어나서 싸운다 " << endl;

					CommandUtil::backMove(unit, closestDefenseBuilding->unit());

					return nullptr;
				}
				else {
					for (auto enemy : me->getEnemiesTargetMe()) {
						// 나를 타게팅하고 있는 유닛이 가까이에 있는 경우
						if (enemy->isInWeaponRange(unit)) {
							if (++attackingUnitCnt > 2) {
								if (unit->isSelected())
									cout << "나를 공격하는 유닛이 너무 많아 도망가자~ " << endl;

								CommandUtil::backMove(unit, closestCombatUnit, true);

								return nullptr;
							}
						}
					}
				}
			}

			// 가장 가까운 유닛이 럴커인 경우 다가가서 일점사 한다.
			if (closestCombatUnit->getType() == Zerg_Lurker) {
				if (unit->isSelected())
					cout << "사거리 밖에 럴커가 있는 경우 일점사 " << closestCombatUnit->getID() << closestCombatUnit->getType();

				CommandUtil::attackUnit(unit, closestCombatUnit);
			}
			// 마인이 가까이 있는 경우 빠진다.
			else if (closestCombatUnit->getType() == Terran_Vulture_Spider_Mine) {
				if (unit->isSelected())
					cout << "마인이 있으면 빠진다. " << closestCombatUnit->getID() << closestCombatUnit->getType();

				CommandUtil::backMove(unit, closestCombatUnit);
			}
			else {
				if (unit->isSelected())
					cout << "사거리 내에 공격 유닛이 있는 경우 공격 " << closestCombatUnit->getID() << closestCombatUnit->getType();

				if (unit->getDistance(closestCombatUnit) < 2 * TILE_SIZE) {

					CommandUtil::holdControll(unit, closestCombatUnit, getBackPostion(me, closestCombatUnit->getPosition(), 2 * TILE_SIZE, true));
				}
				else
					CommandUtil::attackMove(unit, closestCombatUnit->getPosition());
			}
		}
		else {
			if (unit->isSelected())
				cout << "가만히?";
		}
	}
	// 공격 유닛이 없는 경우
	else {
		Unit closest;
		Position target = targetPosition;

		// 방어타워가 있으면 방워타워 공격
		if (closestDefenseBuilding != nullptr) {
			if (unit->isSelected())
				cout << "방어타워가 있으면 방워타워 공격 ";

			closest = closestDefenseBuilding->unit();

			if (!closestDefenseBuilding->isHide() && unit->isInWeaponRange(closestDefenseBuilding->unit())) {
				Position move = closestDefenseBuilding->pos() - unit->getPosition();
				move = move * TILE_SIZE / (abs(move.x) > abs(move.y) ? abs(move.x) : abs(move.y));

				int damage = getDamageAtPosition(unit->getPosition(), unit, INFO.getBuildingsInRadius(E, unit->getPosition(), 15 * TILE_SIZE, true, true, true));
				int damage2 = getDamageAtPosition(unit->getPosition() + move, unit, INFO.getBuildingsInRadius(E, unit->getPosition() + move, 15 * TILE_SIZE, true, true, true));

				if (damage >= damage2) {
					target = unit->getPosition() + move;
				}
				else {
					target = unit->getPosition();
				}

				if (unit->isSelected())
					cout << "데미지 (from, to) : " << damage << ", " << damage2;

			}
			else {
				target = closestDefenseBuilding->pos();
			}
		}
		// 그냥 건물 or 비공격 유닛만 있는 경우 홀드컨트롤로 건물로 다가감
		else {
			closest = closestUncompletedDefenseBuilding == nullptr ? closestUnit == nullptr || closestUnit->isHide() ? closestBuilding : closestUnit->unit() : closestUncompletedDefenseBuilding;

			if (closest->getPosition() != Positions::Unknown) {
				target = getMiddlePositionByDist(closest->getPosition(), unit->getPosition(), 3 * TILE_SIZE);

				if (unit->isSelected())
					cout << "그냥 건물 or 비공격 유닛만 있는 경우 홀드컨트롤로 건물로 다가감 " << closest->getType() << target;
			}
			else {
				if (unit->isSelected())
					cout << "여기서가만히?" << target;
			}
		}

		CommandUtil::holdControll(unit, closest, target, true);
	}

	if (unit->isSelected())
		cout << endl;

	return nullptr;
}

State *GoliathAttackState::action()
{
	Position targetPosition = m_targetPos;

	if (targetPosition == Positions::None) {
		targetPosition = SM.getMainAttackPosition();
	}

	if (targetPosition == Positions::Unknown)
	{
		if (unit->isSelected())
			cout << "targetPosition 이 잘못됨." << endl;

		return new GoliathIdleState();
	}

	return attack(unit, targetPosition);
}

State *GoliathMoveState::action()
{
	Position targetPosition = m_targetPos;

	if (targetPosition == Positions::Unknown)
	{
		return new GoliathIdleState();
	}

	if (unit->getPosition().getApproxDistance(targetPosition) > 4 * TILE_SIZE)
	{
		UnitInfo *e = INFO.getClosestUnit(E, unit->getPosition());

		if (e)
			CommandUtil::holdControll(unit, e->unit(), targetPosition);
		else
			CommandUtil::move(unit, targetPosition);
	}
	else
	{
		return new GoliathIdleState();
	}

	return nullptr;
}

State *GoliathFightState::action()
{
	if (S->hasResearched(TechTypes::Tank_Siege_Mode))
		return new GoliathIdleState();

	vector<UnitType> types = { Terran_Siege_Tank_Tank_Mode, Terran_Goliath };
	UnitInfo *myFrontUnit = INFO.getClosestTypeUnit(S, SM.getMainAttackPosition(), types, 0, false, true);

	// 그냥 20 타일을 봐바
	UnitInfo *closestTank = INFO.getClosestTypeUnit(E, myFrontUnit->pos(), Terran_Siege_Tank_Tank_Mode, 15 * TILE_SIZE, true);

	if (closestTank && SM.getNeedDefenceWithScv())
	{
		if (unit->getGroundWeaponCooldown() == 0)
		{
			if (closestTank->isHide())
				CommandUtil::attackMove(unit, closestTank->pos());
			else
				CommandUtil::attackUnit(unit, closestTank->unit());
		}
		else
		{
			if (unit->getDistance(closestTank->pos()) > TILE_SIZE)
				CommandUtil::move(unit, closestTank->pos());
		}

		return nullptr;
	}

	UnitInfo *closestMarine = INFO.getClosestTypeUnit(E, myFrontUnit->pos(), Terran_Marine, 15 * TILE_SIZE, true);

	if (closestMarine)
	{
		UnitInfo *me = INFO.getUnitInfo(unit, S);
		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(me->pos(), &dangerPoint, false);

		UnitInfo *marine = INFO.getClosestTypeUnit(E, me->pos(), Terran_Marine, 0, true);
		attackFirstkiting(me, marine, dangerPoint, 3 * TILE_SIZE);

		return nullptr;
	}

	Position waitPosition = Positions::None;

	if (SM.getNeedAdvanceWait())
	{
		waitPosition = getDirectionDistancePosition(INFO.getSecondChokePosition(S), INFO.getFirstWaitLinePosition(), 2 * TILE_SIZE);

		if (!bw->isWalkable((WalkPosition)waitPosition))
		{
			waitPosition = getDirectionDistancePosition(INFO.getSecondChokePosition(S), INFO.getFirstWaitLinePosition(), 2 * TILE_SIZE);
		}

		if (!bw->isWalkable((WalkPosition)waitPosition))
		{
			waitPosition = INFO.getSecondChokePosition(S);
		}
	}
	else
		waitPosition = (INFO.getSecondChokePosition(S) + INFO.getFirstExpansionLocation(S)->getPosition()) / 2;

	if (unit->getGroundWeaponCooldown() == 0)
		CommandUtil::attackMove(unit, waitPosition);
	else
	{
		if (unit->getPosition().getApproxDistance(waitPosition) > 3 * TILE_SIZE)
			CommandUtil::move(unit, waitPosition, true);
	}

	return nullptr;
}

State *GoliathDefenseState::action()
{
	uList enemyInMyYard = INFO.enemyInMyYard();
	UnitInfo *me = INFO.getUnitInfo(unit, S);

	Position targetPos = Positions::None;
	UnitInfo *targetUnit = nullptr;
	UnitInfo *targetUnitCantAttack = nullptr;
	Position targetPosCantAttack = Positions::None;
	int dist = INT_MAX;

	for (auto eu : enemyInMyYard)
	{
		if (unit->getPosition().getApproxDistance(eu->pos()) > dist)
			continue;

		Position shortPos;

		if (eu->type().isFlyer() && !isInMyArea(eu->pos()))
		{
			TilePosition targetTilePos1 = TilePositions::None;
			TilePosition targetTilePos2 = TilePositions::None;

			int tileRad = S->getUpgradeLevel(UpgradeTypes::Charon_Boosters) ? 8 : 5;

			targetTilePos1 = findNearestTileInMyArea(TilePosition(MYBASE), TilePosition(eu->pos()), tileRad);
			targetTilePos2 = findNearestTileInMyArea(INFO.getFirstExpansionLocation(S)->getTilePosition(), TilePosition(eu->pos()), tileRad);

			if (targetTilePos1 == TilePositions::None && targetTilePos2 == TilePositions::None)
				continue;
			else	if (targetTilePos1 == TilePositions::None)
				shortPos = (Position)targetTilePos2 + Position(16, 16);
			else if (targetTilePos2 == TilePositions::None)
				shortPos = (Position)targetTilePos1 + Position(16, 16);
			else {
				shortPos = me->pos().getApproxDistance((Position)targetTilePos2) < me->pos().getApproxDistance((Position)targetTilePos1) ?
						   (Position)targetTilePos2 : (Position)targetTilePos1;
				shortPos += Position(16, 16);
			}
		}
		else
		{
			shortPos = eu->pos();
		}

		if (!eu->type().groundWeapon().targetsGround()) // 공격 불가 유닛
		{
			targetUnitCantAttack = eu;
			targetPosCantAttack = shortPos;
			continue;
		}

		int gDist = getGroundDistance(me->pos(), shortPos);

		if (gDist == -1)
			continue;

		if (gDist > 5 * TILE_SIZE)
		{
			if (eu->type().isFlyer())
			{
				if (S->getUpgradeLevel(UpgradeTypes::Charon_Boosters)) // 사업되면 공중 유닛은 3 타일 더 가깝다.
					gDist -= 3 * TILE_SIZE;
			}
			else
			{
				gDist -= TILE_SIZE;
			}
		}

		if (gDist < dist) {
			dist = gDist;
			targetPos = shortPos;
			targetUnit = eu;
		}
	}

	if (!targetUnit) // 적이 없음.
	{
		if (targetUnitCantAttack) // 공격 불가 유닛.
		{
			CommandUtil::holdControll(unit, targetUnitCantAttack->unit(), targetPosCantAttack);
			return nullptr;
		}

		Position waitPos;
		UnitInfo *closestTurret = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Missile_Turret, 15 * TILE_SIZE, false, true);

		if (closestTurret != nullptr)
			waitPos = closestTurret->pos();
		else
			waitPos = INFO.getSecondAverageChokePosition(S);

		if (me->pos().getApproxDistance(waitPos) > 3 * TILE_SIZE)
			CommandUtil::move(unit, waitPos, true);

		return nullptr;
	}

	bw->drawCircleMap(targetPos, 10, Colors::Blue, true);
	bw->drawCircleMap(targetUnit->pos(), 10, Colors::Red, true);

	//////////////////Target Pos 설정 완료
	if (targetUnit->type().isFlyer())
	{
		if (unit->getDistance(targetUnit->pos()) <= UnitUtil::GetAttackRange(unit, true) - 2 * TILE_SIZE)
		{
			UnitInfo *closestTurret = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Missile_Turret, 15 * TILE_SIZE, false, true);

			if (closestTurret == nullptr)
				targetPos = getBackPostion(me, targetUnit->pos(), 3 * TILE_SIZE);
			else
				targetPos = closestTurret->pos();
		}
	}

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(me->pos(), &dangerPoint, false);

	if (targetUnit->isBurrowed()) // 러커
	{
		if (!targetUnit->isHide() && targetUnit->unit()->isDetected())
		{
			CommandUtil::attackUnit(unit, targetUnit->unit());
		}
		else // 안보이거나 Detection이 안된 상황
		{
			if (INFO.getAvailableScanCount() &&
					INFO.getCompletedCount(Terran_Goliath, S) + INFO.getCompletedCount(Terran_Vulture, S) + INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 3)
			{
				CommandUtil::attackMove(unit, targetPos);
			}
			else
			{
				if (dangerPoint > 3 * TILE_SIZE)
				{
					CommandUtil::attackMove(unit, targetPos);
				}
				else
					moveBackPostion(me, targetPos, 2 * TILE_SIZE);
			}
		}

		return nullptr;
	}

	//럴커 아님
	if (targetUnit->isHide()) // 시야에서 사라짐
	{
		Position waitPos;
		UnitInfo *closestTurret = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Missile_Turret, 15 * TILE_SIZE, false, true);

		if (closestTurret != nullptr)
			waitPos = closestTurret->pos();
		else
			waitPos = INFO.getNearestBaseLocation(targetPos, true)->Center();

		if (me->pos().getApproxDistance(waitPos) > 3 * TILE_SIZE)
			CommandUtil::move(unit, waitPos, true);
	}
	else
	{
		if (targetUnit->type().isFlyer() && (dangerUnit == nullptr || dangerUnit->type().isFlyer()))
		{
			CommandUtil::holdControll(unit, targetUnit->unit(), targetPos);
			return nullptr;
		}

		if (targetUnit->unit()->isDetected())
		{
			if (unit->getGroundWeaponCooldown() == 0)
				CommandUtil::attackUnit(unit, targetUnit->unit());
			else if (dangerPoint < 3 * TILE_SIZE)
				moveBackPostion(me, targetPos, 2 * TILE_SIZE);
		}
		else // 다템한테 안전하게
		{
			kiting(me, targetUnit, dangerPoint, 4 * TILE_SIZE);
		}
	}

	return nullptr;
}

State *GoliathProtossAttackState::action()
{
	int cnt = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, unit->getPosition(), 50 * TILE_SIZE).size() + INFO.getTypeUnitsInRadius(Protoss_Interceptor, E, unit->getPosition(), 50 * TILE_SIZE).size();

	if (cnt != 0)
		return new GoliathCarrierAttackState();

	if (checkZeroAltitueAroundMe(unit->getPosition(), 6)) {
		// 길막하고 있는 상태라면 ?
		// 1. 공격 유닛이 주변에 없다면 적진으로 이동
		if (INFO.getUnitsInRadius(E, unit->getPosition(), 8 * TILE_SIZE, true, false, false).size() == 0) {
			CommandUtil::move(unit, SM.getMainAttackPosition());
			return nullptr;
		}
	}

	// 주변에 탱크가 있는데, 캐논이 보이면 홀드
	uList defenceBuildings = INFO.getDefenceBuildingsInRadius(E, unit->getPosition(), 12 * TILE_SIZE, false);

	if (!defenceBuildings.empty())
	{
		int tankCnt = INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S);

		if (tankCnt > 0) {
			// case 1. 주변에 탱크 있으면 대기
			CommandUtil::hold(unit);

			if (unit->isSelected())
				cout << "주변에 탱크가 있는데, 캐논이 보이면 홀드" << endl;

		}
		else {
			// case 2. 탱크는 없지만 우리가 압도적으로 많으면 어택
			uList goli = INFO.getTypeUnitsInRadius(Terran_Goliath, S, unit->getPosition(), 8 * TILE_SIZE);
			uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 12 * TILE_SIZE, true, true, false, true);

			if (goli.size() >= defenceBuildings.size() * 4 + eList.size()) {
				if (unit->isSelected())
					cout << "case 2. 탱크는 없지만 우리가 압도적으로 많으면 어택" << endl;

				CommandUtil::attackMove(unit, GoliathManager::Instance().getDefaultMovePosition());
			}
		}

		return nullptr;
	}

	UnitInfo *closestArbiter = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Arbiter, 8 * TILE_SIZE);
	UnitInfo *closestTank = TM.frontTankOfNotDefenceTank;

	int our_gol = INFO.getTypeUnitsInRadius(Terran_Goliath, S, unit->getPosition(), 10 * TILE_SIZE).size();

	int dangerPoint = 0;

	UnitInfo *dangerUnit = getDangerUnitNPoint(unit->getPosition(), &dangerPoint, false);

	if (closestArbiter != nullptr) {
		CommandUtil::attackUnit(unit, closestArbiter->unit());
		return nullptr;
	}

	if (dangerUnit == nullptr)
	{
		if (closestTank != nullptr)
		{
			if (unit->getPosition().getApproxDistance(closestTank->pos()) < 8 * TILE_SIZE)
				CommandUtil::attackMove(unit, SM.getMainAttackPosition());
			else if (unit->getPosition().getApproxDistance(closestTank->pos()) > 5 * TILE_SIZE) {
				if (unit->getGroundWeaponCooldown() == 0)
					CommandUtil::attackMove(unit, closestTank->pos());
				else
					CommandUtil::move(unit, closestTank->pos());
			}
		} else
			CommandUtil::attackMove(unit, SM.getMainAttackPosition());
	}
	else
	{
		// Photon Cannon
		if (dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
		{
			int photo = INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, dangerUnit->pos(), 8 * TILE_SIZE, false, true).size();

			if (photo > 5 || photo > our_gol * 2)
			{
				if (dangerPoint < 3 * TILE_SIZE)
				{
					CommandUtil::move(unit, INFO.getFirstChokePosition(S));
				}
				else if (dangerPoint < 5 * TILE_SIZE)
				{
					CommandUtil::hold(unit);
				}
				else
				{
					CommandUtil::attackMove(unit, SM.getMainAttackPosition());
				}
			}
			else
			{
				CommandUtil::attackMove(unit, SM.getMainAttackPosition());
			}
		}
		else
		{
			if (closestTank != nullptr)
			{
				if (unit->getPosition().getApproxDistance(closestTank->pos()) < 2 * TILE_SIZE)
					CommandUtil::attackMove(unit, SM.getMainAttackPosition());
				else if (unit->getPosition().getApproxDistance(closestTank->pos()) > 5 * TILE_SIZE) {
					if (unit->getGroundWeaponCooldown() == 0)
						CommandUtil::attackMove(unit, closestTank->pos());
					else
						CommandUtil::move(unit, closestTank->pos());
				}
			}
			else
			{
				CommandUtil::attackMove(unit, SM.getMainAttackPosition());
			}
		}
	}

	return nullptr;
}

State *GoliathCarrierAttackState::action()
{
	uList carrier = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, unit->getPosition(), 50 * TILE_SIZE, false);
	uList interceptor = INFO.getTypeUnitsInRadius(Protoss_Interceptor, E, unit->getPosition(), 50 * TILE_SIZE, false);

	if (carrier.empty() && interceptor.empty())
		return new GoliathProtossAttackState();

	Position targetPosition = GoliathManager::Instance().getDefaultMovePosition();

	// 캐논이 있는 경우
	uList defenceBuildings = INFO.getDefenceBuildingsInRadius(E, unit->getPosition(), 11 * TILE_SIZE, false);

	if (!defenceBuildings.empty())
	{
		int tankCnt = INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S);

		if (tankCnt > 0) {
			CommandUtil::hold(unit);

			if (unit->isSelected()) cout << "주변에 탱크가 있는데, 캐논이 보이면 홀드" << endl;

			return nullptr;

		}
		else {

			// case 2. 탱크는 없지만 우리가 압도적으로 많으면 어택
			uList goli = INFO.getTypeUnitsInRadius(Terran_Goliath, S, unit->getPosition(), 8 * TILE_SIZE);
			uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 12 * TILE_SIZE, true, true, false, true);

			if (goli.size() >= defenceBuildings.size() * 4 + eList.size()) {
				if (unit->isSelected()) cout << "case 2. 탱크는 없지만 우리가 압도적으로 많으면 어택" << endl;

				CommandUtil::attackMove(unit, GoliathManager::Instance().getDefaultMovePosition());
			}
			else {
				if (unit->isSelected()) cout << "case 3. 탱크도 없고 병력도 없어서 뒤로 뺌" << endl;

				if (unit->getGroundWeaponCooldown() == 0) {
					CommandUtil::attackMove(unit, MYBASE);
				}
				else
					CommandUtil::move(unit, MYBASE);

			}

		}

		return nullptr;
	}

	int airAttackRange = UnitUtil::GetAttackRange(unit, true);
	int groundAttackRange = UnitUtil::GetAttackRange(unit, false);
	uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), groundAttackRange + 5 * TILE_SIZE, true, false, false);
	uList tanks = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, unit->getPosition(), 12 * TILE_SIZE);
	UnitInfo *closestTank = TM.frontTankOfNotDefenceTank;

	// 지상군이 꽤 있다고 한다면,
	if (!eList.empty() && eList.size() >= tanks.size()) {


		if (closestTank) {
			if (unit->isSelected())
				cout << "지상군이 꽤 있어서 몸 사림.." << endl;

			if (unit->getGroundWeaponCooldown() == 0) {
				if (!attackCarrierOrInterceptors(unit)) {
					CommandUtil::attackMove(unit, closestTank->pos());
				}
			}
			else
				CommandUtil::move(unit, closestTank->pos());

			return nullptr;

		}
	}

	// 탱크와의 거리
	if (closestTank) {
		if (getGroundDistance(closestTank->pos(), unit->getPosition()) > 8 * TILE_SIZE) {
			if (unit->isSelected())
				cout << "탱크랑 너무 멀어서 공격" << endl;

			if (unit->getGroundWeaponCooldown() == 0) {
				if (!attackCarrierOrInterceptors(unit)) {
					CommandUtil::attackMove(unit, closestTank->pos());
				}
			}
			else
				CommandUtil::move(unit, closestTank->pos());

			return nullptr;
		}
	}

	// case 1. 캐리어 본체 공격
	int minHP = INT_MAX;
	Unit targetCarrier = nullptr;
	uList goliath = INFO.getTypeUnitsInRadius(Terran_Goliath, S, unit->getPosition(), 5 * TILE_SIZE, true);

	for (auto c : carrier) {
		if (minHP > c->hp() && unit->isInWeaponRange(c->unit())) {
			minHP = c->hp();
			targetCarrier = c->unit();
		}
	}

	word intercepterCnt = 0;

	if (targetCarrier)
		intercepterCnt += INFO.getTypeUnitsInRadius(Protoss_Interceptor, E, targetCarrier->getPosition(), 12 * TILE_SIZE, true).size();

	if (targetCarrier && (INFO.getUnitInfo(targetCarrier, E)->hp() <= 100 || intercepterCnt <= 4)) {
		if (unit->isSelected()) cout << "캐리어 본체 공격!!" << endl;

		bw->drawCircleMap(targetCarrier->getPosition(), 5, Colors::Red, true);
		CommandUtil::attackUnit(unit, targetCarrier);
	}
	else {

		UnitInfo *closeCarrier = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Carrier, 50 * TILE_SIZE, false, false);

		if (!closeCarrier) {

			if (unit->isSelected()) cout << "주변 캐리어 없다.." << endl;

			UnitInfo *intercepter = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Interceptor, 50 * TILE_SIZE, false, false);

			if (intercepter && intercepter->pos() != Positions::Unknown) {
				CommandUtil::attackMove(unit, intercepter->pos());
				return nullptr;
			}

			CommandUtil::attackMove(unit, targetPosition);
			return nullptr;
		}

		// 캐리어와 거리가 먼 경우.
		if (closeCarrier->pos().getApproxDistance(unit->getPosition()) >= 10 * TILE_SIZE) {
			if (unit->isSelected()) cout << "캐리어와 거리가 멀다 가까이가자" << endl;

			if (!attackCarrierOrInterceptors(unit)) { //인터셉터 Or 약한 캐리어 공격
				// Walkable 하지 않은 위치의 캐리어는 멀더라도 AttackMove 하지 않고 AttackUnit 해보자.
				if (bw->isWalkable((WalkPosition)closeCarrier->pos()))
					CommandUtil::attackUnit(unit, closeCarrier->unit());
				else
					CommandUtil::attackMove(unit, closeCarrier->pos());
			}

		}
		else {
			if (unit->isSelected()) cout << "캐리어와 거리는 가깝다" << endl;

			if (!attackCarrierOrInterceptors(unit)) {
				//// 캐리어와 근접만..
				if (unit->getGroundWeaponCooldown() == 0) {
					if (!attackCarrierOrInterceptors(unit)) { //인터셉터 공격
						// 홀드하면 공격을 자꾸 안해서 자기 자리 어택땅
						CommandUtil::attackMove(unit, unit->getPosition());
					}
				}
				else
					moveBackPostion(INFO.getUnitInfo(unit, S), closeCarrier->pos(), 3 * TILE_SIZE);
			}

		}

	}

	return nullptr;
}

State *GoliathCarrierDefenceState::action()
{
	if (unit->getPosition().getApproxDistance(MYBASE) > 30 * TILE_SIZE) {
		if (unit->getGroundWeaponCooldown() == 0)
			CommandUtil::attackMove(unit, MYBASE);
		else
			CommandUtil::move(unit, MYBASE);

		return nullptr;

	}

	uList carriers = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, unit->getPosition(), 30 * TILE_SIZE, true);
	int goliCnt = INFO.getTypeUnitsInRadius(Terran_Goliath, S, unit->getPosition(), 10 * TILE_SIZE, true).size();
	int intercepterCnt = INFO.getTypeUnitsInRadius(Protoss_Interceptor, E, unit->getPosition(), 10 * TILE_SIZE, false).size();

	// case 1. 캐리어 본체 공격
	int minHP = INT_MAX;
	Unit targetCarrier = nullptr;
	int carrierCnt = carriers.size();

	for (auto c : carriers) {
		if (minHP > c->hp() && unit->isInWeaponRange(c->unit())) {
			minHP = c->hp();
			targetCarrier = c->unit();
		}
	}

	if (unit->isSelected()) cout << "주변 intercepterCnt = " << intercepterCnt << endl;

	if (targetCarrier && (INFO.getUnitInfo(targetCarrier, E)->hp() <= 150 || intercepterCnt == 0)) {
		if (unit->isSelected()) cout << "캐리어 본체 공격!!" << endl;

		CommandUtil::attackUnit(unit, targetCarrier);
	}
	else {

		UnitInfo *closeCarrier
			= INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Carrier, 50 * TILE_SIZE, false, false);

		if (!closeCarrier) {

			if (unit->isSelected()) cout << "주변 캐리어 없다.." << endl;

			UnitInfo *intercepter = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Interceptor, 50 * TILE_SIZE, false, false);

			if (intercepter && intercepter->pos() != Positions::Unknown) {
				CommandUtil::attackMove(unit, intercepter->pos());
				return nullptr;
			}

			CommandUtil::attackMove(unit, MYBASE);
			return nullptr;
		}

		// 캐리어와 거리가 먼 경우.
		if (closeCarrier->pos().getApproxDistance(unit->getPosition()) >= 12 * TILE_SIZE) {
			if (unit->isSelected()) cout << "캐리어와 거리가 멀다 가까이가자" << endl;

			if (!attackCarrierOrInterceptors(unit)) {
				if (bw->isWalkable((WalkPosition)closeCarrier->pos()))
					CommandUtil::attackUnit(unit, closeCarrier->unit());
				else
					CommandUtil::attackMove(unit, closeCarrier->pos());
			}
		}
		else {

			int intercepterCnt = INFO.getTypeUnitsInRadius(Protoss_Interceptor, E, closeCarrier->pos(), 12 * TILE_SIZE, true).size();

			// 캐리어와 거리가 가까운 경우.
			if (intercepterCnt < goliCnt * 3) {
				//CommandUtil::attackMove(unit, closeCarrier->pos());

				if (unit->isSelected()) cout << "골리앗 많으니 계속 공격 하자" << endl;

				// 골리앗 숫자가 충분하니 어택 무브
				if (unit->getAirWeaponCooldown() == 0) {
					if (unit->isInWeaponRange(closeCarrier->unit()))
						CommandUtil::attackUnit(unit, closeCarrier->unit());
					else
						CommandUtil::attackMove(unit, closeCarrier->pos());
				}
				else {
					if (bw->isWalkable((WalkPosition)closeCarrier->pos()))
						CommandUtil::attackUnit(unit, closeCarrier->unit());
					else
						CommandUtil::move(unit, closeCarrier->pos());
				}
			}
			else {
				if (unit->isSelected()) cout << " 골리앗 없다 moveBack 하자" << endl;

				//// 캐리어와 근접만..
				if (unit->getAirWeaponCooldown() == 0)
					CommandUtil::attackMove(unit, unit->getPosition());
				else
					moveBackPostion(INFO.getUnitInfo(unit, S), closeCarrier->pos(), 8 * TILE_SIZE);

			}
		}

	}

	return nullptr;
}

State *GoliathKeepMultiState::action()
{
	UnitInfo *gInfo = INFO.getUnitInfo(unit, S);

	if (INFO.enemyRace != Races::Terran) // 비 테란전일 때만
	{
		if (SM.getNeedKeepSecondExpansion(gInfo->type()))
			m_targetPos = INFO.getSecondExpansionLocation(S)->Center();
		else if (SM.getNeedKeepThirdExpansion(gInfo->type()))
			m_targetPos = INFO.getThirdExpansionLocation(S)->Center();

		UnitInfo *closestUnit = INFO.getClosestUnit(E, gInfo->pos(), AllKind, 12 * TILE_SIZE);

		if (closestUnit) {
			if (isNeedKitingUnitType(closestUnit->type()))
				attackFirstkiting(gInfo, closestUnit, gInfo->pos().getApproxDistance(closestUnit->pos()), 4 * TILE_SIZE);
			else
				CommandUtil::attackMove(unit, closestUnit->pos());
		}
		else
			CommandUtil::attackMove(unit, m_targetPos);

		return nullptr;
	}

	if (checkFlag == false)
	{
		if (INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, m_targetPos, 5 * TILE_SIZE).size() > 0 || gInfo->pos().getApproxDistance(m_targetPos) < 2 * TILE_SIZE)
		{
			checkFlag = true;
			m_targetPos = getDirectionDistancePosition(m_targetPos, theMap.Center(), 7 * TILE_SIZE);
		}
	}

	if (checkFlag)
	{
		UnitInfo *closestUnit = INFO.getClosestUnit(E, basePos, AllKind, 12 * TILE_SIZE);

		if (closestUnit == nullptr)
		{
			if (gInfo->pos().getApproxDistance(m_targetPos) > 2 * TILE_SIZE)
				CommandUtil::attackMove(unit, m_targetPos);
		}
		else
			CommandUtil::attackUnit(unit, closestUnit->unit());
	}
	else
	{
		int num;

		if (SM.getNeedSecondExpansion() && INFO.getSecondExpansionLocation(S)->Center().getApproxDistance(m_targetPos) < 10 * TILE_SIZE) {
			num = 1;
		}
		else //if (SM.getNeedThirdExpansion() && INFO.getThirdExpansionLocation(S)->Center().getApproxDistance(m_targetPos) < 10 * TILE_SIZE)
		{
			num = 2;
		}

		if (unit->getDistance(m_targetPos) < 10 * TILE_SIZE)
		{
			CommandUtil::attackMove(unit, m_targetPos, true);

			if (unit->getDistance(m_targetPos) < 3 * TILE_SIZE)
			{
				CommandUtil::attackMove(unit, getDirectionDistancePosition(m_targetPos, theMap.Center(), 5 * TILE_SIZE), true);
			}
		}
		else
		{
			UnitInfo *closestKeepMultiTank = getClosestUnit(gInfo->pos(), TM.getKeepMultiTanks(num));

			if (closestKeepMultiTank)
			{
				if (closestKeepMultiTank->pos().getApproxDistance(gInfo->pos()) > 5 * TILE_SIZE)
					CommandUtil::move(unit, closestKeepMultiTank->pos());

				if (closestKeepMultiTank->pos().getApproxDistance(gInfo->pos()) < 3 * TILE_SIZE)
					CommandUtil::attackMove(unit, m_targetPos, true);
			}
			else
				CommandUtil::attackMove(unit, MYBASE, true);
		}
	}

	return nullptr;
}

State *GoliathMultiBreakState::action()
{
	CommandUtil::attackMove(unit, SM.getSecondAttackPosition());

	return nullptr;
}

State *GoliathDropshipState::action()
{
	// 타고 있으면 스킵
	if (unit->isLoaded())
	{
		firstBoard = true;
		return nullptr;
	}

	if (firstBoard == false && (!m_targetUnit->exists() || m_targetUnit->getSpaceRemaining() < Terran_Goliath.spaceRequired() || INFO.getUnitInfo(m_targetUnit, S)->getState() == "Go")) // 타지 못한 상태에서 dropship 격추
		return new GoliathIdleState();

	// 한번도 탄적이 없어
	if (firstBoard == false)
	{
		if (isInMyArea(unit->getPosition()))
			CommandUtil::rightClick(unit, m_targetUnit);
		else
			CommandUtil::move(unit, MYBASE);

		return nullptr;
	}

	// 여기부터 공격
	if (targetPosition == Positions::Unknown || INFO.getTypeBuildingsInRadius(Terran_Command_Center, E, targetPosition, 8 * TILE_SIZE).size() == 0)
	{
		if (timeToClear == 0)
			timeToClear = TIME;

		if (timeToClear + 24 * 60 < TIME)
		{
			timeToClear = 0;

			Base *nearestBase = nullptr;
			int dist = INT_MAX;

			for (auto base : INFO.getOccupiedBaseLocations(E))
			{
				if (INFO.getTypeBuildingsInRadius(Terran_Command_Center, E, base->Center(), 8 * TILE_SIZE).size() == 0)
					continue;

				int tmp = getGroundDistance(unit->getPosition(), base->Center());

				if (tmp >= 0 && dist > tmp)
				{
					dist = tmp;
					nearestBase = (Base *)base;
				}
			}

			if (nearestBase)
				targetPosition = nearestBase->Center();
			else
				targetPosition = SM.getMainAttackPosition();
		}
	}

	// 목적지에 왔음.
	//if (isSameArea(SM.getMainAttackPosition(), unit->getPosition()))
	UnitInfo *closestTank = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Siege_Tank_Tank_Mode, 20 * TILE_SIZE);

	if (closestTank != nullptr && !closestTank->unit()->isLoaded())
	{
		vector<UnitType> types = { Terran_Wraith, Terran_Battlecruiser };
		UnitInfo *closestWraith = INFO.getClosestTypeUnit(E, closestTank->pos(), types, 12 * TILE_SIZE, true, false, true);

		if (closestWraith != nullptr)// 상대방 Wraith가 있으면 탱크 보호
		{
			if (unit->getPosition().getApproxDistance(closestTank->pos()) < 2 * TILE_SIZE)
				CommandUtil::hold(unit);
			else
				CommandUtil::move(unit, closestTank->pos());
		}
		else //탱크는 있는데 Wraith가 없는 경우
		{
			if (closestTank->pos().getApproxDistance(unit->getPosition()) < 6 * TILE_SIZE)
			{
				CommandUtil::attackMove(unit, targetPosition);
			}
			else
			{
				CommandUtil::move(unit, closestTank->pos());
			}
		}
	}
	else //탱크가 없으면 그냥 공격진행
	{
		// 일꾼 있으면 먼저 공격
		UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, unit->getPosition(), INFO.getWorkerType(INFO.enemyRace), 5 * TILE_SIZE);

		if (closestWorker)
			CommandUtil::attackUnit(unit, closestWorker->unit());
		else
			CommandUtil::attackMove(unit, targetPosition);
	}

	return nullptr;
}

State *GoliathProtectTankState::action()
{
	UnitInfo *closestTank = 	TM.getFrontTankFromPos(unit->getPosition());
	UnitInfo *me = INFO.getUnitInfo(unit, S);

	if (closestTank != nullptr)
	{
		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(me->pos(), &dangerPoint, false);

		if (dangerUnit != nullptr && dangerUnit->type() == Terran_Siege_Tank_Siege_Mode && dangerPoint < 4 * TILE_SIZE)
		{
			moveBackPostion(me, dangerUnit->pos(), 3 * TILE_SIZE);
			return nullptr;
		}

		Position p = closestTank->pos();
		vector<UnitType> types = { Terran_Wraith, Terran_Battlecruiser };
		UnitInfo *closestWraith = INFO.getClosestTypeUnit(E, p, types, 12 * TILE_SIZE, true, false, false);

		if (closestWraith != nullptr)
		{
			if (unit->getPosition().getApproxDistance(p) < 10 * TILE_SIZE)
			{
				CommandUtil::attackUnit(unit, closestWraith->unit());
			}
			else
			{
				CommandUtil::move(unit, p);
			}

			return nullptr;
		}
		else
		{
			if (unit->getPosition().getApproxDistance(p) < 2 * TILE_SIZE)
				CommandUtil::hold(unit);
			else
				CommandUtil::move(unit, p);

			return nullptr;
		}
	}
	else
	{
		return new GoliathIdleState();
	}

	return nullptr;
}

bool State::attackCarrierOrInterceptors(Unit u) {

	int maxRange = S->getUpgradeLevel(UpgradeTypes::Charon_Boosters) > 0 ? 7 * TILE_SIZE : 5 * TILE_SIZE; // 보수적으로 가져오자
	uList carriers = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, u->getPosition(), maxRange);
	uList interceptors = INFO.getTypeUnitsInRadius(Protoss_Interceptor, E, u->getPosition(), maxRange);
	int minHP = INT_MAX;
	UnitInfo *weakUnit = nullptr;

	for (auto c : carriers) {
		if (c->hp() < 100 && u->isInWeaponRange(c->unit())) {
			weakUnit = c;
		}
	}

	if (weakUnit) {
		if (unit->isSelected()) cout << "캐리어 공격 !!" << endl;

		CommandUtil::attackUnit(u, weakUnit->unit());
		return true;
	}

	minHP = INT_MAX;
	weakUnit = nullptr;

	for (auto i : interceptors) {
		if (i->hp() < minHP && u->isInWeaponRange(i->unit())) {
			minHP = i->hp();
			weakUnit = i;
		}
	}

	if (weakUnit) {
		if (unit->isSelected()) cout << "인터셉터 공격 !!" << endl;

		CommandUtil::attackUnit(u, weakUnit->unit());
		return true;
	}

	return false;
}

