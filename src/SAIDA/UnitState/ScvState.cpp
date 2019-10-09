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
		else if (!unit->isCarryingGas() && !unit->isCarryingMinerals()) // �ݳ��Ϸ�
		{
			if (unit->isSelected())
			{
				cout << "����?:" << started << "/������?:" << isBeingRepaired(unit) << "/�Ÿ�?:" << unit->getPosition().getDistance(healerScv->getPosition()) << "/����?" << (TIME - repairStartTime) / 24 << "��" << endl;
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
				//cout << unit->getID() << ":����SCV�Ҵ�.." << endl;
				healerScv = ScvManager::Instance().chooseRepairScvforScv(unit, 6 * TILE_SIZE);

				if (healerScv != nullptr)
				{
					UnitCommand currentCommand(unit->getLastCommand());

					if (unit->canReturnCargo() && (currentCommand.getType() != UnitCommandTypes::Return_Cargo))
						unit->returnCargo();
					else if (!unit->isCarryingGas() && !unit->isCarryingMinerals()) // �ݳ��Ϸ�
						unit->move(waitingPosition);

					//unit->rightClick(healerScv);

					return nullptr;
				}
			}
		}
	}

	if (unit->isUnderAttack() || !INFO.getUnitInfo(unit, S)->getEnemiesTargetMe().empty())
	{
		// Ŀ�ǵ� ��ó�� ���� �ִ� ��쿡��, �� �ֺ��� ���� ���� �ָ� �ִ� �̳׶��� ��� �����Ѵ�.
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

	if (unit->isGatheringMinerals() == false) // Mineral�� ĳ�� ���� ����.
	{
		UnitCommand currentCommand(unit->getLastCommand());

		if (unit->canReturnCargo() && (currentCommand.getType() != UnitCommandTypes::Return_Cargo)) // ��� �ִ°� ����.
		{
			unit->returnCargo(); // ��ȯ
		}
		else
		{
			if (!unit->isCarryingGas() && !unit->isCarryingMinerals()) // �ݳ��Ϸ�
				CommandUtil::gather(unit, m_targetUnit);
		}

		return nullptr;
	}

	// 2���� �̻� �پ��������� �̳׶� �� ����.
	if (ScvManager::Instance().getMineralScvCount(m_targetUnit) < 2)
		return nullptr;

	// Skip Cnt 3 sec
	if (skip_cnt) {
		skip_cnt--;
		return nullptr;
	}

	// Mineral ��� �ִ� ��� skip
	if (unit->isCarryingMinerals()) {
		pre_carry = true;
		return nullptr;
	}

	// Mineral Carring -> no Carring ������ Mineral ����
	if (pre_carry)
	{
		unit->rightClick(m_targetUnit);
		pre_carry = false;
	}

	int Dist = unit->getPosition().getApproxDistance(m_targetUnit->getPosition());

	// �Ÿ��� �հ�� �׳� skip
	if (Dist > THR_CHECK)
		return nullptr;

	// �Ÿ��� THR_CHECK ������ ���� ��� ��ǥ Mineral is mining Check
	if (Dist > THR_MINERAL)
	{
		pre_gathered = m_targetUnit->isBeingGathered();
		return nullptr;
	}

	// ������� ������ SCV�� �ش� Mineral�� �پ� �ִ°Ŷ� ���� ����.
	// ���� ���� ������ �̹� ĳ���� �־���(�ٸ�scv��) ���ݵ� ĳ���� �ִ����̸� ����ؼ� ��Ŭ�� �Ѵ�.
	if (pre_gathered && m_targetUnit->isBeingGathered())
	{
		unit->rightClick(m_targetUnit);
		return nullptr;
	}

	// ���� ĳ�� �ʰ� �ִٸ� ��Ŭ�� �ϰ� 3�� ����.
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
	if (m_targetUnit->exists() == false || m_targetUnit->isLoaded() || S->minerals() == 0) // Unit�� ���������� idle (����ó��)
		return new ScvIdleState();

	if (!m_targetUnit->getType().isBuilding() && !isInMyArea(m_targetUnit->getPosition(), true)) //�� ������ ���� ��� idle(�������� ������ ������ �������)
	{
		return new ScvIdleState();
	}

	if (((m_targetUnit->getHitPoints() * 100) / m_targetUnit->getType().maxHitPoints()) >= Threshold)
	{
		if (m_targetUnit->getType() == Terran_Missile_Turret)
		{
			if (INFO.enemyRace == Races::Zerg)// ��Ż�� ���ų� �񸮾��� �������� Idle��
			{
				if (INFO.getTypeUnitsInRadius(Zerg_Mutalisk, E, m_targetUnit->getPosition(), 8 * TILE_SIZE).size() == 0 ||
						INFO.getTypeUnitsInRadius(Terran_Goliath, S, m_targetUnit->getPosition(), 8 * TILE_SIZE).size() > 4)
					return new ScvIdleState();
			}
			else if (INFO.enemyRace == Races::Protoss) // ������ ���� ������ // �� �ܿ��� �׳�
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

			// �����ϴ� ������ ��� ������ ����
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

// �� ������ �ʹݿ� �����̳� �ո��翡 ���ͼ� ��Ŀ�� ����ؾ��ϴ� ��Ȳ.
//State *ScvBunkerDefenceState::action()
//{
//	// ���� �ϼ��� �ȵǼ� �� ���Ѿ� �ϴ� ��Ȳ
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
				// �跰�� 50% �� �Ǳ� ���� ������ �Դٸ� �̸� ������.
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

	// ��뺿�� ���� ��� ����, �ո����� �Ⱥ��̴� �þ߸� Ž���Ѵ�.
	if (!targetUnitInfo) {
		if (!ScvManager::Instance().getScanMyBaseUnit() && m_isEarlyScout)
			return new ScvScanMyBaseState();
		else
			return new ScvIdleState();
	}

	// ��뺿�� �þ߿��� ����� ��� ����� �������� �̵� ��, ����, �ո����� �Ⱥ��̴� �þ߸� Ž���Ѵ�.
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

			// ���� ���� ��Ÿ� �ȿ� �ְų�, ���� ���信�� ��ȣ�޴� ��ġ�̰� ���� ������ ������ ���
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

	// ���� ���� ������ �Դٸ� ���� �߾ӱ��� ���󰬴ٰ� ���ƿ´�.
	if (m_isEarlyScout) {
		// ���� ���̽��� �� ���̽��� �Ÿ�
		int baseDist;
		// �� ���ְ� �� ���̽��� �Ÿ�
		int unitDist;

		theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), INFO.getMainBaseLocation(E)->Center(), &baseDist);
		theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), unit->getPosition(), &unitDist);

		if (baseDist <= unitDist + unitDist) {
			// ��뺿�� �װų� �þ߿��� ����� ��� ����, �ո����� �Ⱥ��̴� �þ߸� Ž���Ѵ�.
			if (!ScvManager::Instance().getScanMyBaseUnit())
				return new ScvScanMyBaseState(m_targetUnit);
			else
				return new ScvIdleState();
		}
	}

	// �Ϲ� �����̶�� ��������ũ����Ʈ������ ���󰬴ٰ� ������ ������ ���ƿ´�.
	else {
		// ������ ������ idle
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
	// ���� ȣ��� ��� Ž���ؾ� �� ����Ʈ list ����.
	if (m_targetPos == Positions::None) {
		// ���� �ո����� ��´�.
		m_targetPos = INFO.getFirstExpansionLocation(S)->getPosition();

		vector<Position> currentAreaVertices = INFO.getMainBaseLocation(S)->GetArea()->getBoundaryVertices();
		bool finishLookAround = true;

		for (auto v : currentAreaVertices)
			if (!bw->isVisible((TilePosition)v))
				searchList.push_back(v);
	}

	// ���� ���� �߾Ӻ��� ����� �Ÿ����� �߰ߵǸ� �ٽ� �Ѿư���.
	if (m_targetUnit != nullptr && m_targetUnit->exists()) {
		// ���� ���̽��� �� ���̽��� �Ÿ�
		int baseDist;
		// �� ���ְ� �� ���̽��� �Ÿ�
		int unitDist;

		theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), INFO.getMainBaseLocation(E)->Center(), &baseDist);
		theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), m_targetUnit->getPosition(), &unitDist);

		if (baseDist > unitDist + unitDist
				&& !(ESM.getEnemyInitialBuild() == Toss_4probes || ESM.getEnemyInitialBuild() == Terran_4scv || ESM.getEnemyInitialBuild() == Zerg_4_Drone_Real || SM.getMainStrategy() != WaitToBase))
		{
			return new ScvScoutUnitDefenceState(m_targetUnit);
		}
	}

	// �þ߿� ���̴� position �� �����Ѵ�.
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

		// ���� ��ġ�κ��� �Ÿ� ���� ����.
		sort(searchList.begin(), searchList.end(), [](Position a, Position b) {
			return myPosition.getApproxDistance(a) > myPosition.getApproxDistance(b);
		});

		m_targetPos = searchList.back();
	}

	CommandUtil::attackMove(unit, m_targetPos);
	return nullptr;
}

