#include "ScvState.h"
#include "../InformationManager.h"
#include "../UnitManager/ScvManager.h"
#include "../TrainManager.h"

using namespace MyBot;


ScvMineralState::ScvMineralState(Unit Mineral)
{
	pre_carry = false;
	pre_gathered = false;
	skip_cnt = 0;
	m_targetUnit = Mineral;
	healerScv = nullptr;
}

State *ScvMineralState::action()
{
	if (healerScv != nullptr && healerScv->exists() && INFO.getUnitInfo(healerScv, S)->getState() != "Repair") healerScv = nullptr;

	if (healerScv == nullptr) {
		//repairScvSetTime = 0;
		started = false;
		repairStartTime = 0;
		startHp = 0;
	};

	Position waitingPosition = getDirectionDistancePosition(MYBASE, (Position)INFO.getFirstChokePoint(S)->Center(), (-1) * 4 * TILE_SIZE);

	if (healerScv != nullptr && healerScv->exists() && unit->getHitPoints() < Terran_SCV.maxHitPoints())
	{
		if (isBeingRepaired(unit) && !started)
		{
			started = true;
			startHp = unit->getHitPoints();
			repairStartTime = TIME;
		}

		if (started && (TIME % (repairStartTime + (6 * 24)) == 0))
		{
			startHp = unit->getHitPoints();
			repairStartTime = TIME;
		}

		UnitCommand currentCommand(unit->getLastCommand());

		if (unit->canReturnCargo() && (currentCommand.getType() != UnitCommandTypes::Return_Cargo))
			unit->returnCargo();
		else if (!unit->isCarryingGas() && !unit->isCarryingMinerals()) // 반납완료
		{
			if (unit->isSelected())
			{
				cout << "시작?:" << started << "/수리중?:" << isBeingRepaired(unit) << "/거리?:" << unit->getPosition().getDistance(healerScv->getPosition()) << "/몇초?" << (TIME - repairStartTime) / 24 << "초" << endl;
			}

			if (started)
			{
				if (TIME - repairStartTime >= 24 * 5 && unit->getHitPoints() - startHp < 2)
				{
					return new ScvIdleState();
				}

				if (healerScv->getHitPoints() < Terran_SCV.maxHitPoints())
				{
					CommandUtil::repair(unit, healerScv);
				}
				else
				{
					CommandUtil::move(unit, waitingPosition);
				}
			}
			else
			{
				CommandUtil::move(unit, waitingPosition);
			}
		}

		return nullptr;
	}

	if ( (( ( ESM.getEnemyInitialBuild() == Toss_4probes || ESM.getEnemyInitialBuild() == Terran_4scv || ESM.getEnemyInitialBuild() == Zerg_4_Drone_Real || ESM.getEnemyInitialBuild() <= Zerg_9_Balup || ESM.getEnemyInitialBuild() == Terran_bunker_rush || ESM.getEnemyInitialBuild() == Toss_cannon_rush || ESM.getEnemyInitialBuild() == Terran_2b_forward)
			&& ESM.getEnemyMainBuild() == UnknownMainBuild) || ESM.getEnemyMainBuild() == Zerg_main_zergling)
			&& ScvManager::Instance().getRepairScvCount() < 1)
	{
		if (INFO.getClosestUnit(E, unit->getPosition(), GroundUnitKind, 12 * TILE_SIZE, true) == nullptr && unit->getDistance(MYBASE) < 3 * TILE_SIZE)
		{
			if (unit->getHitPoints() < 50 && (healerScv == nullptr || (healerScv != nullptr && healerScv->exists())))
			{
				//cout << unit->getID() << ":수리SCV할당.." << endl;
				healerScv = ScvManager::Instance().chooseRepairScvforScv(unit, 6 * TILE_SIZE);

				if (healerScv != nullptr)
				{
					UnitCommand currentCommand(unit->getLastCommand());

					if (unit->canReturnCargo() && (currentCommand.getType() != UnitCommandTypes::Return_Cargo))
						unit->returnCargo();
					else if (!unit->isCarryingGas() && !unit->isCarryingMinerals()) // 반납완료
						unit->move(waitingPosition);

					//unit->rightClick(healerScv);

					return nullptr;
				}
			}
		}
	}

	if (unit->isUnderAttack() || !INFO.getUnitInfo(unit, S)->getEnemiesTargetMe().empty())
	{
		// 커맨드 근처에 적이 있는 경우에는, 내 주변에 적이 오면 멀리 있는 미네랄로 즉시 대피한다.
		Unit farMineral = INFO.getSafestMineral(unit);

		if (farMineral) {
			CommandUtil::rightClick(unit, farMineral);
			return nullptr;
		}

		UnitInfo *closest = INFO.getClosestUnit(E, unit->getPosition(), GroundUnitKind, 2 * TILE_SIZE, true);

		if (unit->getHitPoints() >= 35 && closest != nullptr)
		{
			return new ScvCounterAttackState();
		}
	}

	if (m_targetUnit->exists() == false)
	{
		return new ScvIdleState();
	}

	if (unit->isGatheringMinerals() == false) // Mineral을 캐고 있지 않음.
	{
		UnitCommand currentCommand(unit->getLastCommand());

		if (unit->canReturnCargo() && (currentCommand.getType() != UnitCommandTypes::Return_Cargo)) // 들고 있는게 있음.
		{
			unit->returnCargo(); // 반환
		}
		else
		{
			if (!unit->isCarryingGas() && !unit->isCarryingMinerals()) // 반납완료
				CommandUtil::gather(unit, m_targetUnit);
		}

		return nullptr;
	}

	// 2마리 이상 붙어있을때만 미네랄 락 수행.
	if (ScvManager::Instance().getMineralScvCount(m_targetUnit) < 2)
		return nullptr;

	// Skip Cnt 3 sec
	if (skip_cnt) {
		skip_cnt--;
		return nullptr;
	}

	// Mineral 들고 있는 경우 skip
	if (unit->isCarryingMinerals()) {
		pre_carry = true;
		return nullptr;
	}

	// Mineral Carring -> no Carring 시점에 Mineral 지정
	if (pre_carry)
	{
		unit->rightClick(m_targetUnit);
		pre_carry = false;
	}

	int Dist = unit->getPosition().getApproxDistance(m_targetUnit->getPosition());

	// 거리가 먼경우 그냥 skip
	if (Dist > THR_CHECK)
		return nullptr;

	// 거리가 THR_CHECK 안으로 들어온 경우 목표 Mineral is mining Check
	if (Dist > THR_MINERAL)
	{
		pre_gathered = m_targetUnit->isBeingGathered();
		return nullptr;
	}

	// 여기까지 왔으면 SCV는 해당 Mineral에 붙어 있는거라 볼수 있음.
	// 만약 진입 직전에 이미 캐지고 있었고(다른scv가) 지금도 캐지고 있는중이면 계속해서 우클릭 한다.
	if (pre_gathered && m_targetUnit->isBeingGathered())
	{
		unit->rightClick(m_targetUnit);
		return nullptr;
	}

	// 현재 캐지 않고 있다면 우클릭 하고 3초 쉰다.
	if (!m_targetUnit->isBeingGathered())
	{
		unit->rightClick(m_targetUnit);
		pre_gathered = false;
		skip_cnt = 3 * 24 / FR;
	}

	return nullptr;
}

