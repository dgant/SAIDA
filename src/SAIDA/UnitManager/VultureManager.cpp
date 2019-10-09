#include "VultureManager.h"
#include "TankManager.h"

using namespace MyBot;

VultureManager::VultureManager()
{
	kitingSet.initParam();
	kitingSecondSet.initParam();
	keepMultiSet.initParam();
}

VultureManager &VultureManager::Instance()
{
	static VultureManager instance;
	return instance;
}

void VultureManager::update()
{
	if (TIME < 300 || TIME % 2 != 0) return;

	uList vultureList = INFO.getUnits(Terran_Vulture, S);

	if (vultureList.empty()) return;

	needPcontrol = (!S->getUpgradeLevel(UpgradeTypes::Ion_Thrusters) && (INFO.enemyRace == Races::Zerg || (INFO.enemyRace == Races::Protoss && E->getUpgradeLevel(UpgradeTypes::Leg_Enhancements))));

	setVultureDefence(vultureList);

	setScoutVulture(vultureList);

	checkSpiderMine(vultureList);

	if (SM.getMainStrategy() == Eliminate) {

		for (auto v : vultureList)
		{
			if (v->getState() == "Defence")
				v->action();
		}

		return;
	}

	if (SM.getMainStrategy() != AttackAll && kitingSet.isWaiting() && diveDone == false)
		checkDiveVulture();

	for (auto v : vultureList)
	{
		if (isStuckOrUnderSpell(v))
			continue;

		if (moveAsideForCarrierDefence(v))
			continue;

		string state = v->getState();

		// ���� ������� Vulture��� Idle �� ����
		if (state == "New" && v->isComplete()) {

			//if (SM.getNeedKeepSecondExpansion(Terran_Vulture) || SM.getNeedKeepThirdExpansion(Terran_Vulture) && keepMultiSet.size() < 2)
			//{
			//	v->setState(new VultureKeepMultiState());
			//	keepMultiSet.add(v);
			//}
			 if (needScoutCnt)
			{
				v->setState(new VultureScoutState());
				needScoutCnt--;
				scoutDone = true;
				lastScoutTime = TIME;
			}
			else if (INFO.enemyRace != Races::Terran && SM.getNeedAttackMulti() && (int)kitingSecondSet.size() < needCountForBreakMulti(Terran_Vulture) && kitingSet.size() > 8)
			{
				v->setState(new VultureKitingMultiState());
				kitingSecondSet.add(v);
			}
			else
			{
				v->setState(new VultureIdleState());
			}
		}

		if (state == "Idle") {
			v->setState(new VultureKitingState());
			kitingSet.add(v);
		}
		else if (state == "Kiting")
		{
			if ((EIB == Toss_1g_forward || EIB == Toss_2g_forward))
				continue;

			Base *firstExpension = INFO.getFirstExpansionLocation(E);

			if (firstExpension != nullptr && firstExpension->GetOccupiedInfo() == enemyBase && SM.getMainStrategy() != AttackAll &&
				isSameArea(v->pos(), INFO.getMainBaseLocation(E)->getPosition()))
			{
				v->setState(new VultureDiveState());
				kitingSet.del(v);
				v->action();
			}
		}
		else
			v->action();
	}

	// ���� ����Ʈ �� ���� ���Ϸ��� ó���ϱ� ���ؼ� ���� �۾��� �ʿ��ϴ�.
	if (TIME < (24 * 60 * 10) && (EIB == Toss_1g_forward || EIB == Toss_2g_forward))
		kitingSet.setTarget(checkForwardPylon());
	else
		kitingSet.setTarget(SM.getMainAttackPosition());


	if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion || INFO.enemyRace != Races::Terran)
		kitingSet.action();
	else
		kitingSet.actionVsT();

	if (SM.getNeedAttackMulti() == false)
	{
		kitingSecondSet.clearSet();
	}
	else
	{
		kitingSecondSet.setTarget(SM.getSecondAttackPosition());
		kitingSecondSet.action();
	}
	
	if (needKeepMulti())
	{
		if (SM.getNeedKeepSecondExpansion(Terran_Vulture))
			keepMultiSet.setTarget(INFO.getSecondExpansionLocation(S)->Center());
		else
			keepMultiSet.setTarget(INFO.getThirdExpansionLocation(S)->Center());

		keepMultiSet.action();
	}
	else
	{
		keepMultiSet.clearSet();
	}
	
	return;
}

bool VultureManager::moveAsideForCarrierDefence(UnitInfo *v) {

	if (INFO.enemyRace == Races::Protoss) {
		// ���� ������ �ְ�
		if (isSameArea(v->pos(), MYBASE)) {
			// ��� ĳ��� �� ������ �ְ�
			if (!INFO.getTypeUnitsInArea(Protoss_Carrier, E, MYBASE).empty()) {
				// �񸮾ѵ��� FirstChoke ��ó���� ���� ���ؼ� �������� ���� ��.
				int carrierDefenceCntNearFirstChoke = 0;

				for (auto g : INFO.getUnits(Terran_Goliath, S)) {
					if (g->getState() == "CarrierDefence")
					if (g->pos().getApproxDistance(INFO.getFirstChokePosition(S)) <= 10 * TILE_SIZE)
						carrierDefenceCntNearFirstChoke++;
				}

				// �������� ������ �ش�.
				if (carrierDefenceCntNearFirstChoke >= 10) {
					CommandUtil::attackMove(v->unit(), MYBASE);
					return true;
				}
			}
		}
	}

	return false;
}

bool VultureManager::needKeepMulti()
{
	if (SM.getNeedKeepSecondExpansion(Terran_Vulture))
	{
		if (!SM.getNeedKeepSecondExpansion(Terran_Siege_Tank_Tank_Mode))
		{

			uList vset = keepMultiSet.getUnits();

			for (auto v : vset)
			{
				if (v->unit()->getSpiderMineCount() == 0)
				{
					v->setState(new VultureKitingState());
					kitingSet.add(v);
					keepMultiSet.del(v);
				}
			}
		}

		return true;
	}

	if (SM.getNeedKeepThirdExpansion(Terran_Vulture))
	{
		if (!SM.getNeedKeepThirdExpansion(Terran_Siege_Tank_Tank_Mode))
		{
			// ���θ� �ʿ��� ����
			uList vset = keepMultiSet.getUnits();

			for (auto v : vset)
			{
				if (v->unit()->getSpiderMineCount() == 0)
				{
					v->setState(new VultureKitingState());
					kitingSet.add(v);
					keepMultiSet.del(v);
				}
			}
		}

		return true;
	}

	return false;
}

