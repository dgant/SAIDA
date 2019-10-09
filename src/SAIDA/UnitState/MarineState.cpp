#include "MarineState.h"
using namespace MyBot;

State *MarineIdleState::action()
{
	Unit b = MM.getBunker();

	// 내가 타고 있는 벙커가 getBunker가 아닌 경우 나온다.
	if (b != nullptr && unit->isLoaded() && b->isCompleted() && unit->getTransport() != b)
	{
		unit->getTransport()->unload(unit);

		return nullptr;
	}

	if (b != nullptr && b->getLoadedUnits().size() != 4) {

		if (b->isCompleted())
			unit->rightClick(b);
		else
			CommandUtil::attackMove(unit, b->getPosition(), true);

		return nullptr;
	}


	if (INFO.enemyRace == Races::Zerg) {
		uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

		if (commandCenterList.empty())
			return nullptr;

		if (commandCenterList.size() == 1)
		{
			// 9드론 이하 빌드일 때 마린들 커맨드 아래에 대기
			if (ESM.getEnemyInitialBuild() <= Zerg_12_Ap || ESM.getEnemyInitialBuild() == UnknownBuild)
			{
				Unit mineral = bw->getClosestUnit(commandCenterList[0]->pos(), Filter::IsMineralField);

				if (mineral != nullptr) {
					Position target = (commandCenterList[0]->pos() + mineral->getPosition()) / 2;
					unit->move(target);
					return nullptr;
				}
			}
		}
	}
	else if (ESM.getEnemyInitialBuild() == Toss_cannon_rush && INFO.getSecondChokePosition(S)) {
		if (INFO.getFirstChokePosition(S).getApproxDistance(unit->getPosition()) - INFO.getFirstChokePosition(S).getApproxDistance(INFO.getSecondChokePosition(S)) > 3 * TILE_SIZE)
			CommandUtil::move(unit, INFO.getSecondChokePosition(S), true);
		else {
			uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 7 * TILE_SIZE);

			if (eList.empty()) {
				uList eBuildingList = INFO.getBuildingsInRadius(E, unit->getPosition(), 7 * TILE_SIZE);

				if (eBuildingList.empty())
					CommandUtil::attackMove(unit, INFO.getSecondChokePosition(S));
				else
					CommandUtil::attackUnit(unit, eBuildingList.at(0)->unit());
			}
			else
				return new MarineAttackState(eList.at(0));
		}

		return nullptr;
	}

	if (unit->getDistance(MM.waitingNearCommand) > 50) {
		//unit->move(pos);
		CommandUtil::move(unit, MM.waitingNearCommand, true);
	}

	return nullptr;
}

State *MarineAttackState::action() {
	if (m_targetUnit != nullptr) {
		UnitInfo *eUnit = INFO.getUnitInfo(m_targetUnit, E);

		// 상대봇이 죽은 경우 본진, 앞마당의 안보이는 시야를 탐색한다.
		if (!eUnit) {
			// 근처 유닛이 있으면 변경
			uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 8 * TILE_SIZE);

			if (!eList.empty()) {
				m_targetUnit = eList.at(0)->unit();
				m_targetPos = m_targetUnit->getPosition();
			}
			// 근처 유닛이 없으면 MainAttackPosition 으로 변경
			else if (eList.empty() && (EIB == Toss_1g_forward || EIB == Toss_2g_forward)) {
				m_targetUnit = nullptr;
				m_targetPos = SM.getMainAttackPosition();
			}
			else
				return new MarineIdleState();

			return nullptr;
		}

		// target 이 없어졌으면 idle 변경.
		if (eUnit->pos() == Positions::Unknown) {
			if (unit->isMoving())
				return nullptr;

			return new MarineIdleState();
		}

		m_targetPos = eUnit->pos();
	}

	uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 10 * TILE_SIZE);

	// 주변에 아무 유닛도 없으면 일단 목적지로 간다.
	if (eList.empty()) {
		if (unit->getPosition().getApproxDistance(m_targetPos) < 2 * TILE_SIZE && m_targetUnit == nullptr)
			return new MarineIdleState();

		if (unit->isLoaded()) {
			Unit bunker = MM.getBunker();
			bunker->unload(unit);

			return nullptr;
		}
	}

	int minHP = INT_MAX;
	int minDist = INT_MAX;
	Unit target = nullptr;
	Unit closest = nullptr;
	Unit lowestHP = nullptr;

	for (auto e : eList) {
		// 나를 공격할 수 있는 유닛
		if (e->type().groundWeapon().targetsGround()) {
			int gap;
			int enemyRange = UnitUtil::GetAttackRange(e->unit(), unit);
			int selfRange = UnitUtil::GetAttackRange(unit, e->unit());
			int dist = getAttackDistance(e->unit(), unit);

			// 나를 타게팅 한 적인 경우
			if (e->unit()->getOrderTarget() != nullptr && e->unit()->getOrderTarget() == unit
					|| e->unit()->getTarget() != nullptr && e->unit()->getTarget() == unit) {
				if (enemyRange >= selfRange)
					gap = -64;
				else
					gap = (int)(E->topSpeed(e->type()) * 24);
			}
			// 나를 타게팅 하지 않은 적인 경우
			else {

				// 내 사거리가 많이 긴 경우 계속 거리를 둔다.
				if (enemyRange + 64 < selfRange)
					gap = (int)(E->topSpeed(e->type()) * 14);
				// 차이가 얼마 안나거나 짧은 경우 맞으면서 싸운다.
				else
					gap = -64;
			}

			// 적의 사거리 + x 거리이면 도망
			if (enemyRange + gap >= dist) {
				CommandUtil::backMove(unit, e->unit());
				// moveBackPostion(INFO.getUnitInfo(unit, S), e->pos(), 2);
				return nullptr;
			}

			if (dist < minDist) {
				minDist = dist;
				closest = e->unit();

				if (unit->isInWeaponRange(e->unit())) {
					if (minHP > e->hp()) {
						lowestHP = e->unit();
						minHP = e->hp();
					}
				}
			}
		}
	}

	target = lowestHP ? lowestHP : closest;

	if (!target) {
		uList eBuildings = INFO.getBuildingsInRadius(E, unit->getPosition(), 10 * TILE_SIZE);

		for (auto b : eBuildings) {
			if (b->type() == Protoss_Pylon) {
				target = b->unit();
				break;
			}
		}
	}

	if (target)
		CommandUtil::attackUnit(unit, target);
	else if (m_targetPos.isValid())
		CommandUtil::attackMove(unit, m_targetPos);

	return nullptr;
}