ScvRepairState::ScvRepairState(Unit target)
{
	m_targetUnit = target;

	if (m_targetUnit->getType().isBuilding() && m_targetUnit->getType() != Terran_Bunker && m_targetUnit->getType() != Terran_Missile_Turret)
		Threshold = 60;
	else
		Threshold = 100;
}

State *ScvRepairState::action()
{
	if (m_targetUnit->exists() == false || m_targetUnit->isLoaded() || S->minerals() == 0) // Unit이 없어졌으면 idle (예외처리)
		return new ScvIdleState();

	if (!m_targetUnit->getType().isBuilding() && !isInMyArea(m_targetUnit->getPosition(), true)) //내 영역에 없는 경우 idle(수리중인 유닛이 공격을 나갈경우)
	{
		return new ScvIdleState();
	}

	if (((m_targetUnit->getHitPoints() * 100) / m_targetUnit->getType().maxHitPoints()) >= Threshold)
	{
		if (m_targetUnit->getType() == Terran_Missile_Turret)
		{
			if (INFO.enemyRace == Races::Zerg)// 무탈이 없거나 골리앗이 있을때만 Idle로
			{
				if (INFO.getTypeUnitsInRadius(Zerg_Mutalisk, E, m_targetUnit->getPosition(), 8 * TILE_SIZE).size() == 0 ||
						INFO.getTypeUnitsInRadius(Terran_Goliath, S, m_targetUnit->getPosition(), 8 * TILE_SIZE).size() > 4)
					return new ScvIdleState();
			}
			else if (INFO.enemyRace == Races::Protoss) // 본진은 다템 없을때 // 그 외에는 그냥
			{
				if (isSameArea(MYBASE, m_targetUnit->getPosition()))
				{
					if (INFO.getTypeUnitsInRadius(Protoss_Dark_Templar, E, m_targetUnit->getPosition(), 8 * TILE_SIZE).size() == 0)
						return new ScvIdleState();
				}
				else
					return new ScvIdleState();
			}
			else // Terran
			{
				return new ScvIdleState();
			}
		}
		else
		{
			UnitInfo *myRepairUnit = INFO.getUnitInfo(m_targetUnit, S);

			// 공격하는 유닛이 계속 있으면 유지
			if (myRepairUnit == nullptr || myRepairUnit->getEnemiesTargetMe().size() == 0)
				return new ScvIdleState();

			return nullptr;
		}
	}

	if (m_targetUnit->getType() == Terran_Missile_Turret && m_targetUnit->getHitPoints() == m_targetUnit->getType().maxHitPoints())
		unit->rightClick(m_targetUnit);
	else
		CommandUtil::repair(unit, m_targetUnit);

	return nullptr;
}