Position VultureManager::checkForwardPylon()
{
	// ���� �ǹ� ã�� ���� ������ �� Done
	if (forwardBuildingPosition == Positions::None)
	{
		return ATTACKPOS;
	}

	// ���� ���Ϸ� ã�Ƽ� �ν��� ���
	if (checkedForwardPylon)
	{
		// ���Ϸ��� ���� ���
		if (INFO.getClosestTypeUnit(E, forwardBuildingPosition, Protoss_Pylon, 10 * TILE_SIZE, true) == nullptr)
		{
			// ����Ʈ�� ���� ��� ��Ȳ����
			if (INFO.getClosestTypeUnit(E, forwardBuildingPosition, Protoss_Gateway, 10 * TILE_SIZE, true) == nullptr)
				forwardBuildingPosition = Positions::None;

			return ENBASE;
		}

		return forwardBuildingPosition;
	}

	if (forwardBuildingPosition == Positions::Unknown)// ��� �ǹ��� ã�ƾ���.
	{
		for (auto &g : INFO.getBuildings(E))
		{
			// �� �߾Ӻ��� ����� �ǹ� // ���Ϸ��� ������ ����Ʈ�� ���� ���� ���� �� �����Ƿ� �׸��� ������ �ű⿡ ���Ϸ��� ���� �״�
			if (INFO.getSecondChokePosition(S).getApproxDistance(g.second->pos()) < INFO.getSecondChokePosition(S).getApproxDistance(theMap.Center()) + 10 * TILE_SIZE)
			{
				forwardBuildingPosition = g.second->pos();
				return forwardBuildingPosition;
			}
			else // �� �ۿ��� ������ �׳� ����� Gate��.
			{
				if (g.second->type() == Protoss_Gateway)
				{
					forwardBuildingPosition == Positions::None;
					return ENBASE;
				}
			}
		}

		Position basePos = Positions::Unknown;

		int rank = 5;

		for (Base *b : INFO.getBaseLocations())
		{
			if (b->GetOccupiedInfo() == myBase)
				continue;

			if (b->GetExpectedMyMultiRank() > rank)
				continue;

			if (b->GetLastVisitedTime() > 0)
				continue;

			if (INFO.getSecondChokePosition(S).getApproxDistance(b->Center()) > INFO.getSecondChokePosition(S).getApproxDistance(theMap.Center()) + 10 * TILE_SIZE)
				continue;

			rank = b->GetExpectedMyMultiRank();
			basePos = b->Center();
		}

		if (basePos == Positions::Unknown)
		{
			if (bw->isExplored((TilePosition)theMap.Center()))
				return ENBASE;
			else
				return theMap.Center();
		}
		else
			return basePos;
	}
	else
	{
		if (checkedForwardPylon == false) {
			UnitInfo *forwardPylon = INFO.getClosestTypeUnit(E, forwardBuildingPosition, Protoss_Pylon, 10 * TILE_SIZE, true);

			if (forwardPylon != nullptr) {
				forwardBuildingPosition = forwardPylon->pos();
				checkedForwardPylon = true;
			}
		}

		return forwardBuildingPosition;
	}
}

void VultureManager::checkDiveVulture()
{
	// �ո����� Tower
	UnitInfo *frontVulture = kitingSet.getFrontUnitFromPosition(INFO.getMainBaseLocation(E)->Center());

	if (frontVulture == nullptr)
		return;

	uList diveVultures = INFO.getTypeUnitsInRadius(Terran_Vulture, S, frontVulture->pos(), 5 * TILE_SIZE);

	int VultureCnt = 0;

	for (auto v : diveVultures)
	{
		// hp 60% �̻��� Vulture�� Check
		if (v->hp() > (int)(v->type().maxHitPoints() * 0.6))
			VultureCnt++;
	}

	//  �ո��� �ȸ��� ���¸� ���̺� ����.
	Base *firstExpension = INFO.getFirstExpansionLocation(E);

	if (firstExpension == nullptr || firstExpension->GetOccupiedInfo() != enemyBase)
		return;

	int firstExpensionTower = firstExpension->GetEnemyGroundDefenseBuildingCount();

	if (INFO.enemyRace == Races::Terran)
		firstExpensionTower = firstExpension->GetEnemyBunkerCount();

	// �ո��� ���Ÿ�� �������� ������ ���̺� ����.
	if (firstExpensionTower == 0)
		return;

	bool needDive = false;
	int rangeUnits = 0;

	if (INFO.enemyRace == Races::Protoss)
		rangeUnits = INFO.getCompletedCount(Protoss_Dragoon, E);
	else if (INFO.enemyRace == Races::Zerg)
		rangeUnits = INFO.getCompletedCount(Zerg_Hydralisk, E);
	else
	{
		rangeUnits = INFO.getCompletedCount(Terran_Vulture, E);
		rangeUnits += INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, E);
	}

	if (firstExpensionTower * 2 <= VultureCnt && rangeUnits == 0)
	{
		for (auto v : diveVultures)
		{
			v->setState(new VultureDiveState());
			kitingSet.del(v);
		}

		diveDone = true;
	}
}