// ## 적군 일꾼 정찰 잡기용
State *MarineKillScouterState::action(Position lastScouterPosition)
{
	if (!unit->isMoving() && !unit->isAttacking() && TIME % 24 == 0) {
		unit->stop();
		return nullptr;
	}

	// 앞마당과 본진이 아닌 곳에 있을 경우.
	if (!isSameArea(INFO.getMainBaseLocation(S)->getPosition(), unit->getPosition()) &&
			INFO.getFirstExpansionLocation(S) && !isSameArea(INFO.getFirstExpansionLocation(S)->getPosition(), unit->getPosition())) {

		uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

		if (commandCenterList.empty()) return nullptr;

		if (commandCenterList.size() == 1)
		{
			// 9드론 이하 빌드일 때 마린들 커맨드 아래에 대기
			if (ESM.getEnemyInitialBuild() <= Zerg_9_Balup || ((INFO.enemyRace == Races::Zerg || INFO.enemyRace == Races::Unknown) && ESM.getEnemyInitialBuild() == UnknownBuild))
			{
				Unit mineral = bw->getClosestUnit(commandCenterList[0]->pos(), Filter::IsMineralField);
				Position target = (commandCenterList[0]->pos() + mineral->getPosition()) / 2;
				unit->move(target);

				return nullptr;
			}
		}

		Position pos = (INFO.getSecondChokePosition(S) + INFO.getFirstExpansionLocation(S)->Center()) / 2;
		CommandUtil::attackMove(unit, pos);
		return nullptr;

	}

	UnitInfo *uinfo = INFO.getUnitInfo(unit, S);

	if (uinfo->getVeryFrontEnemyUnit() != nullptr)
	{
		UnitInfo *enemUinfo = INFO.getUnitInfo(uinfo->getVeryFrontEnemyUnit(), E);

		if (!enemUinfo)
			return nullptr;

		attackFirstkiting(uinfo, enemUinfo, enemUinfo->pos().getApproxDistance(unit->getPosition()), 3 * TILE_SIZE);

	}
	else
	{
		UnitInfo *closestEnem = INFO.getClosestUnit(E, uinfo->pos(), MyBot::AllKind, 2000, true);

		if (closestEnem != nullptr)
		{
			//if (unit->isInWeaponRange(closestEnem->unit()) && unit->getGroundWeaponCooldown() == 0)
			CommandUtil::attackUnit(unit, closestEnem->unit());
			//else
			//	CommandUtil::attackMove(unit, lastScouterPosition);
		}
		else
			CommandUtil::attackMove(unit, lastScouterPosition);
	}


	return nullptr;
}