State *ScvGasState::action()
{
	if (m_targetUnit->exists() == false)
	{
		return new ScvIdleState();
	}

	if (!unit->isGatheringGas())
		CommandUtil::gather(unit, m_targetUnit);

	return nullptr;
}

State *ScvCounterAttackState::action()
{
	UnitInfo *closest = INFO.getClosestUnit(E, unit->getPosition(), GroundUnitKind, 2 * TILE_SIZE, true);

	if (closest == nullptr)
	{
		return new ScvIdleState();
	}

	unit->attack(closest->unit());
	unit->move(closest->vPos());

	return nullptr;
}

// 적 유닛이 초반에 본진이나 앞마당에 들어와서 벙커를 방어해야하는 상황.
//State *ScvBunkerDefenceState::action()
//{
//	// 아직 완성이 안되서 꼭 지켜야 하는 상황
//	if (m_targetUnit->getType() == Terran_Bunker && !m_targetUnit->isCompleted()) {
//		UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), GroundUnitKind, 50 * TILE_SIZE);
//
//		if (closeUnit != nullptr) {
//			CommandUtil::attackUnit(unit, closeUnit->unit());
//		}
//		else {
//			CommandUtil::move(unit, m_targetUnit->getPosition());
//		}
//
//		return nullptr;
//	}
//
//	uList turrets = INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, S, m_targetUnit->getPosition(), 4 * TILE_SIZE, false);
//	Unit needRepairTurret = nullptr;
//
//	for (auto t : turrets)
//	{
//		if (t->hp() < Terran_Missile_Turret.maxHitPoints())
//		{
//			needRepairTurret = t->unit();
//			continue;
//		}
//	}
//
//	if (unit->getDistance(m_targetUnit->getPosition()) >= 2 * TILE_SIZE) {
//		CommandUtil::move(this->unit, m_targetUnit->getPosition());
//	}
//	else {
//		if (m_targetUnit->getHitPoints() < Terran_Bunker.maxHitPoints()) {
//			CommandUtil::repair(unit, m_targetUnit);
//		}
//		else if (needRepairTurret != nullptr)	{
//			CommandUtil::repair(unit, needRepairTurret);
//		}
//		else {
//			UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), GroundUnitKind, 6 * TILE_SIZE);
//
//			if (closeUnit != nullptr ) {
//				int guardThreshold = closeUnit->type().groundWeapon().maxRange() >= 2 * TILE_SIZE ? 3 : 1;
//
//				if (unit->getDistance(closeUnit->pos()) <= guardThreshold * TILE_SIZE) {
//					unit->attack(closeUnit->unit());
//					unit->move(closeUnit->vPos());
//				}
//			}
//			else {
//				CommandUtil::move(unit, m_targetUnit->getPosition());
//			}
//		}
//	}
//
//	return nullptr;
//}

ScvScoutUnitDefenceState::ScvScoutUnitDefenceState(Unit eScouter) {
	m_targetUnit = eScouter;

	m_isEarlyScout = false;

	if (INFO.getCompletedCount(Terran_Barracks, S) == 0) {
		uList barracks = INFO.getBuildings(Terran_Barracks, S);

		if (barracks.empty())
			m_isEarlyScout = true;
		else {
			for (auto b : barracks) {
				// 배럭이 50% 가 되기 전에 정찰이 왔다면 이른 정찰임.
				if (b->hp() + b->hp() <= b->type().maxHitPoints()) {
					m_isEarlyScout = true;
					break;
				}
			}
		}
	}
}