void VultureManager::checkSpiderMine(uList &vList)
{
	for (auto m : INFO.getUnits(Terran_Vulture_Spider_Mine, S))
	{
		if (kitingSet.size() == 0)
			return;

		if (INFO.enemyRace != Races::Terran)
		{
			if (m->getKillMe() == NoKill)
			{
				if (!m->isDangerMine())
					continue;

				for (auto t : INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, m->pos(), 3 * TILE_SIZE))
				{
					if (t->type() == Terran_Siege_Tank_Siege_Mode)
					{
						m->setKillMe(KillMe);
						break;
					}
				}
			}
		}

		if (m->getKillMe() == AssignedKill && removeMineMap[m->unit()]->exists() == false)
			m->setKillMe(KillMe);

		if (m->getKillMe() == MustKillAssigned && removeMineMap[m->unit()]->exists() == false)
			m->setKillMe(MustKill);

		if (m->getKillMe() != NoKill)
		{
			if ((m->getKillMe() == AssignedKill || m->getKillMe() == MustKillAssigned) && removeMineMap[m->unit()]->exists())
				continue;

			// kiting Set���� Mine�� ���� ����� Vulture
			UnitInfo *toBeAssigned = kitingSet.getFrontUnitFromPosition(m->pos());

			if (toBeAssigned == nullptr) continue;

			if (m->getKillMe() == KillMe && toBeAssigned->pos().getApproxDistance(m->pos()) > 6 * TILE_SIZE)
				continue;

			// State�� Mine���ŷ� �ٲ���
			toBeAssigned->setState(new VultureRemoveMineState(m->unit()));

			// MineMap�� �Ҵ��� ��
			removeMineMap[m->unit()] = toBeAssigned->unit();
			// KitingSet������ delete
			kitingSet.del(toBeAssigned);

			// KillMe Flag �� 2��
			if (m->getKillMe() == KillMe)
				m->setKillMe(AssignedKill);
			else if (m->getKillMe() == MustKill)
				m->setKillMe(MustKillAssigned);
		}
	}

	/*
	for (auto m : INFO.getUnits(Terran_SCV, S))
	{
	if (m->getKillMe() == AssignedKill && removeMineMap[m->unit()]->exists() == false)
	m->setKillMe(KillMe);

	if (m->getKillMe() == MustKillAssigned && removeMineMap[m->unit()]->exists() == false)
	m->setKillMe(MustKill);

	if (m->getKillMe() != NoKill)
	{
	if ((m->getKillMe() == AssignedKill || m->getKillMe() == MustKillAssigned) && removeMineMap[m->unit()]->exists())
	continue;

	// kiting Set���� Mine�� ���� ����� Vulture
	UnitInfo *toBeAssigned = kitingSet.getFrontUnitFromPosition(m->pos());

	if (toBeAssigned == nullptr) continue;

	if (m->getKillMe() == KillMe && toBeAssigned->pos().getApproxDistance(m->pos()) > 6 * TILE_SIZE)
	continue;

	// State�� Mine���ŷ� �ٲ���
	toBeAssigned->setState(new VultureRemoveMineState(m->unit()));

	// MineMap�� �Ҵ��� ��
	removeMineMap[m->unit()] = toBeAssigned->unit();
	// KitingSet������ delete
	kitingSet.del(toBeAssigned);

	// KillMe Flag �� 2��
	if (m->getKillMe() == KillMe)
	m->setKillMe(AssignedKill);
	else if (m->getKillMe() == MustKill)
	m->setKillMe(MustKillAssigned);
	}
	}*/
}

void VultureManager::onUnitDestroyed(Unit u)
{
	vultureDefenceSet.del(u);
	kitingSet.del(u);
	kitingSecondSet.del(u);
	keepMultiSet.del(u);
}

UnitInfo *VultureManager::getFrontVultureFromPos(Position pos)
{
	if (kitingSet.size() == 0)
		return nullptr;

	return kitingSet.getFrontUnitFromPosition(pos);
}

void VultureManager::setVultureDefence(uList &vList)
{
	uList eList = INFO.enemyInMyYard();
	word needCnt = 0;

	if (eList.empty()) {

		// ���� ��� ����� �� �� ������ �ϴ� Defence
		if (SM.getMainStrategy() != AttackAll &&
			(ESM.getWaitTimeForDrop() > 0 || E->getUpgradeLevel(UpgradeTypes::Ventral_Sacs))
			&& TIME - ESM.getWaitTimeForDrop() <= 24 * 60 * 3
			&& BasicBuildStrategy::Instance().hasSecondCommand()) {

			if (INFO.enemyRace != Races::Protoss || (EMB == Toss_drop || EMB == Toss_dark_drop))
			{
				// �ո��翡 �ʹ� ���� ���� ������ pass
				if (INFO.getUnitsInRadius(E, INFO.getSecondChokePosition(S), 10 * TILE_SIZE, true).size() < 5)
					needCnt = 2;
			}
		}
		else {

			for (auto v : vultureDefenceSet.getUnits()) {
				v->setState(new VultureIdleState());
			}

			vultureDefenceSet.clear();
			return;
		}
	}

	UnitType uType;

	for (auto e : eList) {
		uType = e->type();

		if (uType == Protoss_Shuttle || uType == Terran_Dropship)
			needCnt += ((uType.spaceProvided() - e->getSpaceRemaining()) / 2);
		else if (uType == Protoss_Reaver)
			needCnt += 4;
		else if (uType == Protoss_Dark_Templar)
			needCnt += 3;
		else if (!uType.isFlyer())
			needCnt += 1;
	}

	if (needCnt == 0) {
		for (auto v : vultureDefenceSet.getUnits())
			v->setState(new VultureIdleState());

		vultureDefenceSet.clear();
		return;
	}

	// ���� �߰��ؾ���.
	if (vultureDefenceSet.size() < needCnt) {

		UListSet allVSet;

		for (auto v : vList) {
			allVSet.add(v);
		}

		// �������κ��� ���� ����� ���ֵ�
		uList sortedList = allVSet.getSortedUnitList(INFO.getMainBaseLocation(S)->getPosition());

		for (word i = 0; i < sortedList.size(); i++) {
			if (sortedList[i]->getState() != "Defence" && sortedList[i]->getState() != "Diving" && sortedList[i]->getState() != "Scout" && sortedList[i]->getState() != "RemoveMine" && sortedList[i]->getState() != "KillWorkerFirst") {
				sortedList[i]->setState(new VultureDefenceState());
				vultureDefenceSet.add(sortedList[i]);
				kitingSet.del(sortedList[i]);
				kitingSecondSet.del(sortedList[i]);
				keepMultiSet.del(sortedList[i]);
			}

			if (vultureDefenceSet.size() >= needCnt)
				break;
		}
	}
}