State *MarineKitingState::action()
{
	// 벙커 입성하면 Defence State
	if (unit->isLoaded())
		return new MarineDefenceState();

	// 상대가 에어 유닛이거나 나보다 사정거리가 길거나 같으면 벙커에서 대기하거나 그냥 공격한다.
	UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), AllUnitKind);

	if (closeUnit == nullptr || unit->getDistance(closeUnit->pos()) > 1600)
		return new MarineIdleState();

	int distanceFromMainBase = 0;

	theMap.GetPath(unit->getPosition(), MYBASE, &distanceFromMainBase);

	// 본진에서 너무 멀리간 마린은 DefenceMode로 변환
	if (distanceFromMainBase >= 1600) {
		return new MarineDefenceState();
	}

	Unit bunker = MM.getBunker();

	// 벙커가 완성되어 있거나 근처 유닛이 없으면 Defence Mode로 변환
	if (bunker != nullptr && bunker->isCompleted()) {
		return new MarineDefenceState();
	}

	if (ESM.getEnemyInitialBuild() <= Zerg_12_Pool) // 9발업 이하에서는 마린이 밖으로 나가지 않는다.
	{
		if (INFO.getTypeUnitsInRadius(Zerg_Zergling, E, unit->getPosition(), 8 * TILE_SIZE).size()
				> INFO.getTypeUnitsInRadius(Terran_Marine, S, unit->getPosition(), 8 * TILE_SIZE).size())
		{
			Position myBase = INFO.getMainBaseLocation(S)->Center();

			TilePosition myBaseTile = INFO.getMainBaseLocation(S)->getTilePosition();

			if (!isSameArea(theMap.GetArea((WalkPosition)unit->getPosition()), theMap.GetArea(myBaseTile))
					|| INFO.getFirstChokePosition(S).getApproxDistance(unit->getPosition()) < 3 * TILE_SIZE)
			{
				unit->move(myBase);
				return nullptr;
			}
		}
	}

	if (!unit->isInWeaponRange(closeUnit->unit()))
	{
		CommandUtil::attackUnit(unit, closeUnit->unit());
	}
	// 사거리 안이면
	else {
		uList Scvs = INFO.getTypeUnitsInRadius(Terran_SCV, S, (unit->getPosition() + closeUnit->pos()) / 2, 50);

		if (unit->getGroundWeaponCooldown() == 0 && (Scvs.size() >= 3 || unit->getDistance(closeUnit->unit()) > 2 * TILE_SIZE))
		{
			CommandUtil::attackUnit(unit, closeUnit->unit());
		}
		else
		{
			if (closeUnit->type().groundWeapon().maxRange() < 4 * TILE_SIZE) {
				moveBackPostionMarine(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 1 * TILE_SIZE);
			}
		}

	}

	return nullptr;
}