State *ScvScoutUnitDefenceState::action() {

	if (ESM.getEnemyInitialBuild() == Terran_4scv || ESM.getEnemyInitialBuild() == Toss_4probes || ESM.getEnemyInitialBuild() == Zerg_4_Drone_Real) {
		return new ScvIdleState();
	}

	UnitInfo *targetUnitInfo = INFO.getUnitInfo(m_targetUnit, E);

	// 상대봇이 죽은 경우 본진, 앞마당의 안보이는 시야를 탐색한다.
	if (!targetUnitInfo) {
		if (!ScvManager::Instance().getScanMyBaseUnit() && m_isEarlyScout)
			return new ScvScanMyBaseState();
		else
			return new ScvIdleState();
	}

	// 상대봇이 시야에서 사라진 경우 사라진 지역까지 이동 후, 본진, 앞마당의 안보이는 시야를 탐색한다.
	if (targetUnitInfo->pos() == Positions::Unknown) {
		if (unit->isMoving())
			return nullptr;

		if (!ScvManager::Instance().getScanMyBaseUnit() && m_isEarlyScout)
			return new ScvScanMyBaseState(m_targetUnit);
		else
			return new ScvIdleState();
	}

	if (INFO.enemyRace == Races::Protoss) {
		uList cannons = INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, unit->getPosition(), 400, true);

		for (auto c : cannons) {
			if (c->getRemainingBuildTime() > 24 * 5)
				continue;

			int cannon_probe = getAttackDistance(Protoss_Photon_Cannon, c->pos(), Terran_SCV, targetUnitInfo->pos());
			int cannon_scv = getAttackDistance(Protoss_Photon_Cannon, c->pos(), Terran_SCV, unit->getPosition());

			// 내가 포토 사거리 안에 있거나, 적이 포토에게 보호받는 위치이고 내가 가까이 가려는 경우
			if (cannon_scv <= Protoss_Photon_Cannon.groundWeapon().maxRange() ||
					(cannon_probe <= Protoss_Photon_Cannon.groundWeapon().maxRange() && cannon_scv <= Protoss_Photon_Cannon.groundWeapon().maxRange() + 50)) {
				moveBackPostion(INFO.getUnitInfo(unit, S), c->pos(), 2 * TILE_SIZE);
				return nullptr;
			}
		}
	}

	if (!m_targetUnit->exists()) {
		CommandUtil::attackMove(unit, targetUnitInfo->pos());
		return nullptr;
	}

	// 적이 빠른 정찰을 왔다면 맵의 중앙까지 따라갔다가 돌아온다.
	if (m_isEarlyScout) {
		// 적군 베이스와 내 베이스의 거리
		int baseDist;
		// 내 유닛과 내 베이스의 거리
		int unitDist;

		theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), INFO.getMainBaseLocation(E)->Center(), &baseDist);
		theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), unit->getPosition(), &unitDist);

		if (baseDist <= unitDist + unitDist) {
			// 상대봇이 죽거나 시야에서 사라진 경우 본진, 앞마당의 안보이는 시야를 탐색한다.
			if (!ScvManager::Instance().getScanMyBaseUnit())
				return new ScvScanMyBaseState(m_targetUnit);
			else
				return new ScvIdleState();
		}
	}

	// 일반 정찰이라면 세컨드초크포인트까지만 따라갔다가 마린이 나오면 돌아온다.
	else {
		// 마린이 나오면 idle
		if (INFO.getCompletedCount(Terran_Marine, S)) {
			return new ScvIdleState();
		}
		else {
			if (!isSameArea(INFO.getMainBaseLocation(S)->Center(), unit->getPosition())
					&& !isSameArea(INFO.getFirstExpansionLocation(S)->Center(), unit->getPosition())) {
				CommandUtil::attackMove(unit, INFO.getSecondChokePosition(S));
				return nullptr;
			}
		}
	}

	unit->attack(m_targetUnit);
	unit->move(INFO.getUnitInfo(m_targetUnit, E)->vPos());

	return nullptr;
}