void VultureManager::setScoutVulture(uList &vList)
{
	// 1�п� �ѹ�
	int checkThreshold = (24 * 60 * 1);

	if (lastScoutTime != 0 && (lastScoutTime + checkThreshold) < TIME)
	{
		scoutDone = false;
		lastScoutTime = 0;
	}

	// Waiting�� �ƴϰų� Range�� �ƴ� ���� Scout �� �ʿ� ����.
	if (scoutDone) return;

	if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMainStrategy() != WaitLine) return;
	}
	else
	{
		if (kitingSet.isWaiting() < 2)		return;
	}

	needScoutCnt = S->supplyUsed() > 200 ? 2 : 1;

	//// ���� ���� �ȵǾ� ������ �ϴ� Pass
	////	if (!S->hasResearched(TechTypes::Spider_Mines)) return;

	//Position myBase = INFO.getSecondChokePosition(S);

	//UnitInfo *frontVulture = kitingSet.getFrontUnitFromPosition(SM.getMainAttackPosition());

	//if (frontVulture == nullptr)		return;

	////int needScout = kitingSet.size() > 3 ? 2 : 1;

	//// ���� ���� ���İ� ���߾� �Ѿ �ִٸ� �׸��� �ֺ��� ���� ���ٸ�.
	////	if (myBase.getApproxDistance(theMap.Center()) <= myBase.getApproxDistance(frontVulture->pos()) &&
	////			(INFO.getClosestUnit(E, frontVulture->pos(), GroundCombatKind, 10 * TILE_SIZE) == nullptr || kitingSet.size() > 4)) // 8���ĸ� �׳� scout ����*/
	//{
	//	needScout = true;
	//	/*
	//	uList sortedList = kitingSet.getSortedUnitList(MYBASE); // �ڿ� �ִ� ���İ� ����.

	//	for (auto v : sortedList)
	//	{
	//		if (needScout == 0)
	//			break;

	//		if (v->unit()->getSpiderMineCount() > 0)
	//		{
	//			v->setState(new VultureScoutState());
	//			kitingSet.del(v);
	//			needScout--;
	//			scoutDone = true;
	//			lastScoutTime = TIME;
	//		}
	//	}
	//	*/
	//}
}