State *ScvEarlyRushDefenseState::action() {
	// �������� ���
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
					// targetUnit �� �μ��� ���
					// �ֺ��� �ٸ� �ǹ��� ������ ����
					UnitInfo *target = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Photon_Cannon, 5 * TILE_SIZE);

					if (!target)
						target = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Gateway, 5 * TILE_SIZE);

					if (!target)
						target = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Pylon, 5 * TILE_SIZE);

					if (target)
						return new ScvEarlyRushDefenseState(target->unit(), target->pos());
					// ������ Idle �� ����
					else
						return new ScvIdleState();
				}
				// Ÿ�ٰ� ������ ��� �����̷� �̵�
				else {
					CommandUtil::move(unit, m_targetPos);
					return nullptr;
				}
			}

			// ���Ϸ��� �����ϴٰ� �ֺ��� ���䰡 ����� ���� ���� (����Ʈ ���̰� ���ܵ� ����)
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
					// targetUnit �� �μ��� ���
					// �ֺ��� �ٸ� �ǹ��� ������ ����
					UnitInfo *target = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Pylon, 10 * TILE_SIZE);

					if (!target)
						target = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Gateway, 10 * TILE_SIZE);

					if (target)
						return new ScvEarlyRushDefenseState(target->unit(), target->pos());
					// ������ Idle �� ����
					else
						return new ScvIdleState();
				}
				// Ÿ�ٰ� ������ ��� �����̷� �̵�
				else {
					CommandUtil::move(unit, m_targetPos);
					return nullptr;
				}
			}

			// ����Ʈ���̸� �����ϴٰ� �ֺ��� ���Ϸ��� ����� ���� ����
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
			// targetUnit �� �μ��� ��� Idle �� ����
			if (bw->isVisible((TilePosition)m_targetPos)) {
				return new ScvIdleState();
			}
			// Ÿ�ٰ� ������ ��� �����̷� �̵�
			else {
				CommandUtil::move(unit, m_targetPos);
				return nullptr;
			}
		}

		UnitInfo *eUnit = INFO.getUnitInfo(m_targetUnit, E);

		if (eUnit->type() == Terran_Bunker) {
			// �跰�� ã�� ��� idle �� �����ؼ� �跰���� ���õǰ� ��.
			if (!INFO.getTypeBuildingsInRadius(Terran_Barracks, E).empty())
				return new ScvIdleState();

			// �跰�� �μ� ��� ��Ŀ ����.
			if (INFO.getDestroyedCount(Terran_Barracks, E)) {
				CommandUtil::attackMove(unit, m_targetPos);
			}
			// �跰�� �� �μ� ��� �跰 ã��
			else {
				Position targetPosition;

				// Ȯ������� ������.
				if (INFO.getFirstExpansionLocation(S)->GetLastVisitedTime() + 1200 < TIME) {
					CommandUtil::move(unit, INFO.getFirstExpansionLocation(S)->getPosition());
				}
				// �� �߾��� ������.
				else if (!bw->isExplored((TilePosition)theMap.Center())) {
					CommandUtil::move(unit, theMap.Center());
				}
				// ��ã���� ��Ŀ�� �μ���.
				else {
					CommandUtil::attackUnit(unit, m_targetUnit);
				}
			}
		}
		else if (eUnit->type() == Terran_Barracks) {
			// �ǹ��� ���� ���̸� ����
			if (!eUnit->isComplete() && eUnit->getRemainingBuildTime() > 3 * 24) {
				CommandUtil::attackUnit(unit, m_targetUnit);
				return nullptr;
			}

			// ��Ŀ�� �ϼ����� ���� ���

			// ��Ŀ�� �ϼ��� ���

			UnitInfo *closest = INFO.getClosestTypeUnit(E, unit->getPosition(), Terran_Marine, m_isNecessary ? 180 : 320);

			if (m_isNecessary) {
				// �� ������ ���ݹ��� ���̸� ����
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
				// ������ �ִ� ��� ���� �Ѿư��� ����
				if (closest) {
					// ���� ���̽��� �� ���̽��� �Ÿ�
					int baseDist;
					// �� ���ְ� �� ���̽��� �Ÿ�
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
					// ü���� �ʹ� ���� SCV �� ������ ���� ������ ����
					uList unitList = INFO.getTypeUnitsInRadius(Terran_SCV, S, unit->getPosition(), 3 * TILE_SIZE);

					for (auto u : unitList) {
						// ü���� 60% �̸��̸� �����Ѵ�.
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
		// TODO �º�Ŀ ����
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
				// targetUnit �� �μ��� ���
				// �ֺ��� �ٸ� ������ ������ ����
				UnitInfo *target = INFO.getClosestTypeUnit(E, unit->getPosition(), Zerg_Zergling, 5 * TILE_SIZE);

				if (!target)
					target = INFO.getClosestTypeUnit(E, unit->getPosition(), Zerg_Drone, 5 * TILE_SIZE);

				if (target)
					return new ScvEarlyRushDefenseState(target->unit(), target->pos());
				// ������ Idle �� ����
				else
					return new ScvIdleState();
			}
			// Ÿ�ٰ� ������ ��� �����̷� �̵�
			else {
				CommandUtil::move(unit, m_targetPos);
				return nullptr;
			}
		}

		CommandUtil::attackUnit(unit, m_targetUnit);
	}

	// ������ �ٲ� ��� target�� �������� idle �� �ٲ��ش�.
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

			// ���� ���� 3Ÿ�� �̻��̰�, ���� ������ ��ũ�� �� ������ ��ũ�� ����
			if (targetDist > 3 * TILE_SIZE && targetDist > unit->getDistance(firstChoke))
			{
				if (unit->isSelected())
					cout << unit->getID() << "�� SCV" << targetUnit->getID() << "�� Ÿ�� " << "1 attackMove" << endl;

				CommandUtil::attackMove(unit, firstChoke);
				return nullptr;
			}
			//���� ����
			else if (targetDist > 3 * TILE_SIZE && isSameArea(targetUnit->getPosition(), MYBASE))
			{
				// ���������
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
								cout << unit->getID() << "�� SCV" << targetUnit->getID() << "�� Ÿ�� " << "waitingPosition���� TIME:" << ScvManager::Instance().startWaitingTimeForWorkerCombat << endl;

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
					cout << unit->getID() << "�� SCV" << repairTarget->getID() << "�� Ÿ�� " << "3 repair" << endl;

				//cout << unit->getID() << ": SCV Repair:" << repairTarget->getID() << endl;
				CommandUtil::rightClick(unit, repairTarget, true);
			}
			else
			{
				if (unit->isSelected())
					cout << unit->getID() << "�� SCV" << targetUnit->getID() << "�� Ÿ�� " << "4 attack" << endl;

				CommandUtil::attackUnit(unit, targetUnit);
			}
		}
		else
		{
			return new ScvIdleState();
		}
	}
	else //�׶� �̿��� �����϶�
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