// 이 상태에서 다른 상태로 변경되는 경우, MarineManager 의 zealotDefenceMarine 를 Nullptr 처리 필요.
State *MarineZealotDefenceState::action()
{
	UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), AllUnitKind);

	if (closeUnit == nullptr) {

		Unit bunker = MM.getBunker();

		if (bunker && bunker->isCompleted()) {
			if (!unit->isLoaded()) {
				if (unit->isSelected()) cout << "벙커 들어가자 " << endl;

				unit->rightClick(bunker);
				return nullptr;
			}
		}

		if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) < 4) {

			if (unit->isSelected()) cout << "탱크 한테 붙자" << endl;

			UnitInfo *closeTank = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Siege_Tank_Tank_Mode, 0, false, true);

			if (closeTank) {
				if (unit->getDistance(closeTank->pos()) > 2 * TILE_SIZE) {
					CommandUtil::move(unit, closeTank->pos());
				}
			}
		}

		return nullptr;
	}

	Unit bunker = MM.getBunker();

	if (bunker != nullptr && bunker->isCompleted()) {

		uList vultures = INFO.getTypeUnitsInRadius(Terran_Vulture, S, MYBASE, 30 * TILE_SIZE);

		// 벙커 안에 있는 유닛
		if (unit->isLoaded()) {
			// 벙커에 들어가 있는 경우, 사거리에서 멀어지면 빼서 치고 가까이오면 들어감.
			// 질럿이 가까이오면 들어가고, 멀어지면 뺀다.

			//벌쳐가 있으면 벙커에서 대기
			if (vultures.size() != 0)
				return nullptr;

			if (!unit->isInWeaponRange(closeUnit->unit()))
			{
				bunker->unload(unit);
			}

			return nullptr;
		}
		// 벙커 밖에 있는 유닛
		else {

			if (vultures.size() != 0 && bunker->getLoadedUnits().size() != 4) {
				CommandUtil::rightClick(unit, bunker);
				return nullptr;
			}

			// 사거리 밖이면 가까이감
			if (!unit->isInWeaponRange(closeUnit->unit()))
			{
				CommandUtil::attackUnit(unit, closeUnit->unit());
			}
			// 사거리 안이면
			else {
				// 적당히 치다가 뒤로 빠짐.
				if (unit->getPosition().getDistance(closeUnit->pos()) > 4 * TILE_SIZE) {
					CommandUtil::attackUnit(unit, closeUnit->unit());
				}
				else {
					// 벙커로 도망
					if (bunker->getLoadedUnits().size() != 4) {
						CommandUtil::rightClick(unit, bunker);
					}
					else {
						if (unit->getDistance(MYBASE) <= 1 * TILE_SIZE) {
							CommandUtil::attackUnit(unit, closeUnit->unit());
						}
						else {
							CommandUtil::move(unit, MYBASE);
						}
					}
				}
			}
		}
	}
	else {
		// 배럭과 서플 사이로 이동.
		Unit barrack = MM.getFirstBarrack();
		Unit supply = MM.getNextBarrackSupply();

		if (barrack == nullptr || supply == nullptr)
			return nullptr;

		if (!unit->isInWeaponRange(closeUnit->unit()))
		{
			CommandUtil::attackUnit(unit, closeUnit->unit());
		}
		else {

			// 마린이 이동할 좌표들.
			int top = supply->getTop();
			int bottom = barrack->getBottom();
			int newX = MM.getFirstBarrack()->getRight();

			int margin = TILE_SIZE; // 마린의 위치를 찾을때 정확한 픽셀로 하면 반응이 너무 느림.
			Position posTop = Position(newX + 12, top - margin);
			Position posBottom = Position(newX, bottom + margin);
			Position target;

			// 적당히 치다가 뒤로 빠짐.
			if (unit->getPosition().getDistance(closeUnit->pos()) > 4 * TILE_SIZE) {
				// 3타일 이상이면, 공격
				CommandUtil::attackUnit(unit, closeUnit->unit());
			}
			else {

				// 너무 가까워서 도망가야되는데
				// 1. 배럭의 반대편에 적이 있으면 공격
				if ((unit->getPosition().y <= top && closeUnit->pos().y >= bottom)
						|| (unit->getPosition().y >= bottom && closeUnit->pos().y <= top)) {
					CommandUtil::attackUnit(unit, closeUnit->unit());
				}
				// 2. 사거리 안이면 도망
				else {

					// 마린이 배럭이나 서플 위에 있다. ->
					if (unit->getPosition().y <= top) {
						// 근처까지 갔다.
						if (abs(unit->getPosition().y - (top - margin)) <= margin && abs(unit->getPosition().x - newX) <= margin) {
							target = posBottom; // 아래로 이동
						}
						else {
							target = posTop; // 위로 이동
						}
					}
					// 마린이 배럭이나 서플 위에 아래에 있다.
					else if (unit->getPosition().y > bottom) {
						// 근처까지 갔다.
						if (abs(unit->getPosition().y - (bottom + margin)) <= margin && abs(unit->getPosition().x - newX) <= margin) {
							target = posTop; // 위로 이동
						}
						else {
							target = posBottom; // 아래로 이동
						}
					}
					// 중간에 껴있는 경우.
					else {
						// 마린이 질럿보다 위에 있으면
						if (unit->getPosition().y - closeUnit->pos().y > 0) {
							target = posBottom; // 아래로 이동
						}
						else {
							target = posTop;// 위로 이동
						}
					}

					uList eL = INFO.getUnitsInRadius(E, target, 3 * TILE_SIZE, true, false, false, false, true);

					if (eL.empty())
						CommandUtil::move(unit, target);
					else {

						// 내가 적보다 target 위치에 가까이 있을때만 이동
						int dist_m = 0;
						int dist_e = 0;
						theMap.GetPath(unit->getPosition(), target, &dist_m);

						for (auto e : eL) {
							theMap.GetPath(e->pos(), target, &dist_e);

							if (dist_e < dist_m) {
								UnitInfo *me = INFO.getUnitInfo(unit, S);

								//target 으로 부터 나보다 가까운 적이 있으면 뒤로 도망간다.
								if (me != nullptr) {
									moveBackPostion(me, closeUnit->vPos(), 2 * TILE_SIZE);
									return nullptr;
								}
							}
						}

						CommandUtil::move(unit, target);
					}
				}
			}
		}
	}

	return nullptr;
}

