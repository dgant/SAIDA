#include "MarineManager.h"
#include "../InformationManager.h"
#include "../StrategyManager.h"
#include "../EnemyStrategyManager.h"

using namespace MyBot;

MarineManager::MarineManager()
{
	getFirstDefensePos();
	firstBarrack = nullptr;
	nextBarrackSupply = nullptr;
	bunker = nullptr;
	zealotDefenceMarine = nullptr;
	rangeUnitNearBunker = false;

	Position pos = (Position)INFO.getSecondAverageChokePosition(S);

	uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

	if (!commandCenterList.empty()) {
		Unit mineral = bw->getClosestUnit(commandCenterList[0]->pos(), Filter::IsMineralField);

		if (mineral != nullptr)
			pos = (commandCenterList[0]->pos() + mineral->getPosition()) / 2;
	}

	waitingNearCommand = pos;
}

MarineManager &MarineManager::Instance()
{
	static MarineManager instance;
	return instance;
}

void MarineManager::update()
{
	if (TIME < 300) return;

	if (TIME % 2 != 0) return;

	uList marineList = INFO.getUnits(Terran_Marine, S);

	if (marineList.empty()) return;

	setFirstBunker();

	if (SM.getMainStrategy() != AttackAll)
	{
		if (INFO.enemyRace == Races::Protoss)
		{
			setZealotDefenceMarine(marineList);
		}

		//if (INFO.getCompletedCount(Terran_Factory, S) == 0)

		//setFirstChokeDefenceMarine(marineList); // new or Idle�� ���ֻ��¸� ����
		setDefenceMarine(marineList); // new or Idle�� ���ֻ��¸� ����

		for (auto m : marineList)
		{
			string state = m->getState();

			if (state == "New" && m->isComplete()) {
				m->setState(new MarineIdleState());
				m->action();
			}

			if (state == "Idle")
			{
				m->action();
			}
			else if (state == "Defence") {
				m->action();
			}
			else if (state == "Kiting")
			{
				m->action();
			}
			else if (state == "GoGoGo")
			{
				m->setState(new MarineIdleState());
				m->action();
			}
			else if (state == "KillScouter")
			{
				if (enemyWorkersInMyYard.size() > 0)
					m->action(enemyWorkersInMyYard.front()->pos());
				else if (enemyBuildingsInMyYard.size() > 0)
					m->action(enemyBuildingsInMyYard.front()->pos());
				else
					m->action(lastScouterPosition);
			}
			else if (state == "ZealotDefence") {
				m->action();
			}
			else {
				m->action();
			}

		}
	}
	else if (SM.getMainStrategy() == AttackAll)
	{
		if (INFO.enemyRace == Races::Protoss)
		{
			for (auto m : marineList)
			{
				if (m->getState() != "GoGoGo")
				{
					m->setState(new MarineGoGoGoState());
				}

				m->action();
			}

			for (auto &bunker : INFO.getBuildings(Terran_Bunker, S))
			{
				if (!bunker->unit()->isUnderAttack() && bunker->unit()->getLoadedUnits().size() > 0)
				{
					bunker->unit()->unloadAll();
				}
			}
		}
	}
}

void MarineManager::setFirstBunker()
{
	uList bunkers = INFO.getBuildings(Terran_Bunker, S);

	if (bunkers.size() == 0) {
		bunker = nullptr;
	}
	else {
		if (bunkers.size() == 1)
		{
			bunker = bunkers.at(0)->unit();
			checkRangeUnitNearBunker();
		}
		else // 2��
		{
			for (auto b : bunkers) {
				if (isSameArea(b->pos(), INFO.getFirstExpansionLocation(S)->Center()))
				{
					bunker = b->unit();
					checkRangeUnitNearBunker();
					break;
				}
			}
		}
	}

}

bool MarineManager::isZealotDefenceNeed()
{
	bool zealotDefenceNeed = false;
	uList enemyList = getEnemyInMyYard(1300, Protoss_Zealot, false);
	uList enemyList1 = getEnemyInMyYard(1300, Protoss_Dragoon);
	uList enemyList2 = getEnemyInMyYard(1300, Protoss_Scout);

	if (enemyList.size() != 0 && enemyList1.empty() && enemyList2.empty()) {
		zealotDefenceNeed = true;
	}

	return zealotDefenceNeed;
}