void VultureKiting::action()
{
	if (size() == 0)	return;

	// Time To Checek
	if (waitingTime != 0 && waitingTime + (24 * 30) < TIME)
	{
		waitingTime = 0;
		needWaiting = 0;
		needCheck = true;
	}

	Base *enFirstExp = INFO.getFirstExpansionLocation(E);

	if (enFirstExp && target == enFirstExp->Center())
		target = INFO.getMainBaseLocation(E)->Center();

	UnitInfo *frontUnit = getFrontUnitFromPosition(target);

	if (frontUnit == nullptr) return;

	Position backPos = (INFO.getSecondChokePosition(S) + INFO.getFirstExpansionLocation(S)->getPosition()) / 2;

	UnitInfo *frontTank = TM.getFrontTankFromPos(target);

	if (S->hasResearched(TechTypes::Spider_Mines)) {
		if ((INFO.getSecondExpansionLocation(S) && target == INFO.getSecondExpansionLocation(S)->Center()) ||
			(INFO.getThirdExpansionLocation(S) && target == INFO.getThirdExpansionLocation(S)->Center()))
		{
			defenceMinePos = getRoundPositions(target, 7 * TILE_SIZE, 20);
		}
		else
		{
			vector<Position> roundPos = getRoundPositions(INFO.getSecondChokePosition(S), 5 * TILE_SIZE, 20);
			Position guardMinePos = INFO.getFirstExpansionLocation(S)->Center();

			defenceMinePos.clear();

			for (auto p : roundPos)
			{
				if (!isSameArea(p, guardMinePos))
					defenceMinePos.push_back(p);
			}
		}
	}

	if (needFight)
	{
		if (needBack(frontUnit->pos()))	needFight = false;
	}
	else
	{
		if (canFight(frontUnit->pos()))	needFight = true;
	}

	for (auto v : getUnits())
	{
		if (isStuckOrUnderSpell(v))
			continue;

		if (VultureManager::Instance().moveAsideForCarrierDefence(v))
			continue;

		UnitInfo *closestTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());
		if (SM.getMainStrategy() == AttackAll)
		{
			if (closestTank != nullptr)
				if (getGroundDistance(closestTank->pos(), v->pos()) > 12 * TILE_SIZE)
				{
					CommandUtil::attackMove(v->unit(), closestTank->pos());
					continue;
				}
		}
		if ((TIME - v->unit()->getLastCommandFrame() < 24) &&
			v->unit()->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position)
			continue;

		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(v->pos(), &dangerPoint, false);

		if (dangerUnit == nullptr) // DangerUnit ����.
		{
			if (setGuardMine(v))
				continue;

			// �ո��翡�� ���Ÿ���� �Ǽ��߿� �ִ� ��� �������� �޷������� �Ѵ�.
			UnitInfo *buildingTower = INFO.getClosestTypeUnit(E, v->pos(), INFO.getAdvancedDefenseBuildingType(INFO.enemyRace), 10 * TILE_SIZE);

			// �Ǽ����� Ÿ���� �ִµ�...
			if (buildingTower != nullptr && enFirstExp && isSameArea(buildingTower->pos(), enFirstExp->Center())) {
				// �ո����̸� �������� ����
				CommandUtil::move(v->unit(), target);
				continue;
				// �����̸� ���� ������ ���� �ȴ�. ToDo....
				//else if (isSameArea((TilePosition)buildingTower->pos(), (TilePosition)target))
			}

			UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 15 * TILE_SIZE);

			if (closestWorker != nullptr) // �ϲ� ����
				kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
			else {

				if (INFO.enemyRace == Races::Zerg) {
					UnitInfo *lurker = INFO.getClosestTypeUnit(E, v->pos(), Zerg_Lurker, 8 * TILE_SIZE);

					if (lurker) {
						CommandUtil::attackUnit(v->unit(), lurker->unit());
						continue;
					}
				}

				if (v->pos().getApproxDistance(target) > 5 * TILE_SIZE) // ������ �̵�
					CommandUtil::move(v->unit(), target);
				else if (TIME < (24 * 60 * 10) && (EIB == Toss_1g_forward || EIB == Toss_2g_forward)) // ���� ���Ϸ��� �ƴ� ��쿡 ���Ϸ��� ����.
				{
					UnitInfo *pylon = INFO.getClosestTypeUnit(E, target, Protoss_Pylon, 5 * TILE_SIZE);

					if (pylon) {
						CommandUtil::attackUnit(v->unit(), pylon->unit());
					}
				}
				else
				{
					if (INFO.enemyRace == Races::Zerg)
					{
						UnitInfo *larva = INFO.getClosestTypeUnit(E, v->pos(), Zerg_Larva, 8 * TILE_SIZE);

						if (larva) {
							CommandUtil::attackUnit(v->unit(), larva->unit());
							continue;
						}

						UnitInfo *egg = INFO.getClosestTypeUnit(E, v->pos(), Zerg_Egg, 8 * TILE_SIZE);

						if (egg) {
							CommandUtil::attackUnit(v->unit(), egg->unit());
							continue;
						}
					}
				}
			}
		} // DangerUnit ���� ����
		else //dangerUnit ����
		{
			//�� ���� ������ ī����.. ��� ���۸��� ���� �Ѹ����� ���°� �� ���ñ� �ϴ�.
			if (isNeedKitingUnitType(dangerUnit->type()))
			{
				UnitInfo *closestAttack = INFO.getClosestUnit(E, v->pos(), GroundCombatKind, 10 * TILE_SIZE, true, false, true);

				if (closestAttack == nullptr) // ������ �ʾ�
				{
					/*UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

					if (closestWorker != nullptr)
					kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
					else*/
					CommandUtil::attackMove(v->unit(), target);
				}
				else // Kiting
				{
					UnitInfo *closestTank = INFO.getClosestTypeUnit(S, v->pos(), Terran_Siege_Tank_Tank_Mode);

					// �ֺ��� Tank�� ������ Kiting ���� ���� �׳� �����ϴ°� ����.
					if (closestTank != nullptr && closestTank->pos().getApproxDistance(v->pos()) < 4 * TILE_SIZE)
						CommandUtil::attackUnit(v->unit(), closestAttack->unit());
					else
					{
						if (VM.getNeedPcon())
							pControl(v, closestAttack);
						else
							kiting(v, closestAttack, dangerPoint, 4 * TILE_SIZE);
					}
				}
			} // ���� Kiting ����
			else // ���� �ƴ�
			{

				if (needFight)
				{
					UnitInfo *closestAttack = INFO.getClosestUnit(E, frontUnit->pos(), GroundCombatKind, 15 * TILE_SIZE, false, false, true);

					if (closestAttack == nullptr) // ������ �ʾ�
					{
						UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

						if (closestWorker != nullptr)
							kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
						else if (v->pos().getApproxDistance(target) > 5 * TILE_SIZE) // ������ �̵�
							CommandUtil::move(v->unit(), target);
					}
					else // Kiting
					{
						UnitInfo *weakUnit = getGroundWeakTargetInRange(v);

						if (weakUnit != nullptr)
						{
							closestAttack = weakUnit;
							kiting(v, closestAttack, dangerPoint, 4 * TILE_SIZE);
						}

						if (closestAttack->type() == Terran_Bunker)
						{
							UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, closestAttack->pos(), INFO.getWorkerType(INFO.enemyRace), 2 * TILE_SIZE);

							if (closestWorker != nullptr)
								CommandUtil::attackUnit(v->unit(), closestWorker->unit());

							continue;
						}

						///////////////////////////////////////////
						CommandUtil::attackUnit(v->unit(), closestAttack->unit());

						if (v->posChange(closestAttack) == PosChange::Farther)
							v->unit()->move((v->pos() + closestAttack->vPos()) / 2);
					}

					continue;
				}// need Fight ����

				if (SM.getMainStrategy() == AttackAll)
				{
					if (dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
					{
						if (dangerPoint > 4 * TILE_SIZE) // ���ġ�� �׳� �����ϰ�...
						if (setDefenceMine(v))
							continue;

						if (frontTank)
						{
							int distTankToTower = frontTank->pos().getApproxDistance(dangerUnit->pos());

							if (v->pos().getApproxDistance(dangerUnit->pos()) < distTankToTower + TILE_SIZE)
								CommandUtil::move(v->unit(), backPos);
							else if (v->pos().getApproxDistance(dangerUnit->pos()) > distTankToTower + 3 * TILE_SIZE)
								CommandUtil::hold(v->unit());
							else
								CommandUtil::attackMove(v->unit(), target);
						}
						else
						{
							CommandUtil::move(v->unit(), backPos);
						}

						continue;
					}

					if (dangerPoint > 4 * TILE_SIZE) // ���ġ�� �׳� �����ϰ�...
					{
						if (setDefenceMine(v))
							continue;

						else
							CommandUtil::attackMove(v->unit(), target);
					}
					else
					{
						if (dangerUnit->type() == Terran_Siege_Tank_Siege_Mode)
							CommandUtil::move(v->unit(), backPos);
						else // ���� ������ ��� Tank�� ����
						{
							UnitInfo *closestTank = INFO.getClosestTypeUnit(S, v->pos(), Terran_Siege_Tank_Tank_Mode);
							UnitInfo *closestGoliath = INFO.getClosestTypeUnit(S, v->pos(), Terran_Goliath);
							UnitInfo *closest = nullptr;

							if (closestTank == nullptr && closestGoliath == nullptr)
								CommandUtil::move(v->unit(), backPos);
							else
							{
								if (closestTank == nullptr)
									closest = closestGoliath;
								else if (closestGoliath == nullptr)
									closest = closestTank;
								else
								{
									if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 5 ||
										INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > INFO.getCompletedCount(Terran_Goliath, S))
										closest = closestTank;
									else
										closest = closestGoliath;
								}

								int Threshold_Back = 5 * TILE_SIZE;
								int Threshold_Go = 3 * TILE_SIZE;

								if (closest == closestTank && !closest->unit()->canUnsiege()) // ��ũ ����� �ʹ� �ָ����� ����
								{
									Threshold_Back = 3 * TILE_SIZE;
									Threshold_Go = 2 * TILE_SIZE;
								}

								if (closest->pos().getApproxDistance(v->pos()) < Threshold_Go)
									CommandUtil::attackMove(v->unit(), target);
								else if (closest->pos().getApproxDistance(v->pos()) > Threshold_Back)
									CommandUtil::move(v->unit(), closest->pos());
							}
						}
					}
				}// Attack All ����
				else // Attack All �ƴ�
				{
					if (needWaiting) // ����� ����
					{
						if (!dangerUnit->isHide()) // ���� ������ ���� ���� Time ����
							waitingTime = TIME;

						// ���� �̸� ������ ���� �˳��ϰ�
						int backThreshold = 8;

						// ����� Ÿ���� �׵θ���
						if (dangerUnit->type() == Terran_Siege_Tank_Siege_Mode || dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
							backThreshold = 4;

						// ��Ŀ�� ���� ó��
						if (dangerUnit->type() == Zerg_Lurker)
							backThreshold = 3;

						// ������ ����
						if (dangerPoint > backThreshold * TILE_SIZE)
						{
							// Mine ������ ��ŵ
							if (setDefenceMine(v))
								continue;

							if (size() <= 5)
							{
								if (v == frontUnit)
									CommandUtil::hold(v->unit());
								else if (v->pos().getApproxDistance(frontUnit->pos()) < 3 * TILE_SIZE)
									CommandUtil::move(v->unit(), frontUnit->pos());
							}
						}
						else // ������ ����
						{
							// ��Ŀ�� �ȳ����µ�..
							UnitInfo *closestUnit = INFO.getClosestUnit(E, frontUnit->pos(), GroundCombatKind, 15 * TILE_SIZE, false, false, true);

							if (closestUnit == nullptr && setDefenceMine(v))
								continue;

							UnitInfo *closestTank = INFO.getClosestTypeUnit(S, v->pos(), Terran_Siege_Tank_Tank_Mode);

							if (closestTank == nullptr)
							{
								if (v->pos().getApproxDistance(backPos) < 2 * TILE_SIZE)
									v->unit()->holdPosition();
								else if (v->pos().getApproxDistance(backPos) > 4 * TILE_SIZE)
									CommandUtil::move(v->unit(), backPos);
							}
							else
							{
								if (closestTank->pos().getApproxDistance(v->pos()) < 2 * TILE_SIZE)
									CommandUtil::attackMove(v->unit(), target);
								else if (closestTank->pos().getApproxDistance(v->pos()) > 4 * TILE_SIZE)
									CommandUtil::move(v->unit(), closestTank->pos());
							}
						}
					} // ����� ����
					else // Waiting �ƴ�
					{
						// dangerUnit�� Tower�϶�
						if (dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
						{
							if (enFirstExp && isSameArea(dangerUnit->pos(), enFirstExp->Center()))
							{
								needWaiting = 1;
								waitingTime = TIME;
							}
							else // �ո��� ���� Tower
							{
								UnitInfo *closestAttack = INFO.getClosestUnit(E, v->pos(), GroundUnitKind, 10 * TILE_SIZE, false, false, true);

								if (closestAttack == nullptr)
								{
									UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

									if (closestWorker != nullptr)
									{
										kiting(v, closestWorker, dangerPoint, 4 * TILE_SIZE);
									}
									else
									{
										//int direct = v->getDirection();

										if (goWithoutDamage(v->unit(), target, direction, 4 * TILE_SIZE) == false)
										{
											if (direction == 1) direction *= -1;
											else
											{
												needWaiting = 1;
												waitingTime = TIME;
											}
										}
									}
								}
								else // closest Attacker ����
								{
									if (isNeedKitingUnitType(closestAttack->type()))
									{
										kiting(v, closestAttack, dangerPoint, 4 * TILE_SIZE);
									}
								}
							}
						}
						else // Ÿ���� �ƴѰ��
						{
							if (needCheck == true && dangerUnit->isHide())
							{
								CommandUtil::move(v->unit(), target);
								continue;
							}

							needWaiting = 2;
							waitingTime = TIME;
						}
					} //// Waiting �ƴ�
				} //Attack All �ƴ�
			}//���� �ƴ�
		}// Danger ����.
	}

	return;
}