State *ScvScanMyBaseState::action() {
	// 최초 호출된 경우 탐험해야 할 포인트 list 저장.
	if (m_targetPos == Positions::None) {
		// 최초 앞마당을 찍는다.
		m_targetPos = INFO.getFirstExpansionLocation(S)->getPosition();

		vector<Position> currentAreaVertices = INFO.getMainBaseLocation(S)->GetArea()->getBoundaryVertices();
		bool finishLookAround = true;

		for (auto v : currentAreaVertices)
			if (!bw->isVisible((TilePosition)v))
				searchList.push_back(v);
	}

	// 적이 맵의 중앙보다 가까운 거리에서 발견되면 다시 쫓아간다.
	if (m_targetUnit != nullptr && m_targetUnit->exists()) {
		// 적군 베이스와 내 베이스의 거리
		int baseDist;
		// 내 유닛과 내 베이스의 거리
		int unitDist;

		theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), INFO.getMainBaseLocation(E)->Center(), &baseDist);
		theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), m_targetUnit->getPosition(), &unitDist);

		if (baseDist > unitDist + unitDist
				&& !(ESM.getEnemyInitialBuild() == Toss_4probes || ESM.getEnemyInitialBuild() == Terran_4scv || ESM.getEnemyInitialBuild() == Zerg_4_Drone_Real || SM.getMainStrategy() != WaitToBase))
		{
			return new ScvScoutUnitDefenceState(m_targetUnit);
		}
	}

	// 시야에 보이는 position 은 삭제한다.
	vector <Position> delVec;

	for (auto v : searchList)
		if (bw->isVisible((TilePosition)v))
			delVec.push_back(v);

	bool checked = false;

	for (auto v : delVec) {
		if (m_targetPos == v)
			checked = true;

		auto del_pos = find_if(searchList.begin(), searchList.end(), [v](Position pos) {
			return pos == v;
		});

		if (del_pos != searchList.end())
			BWEM::utils::fast_erase(searchList, distance(searchList.begin(), del_pos));
	}

	UnitInfo *myUnit = INFO.getUnitInfo(unit, S);

	if (checked || unit->getPosition().getApproxDistance(m_targetPos) < 2 * TILE_SIZE || myUnit->getLastPositionTime() + 3 * 24 < TIME) {
		if (searchList.empty())
			return new ScvIdleState();

		static Position myPosition = m_targetPos;

		// 현재 위치로부터 거리 역순 정렬.
		sort(searchList.begin(), searchList.end(), [](Position a, Position b) {
			return myPosition.getApproxDistance(a) > myPosition.getApproxDistance(b);
		});

		m_targetPos = searchList.back();
	}

	CommandUtil::attackMove(unit, m_targetPos);
	return nullptr;
}