// �⺻ ��� State �̹Ƿ�, Ư���� State�� �ʿ��� ���, �� �Լ�ȣ�⺸�� ���� �ض�.
void MarineManager::setDefenceMarine(uList &marineList) {
	if (ESM.getEnemyInitialBuild() == Toss_1g_forward || ESM.getEnemyInitialBuild() == Toss_2g_forward) {
		if (INFO.getCompletedCount(Protoss_Zealot, E) <= 1 && INFO.getDestroyedCount(Protoss_Zealot, E) > 0
				&& INFO.getCompletedCount(Protoss_Dragoon, E) == 0) {
			UnitInfo *gateway = INFO.getClosestTypeUnit(E, MYBASE, Protoss_Gateway, 0, true, true);

			if (gateway) {
				if (INFO.getSecondChokePosition(S).getApproxDistance(theMap.Center()) + 10 * TILE_SIZE
						> INFO.getSecondChokePosition(S).getApproxDistance(gateway->pos())) {
					for (auto m : marineList)
					{
						m->setState(new MarineAttackState(gateway));
					}

					return;
				}
			}
		}
	}

	enemyWorkersInMyYard = getEnemyInMyYard(1700, INFO.getWorkerType(INFO.enemyRace));
	enemyBuildingsInMyYard = getEnemyInMyYard(1700, UnitTypes::Buildings);
	enemyInMyYard = getEnemyInMyYard(1700, Men, false);

	int totEnemNum = enemyInMyYard.size() + enemyBuildingsInMyYard.size();

	// �ϲ� ���� �ƹ��͵� ���� ���
	if (totEnemNum == 0 && (TIME - lastSawScouterTime > 24 * 5)) {
		for (auto m : marineList)
		{
			if (m->getState() == "Defence" || m->getState() == "KillScouter")
				m->setState(new MarineIdleState());
		}
	}
	// �ϲ� Ȥ�� ���� �ִ� ���
	else {
		string state;

		int enemBuildingNum = enemyBuildingsInMyYard.size();
		int enemWorkerNum = enemyWorkersInMyYard.size();

		if (enemWorkerNum > 0 || enemBuildingNum > 0)
		{
			lastSawScouterTime = TIME;

			if (enemWorkerNum > 0)
				if (enemyWorkersInMyYard.front()->vPos() != Positions::None)
					lastScouterPosition = enemyWorkersInMyYard.front()->vPos();
				else
					lastScouterPosition = enemyWorkersInMyYard.front()->pos();
			else
				lastScouterPosition = enemyBuildingsInMyYard.front()->pos();
		}

		bool goKillScouter = true;

		if (totEnemNum > 0 && totEnemNum - enemWorkerNum - enemBuildingNum == 0 && ESM.getEnemyInitialBuild() != Terran_bunker_rush) {
			for (auto eBuilding : enemyBuildingsInMyYard) {
				if (eBuilding->type().groundWeapon().targetsGround()) {
					goKillScouter = false;
					break;
				}
			}
		}
		else {
			goKillScouter = false;
		}

		for (auto m : marineList)
		{
			state = m->getState();

			if (state == "New" || state == "Idle" || state == "KillScouter" || state == "Defence") {
				//�ϲ� �ܿ� �ٸ� �� ������ ���� ��
				if (ESM.getEnemyInitialBuild() == Toss_cannon_rush)
					m->setState(new MarineAttackState(INFO.getSecondChokePosition(S)));
				else if (goKillScouter) {
					Unit b = getBunker();

					if ((b && m == marineList.at(0)) || !b)
						m->setState(new MarineKillScouterState());
				}
				else
					m->setState(new MarineDefenceState());
			}
		}
	}

	return;
}

// �跰�� �ٷ� �پ��ִ� ���ö��̸� ã�� ����.
// �ı��ǰų� ���߿� �� ä�� �̵��ϰų�, �ٽ� ���� �� �ֱ� ������ �Ź� �ٽ� ������
bool MarineManager::findFirstBarrackAndSupplyPosition()
{
	nextBarrackSupply = nullptr;

	for (auto b : INFO.getBuildings(Terran_Barracks, S)) {
		if (!b->unit()->isLifted()) {

			TilePosition barrackTile = b->unit()->getTilePosition();

			for (auto &u : Broodwar->getUnitsOnTile(TilePosition(barrackTile.x + 4, barrackTile.y + 1))) {
				if (u->getType() == Terran_Supply_Depot) {
					nextBarrackSupply = u;
					break;
				}
			}

		}

		firstBarrack = b->unit();
	}

	if (firstBarrack == nullptr || nextBarrackSupply == nullptr)
		return false;

	return true;
}
//
//void MarineManager::checkKitingMarine(UnitInfo *&m)
//{
//	Unit b = MarineManager::Instance().getBunker();
//
//	if (b != nullptr && b->isCompleted())
//	{
//		m->setState(new MarineDefenceState());
//		return;
//	}
//
//	UnitInfo *closeUnit = INFO.getClosestUnit(E, m->pos(), AllUnitKind);
//
//	int distanceFromMainBase = 0;
//
//	theMap.GetPath(m->pos(), INFO.getMainBaseLocation(S)->getPosition(), &distanceFromMainBase);
//
//	// �������� �ʹ� �ָ��� ������ DefenceMode�� ��ȯ
//	if (distanceFromMainBase >= 1600) {
//		m->setState(new MarineDefenceState());
//	}
//
//	// ��Ŀ�� �ϼ��Ǿ� �ְų� ��ó ������ ������ Defence Mode�� ��ȯ
//	if ((b != nullptr && b->isCompleted()) || closeUnit == nullptr) {
//		m->setState(new MarineDefenceState());
//	}
//
//	return;
//}