State *MarineDefenceState::action()
{
	if (ESM.getEnemyInitialBuild() == Terran_bunker_rush) {
		if (unit->isSelected()) cout << "BunkerRush 수비 " << endl;

		bunkerRushDefence();
		return nullptr;
	}

	// 다크템플러, 셔틀 포함 같은 보이지 않는 유닛 포함
	UnitInfo *closeUnit = getMarineDefenceTargetUnit(unit);

	Unit bunker = MM.getBunker();

	if (closeUnit == nullptr) {
		if (unit->isSelected()) cout << "Close unit 없당 " << endl;

		if (bunker && bunker->isCompleted()) {
			if (!unit->isLoaded()) {
				if (unit->isSelected()) cout << "벙커 들어가자 " << endl;

				unit->rightClick(bunker);
				return nullptr;
			}
		}

		if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) < 4) {

			if (unit->isSelected()) cout << "탱크 한테 붙자" << endl;

			UnitInfo *closeTank = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Siege_Tank_Tank_Mode, 0, false, true);

			if (closeTank) {
				if (unit->getDistance(closeTank->pos()) > 2 * TILE_SIZE) {
					CommandUtil::move(unit, closeTank->pos());
				}
			}
		}

		return nullptr;
	}

	// 내가 영역 밖으로 나가면 안으로 들어오게 하는 코드
	if (!isInMyArea(unit->getPosition())) {
		if (unit->isSelected()) cout << "내가 영역 밖으로 나갔다" << endl;

		// 영역 밖에서 공격하는 유닛에 대한 대처 필요
		if (getGroundDistance(MYBASE, closeUnit->pos()) < getGroundDistance(MYBASE, unit->getPosition()))
			CommandUtil::attackUnit(unit, closeUnit->unit());
		else
			CommandUtil::move(unit, MYBASE, true);

		return nullptr;
	}

	if (INFO.enemyRace == Races::Protoss) {

		// 본진 및 앞마당에 들어온 셔틀 방어로직. 명령이 들어가면 return true;
		if (shuttleDefence(unit)) {
			if (unit->isSelected()) cout << "셔틀 수비 " << endl;

			return nullptr;
		}

		// 다템 방어 로직. 명령이 들어가면 return true;
		if (closeUnit->type() == Protoss_Dark_Templar)
			if (darkDefence(unit, closeUnit)) {
				if (unit->isSelected()) cout << "다크 수비 " << endl;

				return nullptr;
			}

	}

	uList vultures = INFO.getTypeUnitsInRadius(Terran_Vulture, S, MYBASE, 30 * TILE_SIZE);
	uList tanks = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, MYBASE, 30 * TILE_SIZE);
	uList marineList = INFO.getUnits(Terran_Marine, S);

	if (bunker && bunker->isCompleted()) {
		if (unit->isLoaded()) {
			// 이미 벌처가 있으면 벙커에서 대기.
			if (!vultures.empty() || !tanks.empty())
				return nullptr;

			if (MM.isRangeUnitClose())
				return nullptr;

			// 벙커에 들어가 있는 경우, 사거리에서 멀어지면 빼서 치고 가까이오면 들어감.
			// 질럿이 가까이오면 들어가고, 멀어지면 뺀다.
			bool isUnderAttack = false;
			uList baseUnits = INFO.getUnits(Terran_SCV, S);
			uList baseBuildings = INFO.getBuildingsInRadius(S, MYBASE, 15 * TILE_SIZE);

			for (auto u : baseUnits)
			{
				if (u->unit()->isUnderAttack() && u->getState() != "Scout")
				{
					isUnderAttack = true;
					break;
				}
			}

			if (isUnderAttack == false)
			{
				for (auto u : baseBuildings)
				{
					if (u->unit()->isUnderAttack())
					{
						isUnderAttack = true;
						break;
					}
				}
			}

			if (isUnderAttack && unit == marineList.at(0)->unit() && bunker->getDistance(closeUnit->unit()) > 5 * TILE_SIZE && bunker->canUnload(unit)) {
				bunker->unload(unit);
			}

			return nullptr;
		}
		else {

			if (escapeFromSpell(INFO.getUnitInfo(unit, S)))
				return nullptr;

			// 벌쳐가 나와있으면 벙커안으로 들어간다.
			if ((!vultures.empty() || !tanks.empty()) && bunker->getLoadedUnits().size() != 4) {

				CommandUtil::rightClick(unit, bunker);
				/*if (bunker->getPosition().getApproxDistance(unit->getPosition()) <= 1 * TILE_SIZE)
				else {
				CommandUtil::attackMove(unit, bunker->getPosition());
				}*/
				return nullptr;
			}
			else {

				if (closeUnit->type().isWorker() &&
						INFO.getTypeUnitsInArea(INFO.getWorkerType(INFO.enemyRace), E, MYBASE).size()
						+ INFO.getTypeUnitsInArea(INFO.getWorkerType(INFO.enemyRace), E, INFO.getFirstExpansionLocation(S)->Center()).size() <= 1) {
					// 벙커로 들어가자.
					if (bunker->getPosition().getApproxDistance(unit->getPosition()) < 2 * TILE_SIZE) {
						CommandUtil::rightClick(unit, bunker);
					}
					else {
						CommandUtil::attackMove(unit, bunker->getPosition());
					}

					return nullptr;
				}

				// 벌쳐가 없는 경우,
				// 벙커에 들어가지 않고 마린이 밖으로 나와야 하는 경우는
				// 본진 난입한 적을 막기 위한 경우 외에는 없다.

				// 벙커가 앞마당에 있고, 적이 내 본진에 있으면
				if (INFO.getFirstExpansionLocation(S) &&
						isSameArea(INFO.getFirstExpansionLocation(S)->Center(), bunker->getPosition())
						&& isSameArea(closeUnit->pos(), MYBASE)) {
					// 계속 나와서 싸운다.
					if (unit->getGroundWeaponCooldown() == 0) {
						CommandUtil::attackUnit(unit, closeUnit->unit());
					}
					else {
						int range = closeUnit->type().groundWeapon().maxRange();

						if (unit->getDistance(closeUnit->unit()) > range + 2 * TILE_SIZE)
							CommandUtil::attackMove(unit, closeUnit->pos());
						else
							moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 3 * TILE_SIZE);
					}

					return nullptr;
				}
				else {

					int dist = unit->getPosition().getApproxDistance(closeUnit->pos());

					if (bunker->getLoadedUnits().size() != 4) {
						// 벙커 들어가기
						if (isNeedKitingUnitType(closeUnit->type()) && unit->getGroundWeaponCooldown() == 0 && dist > 4 * TILE_SIZE) {
							CommandUtil::attackUnit(unit, closeUnit->unit());
						}
						else {
							CommandUtil::rightClick(unit, bunker);
						}
					}
					else {
						// 밖에서 카이팅
						if (unit->getGroundWeaponCooldown() == 0) {
							CommandUtil::attackUnit(unit, closeUnit->unit());
						}
						else {
							int range = closeUnit->type().groundWeapon().maxRange();

							if (unit->getDistance(closeUnit->unit()) > range + 2 * TILE_SIZE)
								CommandUtil::attackMove(unit, closeUnit->pos());
							else
								moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 3 * TILE_SIZE);
						}
					}
				}

				return nullptr;
			}

		}
	}
	else {

		if (escapeFromSpell(INFO.getUnitInfo(unit, S)))
			return nullptr;

		if (vultures.size() != 0) {

			if (INFO.getUnits(Protoss_Dragoon, E).empty()) {
				if (unit->getDistance(MM.waitingNearCommand) > 50)
					CommandUtil::attackMove(unit, MM.waitingNearCommand, true);

				return nullptr;
			}
			else {
				if (INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S).size() >= 2)
					return nullptr;
			}
		}

		if (INFO.enemyRace == Races::Zerg) {
			if (unit->getDistance(closeUnit->unit()) < 5 * TILE_SIZE) {
				MM.doKiting(unit);
				return nullptr;
			}

			bool isUnderAttack = false;

			uList baseUnits = INFO.getUnits(Terran_SCV, S);
			uList baseBuildings = INFO.getBuildingsInRadius(S, MYBASE, 15 * TILE_SIZE);

			for (auto u : baseUnits)
			{
				if (u->unit()->isUnderAttack() && u->getState() != "Scout")
				{
					isUnderAttack = true;
					break;
				}
			}

			if (isUnderAttack == false)
			{
				for (auto u : baseBuildings)
				{
					if (u->unit()->isUnderAttack())
					{
						isUnderAttack = true;
						break;
					}
				}
			}

			if (isUnderAttack) {
				MM.doKiting(unit);
				return nullptr;
			}
			else
			{
				uList centers = INFO.getBuildings(Terran_Command_Center, S);

				if (centers.size())
				{
					UnitInfo *center = centers[0];
					Unit mineral = bw->getClosestUnit(center->pos(), Filter::IsMineralField);

					if (mineral != nullptr) {
						Position target = (center->pos() + mineral->getPosition()) / 2;
						unit->move(target);
					}
				}
			}
		}
		else {
			MM.doKiting(unit);
		}

	}

	return nullptr;
}

