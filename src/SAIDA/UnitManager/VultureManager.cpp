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

		// 새로 만들어진 Vulture라면 Idle 로 설정
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

	// 전진 게이트 일 때는 파일런을 처리하기 위해서 별도 작업이 필요하다.
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
		// 내가 본진에 있고
		if (isSameArea(v->pos(), MYBASE)) {
			// 상대 캐리어가 내 본진에 있고
			if (!INFO.getTypeUnitsInArea(Protoss_Carrier, E, MYBASE).empty()) {
				// 골리앗들이 FirstChoke 근처에서 수비를 위해서 움직임을 취할 때.
				int carrierDefenceCntNearFirstChoke = 0;

				for (auto g : INFO.getUnits(Terran_Goliath, S)) {
					if (g->getState() == "CarrierDefence")
					if (g->pos().getApproxDistance(INFO.getFirstChokePosition(S)) <= 10 * TILE_SIZE)
						carrierDefenceCntNearFirstChoke++;
				}

				// 본진으로 움직여 준다.
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
			// 마인만 필요한 상태
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
	// 전진 건물 찾는 동작 마무리 됨 Done
	if (forwardBuildingPosition == Positions::None)
	{
		return ATTACKPOS;
	}

	// 전진 파일런 찾아서 부숴진 경우
	if (checkedForwardPylon)
	{
		// 파일런이 없는 경우
		if (INFO.getClosestTypeUnit(E, forwardBuildingPosition, Protoss_Pylon, 10 * TILE_SIZE, true) == nullptr)
		{
			// 게이트도 없는 경우 상황종료
			if (INFO.getClosestTypeUnit(E, forwardBuildingPosition, Protoss_Gateway, 10 * TILE_SIZE, true) == nullptr)
				forwardBuildingPosition = Positions::None;

			return ENBASE;
		}

		return forwardBuildingPosition;
	}

	if (forwardBuildingPosition == Positions::Unknown)// 모든 건물을 찾아야함.
	{
		for (auto &g : INFO.getBuildings(E))
		{
			// 맵 중앙보다 가까운 건물 // 파일런을 못보고 게이트만 봤을 때도 있을 수 있으므로 그리고 어차피 거기에 파일런이 있을 테니
			if (INFO.getSecondChokePosition(S).getApproxDistance(g.second->pos()) < INFO.getSecondChokePosition(S).getApproxDistance(theMap.Center()) + 10 * TILE_SIZE)
			{
				forwardBuildingPosition = g.second->pos();
				return forwardBuildingPosition;
			}
			else // 그 밖에서 봤으면 그냥 생까는 Gate다.
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
	// 앞마당의 Tower
	UnitInfo *frontVulture = kitingSet.getFrontUnitFromPosition(INFO.getMainBaseLocation(E)->Center());

	if (frontVulture == nullptr)
		return;

	uList diveVultures = INFO.getTypeUnitsInRadius(Terran_Vulture, S, frontVulture->pos(), 5 * TILE_SIZE);

	int VultureCnt = 0;

	for (auto v : diveVultures)
	{
		// hp 60% 이상인 Vulture만 Check
		if (v->hp() > (int)(v->type().maxHitPoints() * 0.6))
			VultureCnt++;
	}

	//  앞마당 안먹은 상태면 다이브 없음.
	Base *firstExpension = INFO.getFirstExpansionLocation(E);

	if (firstExpension == nullptr || firstExpension->GetOccupiedInfo() != enemyBase)
		return;

	int firstExpensionTower = firstExpension->GetEnemyGroundDefenseBuildingCount();

	if (INFO.enemyRace == Races::Terran)
		firstExpensionTower = firstExpension->GetEnemyBunkerCount();

	// 앞마당 방어타워 없을때는 어차피 다이브 안해.
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

			// kiting Set에서 Mine에 가장 가까운 Vulture
			UnitInfo *toBeAssigned = kitingSet.getFrontUnitFromPosition(m->pos());

			if (toBeAssigned == nullptr) continue;

			if (m->getKillMe() == KillMe && toBeAssigned->pos().getApproxDistance(m->pos()) > 6 * TILE_SIZE)
				continue;

			// State를 Mine제거로 바꿔줌
			toBeAssigned->setState(new VultureRemoveMineState(m->unit()));

			// MineMap에 할당해 줌
			removeMineMap[m->unit()] = toBeAssigned->unit();
			// KitingSet에서는 delete
			kitingSet.del(toBeAssigned);

			// KillMe Flag 는 2로
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

	// kiting Set에서 Mine에 가장 가까운 Vulture
	UnitInfo *toBeAssigned = kitingSet.getFrontUnitFromPosition(m->pos());

	if (toBeAssigned == nullptr) continue;

	if (m->getKillMe() == KillMe && toBeAssigned->pos().getApproxDistance(m->pos()) > 6 * TILE_SIZE)
	continue;

	// State를 Mine제거로 바꿔줌
	toBeAssigned->setState(new VultureRemoveMineState(m->unit()));

	// MineMap에 할당해 줌
	removeMineMap[m->unit()] = toBeAssigned->unit();
	// KitingSet에서는 delete
	kitingSet.del(toBeAssigned);

	// KillMe Flag 는 2로
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

		// 적이 없어도 드랍이 올 것 같으면 일단 Defence
		if (SM.getMainStrategy() != AttackAll &&
			(ESM.getWaitTimeForDrop() > 0 || E->getUpgradeLevel(UpgradeTypes::Ventral_Sacs))
			&& TIME - ESM.getWaitTimeForDrop() <= 24 * 60 * 3
			&& BasicBuildStrategy::Instance().hasSecondCommand()) {

			if (INFO.enemyRace != Races::Protoss || (EMB == Toss_drop || EMB == Toss_dark_drop))
			{
				// 앞마당에 너무 많은 적이 있으면 pass
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

	// 벌쳐 추가해야함.
	if (vultureDefenceSet.size() < needCnt) {

		UListSet allVSet;

		for (auto v : vList) {
			allVSet.add(v);
		}

		// 본진으로부터 가장 가까운 유닛들
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
	// 1분에 한번
	int checkThreshold = (24 * 60 * 1);

	if (lastScoutTime != 0 && (lastScoutTime + checkThreshold) < TIME)
	{
		scoutDone = false;
		lastScoutTime = 0;
	}

	// Waiting이 아니거나 Range가 아닐 때는 Scout 할 필요 없음.
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

	//// 마인 개발 안되어 있으면 일단 Pass
	////	if (!S->hasResearched(TechTypes::Spider_Mines)) return;

	//Position myBase = INFO.getSecondChokePosition(S);

	//UnitInfo *frontVulture = kitingSet.getFrontUnitFromPosition(SM.getMainAttackPosition());

	//if (frontVulture == nullptr)		return;

	////int needScout = kitingSet.size() > 3 ? 2 : 1;

	//// 만약 선두 벌쳐가 맵중앙 넘어에 있다면 그리고 주변에 적이 없다면.
	////	if (myBase.getApproxDistance(theMap.Center()) <= myBase.getApproxDistance(frontVulture->pos()) &&
	////			(INFO.getClosestUnit(E, frontVulture->pos(), GroundCombatKind, 10 * TILE_SIZE) == nullptr || kitingSet.size() > 4)) // 8벌쳐면 그냥 scout 가자*/
	//{
	//	needScout = true;
	//	/*
	//	uList sortedList = kitingSet.getSortedUnitList(MYBASE); // 뒤에 있는 벌쳐가 간다.

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

		if (dangerUnit == nullptr) // DangerUnit 없음.
		{
			if (setGuardMine(v))
				continue;

			// 앞마당에서 방어타워가 건설중에 있는 경우 본진으로 달려가도록 한다.
			UnitInfo *buildingTower = INFO.getClosestTypeUnit(E, v->pos(), INFO.getAdvancedDefenseBuildingType(INFO.enemyRace), 10 * TILE_SIZE);

			// 건설중인 타워가 있는데...
			if (buildingTower != nullptr && enFirstExp && isSameArea(buildingTower->pos(), enFirstExp->Center())) {
				// 앞마당이면 본진으로 들어가고
				CommandUtil::move(v->unit(), target);
				continue;
				// 본진이면 범위 밖으로 가야 된다. ToDo....
				//else if (isSameArea((TilePosition)buildingTower->pos(), (TilePosition)target))
			}

			UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 15 * TILE_SIZE);

			if (closestWorker != nullptr) // 일꾼 공격
				kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
			else {

				if (INFO.enemyRace == Races::Zerg) {
					UnitInfo *lurker = INFO.getClosestTypeUnit(E, v->pos(), Zerg_Lurker, 8 * TILE_SIZE);

					if (lurker) {
						CommandUtil::attackUnit(v->unit(), lurker->unit());
						continue;
					}
				}

				if (v->pos().getApproxDistance(target) > 5 * TILE_SIZE) // 목적지 이동
					CommandUtil::move(v->unit(), target);
				else if (TIME < (24 * 60 * 10) && (EIB == Toss_1g_forward || EIB == Toss_2g_forward)) // 본진 파일런이 아닌 경우에 파일런은 깬다.
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
		} // DangerUnit 없음 종료
		else //dangerUnit 있음
		{
			//질 저는 무조건 카이팅.. 사실 저글링은 벌쳐 한마리로 막는게 좀 빡시긴 하다.
			if (isNeedKitingUnitType(dangerUnit->type()))
			{
				UnitInfo *closestAttack = INFO.getClosestUnit(E, v->pos(), GroundCombatKind, 10 * TILE_SIZE, true, false, true);

				if (closestAttack == nullptr) // 보이질 않아
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

					// 주변에 Tank가 있으면 Kiting 하지 말고 그냥 공격하는게 낫다.
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
			} // 질저 Kiting 종료
			else // 질저 아님
			{

				if (needFight)
				{
					UnitInfo *closestAttack = INFO.getClosestUnit(E, frontUnit->pos(), GroundCombatKind, 15 * TILE_SIZE, false, false, true);

					if (closestAttack == nullptr) // 보이질 않아
					{
						UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

						if (closestWorker != nullptr)
							kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
						else if (v->pos().getApproxDistance(target) > 5 * TILE_SIZE) // 목적지 이동
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
				}// need Fight 종료

				if (SM.getMainStrategy() == AttackAll)
				{
					if (dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
					{
						if (dangerPoint > 4 * TILE_SIZE) // 멀찌감치면 그냥 공격하고...
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

					if (dangerPoint > 4 * TILE_SIZE) // 멀찌감치면 그냥 공격하고...
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
						else // 지상 유닛의 경우 Tank로 간다
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

								if (closest == closestTank && !closest->unit()->canUnsiege()) // 탱크 모드라면 너무 멀리가지 말자
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
				}// Attack All 종료
				else // Attack All 아님
				{
					if (needWaiting) // 대기모드 상태
					{
						if (!dangerUnit->isHide()) // 위험 유닛을 실제 보면 Time 갱신
							waitingTime = TIME;

						// 보고 미리 빠지기 위해 넉넉하게
						int backThreshold = 8;

						// 시즈와 타워는 테두리에
						if (dangerUnit->type() == Terran_Siege_Tank_Siege_Mode || dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
							backThreshold = 4;

						// 러커는 따로 처리
						if (dangerUnit->type() == Zerg_Lurker)
							backThreshold = 3;

						// 안전한 상태
						if (dangerPoint > backThreshold * TILE_SIZE)
						{
							// Mine 심으면 스킵
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
						else // 위험한 상태
						{
							// 러커는 안나오는데..
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
					} // 대기모드 종료
					else // Waiting 아님
					{
						// dangerUnit이 Tower일때
						if (dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
						{
							if (enFirstExp && isSameArea(dangerUnit->pos(), enFirstExp->Center()))
							{
								needWaiting = 1;
								waitingTime = TIME;
							}
							else // 앞마당 말고 Tower
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
								else // closest Attacker 존재
								{
									if (isNeedKitingUnitType(closestAttack->type()))
									{
										kiting(v, closestAttack, dangerPoint, 4 * TILE_SIZE);
									}
								}
							}
						}
						else // 타워가 아닌경우
						{
							if (needCheck == true && dangerUnit->isHide())
							{
								CommandUtil::move(v->unit(), target);
								continue;
							}

							needWaiting = 2;
							waitingTime = TIME;
						}
					} //// Waiting 아님
				} //Attack All 아님
			}//질저 아님
		}// Danger 없음.
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
			// DrawLine -> 공격
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

				if (weakUnit != nullptr) // 공격 대상 설정 변경
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

		// Attack All 일때는 왠만하면 벌쳐끼리 공격가지 말자.
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