State *ScvEarlyRushDefenseState::action() {
	// 가스러시 방어
	if (m_targetUnit != nullptr && m_targetUnit->getType().isRefinery()) {
		if (!m_targetUnit->exists()) {
			if (bw->isVisible((TilePosition)m_targetPos)) {
				return new ScvIdleState();
			}
			else {
				CommandUtil::move(unit, m_targetPos);
				return nullptr;
			}
		}

		CommandUtil::attackUnit(unit, m_targetUnit);
		return nullptr;
	}

	if (INFO.enemyRace == Races::Protoss) {
		if (ESM.getEnemyInitialBuild() == UnknownBuild || ESM.getEnemyInitialBuild() == Toss_cannon_rush) {
			if (!m_targetUnit->exists()) {
				if (bw->isVisible((TilePosition)m_targetPos)) {
					// targetUnit 이 부서진 경우
					// 주변에 다른 건물이 있으면 공격
					UnitInfo *target = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Photon_Cannon, 5 * TILE_SIZE);

					if (!target)
						target = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Gateway, 5 * TILE_SIZE);

					if (!target)
						target = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Pylon, 5 * TILE_SIZE);

					if (target)
						return new ScvEarlyRushDefenseState(target->unit(), target->pos());
					// 없으면 Idle 로 변경
					else
						return new ScvIdleState();
				}
				// 타겟과 떨어진 경우 가까이로 이동
				else {
					CommandUtil::move(unit, m_targetPos);
					return nullptr;
				}
			}

			// 파일런을 공격하다가 주변에 포토가 생기면 먼저 공격 (게이트 웨이가 생겨도 공격)
			if (m_targetUnit->getType() == Protoss_Pylon) {
				uList eBuildings = INFO.getBuildingsInRadius(E, unit->getPosition(), 5 * TILE_SIZE, true, false, false);

				for (auto eBuilding : eBuildings) {
					if (eBuilding->type() == Protoss_Photon_Cannon || eBuilding->type() == Protoss_Gateway) {
						return new ScvEarlyRushDefenseState(eBuilding->unit(), eBuilding->pos());
					}
				}
			}

			CommandUtil::attackUnit(unit, m_targetUnit);
		}
		else if (ESM.getEnemyInitialBuild() == Toss_1g_forward || ESM.getEnemyInitialBuild() == Toss_2g_forward) {
			if (!m_targetUnit->exists()) {
				if (bw->isVisible((TilePosition)m_targetPos)) {
					// targetUnit 이 부서진 경우
					// 주변에 다른 건물이 있으면 공격
					UnitInfo *target = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Pylon, 10 * TILE_SIZE);

					if (!target)
						target = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Gateway, 10 * TILE_SIZE);

					if (target)
						return new ScvEarlyRushDefenseState(target->unit(), target->pos());
					// 없으면 Idle 로 변경
					else
						return new ScvIdleState();
				}
				// 타겟과 떨어진 경우 가까이로 이동
				else {
					CommandUtil::move(unit, m_targetPos);
					return nullptr;
				}
			}

			// 게이트웨이를 공격하다가 주변에 파일런이 생기면 먼저 공격
			if (m_targetUnit->getType() == Protoss_Gateway) {
				uList eBuildings = INFO.getBuildingsInRadius(E, unit->getPosition(), 10 * TILE_SIZE, true, false, false);

				for (auto eBuilding : eBuildings) {
					if (eBuilding->type() == Protoss_Pylon) {
						return new ScvEarlyRushDefenseState(eBuilding->unit(), eBuilding->pos());
					}
				}
			}

			CommandUtil::attackUnit(unit, m_targetUnit);
		}
	}
	else if (ESM.getEnemyInitialBuild() == Terran_bunker_rush) {
		if (!m_targetUnit->exists()) {
			// targetUnit 이 부서진 경우 Idle 로 변경
			if (bw->isVisible((TilePosition)m_targetPos)) {
				return new ScvIdleState();
			}
			// 타겟과 떨어진 경우 가까이로 이동
			else {
				CommandUtil::move(unit, m_targetPos);
				return nullptr;
			}
		}

		UnitInfo *eUnit = INFO.getUnitInfo(m_targetUnit, E);

		if (eUnit->type() == Terran_Bunker) {
			// 배럭을 찾은 경우 idle 로 변경해서 배럭으로 세팅되게 함.
			if (!INFO.getTypeBuildingsInRadius(Terran_Barracks, E).empty())
				return new ScvIdleState();

			// 배럭을 부순 경우 벙커 공격.
			if (INFO.getDestroyedCount(Terran_Barracks, E)) {
				CommandUtil::attackMove(unit, m_targetPos);
			}
			// 배럭을 안 부순 경우 배럭 찾기
			else {
				Position targetPosition;

				// 확장기지를 가본다.
				if (INFO.getFirstExpansionLocation(S)->GetLastVisitedTime() + 1200 < TIME) {
					CommandUtil::move(unit, INFO.getFirstExpansionLocation(S)->getPosition());
				}
				// 맵 중앙을 가본다.
				else if (!bw->isExplored((TilePosition)theMap.Center())) {
					CommandUtil::move(unit, theMap.Center());
				}
				// 못찾으면 벙커라도 부수자.
				else {
					CommandUtil::attackUnit(unit, m_targetUnit);
				}
			}
		}
		else if (eUnit->type() == Terran_Barracks) {
			// 건물을 짓는 중이면 공격
			if (!eUnit->isComplete() && eUnit->getRemainingBuildTime() > 3 * 24) {
				CommandUtil::attackUnit(unit, m_targetUnit);
				return nullptr;
			}

			// 벙커가 완성되지 않은 경우

			// 벙커가 완성된 경우

			UnitInfo *closest = INFO.getClosestTypeUnit(E, unit->getPosition(), Terran_Marine, m_isNecessary ? 180 : 320);

			if (m_isNecessary) {
				// 적 마린이 공격범위 안이면 공격
				if (closest && Terran_SCV.seekRange() >= getAttackDistance(unit, closest->unit())) {
					CommandUtil::attackUnit(unit, closest->unit());
				}
				else if (unit->getPosition() == m_targetPos) {
					if (unit->isInWeaponRange(m_targetUnit))
						CommandUtil::attackUnit(unit, m_targetUnit);
					else
						CommandUtil::hold(unit);
				}
				else {
					CommandUtil::move(unit, m_targetPos);
				}
			}
			else {
				// 마린이 있는 경우 마린 쫓아가서 공격
				if (closest) {
					// 적군 베이스와 내 베이스의 거리
					int baseDist;
					// 내 유닛과 내 베이스의 거리
					int unitDist;

					theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), INFO.getMainBaseLocation(E)->Center(), &baseDist);
					theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), closest->pos(), &unitDist);

					if (baseDist < unitDist + unitDist) {
						CommandUtil::attackUnit(unit, m_targetUnit);
					}
					else {
						unit->attack(closest->unit());
						unit->move(closest->vPos());
					}
				}
				else {
					// 체력이 너무 낮은 SCV 가 있으면 수리 없으면 공격
					uList unitList = INFO.getTypeUnitsInRadius(Terran_SCV, S, unit->getPosition(), 3 * TILE_SIZE);

					for (auto u : unitList) {
						// 체력이 60% 미만이면 수리한다.
						if (u->id() != unit->getID() && u->hp() * 100 <= 60 * Terran_SCV.maxHitPoints()) {
							CommandUtil::repair(unit, u->unit());

							return nullptr;
						}
					}

					CommandUtil::attackUnit(unit, m_targetUnit);
				}
			}
		}
	}
	else if (ESM.getEnemyInitialBuild() == Terran_1b_forward || ESM.getEnemyInitialBuild() == Terran_2b_forward) {
		// TODO 맞벙커 로직
		UnitInfo *closest = INFO.getClosestTypeUnit(E, unit->getPosition(), Terran_Marine, 1600);

		if (closest) {
			unit->attack(closest->unit());
			unit->move(closest->vPos());
		}
		else {
			if (unit->isMoving())
				return nullptr;
			else
				return new ScvIdleState();
		}
	}
	else if (ESM.getEnemyInitialBuild() == Zerg_sunken_rush) {
		if (!m_targetUnit->exists()) {
			if (bw->isVisible((TilePosition)m_targetPos)) {
				// targetUnit 이 부서진 경우
				// 주변에 다른 유닛이 있으면 공격
				UnitInfo *target = INFO.getClosestTypeUnit(E, unit->getPosition(), Zerg_Zergling, 5 * TILE_SIZE);

				if (!target)
					target = INFO.getClosestTypeUnit(E, unit->getPosition(), Zerg_Drone, 5 * TILE_SIZE);

				if (target)
					return new ScvEarlyRushDefenseState(target->unit(), target->pos());
				// 없으면 Idle 로 변경
				else
					return new ScvIdleState();
			}
			// 타겟과 떨어진 경우 가까이로 이동
			else {
				CommandUtil::move(unit, m_targetPos);
				return nullptr;
			}
		}

		CommandUtil::attackUnit(unit, m_targetUnit);
	}

	// 전략이 바뀐 경우 target이 없어지면 idle 로 바꿔준다.
	if (m_targetUnit == nullptr || !m_targetUnit->exists()) {
		return new ScvIdleState();
	}

	return nullptr;
}