State *MarineDefenceState::bunkerRushDefence()
{
	// 적 마린이 있는 경우
	if (INFO.getCompletedCount(Terran_Marine, E)) {
		// 완성된 벙커가 있는 경우
		if (INFO.getCompletedCount(Terran_Bunker, E)) {
			if (INFO.getCompletedCount(Terran_Marine, S) < 3)
				return nullptr;
			else {
				uList eSCV = INFO.getTypeUnitsInRadius(Terran_SCV, E);

				for (auto scv : eSCV) {
					if (scv->unit()->isRepairing()) {
						CommandUtil::attackUnit(unit, scv->unit());
						return nullptr;
					}
				}

				uList eBunker = INFO.getTypeBuildingsInRadius(Terran_Bunker, E);

				for (auto bunker : eBunker) {
					if (bunker->isHide()) {
						CommandUtil::attackMove(unit, bunker->pos());
						break;
					}
					else {
						CommandUtil::attackUnit(unit, bunker->unit());
						break;
					}
				}
			}
		}
		else {
			uList eSCV = INFO.getTypeUnitsInRadius(Terran_SCV, E);

			for (auto scv : eSCV) {
				if (scv->unit()->isConstructing()) {
					CommandUtil::attackUnit(unit, scv->unit());
					return nullptr;
				}
			}

			UnitInfo *e = INFO.getClosestUnit(E, unit->getPosition(), AllKind, 0, true, false, true);

			if (e)
				CommandUtil::attackUnit(unit, e->unit());
			else
				CommandUtil::attackMove(unit, INFO.getSecondChokePosition(S));
		}
	}
	// 적 마린이 없는 경우
	else {
		uList eSCV = INFO.getTypeUnitsInRadius(Terran_SCV, E);

		for (auto scv : eSCV) {
			if (scv->unit()->isConstructing() || scv->unit()->isRepairing()) {
				CommandUtil::attackUnit(unit, scv->unit());
				return nullptr;
			}
		}

		UnitInfo *e = INFO.getClosestUnit(E, unit->getPosition(), AllKind, 0, true, false, true);

		if (e)
			CommandUtil::attackUnit(unit, e->unit());
		else
			CommandUtil::attackMove(unit, INFO.getSecondChokePosition(S));
	}

	return nullptr;
}