void VultureKiting::actionVsT()
{
	if (size() == 0)	return;

	if (changedTime != 0 && changedTime + (24 * 5) > TIME)
		return;

	if (preStage != DrawLine && SM.getMainStrategy() == DrawLine)
	{
		changedTime = TIME;
		preStage = SM.getMainStrategy();
		return;
	}

	preStage = SM.getMainStrategy();

	// Time To Checek
	if (waitingTime != 0 && waitingTime + (24 * 90) < TIME)
	{
		waitingTime = 0;
		needWaiting = false;
		needCheck = true;
	}

	Base *enemyFirstExpension = INFO.getFirstExpansionLocation(E);

	Position backPos = (INFO.getSecondChokePosition(S) + INFO.getFirstExpansionLocation(S)->getPosition()) / 2;

	Position VultureWaitPos;
	UnitInfo *closestEnemy = nullptr;
	int enSize = 0;

	if (SM.getMainStrategy() != AttackAll)
	{
		uList enemyToCut = getEnemyOutOfRange(SM.getWaitLinePosition(), true);

		closestEnemy = getClosestUnit(MYBASE, enemyToCut, true);

		if (closestEnemy)
			target = closestEnemy->pos();
		else
		{
			if (SM.getMainStrategy() == DrawLine)
				VultureWaitPos = SM.getDrawLinePosition();
			else
			{
				if (SM.getSecondAttackPosition() == Positions::Unknown)
					VultureWaitPos = getDirectionDistancePosition(SM.getWaitLinePosition(), INFO.getFirstWaitLinePosition(), 5 * TILE_SIZE);
				else {
					VultureWaitPos = SM.getSecondAttackPosition();
				}
			}
		}

		bw->drawCircleMap(target, 10, Colors::Cyan, true);
	}

	UnitInfo *frontUnit = getFrontUnitFromPosition(target);

	if (frontUnit == nullptr) return;

	if (needFight)
	{
		if (needBack(frontUnit->pos()))	needFight = false;
	}
	else {
		if (canFight(frontUnit->pos()))	needFight = true;
	}

	bool mineUp = S->hasResearched(TechTypes::Spider_Mines);

	for (auto v : getUnits())
	{
		if (isStuckOrUnderSpell(v))
			continue;

		if ((TIME - v->unit()->getLastCommandFrame() < 24) &&
			v->unit()->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position)
			continue;

		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(v->pos(), &dangerPoint, false);

		if (SM.getMainStrategy() == AttackAll)
		{
			// DrawLine -> ����
			if (mineUp && v->unit()->getSpiderMineCount() > 0)
			{
				UnitInfo *closestTank = INFO.getClosestTypeUnit(E, v->pos(), Terran_Siege_Tank_Tank_Mode, 8 * TILE_SIZE, true, true);

				if (closestTank && INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, closestTank->pos(), 3 * TILE_SIZE).size() == 0)
				{
					if (v->pos().getApproxDistance(closestTank->pos()) < 2 * TILE_SIZE)
						v->unit()->useTech(TechTypes::Spider_Mines, v->pos());
					else
						CommandUtil::move(v->unit(), closestTank->pos());

					continue;
				}
			}

			CommandUtil::attackMove(v->unit(), target);
		}
		else if (SM.getMainStrategy() == WaitLine || SM.getMainStrategy() == DrawLine)
		{
			if (closestEnemy && needFight)
			{
				UnitInfo *closestTank = INFO.getClosestTypeUnit(E, v->pos(), Terran_Siege_Tank_Tank_Mode, 8 * TILE_SIZE, true, true);

				if (closestTank && INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, closestTank->pos(), 3 * TILE_SIZE).size() == 0)
				{
					if (mineUp && v->unit()->getSpiderMineCount() > 0) {
						if (v->pos().getApproxDistance(closestTank->pos()) < 2 * TILE_SIZE)
							v->unit()->useTech(TechTypes::Spider_Mines, v->pos());
						else
							CommandUtil::move(v->unit(), closestTank->pos());
					}

					continue;
				}

				UnitInfo *weakUnit = getGroundWeakTargetInRange(v, true);

				if (weakUnit != nullptr) // ���� ��� ���� ����
					closestEnemy = weakUnit;

				if (closestEnemy->isHide())
					CommandUtil::attackMove(v->unit(), closestEnemy->pos());
				else
				{
					CommandUtil::attackUnit(v->unit(), closestEnemy->unit());

					if (v->posChange(closestEnemy) == PosChange::Farther)
						v->unit()->move((v->pos() + closestEnemy->vPos()) / 2);
				}
			}
			else
			{
				if (SM.getMainStrategy() == WaitLine)
				{
					if (dangerUnit && dangerUnit->type() == Terran_Siege_Tank_Siege_Mode && dangerPoint < 4 * TILE_SIZE)
					{
						moveBackPostion(v, dangerUnit->pos(), 3 * TILE_SIZE);
						continue;
					}

					if (mineUp && v->unit()->getSpiderMineCount() > 1)
					{
						if (INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, v->pos(), 5 * TILE_SIZE).size() == 0)
						{
							v->unit()->useTech(TechTypes::Spider_Mines, v->pos());
							continue;
						}
					}

					if (target.getApproxDistance(v->pos()) > 4 * TILE_SIZE)
						CommandUtil::attackMove(v->unit(), VultureWaitPos, true);

				}
				else // Draw Line
				{
					CommandUtil::attackMove(v->unit(), VultureWaitPos);
				}
			}
		}
	}

	return;
}