State *ScvWorkerCombatState::action()
{
	int radius = 0;

	if (INFO.enemyRace == Races::Terran)
	{
		radius = 27 * TILE_SIZE;
	}
	else
	{
		radius = 50 * TILE_SIZE;
	}

	if (targetUnit != nullptr && !targetUnit->exists())
		targetUnit = nullptr;

	if (repairTarget != nullptr && !repairTarget->exists())
		repairTarget = nullptr;

	UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), GroundUnitKind, radius, true);
	UnitInfo *closestMyScv = nullptr;// INFO.getClosestTypeUnit(S, unit, Terran_SCV, 1 * TILE_SIZE);

	int dist = INT_MAX;

	for (auto &s : INFO.getTypeUnitsInRadius(Terran_SCV, S, unit->getPosition(), 1 * TILE_SIZE))
	{
		if ( s->unit()->isUnderAttack() && s->pos().getDistance(unit->getPosition()) < dist) {
			closestMyScv = s;
			dist = s->pos().getApproxDistance(unit->getPosition());
		}
	}

	UnitInfo *weakTarget = getGroundWeakTargetInRange(INFO.getUnitInfo(unit, S), true);

	if (weakTarget != nullptr)
	{
		targetUnit = weakTarget->unit();
	}

	if (closeUnit != nullptr)
	{
		if (targetUnit == nullptr || (targetUnit->getPosition().getDistance(unit->getPosition()) - closeUnit->pos().getDistance(unit->getPosition()) > 0.5 * TILE_SIZE)
				|| (targetUnit->getHitPoints() > closeUnit->hp()))
		{
			targetUnit = closeUnit->unit();
		}
	}

	if (closestMyScv != nullptr)
	{
		if (repairTarget == nullptr)
		{
			repairTarget = closestMyScv->unit();
		}
		else
		{
			if (repairTarget->getHitPoints() == 60 && !repairTarget->isUnderAttack())
			{
				repairTarget = closestMyScv->unit();
			}
			else if (repairTarget->getDistance(unit) > 1 * TILE_SIZE)
			{
				repairTarget = closestMyScv->unit();
			}
		}
	}

	if (INFO.enemyRace == Races::Terran)
	{

		if (targetUnit != nullptr)
		{
			Position firstChoke = (Position) INFO.getFirstChokePoint(S)->Center();
			int targetDist = targetUnit->getDistance(unit);

			// 적이 나와 3타일 이상이고, 내가 적보다 초크에 더 가까우면 초크로 공격
			if (targetDist > 3 * TILE_SIZE && targetDist > unit->getDistance(firstChoke))
			{
				if (unit->isSelected())
					cout << unit->getID() << "번 SCV" << targetUnit->getID() << "번 타겟 " << "1 attackMove" << endl;

				CommandUtil::attackMove(unit, firstChoke);
				return nullptr;
			}
			//적이 나와
			else if (targetDist > 3 * TILE_SIZE && isSameArea(targetUnit->getPosition(), MYBASE))
			{
				// 대기포지션
				Position waitingPos = Positions::None;
				const Base *base = INFO.getMainBaseLocation(S);
				Unit firstM = base->getEdgeMinerals().first;
				Unit secondM = base->getEdgeMinerals().second;

				if (firstM != nullptr && secondM != nullptr)
				{
					waitingPos = (firstM->getInitialPosition() + secondM->getInitialPosition()) / 2;

					waitingPos = (waitingPos * 2 + firstChoke) / 3;

					bw->drawCircleMap(waitingPos, 5, Colors::Green, true);

					for (int i = 0; i < 5; i++)
					{
						waitingPos = getDirectionDistancePosition(waitingPos, targetUnit->getPosition(), i * TILE_SIZE);

						if (waitingPos.isValid() && bw->isWalkable((WalkPosition)waitingPos)
								&& isSameArea(MYBASE, waitingPos)) break;
					}

					if (waitingPos.isValid())
					{
						bw->drawCircleMap(waitingPos, 5, Colors::Purple, true);


						if (ScvManager::Instance().startWaitingTimeForWorkerCombat == 0) {
							ScvManager::Instance().startWaitingTimeForWorkerCombat = TIME;
						}

						if (TIME - ScvManager::Instance().startWaitingTimeForWorkerCombat < 24 * 10)
						{
							if (unit->isSelected())
								cout << unit->getID() << "번 SCV" << targetUnit->getID() << "번 타겟 " << "waitingPosition으로 TIME:" << ScvManager::Instance().startWaitingTimeForWorkerCombat << endl;

							CommandUtil::move(unit, waitingPos);
							return nullptr;
						}
					}
				}

			}


			if (targetUnit->getPosition().getDistance(unit->getPosition()) > 1 * TILE_SIZE && repairTarget != nullptr && (repairTarget->isUnderAttack() || repairTarget->getHitPoints() < Terran_SCV.maxHitPoints())
					&& INFO.getUnitInfo(unit, S)->getEnemiesTargetMe().empty() && repairTarget->getPosition().getDistance(unit->getPosition()) <= 1 * TILE_SIZE
					&& targetUnit->getPosition().getApproxDistance(firstChoke) < 5 * TILE_SIZE)
			{
				if (unit->isSelected())
					cout << unit->getID() << "번 SCV" << repairTarget->getID() << "번 타겟 " << "3 repair" << endl;

				//cout << unit->getID() << ": SCV Repair:" << repairTarget->getID() << endl;
				CommandUtil::rightClick(unit, repairTarget, true);
			}
			else
			{
				if (unit->isSelected())
					cout << unit->getID() << "번 SCV" << targetUnit->getID() << "번 타겟 " << "4 attack" << endl;

				CommandUtil::attackUnit(unit, targetUnit);
			}
		}
		else
		{
			return new ScvIdleState();
		}
	}
	else //테란 이외의 종족일때
	{
		if (targetUnit != nullptr)
		{
			if (unit->getGroundWeaponCooldown() == 0)
			{
				unit->attack(targetUnit);
			}
			else
			{
				if (INFO.getUnitInfo(targetUnit, S) != nullptr)
				{
					unit->move(INFO.getUnitInfo(targetUnit, E)->vPos());
				}
			}
		}
		else
		{
			return new ScvIdleState();
		}
	}

	return nullptr;
}