UnitInfo *MarineDefenceState::getMarineDefenceTargetUnit(const Unit &unit) {
	UnitInfo *closeUnit = nullptr;
	uList eList = getEnemyInMyYard(1700, Men, false);
	uList eList2;

	if (INFO.getFirstExpansionLocation(S))
		eList2 = INFO.getUnitsInArea(E, INFO.getFirstExpansionLocation(S)->Center(), true, true, true, false);

	eList.insert(eList.end(), eList2.begin(), eList2.end());

	int dist = MAXINT;
	int temp = 0;

	for (auto e : eList) {
		theMap.GetPath(e->pos(), unit->getPosition(), &temp);

		if (e->type().isWorker()) {
			if (!closeUnit || (closeUnit->type().isWorker() && (temp >= 0 && dist > temp))) {
				dist = temp;
				closeUnit = e;
			}
		}
		else {
			if (temp >= 0 && dist > temp) {
				dist = temp;
				closeUnit = e;
			}
		}
	}

	if (!closeUnit) {
		uList enemyBuildingsInMyYard = getEnemyInMyYard(1700, UnitTypes::Buildings);
		dist = MAXINT;
		temp = 0;

		for (auto e : enemyBuildingsInMyYard) {
			theMap.GetPath(e->pos(), unit->getPosition(), &temp);

			if (temp >= 0 && dist > temp) {
				dist = temp;
				closeUnit = e;
			}
		}
	}

	return closeUnit;
}

/// 본진이나 앞마당에 들어온 셔틀 방어로직. 명령이 들어가면 return true; 아니면 return false;
bool MarineDefenceState::shuttleDefence(const Unit &unit) {

	uList shuttles = INFO.getTypeUnitsInArea(Protoss_Shuttle, E, MYBASE, false);
	uList shuttles2;

	if (INFO.getFirstExpansionLocation(S) != nullptr)
		shuttles2 = INFO.getTypeUnitsInArea(Protoss_Shuttle, E, INFO.getFirstExpansionLocation(S)->getPosition(), false);

	uList goliaths = INFO.getTypeUnitsInArea(Terran_Goliath, S, MYBASE, false);

	if ((shuttles.empty() && shuttles2.empty()) || !goliaths.empty())
		return false;

	Unit bunker = MM.getBunker();

	if (bunker && bunker->isCompleted()) {

		if (unit->isLoaded()) {
			if (INFO.getUnitsInRadius(E, bunker->getPosition(), 7 * TILE_SIZE, true, true).empty() && bunker->canUnload(unit)) {
				bunker->unload(unit);
				return true;
			}
		}
	}

	// 근처에 위험한 유닛이 있으면 일단 걔한테 대처
	UnitInfo *closeDangerUnit = INFO.getClosestUnit(E, unit->getPosition(), GroundCombatKind, 4 * TILE_SIZE, false);

	if (closeDangerUnit) {
		if (unit->getGroundWeaponCooldown() == 0)
			CommandUtil::attackUnit(unit, closeDangerUnit->unit());
		else
			moveBackPostionMarine(INFO.getUnitInfo(unit, S), closeDangerUnit->vPos(), 2 * TILE_SIZE);

		return true;
	}

	UnitInfo *closeShuttle = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Shuttle, 30 * TILE_SIZE, false);

	if (closeShuttle && isInMyArea(closeShuttle)) {
		CommandUtil::attackUnit(unit, closeShuttle->unit());
		return true;
	}

	return false;

}