void VultureKiting::clearSet() {
	if (!size())	return;

	for (auto v : getUnits()) {
		v->setState(new VultureIdleState());
	}

	clear();
}


bool VultureKiting::canFight(Position frontPos)
{
	int range = 10 * TILE_SIZE;

	if (size() > 5)
		range += 5 * TILE_SIZE;

	word vultureCnt = INFO.enemyRace == Races::Terran ? size() : INFO.getTypeUnitsInRadius(Terran_Vulture, S, frontPos, range).size();

	if (vultureCnt <= size() / 2)
		return false;

	if (INFO.enemyRace == Races::Protoss)
	{
		word dragoon = INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, frontPos, 15 * TILE_SIZE, true).size();
		word zealot = INFO.getTypeUnitsInRadius(Protoss_Zealot, E, frontPos, 15 * TILE_SIZE, true).size();
		word photo = INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, frontPos, 15 * TILE_SIZE, true).size();

		word airCombat = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, frontPos, 15 * TILE_SIZE, true).size();
		airCombat += INFO.getTypeUnitsInRadius(Protoss_Scout, E, frontPos, 15 * TILE_SIZE, true).size();

		// Attack All �϶��� �ظ��ϸ� ���ĳ��� ���ݰ��� ����.
		/*if (SM.getMainStrategy() == AttackAll)
		dragoon *= 2;*/

		//preAmountOfEnemy = dragoon + zealot + photo;

		if ((dragoon * 2) + (photo * 2) + (zealot / 4) < vultureCnt && airCombat == 0)
			return true;
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		word hydra = INFO.getTypeUnitsInRadius(Zerg_Hydralisk, E, frontPos, 15 * TILE_SIZE, true).size();
		word zergling = INFO.getTypeUnitsInRadius(Zerg_Zergling, E, frontPos, 15 * TILE_SIZE, true).size();
		word sunken = INFO.getTypeBuildingsInRadius(Zerg_Sunken_Colony, E, frontPos, 15 * TILE_SIZE, true).size();
		word mutal = INFO.getTypeUnitsInRadius(Zerg_Mutalisk, E, frontPos, 15 * TILE_SIZE, true).size();
		uList luckers = INFO.getTypeUnitsInRadius(Zerg_Lurker, E, frontPos, 15 * TILE_SIZE, true);
		word lucker = 0;

		for (auto l : luckers)
		if (!l->unit()->canBurrow(false))
			lucker++;

		preAmountOfEnemy = hydra + zergling + sunken + lucker;

		if ((sunken * 3) + hydra + (lucker) < vultureCnt && mutal == 0)
			return true;
	}
	else
	{
		word vulture = INFO.getTypeUnitsInRadius(Terran_Vulture, E, frontPos, 15 * TILE_SIZE, true).size();
		word tank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, frontPos, 20 * TILE_SIZE, true).size();
		word goliath = INFO.getTypeUnitsInRadius(Terran_Goliath, E, frontPos, 15 * TILE_SIZE, true).size();
		word marine = INFO.getTypeUnitsInRadius(Terran_Marine, E, frontPos, 15 * TILE_SIZE, true).size();
		uList bunkers = INFO.getTypeBuildingsInRadius(Terran_Bunker, E, frontPos, 15 * TILE_SIZE, true);
		word bunker = 0;

		for (auto b : bunkers)
		if (b->getMarinesInBunker())
			bunker++;

		//preAmountOfEnemy = vulture + tank + goliath + bunker + marine;

		if (vulture + (goliath * 1) + (tank * 2) + (bunker * 4)  < vultureCnt)
			return true;
	}

	return false;
}