void MarineManager::setZealotDefenceMarine(uList &marineList)
{
	bool zealotDefenceMarineNeed = isZealotDefenceNeed();
	bool barrarckAndSupplyFound = findFirstBarrackAndSupplyPosition();

	if (zealotDefenceMarineNeed && barrarckAndSupplyFound)
	{
		// zealotDefenceMarine �Ҵ� �۾�
		if (zealotDefenceMarine == nullptr) {
			// ���������� ������ ���� ���� �� �ϳ��� �������� ����.
			// ���� ���⼭ ������ �ʾƵ� �Ǵµ�, �ڵ� �������� ���ؼ� ���⼭ ������.
			for (auto m : marineList) {
				string state = m->getState();

				if (state == "Idle" || state == "Defence" || state == "Kiting" || state == "KillScouter" || state == "FirstChokeDefence") {
					m->setState(new MarineZealotDefenceState());
					zealotDefenceMarine = m->unit();
					break;
				}
			}
		}
	}
	else {
		// zealotDefenceMarine �Ҵ� ����
		if (zealotDefenceMarine != nullptr) {
			UnitInfo *marine = INFO.getUnitInfo(zealotDefenceMarine, S);

			if (marine != nullptr && marine->getState() == "ZealotDefence") {
				marine->setState(new MarineDefenceState());
			}

			zealotDefenceMarine = nullptr;
		}
	}

	return;
}

void MarineManager::onUnitDestroyed(Unit u)
{
	if (!u->isCompleted() || u == nullptr)
		return;

	UnitInfo *marine = INFO.getUnitInfo(u, S);

	if (marine == nullptr)
		return;

	if (marine->getState() == "ZealotDefence") {
		zealotDefenceMarine = nullptr;
	}

}

void MarineManager::getFirstDefensePos()
{
	WalkPosition cpEnd1 = theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition())->ChokePoints().at(0)->Pos(BWEM::ChokePoint::end1);
	WalkPosition cpMiddle = theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition())->ChokePoints().at(0)->Pos(BWEM::ChokePoint::middle);
	Position mybase = INFO.getMainBaseLocation(S)->getPosition();

	Position cpEnd1Pos = Position(cpEnd1);
	Position cpMiddlePos = Position(cpMiddle);

	int x = cpEnd1Pos.x - cpMiddlePos.x;
	int y = cpEnd1Pos.y - cpMiddlePos.y;

	double m = (double)y / (double)x;
	double nm = -1 * (1 / m); // y = nm*x

	Position pos1(cpMiddlePos.x - 100, cpMiddlePos.y - (int)(nm * 100));
	Position pos2(cpMiddlePos.x + 100, cpMiddlePos.y + (int)(nm * 100));

	int defence_gap = 15;

	if (pos1.getDistance(mybase) < pos2.getDistance(mybase)) // pos 1 ���� �����̸�
		FirstDefensePos = Position(cpMiddlePos.x + defence_gap, cpMiddlePos.y + (int)(nm * defence_gap));
	else
		FirstDefensePos = Position(cpMiddlePos.x - defence_gap, cpMiddlePos.y - (int)(nm * defence_gap));
}

bool MarineManager::checkZerglingDefenceNeed()
{
	Position myMainBasePosition = INFO.getMainBaseLocation(S)->getPosition();
	bool isEnemyZerglingExistNearMyBase = false;

	uList zerglingList = INFO.getUnits(Zerg_Zergling, E);

	for (auto z : zerglingList)
	{
		if (myMainBasePosition.getDistance(z->pos()) < 15 * TILE_SIZE)
		{
			isEnemyZerglingExistNearMyBase = true;
			break;
		}
	}

	return isEnemyZerglingExistNearMyBase;
}

void MarineManager::checkRangeUnitNearBunker()
{
	rangeUnitNearBunker = false;

	if (bunker == nullptr) {
		return;
	}

	uList units = INFO.getUnitsInRadius(E, bunker->getPosition(), 12 * TILE_SIZE);

	for (auto u : units) {

		if (u->getLift()) {
			rangeUnitNearBunker = true;
			return;
		}
		else if (Terran_Marine.groundWeapon().maxRange() <= u->type().groundWeapon().maxRange()
				 || u->type() == Zerg_Lurker) {
			rangeUnitNearBunker = true;
			return;
		}
	}

	return;
}