/// 마린의 다크템플러 방어 로직. 명령이 들어가면 return true, 아니면 return false;
bool MarineDefenceState::darkDefence(const Unit &unit, const UnitInfo *closeUnit) {

	Unit bunker = MM.getBunker();

	if (bunker && bunker->isCompleted()) {

		if (!unit->isLoaded()) {

			int gap = 0;

			if (!closeUnit->unit()->isDetected())
				gap = 7 * TILE_SIZE; // 도망 모드
			else
				gap = 3 * TILE_SIZE;

			int distFromDark = unit->getDistance(closeUnit->unit());
			int distFromBunker = unit->getDistance(bunker);

			// 보이면서 가까우면 거리가 좀 떨어져 있으면 공격
			if (closeUnit->unit()->isDetected() && (distFromDark > gap || unit->getGroundWeaponCooldown() == 0))
				CommandUtil::attackUnit(unit, closeUnit->unit());
			else {
				// 안보이거나 거리가 너무 가까울 때, 벙커로 들어가는게 낫겠지?

				//다크보다 벙커에 더 가까우면 들어간다.
				if (distFromDark > distFromBunker) {
					unit->rightClick(bunker);
					return true;
				}

				// 다크와 gap 을 유지하면서 움직임
				if (unit->getDistance(closeUnit->unit()) < gap) {
					moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 3 * TILE_SIZE);
				}
				else {
					CommandUtil::attackMove(unit, closeUnit->vPos());
				}

			}
		}

		return true;
	}
	else {
		int gap = 0;

		if (!closeUnit->unit()->isDetected())
			gap = 7 * TILE_SIZE; // 도망 모드
		else
			gap = 3 * TILE_SIZE;

		if (closeUnit->unit()->isDetected() && (unit->getDistance(closeUnit->unit()) > gap || unit->getGroundWeaponCooldown() == 0))
			CommandUtil::attackUnit(unit, closeUnit->unit());
		else
			moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 3 * TILE_SIZE);

		return true;
	}

	return false;

}

MyBot::MarineFirstChokeDefenceState::MarineFirstChokeDefenceState(Position p)
{
	defencePosition = p;
}

// 마린 2기 + SCV를 이용해서 초반에 입구로 들어오는 적을 막는다.
State *MarineFirstChokeDefenceState::action()
{
	if (INFO.isTimeToMoveCommandCenterToFirstExpansion()) {

		UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), TypeKind::GroundCombatKind, 20 * TILE_SIZE, false);

		UnitInfo *closeTank = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Siege_Tank_Tank_Mode, 0, false, true);

		if (closeUnit && closeTank) {
			// 언덕 시즈와 너무 떨어지지 않는다.

			if (unit->getDistance(closeTank->pos()) <= 2 * TILE_SIZE) {
				CommandUtil::attackMove(unit, closeUnit->pos());
			}
			else {
				CommandUtil::attackMove(unit, closeTank->pos());
			}

			return nullptr;
		}
		else {

			if (INFO.getFirstExpansionLocation(S)) {
				uList tList = INFO.getTypeBuildingsInArea(Terran_Missile_Turret, S, INFO.getFirstExpansionLocation(S)->Center());

				if (!tList.empty()) {
					CommandUtil::attackMove(unit, tList.at(0)->pos());
				}
				else {
					if (closeTank)
						if (closeTank->pos().getApproxDistance(unit->getPosition()) > 3 * TILE_SIZE)
							CommandUtil::attackMove(unit, closeTank->pos());
						else
							CommandUtil::attackMove(unit, INFO.getSecondAverageChokePosition(S));
					else
						CommandUtil::attackMove(unit, INFO.getSecondAverageChokePosition(S));
				}
			}
		}

		return nullptr;
	}

	if (unit->getDistance(defencePosition) > 16 ) {
		CommandUtil::move(unit, defencePosition, true);
	}
	else {
		CommandUtil::hold(unit, false);
	}

	return nullptr;
}

State *MarineGoGoGoState::action()
{
	if (unit->isLoaded()) return nullptr;

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(unit->getPosition(), &dangerPoint, false);
	UnitInfo *closestTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

	if (dangerUnit == nullptr)
	{
		CommandUtil::attackMove(unit, SM.getMainAttackPosition());
	}
	else if (dangerUnit != nullptr && closestTank != nullptr)
	{
		if (unit->getPosition().getApproxDistance(closestTank->pos()) < 5 * TILE_SIZE)
		{
			CommandUtil::attackMove(unit, dangerUnit->pos(), true);
		}
		else if (unit->getPosition().getApproxDistance(closestTank->pos()) > 8 * TILE_SIZE)
		{
			CommandUtil::attackMove(unit, closestTank->pos());
		}

	}

	return nullptr;
}