bool VultureKiting::needBack(Position frontPos)
{
	int range = 10 * TILE_SIZE;

	if (size() > 5)
		range += 5 * TILE_SIZE;

	word vultureCnt = INFO.enemyRace == Races::Terran ? size() : INFO.getTypeUnitsInRadius(Terran_Vulture, S, frontPos, range).size();

	if (INFO.enemyRace == Races::Protoss)
	{
		word dragoon = INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, frontPos, 15 * TILE_SIZE, true).size();
		word zealot = INFO.getTypeUnitsInRadius(Protoss_Zealot, E, frontPos, 15 * TILE_SIZE, true).size();
		word photo = INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, frontPos, 15 * TILE_SIZE, true).size();

		word airCombat = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, frontPos, 15 * TILE_SIZE, true).size();
		airCombat += INFO.getTypeUnitsInRadius(Protoss_Scout, E, frontPos, 15 * TILE_SIZE, true).size();

		/*if ((preAmountOfEnemy < dragoon + zealot + photo) ||
		((dragoon * 2) + (photo * 4)) > vultureCnt || airCombat != 0)*/
		if (((dragoon * 2) + (photo * 3)) > vultureCnt || airCombat != 0)
			return true;
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		word hydra = INFO.getTypeUnitsInRadius(Zerg_Hydralisk, E, frontPos, 15 * TILE_SIZE, true).size();
		word zergling = INFO.getTypeUnitsInRadius(Zerg_Zergling, E, frontPos, 15 * TILE_SIZE, true).size();
		word sunken = INFO.getTypeBuildingsInRadius(Zerg_Sunken_Colony, E, frontPos, 15 * TILE_SIZE, true).size();
		word mutal = INFO.getTypeUnitsInRadius(Zerg_Mutalisk, E, frontPos, 15 * TILE_SIZE, true).size();
		uList luckers = INFO.getTypeUnitsInRadius(Zerg_Lurker, E, frontPos, 15 * TILE_SIZE, true);
		word lucker = 0;

		for (auto l : luckers)
		if (!l->unit()->canBurrow(false))
			lucker++;

		/*if ( (preAmountOfEnemy < hydra + zergling + sunken + lucker) ||
		((int)(zergling / 2) + (sunken * 4) + hydra + (lucker * 2)) >= vultureCnt || mutal != 0)*/
		if (((sunken * 3) + hydra + (lucker)) >= vultureCnt || mutal != 0)
			return true;
	}
	else
	{
		word vulture = INFO.getTypeUnitsInRadius(Terran_Vulture, E, frontPos, 15 * TILE_SIZE, true).size();
		word tank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, frontPos, 20 * TILE_SIZE, true).size();
		word goliath = INFO.getTypeUnitsInRadius(Terran_Goliath, E, frontPos, 15 * TILE_SIZE, true).size();
		word marine = INFO.getTypeUnitsInRadius(Terran_Marine, E, frontPos, 15 * TILE_SIZE, true).size();
		uList bunkers = INFO.getTypeBuildingsInRadius(Terran_Bunker, E, frontPos, 15 * TILE_SIZE, true);
		word bunker = 0;

		for (auto b : bunkers)
		if (b->getMarinesInBunker())
			bunker++;

		if ((vulture + (goliath * 1) + (tank * 2) + (bunker * 6)) > vultureCnt)
			return true;
	}

	return false;
}

bool VultureKiting::setDefenceMine(UnitInfo *v)
{
	if (!S->hasResearched(TechTypes::Spider_Mines) || v->unit()->getSpiderMineCount() == 0)
		return false;

	if (isInMyArea(v->pos()))
		return false;

	if (INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, v->pos(), 7 * TILE_SIZE).size() > 8)
		return false;

	for (auto t : INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, v->pos(), 4 * TILE_SIZE))
	{
		if (t->type() == Terran_Siege_Tank_Siege_Mode)
			return false;
	}

	v->unit()->useTech(TechTypes::Spider_Mines, findRandomeSpot(v->pos()));
	return true;
}

bool VultureKiting::setGuardMine(UnitInfo *v)
{
	if (!S->hasResearched(TechTypes::Spider_Mines) || v->unit()->getSpiderMineCount() == 0 || defenceMinePos.size() == 0)
		return false;

	if ((INFO.getSecondExpansionLocation(S) && target == INFO.getSecondExpansionLocation(S)->Center()) ||
		(INFO.getThirdExpansionLocation(S) && target == INFO.getThirdExpansionLocation(S)->Center()))
	{
		if (v->pos().getApproxDistance(target) > 10 * TILE_SIZE)
			return false;

		if (INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, v->pos(), 7 * TILE_SIZE).size() > 10)
			return false;
	}
	else
	{
		if (v->pos().getApproxDistance(INFO.getSecondChokePosition(S)) > 6 * TILE_SIZE)
			return false;

		if (INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, INFO.getSecondChokePosition(S), 7 * TILE_SIZE).size() > 10)
			return false;
	}

	if (mine_idx >= defenceMinePos.size()) mine_idx = 0;

	v->unit()->useTech(TechTypes::Spider_Mines, findRandomeSpot(defenceMinePos[mine_idx++]));

	return true;
}