void MarineManager::doKiting(Unit unit) {

	// ��밡 ���� �����̰ų� ������ �����Ÿ��� ��ų� ������ ��Ŀ���� ����ϰų� �׳� �����Ѵ�.
	UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), AllUnitKind);

	if (closeUnit == nullptr || unit->getDistance(closeUnit->pos()) > 1600)
		return;

	Unit bunker = MarineManager::Instance().getBunker();

	// ��Ŀ�� �ϼ��Ǿ� �ְų� ��ó ������ ������ Defence Mode�� ��ȯ
	if (bunker != nullptr && bunker->isCompleted() && bunker->getLoadedUnits().size() != 4) {
		CommandUtil::rightClick(unit, bunker);
		return;
	}

	if (ESM.getEnemyInitialBuild() <= Zerg_12_Pool) // 9�߾� ���Ͽ����� ������ ������ ������ �ʴ´�.
	{
		if (INFO.getTypeUnitsInRadius(Zerg_Zergling, E, unit->getPosition(), 8 * TILE_SIZE).size()
				> INFO.getTypeUnitsInRadius(Terran_Marine, S, unit->getPosition(), 8 * TILE_SIZE).size())
		{
			Position myBase = INFO.getMainBaseLocation(S)->Center();

			TilePosition myBaseTile = INFO.getMainBaseLocation(S)->getTilePosition();

			if (!isSameArea(theMap.GetArea((WalkPosition)unit->getPosition()), theMap.GetArea(myBaseTile))
					|| INFO.getFirstChokePosition(S).getApproxDistance(unit->getPosition()) < 3 * TILE_SIZE)
			{
				CommandUtil::move(unit, myBase);
				return ;
			}
		}
	}

	if (!unit->isInWeaponRange(closeUnit->unit()))
	{
		CommandUtil::attackUnit(unit, closeUnit->unit());
	}
	// ��Ÿ� ���̸�
	else {
		UnitInfo *me = INFO.getUnitInfo(unit, S);

		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(me->pos(), &dangerPoint, false);

		if (INFO.enemyRace == Races::Zerg)
			attackFirstkiting(me, closeUnit, dangerPoint, 4 * TILE_SIZE);
		else
			kiting(me, closeUnit, dangerPoint, 4 * TILE_SIZE);

		//uList Scvs = INFO.getTypeUnitsInRadius(Terran_SCV, S, (unit->getPosition() + closeUnit->pos()) / 2, 50);

		//if (unit->getGroundWeaponCooldown() == 0 && (Scvs.size() >= 3 || unit->getDistance(closeUnit->unit()) > 2 * TILE_SIZE))
		//{
		//	CommandUtil::attackUnit(unit, closeUnit->unit());
		//}
		//else
		//{
		//	if (closeUnit->type().groundWeapon().maxRange() < 4 * TILE_SIZE) {
		//		moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 2 * TILE_SIZE);
		//	}
		//}

	}

	return;
}

void MarineManager::setFirstChokeDefenceMarine(uList &marineList)
{
	int needCnt = INFO.getFirstChokeDefenceCnt();

	if (needCnt != 0 && INFO.getFirstChokePoint(S)) {

		vector<Position> positions = getWidePositions((Position)INFO.getFirstChokePoint(S)->Center(), INFO.getWaitingPositionAtFirstChoke(4, 6),
													  true, 3 * TILE_SIZE, 20);

		if (positions.size() >= marineList.size()) {

			for (int i = 0; i < (int)marineList.size(); i++) {
				UnitInfo *m = marineList.at(i);

				if (m->getState() != "FirstChokeDefence" && m->getState() != "ZealotDefence") {

					WalkPosition wk(positions[i]);

					if (positions[i].isValid() && theMap.GetMiniTile(wk).Altitude() != 0) {
						m->setState(new MarineFirstChokeDefenceState(positions[i]));
					}
					else {
						m->setState(new MarineFirstChokeDefenceState(positions[0]));
					}


				}
			}
		}
		else {
			// ��� ������ ����
			for (int i = 0; i < (int)marineList.size(); i++) {
				UnitInfo *m = marineList.at(i);

				if (m->getState() != "FirstChokeDefence" && m->getState() != "ZealotDefence") {
					if (i % 2 == 0)
						m->setState(
							new MarineFirstChokeDefenceState(
								(INFO.getFirstChokePosition(S, ChokePoint::end1) + INFO.getWaitingPositionAtFirstChoke(3, 6) * 2) / 3));
					else
						m->setState(new MarineFirstChokeDefenceState(
										(INFO.getFirstChokePosition(S, ChokePoint::end2) + INFO.getWaitingPositionAtFirstChoke(3, 6) * 2) / 3));

					break;
				}
			}
		}
	}
	else {
		for (auto m : marineList) {
			if (m->getState() == "FirstChokeDefence")
				m->setState(new MarineIdleState);
		}
	}
}