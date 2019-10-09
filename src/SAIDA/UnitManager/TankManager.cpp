#include "TankManager.h"

using namespace MyBot;

TankManager::TankManager()
{
	waitingPosition = INFO.getSecondAverageChokePosition(S);
	zealotAllinRush = false;
}

TankManager &TankManager::Instance()
{
	static TankManager instance;
	return instance;
}

void TankManager::update()
{
	if (TIME < 300 || TIME % 2 != 0)
		return;

	uList tankList;

	try {
		frontTankOfNotDefenceTank = nullptr;
		notDefenceTankList.clear();

		if (firstDefencePositions.empty() || (SM.getMainStrategy() == WaitToFirstExpansion && TIME % (24 * 60) == 0 )) // 1분에 한번 씩 refresh
			getClosestPosition();

		tankList = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);

		if (tankList.empty())
			return;

		setFirstExpansionSecure();
		setTankDefence();
		checkMultiBreak();
		checkKeepMulti();
		checkKeepMulti2();
		checkDropship();

		if (INFO.enemyRace == Races::Terran)
			checkProtectAdditionalMulti();
	}
	catch (SAIDA_Exception e) {
		Logger::error("common logic Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("common logic Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("common logic Unknown Error.\n");
		throw;
	}

	/////////////////// 테란 전
	if (INFO.enemyRace == Races::Terran)
	{
		for (auto t : tankList)
		{
			if (isStuckOrUnderSpell(t))
				continue;

			string state = t->getState();

			if (state == "New")
				setIdleState(t);

			if (state == "Idle")
			{
				if (S->hasResearched(TechTypes::Tank_Siege_Mode))
					setStateToTank(t, new TankSiegeLineState());
				else
					setStateToTank(t, new TankFightState());
			}

			if (state != "KeepMulti")
			{
				t->action();
			}
		}

		keepMultiSet.action(1);

		if (setMiddlePosition && SM.getNeedSecondExpansion() && !SM.getNeedThirdExpansion())
		{
			keepMultiSet2.action(2, false);
		}
		else
		{
			keepMultiSet2.action(2);
		}

		return;
	}

	////////////// 테란 그 외 종족
	if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion)
	{
		for (auto t : tankList)
		{
			if (isStuckOrUnderSpell(t))
				continue;

			string state = t->getState();

			if (state == "New" || state == "AttackMove" ||
					(state == "BackMove" && t->pos().getApproxDistance(INFO.getSecondChokePosition(S)) < 3 * TILE_SIZE) ) {
				setIdleState(t);
			}

			t->action();
		}

		tankFirstExpansionSecureSet.action();
	}
	else if (SM.getMainStrategy() == BackAll) {
		for (auto t : tankList)
		{
			if (isStuckOrUnderSpell(t))
				continue;

			if (t->getState() != "BackMove") {
				setStateToTank(t, new TankBackMove(INFO.getSecondChokePosition(S)));
			}

			t->action();
		}
	}
	else if (SM.getMainStrategy() == AttackAll) {

		try {
			frontTankOfNotDefenceTank = notDefenceTankList.getFrontUnitFromPosition(SM.getMainAttackPosition());
		}
		catch (SAIDA_Exception e) {
			Logger::error("getFrontUnitFromPosition Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
			throw e;
		}
		catch (const exception &e) {
			Logger::error("getFrontUnitFromPosition Error. (Error : %s)\n", e.what());
			throw e;
		}
		catch (...) {
			Logger::error("getFrontUnitFromPosition Unknown Error.\n");
			throw;
		}

		bool gatherFlag = false;

		try {
			if (INFO.enemyRace == Races::Protoss)
				gatherFlag = needGathering();
		}
		catch (SAIDA_Exception e) {
			Logger::error("needGathering Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
			throw e;
		}
		catch (const exception &e) {
			Logger::error("needGathering Error. (Error : %s)\n", e.what());
			throw e;
		}
		catch (...) {
			Logger::error("needGathering Unknown Error.\n");
			throw;
		}

		setNextMovingPoint();
		setWaitAtChokePoint();
		gatheringSet.init();
		siegeAllSet.clear();

		if (gatherFlag) {
			setGatheringMode(notDefenceTankList.getUnits());

			try {
				gatheringSet.action();
			}
			catch (SAIDA_Exception e) {
				Logger::error("gatheringSet Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
				throw e;
			}
			catch (const exception &e) {
				Logger::error("gatheringSet Error. (Error : %s)\n", e.what());
				throw e;
			}
			catch (...) {
				Logger::error("gatheringSet Unknown Error.\n");
				throw;
			}

		}
		else {
			gatheringSet.init();
			setSiegeNeedTank(notDefenceTankList.getUnits());

			try {
				if (needWaitAtFirstenemyExapnsion()) {
					siegeWatingMode();
				}
				else {
					stepByStepMove();
					useScanForCannonOnTheHill();
				}

			}
			catch (SAIDA_Exception e) {
				Logger::error("stepByStepMove Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
				throw e;
			}
			catch (const exception &e) {
				Logger::error("stepByStepMove Error. (Error : %s)\n", e.what());
				throw e;
			}
			catch (...) {
				Logger::error("stepByStepMove Unknown Error.\n");
				throw;
			}
		}

		// NotDefence 외의 탱크들에 대한 처리
		for (auto t : tankList)
			if (!isStuckOrUnderSpell(t))
				t->action();
	}

	else {
		// Eliminate
		for (auto t : tankList)
			t->action();
	}
}

void TankManager::useScanForCannonOnTheHill()
{
	if (frontTankOfNotDefenceTank) {
		uList buildings = INFO.getBuildingsInRadius(E, frontTankOfNotDefenceTank->pos(), WeaponTypes::Arclite_Shock_Cannon.maxRange() - 2 * TILE_SIZE, true, false, true);

		for (auto b : buildings) {
			// 언덕위에 있는 캐논
			if (b->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace)) {
				if (INFO.getAltitudeDifference((TilePosition)b->pos(), (TilePosition)frontTankOfNotDefenceTank->pos()) > 0) {
					if (b->isHide()) {
						if (ComsatStationManager::Instance().getAvailableScanCount() > 3) {
							ComsatStationManager::Instance().useScan(b->pos());
							break;
						}
					}
				}
			}
		}

	}
}

bool TankManager::needWaitAtFirstenemyExapnsion() {

	if (exceedMaximumSupply && S->supplyUsed() < 200) {
		exceedMaximumSupply = false;
	}

	// 탱크가 적 앞마당에 도착했고, 적 앞마당 넥서스 날렸을 때
	if (!SM.getNeedAttackMulti() || INFO.getFirstChokePosition(E) == Positions::None) {
		return false;
	}

	if (EMB == Toss_fast_carrier)
		return false;

	if (!INFO.getMainBaseLocation(E))
		return false;

	// 본진 넥서스 날라갔을때
	uList nexus = INFO.getTypeBuildingsInRadius(INFO.getBasicResourceDepotBuildingType(INFO.enemyRace), E, INFO.getMainBaseLocation(E)->Center(), 5 * TILE_SIZE, true);

	if (nexus.empty())
		return false;

	if (INFO.getFirstExpansionLocation(E)) {

		Position firstExpansionE = INFO.getFirstExpansionLocation(E)->Center();
		UnitInfo *firstTank = TM.getFrontTankFromPos(firstExpansionE);

		// 탱크가 적 앞마당에 도착했고, 적 앞마당 넥서스 날렸을 때
		if (firstTank && isSameArea(firstTank->pos(), firstExpansionE)
				&& INFO.getTypeBuildingsInArea(INFO.getBasicResourceDepotBuildingType(INFO.enemyRace), E, firstExpansionE, true).empty()) {

			// 조이고 있는 상태에서 인구수가 198이 한번이라도 넘으면 앞으로는 기다리지 않는다.
			if ((S->supplyUsed() >= 396 || exceedMaximumSupply)) {
				exceedMaximumSupply = true;
				return false;
			}
		}
	}

	return true;
}

void TankManager::checkDropship()
{
	if (SM.getDropshipMode() == false)
		return;

	int TankSpace = Terran_Siege_Tank_Tank_Mode.spaceRequired();

	word dropshipSetNum = 4;
	UnitInfo *availDropship = nullptr;

	for (auto ds : INFO.getUnits(Terran_Dropship, S))
	{
		if (ds->getSpaceRemaining() >= TankSpace &&
				(ds->getState() == "Boarding" || (ds->getState() == "Back" && DropshipManager::Instance().getBoardingState())))
		{
			availDropship = ds;
			break;
		}
	}

	if (availDropship != nullptr)
	{
		if (INFO.enemyRace == Races::Terran)
		{
			uList sortedTank = siegeLineSet.getSortedUnitList(MYBASE);

			for (auto t : sortedTank)
			{
				//Dropship TEST
				if (dropshipSet.size() < dropshipSetNum)
				{
					setStateToTank(t, new TankDropshipState(availDropship->unit()));
					availDropship->delSpaceRemaining(TankSpace);

					if (availDropship->getSpaceRemaining() < TankSpace)
						break;
				}
			}
		}
		else
		{
			uList sortedTank = notDefenceTankList.getSortedUnitList(availDropship->pos());

			for (auto t : sortedTank)
			{
				//Dropship TEST
				if (dropshipSet.size() < dropshipSetNum)
				{
					setStateToTank(t, new TankDropshipState(availDropship->unit()));
					availDropship->delSpaceRemaining(TankSpace);

					if (availDropship->getSpaceRemaining() < TankSpace)
						break;
				}
			}
		}
	}
}

bool TankManager::enoughTankForDrop()
{
	if (INFO.enemyRace == Races::Terran)
	{
		//bw->drawTextScreen(Position(100, 100), " SizeLineSet : %d", siegeLineSet.size());

		if (siegeLineSet.size() + dropshipSet.size() >= 8)
		{
			return true;
		}
	}
	else
	{
		if (notDefenceTankList.size() + dropshipSet.size() >= 8)
		{
			return true;
		}
	}

	return false;
}

void TankManager::checkMultiBreak()
{
	if (SM.getNeedAttackMulti() == false)
	{
		if (multiBreakSet.size())
		{
			uList listToIdle = multiBreakSet.getUnits();

			for (auto t : listToIdle)
				setStateToTank(t, new TankIdleState());
		}

		return;
	}

	if (SM.getSecondAttackPosition() == Positions::Unknown || !SM.getSecondAttackPosition().isValid())
		return;

	int needTankCnt = needCountForBreakMulti(Terran_Siege_Tank_Tank_Mode);

	// 이부분에서 Multi Break 병력을 빼준다.
	if ((int)multiBreakSet.size() < needTankCnt)
	{
		if (INFO.enemyRace == Races::Terran)
		{
			uList sortedList = siegeLineSet.getSortedUnitList(SM.getSecondAttackPosition());

			for (auto t : sortedList)
			{
				setStateToTank(t, new TankMultiBreakState());

				if (multiBreakSet.size() == needTankCnt || siegeLineSet.size() < 5)
					break;
			}
		}
		else
		{
			uList sortedList = notDefenceTankList.getSortedUnitList(INFO.getMainBaseLocation(S)->Center());

			for (auto t : sortedList)
			{
				setStateToTank(t, new TankMultiBreakState());
				notDefenceTankList.del(t); // State로 관리되지 않기때문에 이부분은 따로 빼줘야 함.

				word minValue = INFO.enemyRace == Races::Protoss ? 8 : 5;

				if (multiBreakSet.size() == needTankCnt || notDefenceTankList.size() < minValue)
					break;
			}
		}
	}
}

void TankManager::checkKeepMulti()
{
	if (!SM.getNeedSecondExpansion())
		return;

	Base *base = INFO.getSecondExpansionLocation(S);

	if (base == nullptr || base->Minerals().size() == 0)
	{
		uList tanksToIdle = keepMultiSet.getUnits();

		for (auto t : tanksToIdle)
			setStateToTank(t, new TankIdleState());

		return;
	}

	if (INFO.enemyRace == Races::Terran)
	{
		if (multiBase == Positions::Unknown)
			multiBase = INFO.getSecondExpansionLocation(S)->getPosition();

		if (multiBase == Positions::Unknown)
			return;

		if (keepMultiSet.size() > 1)
			return;

		uList sortedTank = siegeLineSet.getSortedUnitList(multiBase);

		for (auto t : sortedTank)
		{
			setStateToTank(t, new TankKeepMultiState(multiBase));

			if (keepMultiSet.size() > 1)
				break;
		}
	}
	else
	{
		if (!SM.getNeedKeepSecondExpansion(Terran_Siege_Tank_Tank_Mode) && !SM.getNeedKeepThirdExpansion(Terran_Siege_Tank_Tank_Mode)) {
			uList tanksToIdle = keepMultiSet.getUnits();

			for (auto t : tanksToIdle)
				setStateToTank(t, new TankIdleState());
		}
		else
		{
			for (auto t : allTanks.getUnits())
			{
				if (t->getState() == "New" && t->isComplete())
				{
					setStateToTank(t, new TankKeepMultiState()); // action에서 target Position을 잡음.
					notDefenceTankList.del(t);
				}
			}
		}
	}

	return;
}

void TankManager::checkKeepMulti2()
{
	if (!SM.getNeedSecondExpansion())
		return;

	if (!SM.getNeedThirdExpansion())
	{
		if (!setMiddlePosition)
		{
			if (SM.getNeedSecondExpansion() && !keepMultiSet.isEmpty())
			{
				int distanceFromSecondChokePoint = 0;
				int distanceFromKeepMultiSet = 0;
				Position firstTankPosition = (*keepMultiSet.getSortedUnitList(INFO.getSecondExpansionLocation(S)->getPosition()).begin())->pos();
				theMap.GetPath(INFO.getSecondChokePosition(S), INFO.getSecondExpansionLocation(S)->getPosition(), &distanceFromSecondChokePoint);
				theMap.GetPath(firstTankPosition, INFO.getSecondExpansionLocation(S)->getPosition(), &distanceFromKeepMultiSet);

				if (distanceFromSecondChokePoint != -1 && distanceFromKeepMultiSet != -1
						&& distanceFromSecondChokePoint / 2 > distanceFromKeepMultiSet)
				{
					multiBase2 = firstTankPosition;
					Position tempPos = getDirectionDistancePosition(firstTankPosition, theMap.Center(), 3 * TILE_SIZE);

					if (!tempPos.isValid())
					{
						tempPos = firstTankPosition;
					}

					keepMultiSet2.setMiddlePos(tempPos);
					setMiddlePosition = true;
					cout << "#### [Tank] multiBase2 = " << multiBase2 / TILE_SIZE << " 구했어!" << endl;
				}
			}
			else
				return;
		}
	}
	else
	{
		Base *base = INFO.getThirdExpansionLocation(S);

		if (base == nullptr || base->Minerals().size() == 0)
		{
			uList tanksToIdle = keepMultiSet2.getUnits();

			for (auto t : tanksToIdle)
				setStateToTank(t, new TankIdleState());

			return;
		}

		if (multiBase2 != INFO.getThirdExpansionLocation(S)->getPosition())
			multiBase2 = INFO.getThirdExpansionLocation(S)->getPosition();

		if (setMiddlePosition)
		{
			for (auto k : keepMultiSet2.getUnits())
			{
				k->setState(new TankKeepMultiState(multiBase2));
			}

			setMiddlePosition = false;
			cout << "#### [Tank] multiBase2 = " << multiBase2 / TILE_SIZE << " 로 리셋!!! 이동하자 이동" << endl;
		}
	}

	if (setMiddlePosition)
	{
		uList commandCenter = INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, INFO.getSecondExpansionLocation(S)->getPosition(), 5 * TILE_SIZE);
		bool needSet2 = false;

		for (auto c : commandCenter)
		{
			if (c->isComplete())
			{
				needSet2 = true;
			}
			else
			{
				if ((c->unit()->getRemainingBuildTime() * 100) / c->type().buildTime() < 30)
					needSet2 = true;
			}
		}

		if (!needSet2)
			return;
	}

	if (!multiBase2.isValid())
		return;

	if (keepMultiSet2.size() > 1)
		return;

	if (INFO.enemyRace == Races::Terran)
	{
		uList sortedTank = siegeLineSet.getSortedUnitList(multiBase2);

		for (auto t : sortedTank)
		{
			siegeLineSet.del(t);
			keepMultiSet2.add(t);
			t->setState(new TankKeepMultiState(multiBase2));
			cout << "--------- [Tank] KeepMulti2 부여" << endl;
			//setStateToTank(t, new TankKeepMultiState(multiBase2));

			if (keepMultiSet2.size() > 1)
				break;
		}
	}
	else
	{

	}

	return;
}

void TankManager::checkZealotAllinRush(const uList &tankList) {

	zealotAllinRush = false;
	int zSize = INFO.getTypeUnitsInRadius(Protoss_Zealot, E, INFO.getSecondChokePosition(S), 30 * TILE_SIZE).size();
	int vSize = INFO.getTypeUnitsInRadius(Terran_Vulture, S, INFO.getSecondChokePosition(S), 30 * TILE_SIZE).size();
	int tSize = tankList.size();

	if (zSize - vSize >= tSize)
		zealotAllinRush = true;

	return;
}

void TankManager::checkProtectAdditionalMulti()
{
	for (auto &b : INFO.getAdditionalExpansions())
	{
		Position p = b->Center();

		if (INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, p, 5 * TILE_SIZE, true).empty()) {
			continue;
		}

		if (protectAdditionalMultiMap[p] == nullptr || !protectAdditionalMultiMap[p]->exists())
		{
			UnitInfo *tank = getFrontTankFromPos(p);

			if (tank != nullptr)
			{
				protectAdditionalMultiMap[p] = tank->unit();
				setStateToTank(tank, new TankProtectAdditionalMultiState(b));
			}
		}
	}
}

void TankManager::setFirstExpansionSecure()
{
	if (INFO.enemyRace == Races::Terran)
		return;

	if (SM.getMainStrategy() != WaitToBase && SM.getMainStrategy() != WaitToFirstExpansion)
		return;

	// 평지에서 시즈모드 안되어 있으면 skip
	if (INFO.getAltitudeDifference() == 0 && !S->hasResearched(TechTypes::Tank_Siege_Mode))
		return;

	if (INFO.getFirstChokeDefenceCnt() != 0 || allTanks.size() > 1) {

		if (tankFirstExpansionSecureSet.size() >= 2)
			return;

		word needCnt = 0;

		Position posOnHill = TM.getPositionOnTheHill();
		Position posOnMineral = TM.getPositionNearMineral();

		if (posOnHill != Positions::None)
			needCnt++;

		if (posOnMineral != Positions::None && allTanks.size() > 2)
			needCnt++;

		if (needCnt == 0)
			needCnt++;

		uList sortedList = allTanks.getSortedUnitList(MYBASE);

		for (auto t : sortedList) {
			if (tankFirstExpansionSecureSet.size() >= needCnt)
				break;

			if (t->getState() != "FirstExpansionSecure" && t->getState() != "BaseDefence" && t->getState() != "Defence")
				setStateToTank(t, new TankFirstExpansionSecureState());
		}
	}
	else {
		for (auto t : allTanks.getUnits()) {
			if (t->getState() == "FirstExpansionSecure")
				setStateToTank(t, new TankIdleState());
		}
	}

}

// [Action] : 단체로 시즈모드/시즈모드해제 하면서 전진
void TankManager::setAllSiegeMode(const uList &tankList)
{

	//for (auto t : tankList)
	//	if (t->getState() != "SiegeAllState")
	//		setStateToTank(t, new TankSiegeAllState());

	//siegeAllSet.setSiegeNeedTank();

}

void TankManager::setGatheringMode(const uList &tankList)
{
	for (auto t : tankList)
		setStateToTank(t, new TankGatheringState());
}

void TankManager::setIdleState(UnitInfo *t)
{
	if (!isMyCommandAtFirstExpansion()) {
		t->setState(new TankIdleState(INFO.getWaitingPositionAtFirstChoke(5, 8)));
		return;
	}

	// 이미 할당된 위치가 있는 탱크라면
	for (auto &p : firstDefencePositions)
	{
		if (p.second == t) {
			t->setState(new TankIdleState(p.first));
			return;
		}
	}

	Position candidate = Positions::None;
	int tmp = 0;
	int minDist = MAXINT;

	// 새롭게 할당해야 하는 애라면, idle pos중 가장 가까운 애를 선택한다.
	for (auto &p : firstDefencePositions)
	{
		// 할당 안된 위치라면 가장 가까운 애를 찾음.
		if (p.second == nullptr) {
			tmp = 3 * p.first.getApproxDistance(INFO.getSecondAverageChokePosition(S)) + p.first.getApproxDistance(INFO.getFirstChokePosition(S));

			if (minDist > tmp) {
				minDist = tmp;
				candidate = p.first;
			}
		}
	}

	if (candidate != Positions::None) {
		t->setState(new TankIdleState(candidate));
		firstDefencePositions[candidate] = t;
		return;
	}

	t->setState(new TankIdleState(INFO.getSecondAverageChokePosition(S)));

	if (INFO.getFirstExpansionLocation(S)->getPosition().getApproxDistance((Position)INFO.getSecondChokePoint(S)->Center()) > 540) {
		t->setState(new TankIdleState(INFO.getSecondAverageChokePosition(S, 3, 1)));
	}

	// 할당된 위치가 없으면,
}

void TankManager::setTankDefence()
{
	uList elist;

	if (INFO.enemyRace == Races::Terran)
		elist = INFO.enemyInMyArea();
	else
		elist = getEnemyInMyYard(getGroundDistance(MYBASE, INFO.getSecondChokePosition(S)) + 12 * TILE_SIZE, UnitTypes::AllUnits, false);

	word needCnt = 0;
	word baseNeedCnt = 0;
	word overloadCnt = 0;
	UnitType uType;

	for (auto e : elist) {

		if (e->type() != Zerg_Lurker && e->isHide())
			continue;

		uType = e->type();


		// 병력이 내 본진 안에 있다면
		if (isSameArea(e->pos(), MYBASE)) {
			// 병력이 있는 수송선이 있다면
			if (uType == Protoss_Shuttle || uType == Terran_Dropship)
				baseNeedCnt += ((uType.spaceProvided() - e->getSpaceRemaining()) / 2);
			else if (uType == Protoss_Reaver)
				baseNeedCnt += 2;
			else if (uType == Protoss_Dark_Templar)
				baseNeedCnt += 2;
			else if (!e->type().isFlyer() && !e->type().isWorker())
				baseNeedCnt += 1;
		}
		// 병력이 본진 밖에 있다면
		else {
			// 병력이 있는 수송선이 있다면
			if (uType == Protoss_Shuttle || uType == Terran_Dropship)
				needCnt += ((uType.spaceProvided() - e->getSpaceRemaining()) / 2);
			else if (uType == Protoss_Reaver)
				needCnt += 2;
			else if (uType == Protoss_Dark_Templar)
				needCnt += 2;
			else if (!e->type().isFlyer())
				needCnt += 1;
		}

		if (uType == Zerg_Overlord)
			overloadCnt += ((uType.spaceProvided() - e->getSpaceRemaining()) / 2);
	}

	// 저그 드랍 가능성 있음.
	if (overloadCnt > 8)
		baseNeedCnt += (overloadCnt * 3 / 8);

	if (baseNeedCnt == 0) {
		// 상대가 드롭을 할 수 있는 상황이면 항상 2마리의 시즈를 대기시킨다.
		// 이미 드랍을 왔던 상황이 아니라면
		if ( SM.getMainStrategy() != AttackAll &&
				(ESM.getWaitTimeForDrop() > 0 || E->getUpgradeLevel(UpgradeTypes::Ventral_Sacs))
				&& TIME - ESM.getWaitTimeForDrop() <= 24 * 60 * 3
				&& BasicBuildStrategy::Instance().hasSecondCommand()
				&& INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 2) {

			if (INFO.enemyRace != Races::Protoss || (EMB == Toss_drop || EMB == Toss_dark_drop))
			{
				// 앞마당에 너무 많은 적이 있으면 pass
				if (INFO.getUnitsInRadius(E, INFO.getSecondChokePosition(S), 10 * TILE_SIZE, true).size() < 5)
					baseNeedCnt = 1;
			}

		}

	}

	if (needCnt + baseNeedCnt > 0) {
		// 공격이 아닐때는 수비에 좀 더 치중해도 된다. // 테란전은 WaitLine DrawLine이기 때문에 예외
		if (INFO.enemyRace != Races::Terran && SM.getMainStrategy() != AttackAll && SM.getMainStrategy() != Eliminate) {
			needCnt *= 2;
			baseNeedCnt *= 2;
		}

		int frontNeedCnt = allTanks.size() * needCnt / (needCnt + baseNeedCnt);

		needCnt = frontNeedCnt == 0 && needCnt > 0 ? 1 : frontNeedCnt;
		baseNeedCnt = min(allTanks.size() >= needCnt ? allTanks.size() - needCnt : 0, baseNeedCnt);
	}

	if (SM.getMainStrategy() == AttackAll) {
		needCnt = needCnt >= 3 ? 3 : needCnt;
		baseNeedCnt = baseNeedCnt >= 3 ? 3 : baseNeedCnt;
	}

	if (INFO.enemyRace == Races::Terran) // 테란전은 따로 가져간다.
	{
		if (!S->hasResearched(TechTypes::Tank_Siege_Mode))
		{
			baseNeedCnt = 0;

			for (auto eu : INFO.enemyInMyArea())
				if (!eu->type().isFlyer())
					needCnt++;
		}

		// baseDefence가 따로 없음.
		if (defenceTankSet.size() < baseNeedCnt + needCnt)
		{
			uList sortedTank;

			if (S->hasResearched(TechTypes::Tank_Siege_Mode))
				sortedTank = siegeLineSet.getSortedUnitList(MYBASE);
			else
				sortedTank = allTanks.getSortedUnitList(MYBASE);

			for (auto t : sortedTank)
			{
				setStateToTank(t, new TankDefenceState());

				if (defenceTankSet.size() >= baseNeedCnt + needCnt)
					break;
			}
		}

		if (defenceTankSet.size() > baseNeedCnt + needCnt)
		{
			uList sortedTank = defenceTankSet.getSortedUnitList(MYBASE, true);

			for (auto t : sortedTank)
			{
				setStateToTank(t, new TankIdleState());

				if (defenceTankSet.size() <= baseNeedCnt + needCnt)
					break;
			}
		}
	}
	else
	{
		if (baseDefenceTankSet.size() < baseNeedCnt) {
			// 본진으로부터 가장 가까운 유닛들
			uList sortedList = allTanks.getSortedUnitList(MYBASE);

			for (word i = 0; i < sortedList.size() && baseDefenceTankSet.size() < baseNeedCnt; i++) {

				if (sortedList[i]->getState() != "Defence" && sortedList[i]->getState() != "BaseDefence" && sortedList[i]->getState() != "MultiBreak" && sortedList[i]->getState() != "KeepMulti" && sortedList[i]->getState() != "Dropship") {

					if (sortedList[i]->getState() == "FirstExpansionSecure") {
						if (isSameArea(sortedList[i]->pos(), MYBASE) || allTanks.size() == 1) {
							if (i == 0) siegeModeDefenceTank = sortedList[i]->id();

							setStateToTank(sortedList[i], new TankBaseDefenceState());
						}
					}
					else {
						if (i == 0) siegeModeDefenceTank = sortedList[i]->id();

						setStateToTank(sortedList[i], new TankBaseDefenceState());
					}


				}
			}
		}
		else if (baseDefenceTankSet.size() > baseNeedCnt) {
			uList sortedList = baseDefenceTankSet.getSortedUnitList(MYBASE, true);

			for (word i = 0; i < sortedList.size() && baseDefenceTankSet.size() > baseNeedCnt; i++) {
				setStateToTank(sortedList[i], new TankIdleState());
			}
		}

		// 탱크 추가해야함.
		if (defenceTankSet.size() < needCnt) {
			// 본진으로부터 가장 가까운 유닛들
			uList sortedList = allTanks.getSortedUnitList(MYBASE);

			for (word i = 0; i < sortedList.size() && defenceTankSet.size() < needCnt; i++) {
				if (sortedList[i]->getState() != "Defence" && sortedList[i]->getState() != "BaseDefence" && sortedList[i]->getState() != "MultiBreak" && sortedList[i]->getState() != "KeepMulti" && sortedList[i]->getState() != "Dropship") {

					if (sortedList[i]->getState() == "FirstExpansionSecure") {
						if (INFO.getFirstChokeDefenceCnt() == 0 && allTanks.size() == 1) {
							setStateToTank(sortedList[i], new TankDefenceState());
						}
					}
					else
						setStateToTank(sortedList[i], new TankDefenceState());
				}
			}
		}
		else if (defenceTankSet.size() > needCnt) {
			uList sortedList = defenceTankSet.getSortedUnitList(MYBASE);

			for (word i = 0; i < sortedList.size() && defenceTankSet.size() > needCnt; i++) {
				setStateToTank(sortedList[i], new TankIdleState());
			}
		}

		for (auto t : allTanks.getUnits()) {
			if (t->getState() != "Defence" && t->getState() != "BaseDefence" && t->getState() != "MultiBreak" && t->getState() != "KeepMulti" && t->getState() != "Dropship")
				notDefenceTankList.add(t);
		}

		checkZealotAllinRush(allTanks.getUnits());

	}
}

word TankManager::getEnemyNeerMyAllTanks(uList tankList) {
	set<UnitInfo * > eSet;

	for (auto tank : tankList) {

		uList eList = INFO.getUnitsInRadius(E, tank->pos(), 12 * TILE_SIZE, true, false, true, true);

		for (auto ee : eList)
			eSet.insert(ee);
	}

	return eSet.size();
}

void TankManager::stepByStepMove()
{
	UnitInfo *frontUnit = frontTankOfNotDefenceTank;
	Position targetPos = TM.getNextMovingPoint();

	if (frontUnit == nullptr || targetPos == Positions::None)
		return;

	int distanceFromtarget = getGroundDistance(frontUnit->pos(), targetPos);

	double threshold = 0.3;
	bool needWaitGoliath = false;

	// 상대가 캐리어 + 지상군일때, 시즈가 골리앗보다 먼저 앞으로 가지 않도록 처리하자.
	if (EMB == Toss_arbiter_carrier || EMB == Toss_fast_carrier) {

		//지상군이 있고,
		if (SM.getMainAttackPosition() != Positions::Unknown &&
				INFO.getTypeUnitsInRadius(Protoss_Zealot, E, frontUnit->pos(), 30 * TILE_SIZE).size() + INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, frontUnit->pos(), 30 * TILE_SIZE).size() > 0) {
			UnitInfo *closeG = INFO.getClosestTypeUnit(S, SM.getMainAttackPosition(), Terran_Goliath, 0, false, true);

			if (closeG &&
					SM.getMainAttackPosition().getApproxDistance(closeG->pos()) >
					SM.getMainAttackPosition().getApproxDistance(frontUnit->pos()))
				needWaitGoliath = true;
		}

	}

	if (waitAtChokePoint || needWaitGoliath)
		threshold = 0.1;
	else if (INFO.getUnitsInRadius(E, frontUnit->pos(), 30 * TILE_SIZE, true, false, false, true, true).size() <
			 INFO.getUnitsInRadius(S, frontUnit->pos(), 15 * TILE_SIZE, true, true, false, false).size())
		threshold = 1;
	else if (frontUnit->pos().getApproxDistance(TM.getNextMovingPoint()) < 8 * TILE_SIZE)
		threshold = 0.5;

	if (INFO.enemyRace == Races::Zerg)
		threshold = 1;

	// [Action] : 후방 탱크중 30%씩 앞으로 보내면서 조이기 모드 시전
	uList sortTank = notDefenceTankList.getSortedUnitList(targetPos, true);
	int tankSize = (int)sortTank.size();
	int moveFrontCount = (int)(tankSize * threshold);
	moveFrontCount = moveFrontCount == 0 ? 1 : moveFrontCount;

	// Target으로부터 가장 멀리 있는 탱크들빼서 앞으로 보낼 대상이 되는 애들

	uList eList;
	uList eDList;
	uList eBList;
	Unit me;
	int i = 0;

	//Egg 없애는 로직 추가
	if (!eggRemoved && !sortTank.empty())
	{
		removeEgg();
		return;
	}


	for (; i < min(moveFrontCount, tankSize - 1); i++) {
		if (sortTank.at(i)->getState() != "SiegeMovingState")
			setStateToTank(sortTank.at(i), new TankSiegeMovingState());

		if (isStuckOrUnderSpell(sortTank.at(i)))
			continue;

		if (siegeAllSet.find(sortTank.at(i)) != siegeAllSet.end()) {
			if (sortTank.at(i)->unit()->isSieged()) {
				eList = INFO.getUnitsInRadius(E, sortTank.at(i)->pos(), 12 * TILE_SIZE, true, false, false);
				attackFirstTargetOfTank(sortTank.at(i)->unit(), eList);
			}
			else {
				if (sortTank.at(i)->unit()->isSelected()) cout << sortTank.at(i)->id() << ", 뒤에 있는 탱크, siegeAllSet에 있으므로 시즈" << endl;

				siege(sortTank.at(i)->unit());
			}

			continue;
		}

		// 시즈상태에서 주변에 적이 있으면
		if (sortTank.at(i)->unit()->isSieged()) {

			eList = INFO.getUnitsInRadius(E, sortTank.at(i)->pos(), 12 * TILE_SIZE, true, false, false);

			if (eList.size() != 0) {
				attackFirstTargetOfTank(sortTank.at(i)->unit(), eList);
				continue;
			}
		}

		// Case1 : 우리편 첫번째 탱크와 너무 멀리 떨어져 있으면 한마리 더 앞으로 보냄.
		if (getGroundDistance(sortTank.at(i)->pos(), frontUnit->pos()) >= 30 * TILE_SIZE)
			moveFrontCount++;

		moveForward(sortTank.at(i)->unit(), targetPos);
	}

	// 모두 전진
	if (threshold == 1) {
		if (!isStuckOrUnderSpell(frontUnit))
		{
			// 제일 앞에 있는 탱크에 대한 설정
			if (siegeAllSet.find(frontUnit) != siegeAllSet.end()) {
				if (frontUnit->unit()->isSieged()) {
					eList = INFO.getUnitsInRadius(E, frontUnit->pos(), 12 * TILE_SIZE, true, false, false);
					attackFirstTargetOfTank(frontUnit->unit(), eList);
				}
				else {
					if (frontUnit->unit()->isSelected()) cout << frontUnit->id() << ", 제일 앞에 있는 탱크, siegeAllSet 이라 시즈" << endl;

					siege(frontUnit->unit());
				}
			}
			else
				moveForward(frontUnit->unit(), targetPos);

			for (; i < tankSize; i++)
				if (sortTank.at(i)->getState() != "SiegeMovingState")
					setStateToTank(sortTank.at(i), new TankSiegeMovingState());

			return;
		}
	}

	for (; i < tankSize; i++) {

		if (sortTank.at(i)->getState() != "SiegeMovingState")
			setStateToTank(sortTank.at(i), new TankSiegeMovingState());

		if (isStuckOrUnderSpell(sortTank.at(i)))
			continue;

		eList = INFO.getUnitsInRadius(E, sortTank.at(i)->pos(), 12 * TILE_SIZE, true, false, false);
		eDList = INFO.getDefenceBuildingsInRadius(E, sortTank.at(i)->pos(), 12 * TILE_SIZE, true, false);
		eBList = INFO.getBuildingsInRadius(E, sortTank.at(i)->pos(), 10 * TILE_SIZE, true, false);
		me = sortTank.at(i)->unit();

		//SeigeAll 때문에 시즈해야하는 애들
		if (siegeAllSet.find(sortTank.at(i)) != siegeAllSet.end()) {
			if (!me->isSieged()) {
				siege(me);

				if (me->isSelected()) cout << me->getID() << ", 앞에 있는 탱크, siegeAllSet 이라 시즈" << endl;
			}
			else {
				eList.insert(eList.end(), eDList.begin(), eDList.end());
				attackFirstTargetOfTank(me, eList);
			}

			continue;
		}

		// 내가 길막하고 있는 경우, 무조건 전진
		if (eDList.empty() && (checkZeroAltitueAroundMe(sortTank.at(i)->pos(), 8) || (TIME - sortTank.at(i)->getLastPositionTime() > 20 * 24)))
		{
			if (me->isSieged()) {
				if (me->isSelected()) cout << me->getID() << ", 앞에 있는 탱크, 길막이라 unsiege" << endl;

				unsiege(me, true);
			}
			else {
				if (me->getGroundWeaponCooldown() == 0)
					CommandUtil::attackMove(me, targetPos);
				else
					CommandUtil::move(me, targetPos);
			}

			continue;
		}

		// Case 1. 길막하지 않는 시즈 상태에서는 그냥 시즈 상태로 유지한다.
		// Case 2. 시즈상태가 아니라면 시즈를 한다. 언제? Front 보다 전진하게 되면
		if (!me->isSieged()) {

			if (amIMoreCloseToTarget(me, targetPos, distanceFromtarget)
					&& INFO.getUnitsInRadius(E, me->getPosition(), 6 * TILE_SIZE, true, false, false, false).empty()) {
				if (me->isSelected()) cout << me->getID() << ", 앞에 있는 탱크, 제일 앞서나가서 시즈" << endl;

				siege(me);
			}
			else {
				if (me->getGroundWeaponCooldown() == 0)
					CommandUtil::attackMove(me, targetPos);
				else
					CommandUtil::move(me, targetPos);

			}
		}
		else {
			eList.insert(eList.end(), eDList.begin(), eDList.end());
			attackFirstTargetOfTank(me, eList);
		}

	}

}

void TankManager::siegeWatingMode()
{
	Position waitingPosition = INFO.getFirstChokePosition(E);

	if (waitingPosition == Positions::None)
		return;

	for (auto t : notDefenceTankList.getUnits()) {
		if (t->getState() != "SiegeMovingState")
			setStateToTank(t, new TankSiegeMovingState());

		uList eList = INFO.getUnitsInRadius(E, t->pos(), 12 * TILE_SIZE, true, false);
		uList closeElist = INFO.getUnitsInRadius(E, t->pos(), 6 * TILE_SIZE, true, false);

		if (t->unit()->isSieged()) {
			// 주변에 적 유닛이 없으면,
			// 시즈를 풀고 waitingPosition에 15 타일 이하로 접근한다.
			if (!eList.empty()) {

				// 모든 적이 가까이 있으면
				if (eList.size() == closeElist.size())
					unsiege(t->unit(), false);
				else {
					attackFirstTargetOfTank(t->unit(), eList);
				}
			}
			else {
				// 주변에 적이 없어
				// 공격 가능한 적이 없으면, 가까이 다가가기 위해서 Unsiege
				if (t->pos().getApproxDistance(waitingPosition) > 15 * TILE_SIZE)
					unsiege(t->unit(), false);
			}
		}
		else {

			// 적이 없음
			if (eList.empty()) {
				// WaitingPosition에 가까이 간다.
				if (t->pos().getApproxDistance(waitingPosition) > 15 * TILE_SIZE)
					CommandUtil::attackMove(t->unit(), waitingPosition);
				else if (!checkZeroAltitueAroundMe(t->pos(), 6))
					siege(t->unit());
			}
			else {
				// 적이 있으면,
				// 모든 적이 가까이 있지 않으면 시즈
				if (eList.size() != closeElist.size()) {
					siege(t->unit());
				}
				else {
					//모든 적이 가까이 있으면 카이팅
					UnitInfo *closeUnitInfo = INFO.getClosestUnit(E, t->pos(), TypeKind::GroundUnitKind, WeaponTypes::Arclite_Shock_Cannon.maxRange(), false);

					if (closeUnitInfo == nullptr)
						continue;

					attackFirstkiting(t, closeUnitInfo, t->pos().getApproxDistance(closeUnitInfo->pos()), 5 * TILE_SIZE);
				}
			}
		}
	}
}

bool TankManager::amIMoreCloseToTarget(Unit me, Position targetPos, int dFromFrontToTarget) {
	int dBetweenMeAndTarget = 0;

	theMap.GetPath(me->getPosition(), targetPos, &dBetweenMeAndTarget);

	return dBetweenMeAndTarget != -1 && (dFromFrontToTarget - dBetweenMeAndTarget > 2 * TILE_SIZE ||
										 dFromFrontToTarget == dBetweenMeAndTarget);
}

void TankManager::moveForward(Unit me, Position pos) {


	if (me->isSieged()) {
		uList eListFar = INFO.getUnitsInRadius(E, me->getPosition(), 12 * TILE_SIZE, true, false, false);
		uList eBList = INFO.getBuildingsInRadius(E, me->getPosition(), 8 * TILE_SIZE, true, false, false);
		uList eListClose = INFO.getUnitsInRadius(E, me->getPosition(), 5 * TILE_SIZE, true, false, false);

		if (eBList.size() < 3 && (eListFar.size() == 0 || eListFar.size() != eListClose.size())) {
			if (me->isSelected()) cout << me->getID() << ", moveForward, 적이 근처에 없으므로 Unsiege" << endl;

			unsiege(me, false);
		}
	}
	else {

		UnitInfo *closest = INFO.getClosestUnit(E, me->getPosition(), GroundUnitKind, 8 * TILE_SIZE, true, false, false);

		if (closest) {

			if (me->getGroundWeaponCooldown() == 0)
				CommandUtil::attackUnit(me, closest->unit());
			else {
				int weaponRange = closest->type().groundWeapon().maxRange();

				if (me->getPosition().getApproxDistance(closest->pos()) > weaponRange + 2 * TILE_SIZE)
					CommandUtil::attackMove(me, closest->pos());
				else
					moveBackPostion(INFO.getUnitInfo(me, S), closest->pos(), 3 * TILE_SIZE);

			}
		}
		else {
			if (me->getGroundWeaponCooldown() == 0)
				CommandUtil::attackMove(me, nextMovingPoint);
			else {
				CommandUtil::move(me, nextMovingPoint);
			}
		}

	}
}

bool TankManager::attackFirstTargetOfTank(Unit me, uList &eList) {

	UnitInfo *target = getFirstAttackTargetOfTank(me, eList);

	if (target != nullptr) {
		if (me->getGroundWeaponCooldown() < 2)
			target->setDamage(me);

		CommandUtil::attackUnit(me, target->unit());
		return true;
	}

	return false;
}

bool TankManager::isSiegeTankNeerMe(Unit u) {

	uList myUnits = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, u->getPosition(), 2 * TILE_SIZE);
	bool myUnitNeer = false;

	for (auto my : myUnits)
		if (my->unit()->isSieged()) {
			myUnitNeer = true;
			break;
		}

	return myUnitNeer;
}

void TankManager::siege(Unit u)
{
	UnitInfo *uinfo = INFO.getUnitInfo(u, S);

	if (!uinfo)
		return;

	if (!u->canSiege() && !u->canUnsiege())
		return;

	uList br = INFO.getTypeBuildingsInRadius(Terran_Barracks, S, u->getPosition(), 3 * TILE_SIZE);

	// 배럭이 랜딩중인지 체크
	// 짓고 있는 터렛이 있는지 체크
	for (auto b : br) {
		if (!b->unit()->canLand() && !b->unit()->canLift()
				|| (b->type() == Terran_Missile_Turret && !b->isComplete())) {
			return;
		}
	}

	// 앞마당 컴셋 위치에 시즈 하지 않는다.
	// 커맨드 센터
	Position c = (Position)INFO.getFirstExpansionLocation(S)->getTilePosition();
	Position cr = c + (Position)Terran_Command_Center.tileSize();

	// 컴셋
	Position cs = Position(cr.x, c.y + 32);
	Position csr = cs + (Position)Terran_Comsat_Station.tileSize();

	Position newPos = u->getPosition();

	if (newPos.x >= cs.x - 16 && newPos.y >= cs.y - 16
			&& newPos.x <= csr.x + 16 && newPos.y <= csr.y + 16)
		return;

	if (!u->isSieged()) {

		u->siege();
		uinfo->setLastSiegeOrUnsiegeTime(TIME);
	}
}

void TankManager::unsiege(Unit u, bool imme)
{
	UnitInfo *uinfo = INFO.getUnitInfo(u, S);

	if (!uinfo)
		return;

	if (!u->canSiege() && !u->canUnsiege())
		return;

	if (u->isSieged()) {
		// 너무 자주 시즈 & 언시즈를 반복하지 않도록 5초의 유예시간을 준다.
		if (!imme && TIME - uinfo->getLastSiegeOrUnsiegeTime() < 5 * 24)
			return;

		u->unsiege();
		uinfo->setLastSiegeOrUnsiegeTime(TIME);
	}
}

UnitInfo *TankManager::getFirstAttackTargetOfTank(Unit me, uList &eList)
{
	UnitInfo *target = nullptr;
	UnitInfo *buildingTarget = nullptr;

	for (auto u : eList) {

		if (!me->isInWeaponRange(u->unit()))
			continue;

		if (u->hp() < u->expectedDamage())
			continue;

		if (!u->unit()->isDetected())
			continue;

		UnitType utype = u->type();

		bool dangerUnitDetected = false;

		if (INFO.enemyRace == Races::Protoss) {
			// 토스전 : 1. 리버, 2. 하이템플러, 3. 다크템플러, 4. 라지 유닛
			if (utype == Protoss_Reaver) {
				target = u;
				break;
			} else if (utype == Protoss_High_Templar) {
				target = u;
				dangerUnitDetected = true;
			}
			else if (utype == Protoss_Dark_Templar) {
				if (target == nullptr || target->type() != Protoss_High_Templar)
					target = u;

				dangerUnitDetected = true;
			}
		}
		else if (INFO.enemyRace == Races::Terran) {
			// 테란전 : 1. 탱크 2. 라지 유닛
			if (utype == Terran_Siege_Tank_Siege_Mode)
			{
				target = u;
			}
			else if (utype == Terran_Siege_Tank_Tank_Mode &&
					 (target == nullptr || target->type() != Terran_Siege_Tank_Siege_Mode))
			{
				target = u;
			}
			else if (utype == Terran_Goliath &&
					 (target == nullptr ||
					  (target->type() != Terran_Siege_Tank_Siege_Mode && target->type() != Terran_Siege_Tank_Tank_Mode)))
			{
				target = u;
			}
			else if (utype == Terran_Vulture &&
					 (target == nullptr ||
					  (target->type() != Terran_Siege_Tank_Siege_Mode && target->type() != Terran_Siege_Tank_Tank_Mode && target->type() != Terran_Goliath)))
			{
				target = u;
			}
			else if (utype == Terran_Marine &&
					 (target == nullptr ||
					  (target->type() != Terran_Siege_Tank_Siege_Mode && target->type() != Terran_Siege_Tank_Tank_Mode && target->type() != Terran_Goliath && target->type() != Terran_Vulture)))
			{
				target = u;
			}
			else if (utype == Terran_Medic &&
					 (target == nullptr ||
					  (target->type() != Terran_Siege_Tank_Siege_Mode && target->type() != Terran_Siege_Tank_Tank_Mode && target->type() != Terran_Goliath && target->type() != Terran_Vulture && target->type() != Terran_Marine)))
			{
				target = u;
			}
			else if (utype == Terran_SCV &&
					 (target == nullptr ||
					  (target->type() != Terran_Siege_Tank_Siege_Mode && target->type() != Terran_Siege_Tank_Tank_Mode && target->type() != Terran_Goliath && target->type() != Terran_Vulture && target->type() != Terran_Marine && target->type() != Terran_Medic)))
			{
				target = u;
			}
			else if (utype == Terran_Command_Center && target == nullptr)
			{
				target = u;
			}
		}
		else {

			if (u->unit()->isUnderDarkSwarm())
				continue;

			// 저그전 :
			if (utype == Zerg_Defiler) {
				target = u;
				break;
			}
			else if (utype == Zerg_Lurker) {
				target = u;
				dangerUnitDetected = true;
			}
		}

		if (dangerUnitDetected)
			break;

		if (!dangerUnitDetected && INFO.enemyRace != Races::Terran) {
			if (!utype.isBuilding() && utype.size() == UnitSizeTypes::Large) {
				target = u;
			}
			else if (utype == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace)) {
				buildingTarget = u;
			}
			else if (!buildingTarget) {
				if (utype == INFO.getBasicResourceDepotBuildingType(INFO.enemyRace)) {
					buildingTarget = u;
				}
			}
		}
	}

	return target != nullptr ? target : buildingTarget;
}

bool TankManager::needGathering()
{
	if (notDefenceTankList.isEmpty())
		return false;

	if (frontTankOfNotDefenceTank == nullptr)
		return false;

	if (isSameArea(TM.getNextMovingPoint(), frontTankOfNotDefenceTank->pos())
			|| !INFO.getUnitsInRadius(E, frontTankOfNotDefenceTank->pos(), 12 * TILE_SIZE, true, false, false).empty())
		return false;

	uList sorted = notDefenceTankList.getSortedUnitList(TM.getNextMovingPoint());

	if (sorted.empty())
		return false;

	UnitInfo *firstTank = sorted.at(0);

	float threashold = 15; // 첫번째 탱크와 이보다 멀리 떨어져 있는지 체크하기 위한 값
	float threashold2 = 2; // 첫번째 탱크과 가까이 있어야 하는 비율.(클수록 촘촘하게 움직임)

	if (INFO.getFirstExpansionLocation(S)) {
		// 초반에는 더 촘촘히 가기 위해
		if (getGroundDistance(INFO.getFirstExpansionLocation(S)->Center(), firstTank->pos()) < 40 * TILE_SIZE) {
			threashold = 10;
			threashold2 = 3;
		}
	}

	// 첫번째 탱크와의 거리를 계산해서, 몇개의 탱크가 뒤쳐져 있는지 계산
	int needCnt = (int)(notDefenceTankList.size() / threashold2);

	for (auto t : notDefenceTankList.getUnits()) {
		if (t->unit() == firstTank->unit())
			continue;

		if (getGroundDistance(t->pos(), firstTank->pos()) > threashold * TILE_SIZE) {
			if (--needCnt <= 0)
				return true;
		}
	}

	return false;
}

bool TankManager::checkAllSiegeNeed()
{
	Position targetPos = TM.getNextMovingPoint();

	if (frontTankOfNotDefenceTank == nullptr) {
		return false;
	}

	int distanceFromTarget = getGroundDistance(frontTankOfNotDefenceTank->pos(), targetPos); // 가장 선두 탱크와 targetPos과의 거리
	int distanceFromMainBase = getGroundDistance(frontTankOfNotDefenceTank->pos(), MYBASE); // 가장 선두 탱크와 targetPos과의 거리
	word enemyCnt = getEnemyNeerMyAllTanks(notDefenceTankList.getUnits());//내 모든 탱크 주위의 적

	if (distanceFromMainBase < 50 * TILE_SIZE || distanceFromTarget >= 1600 || (enemyCnt > notDefenceTankList.size() / 2 && distanceFromTarget >= 500))
		return true;
	else
		return false;

}

void TankManager::getClosestPosition()
{
	firstDefencePositions.clear();

	Position avgChoke = INFO.getSecondAverageChokePosition(S);

	if (INFO.getFirstExpansionLocation(S)->getPosition().getApproxDistance((Position)INFO.getSecondChokePoint(S)->Center())
			> 540) {
		avgChoke = INFO.getSecondAverageChokePosition(S, 1, 1);
	}

	Position firstChoke = INFO.getFirstChokePosition(S);
	Position secondChokeMiddle = INFO.getSecondChokePosition(S, ChokePoint::middle);
	Position secondChokeEnd1 = INFO.getSecondChokePosition(S, ChokePoint::end1);
	Position secondChokeEnd2 = INFO.getSecondChokePosition(S, ChokePoint::end2);
	Base *firstExpansion = INFO.getFirstExpansionLocation(S);
	double radius = 5;

	while (radius <= 15) {
		for (auto newPos : getRoundPositions(secondChokeMiddle, (int)(radius * TILE_SIZE), 20))
		{
			if (isSameArea(newPos, firstExpansion->Center()) && getAltitude(newPos) > 0) { // 앞마당이면서 유효한 지점
				if (secondChokeMiddle.getApproxDistance(avgChoke) + TILE_SIZE <= secondChokeMiddle.getApproxDistance(newPos) // 벙커위치보다 멀리 떨어진곳
						&& secondChokeEnd1.getApproxDistance(newPos) > 3 * TILE_SIZE
						&& secondChokeEnd2.getApproxDistance(newPos) > 3 * TILE_SIZE
						&& firstChoke.getApproxDistance(newPos) > 4 * TILE_SIZE) { // first 초크를 막으면 안되니까 초크포인트 근처 제외

					// 커맨드 센터
					Position c = (Position)firstExpansion->getTilePosition();
					Position cr = c + (Position)Terran_Command_Center.tileSize();

					if (newPos.x >= c.x - 16 && newPos.y >= c.y - 16
							&& newPos.x <= cr.x + 16 && newPos.y <= cr.y + 16)
						continue;

					// 컴셋
					Position cs = Position(cr.x, c.y + 32);
					Position csr = cs + (Position)Terran_Comsat_Station.tileSize();

					if (newPos.x >= cs.x - 16 && newPos.y >= cs.y - 16
							&& newPos.x <= csr.x + 16 && newPos.y <= csr.y + 16)
						continue;

					Position b = (Position)TerranConstructionPlaceFinder::Instance().getBarracksPositionInSCP();
					Position br = b + (Position)Terran_Barracks.tileSize();

					if (newPos.x >= b.x && newPos.y >= b.y
							&& newPos.x <= br.x && newPos.y <= br.y)
						continue;

					Position e = (Position)TerranConstructionPlaceFinder::Instance().getEngineeringBayPositionInSCP();
					Position er = e + (Position)Terran_Engineering_Bay.tileSize();

					if (newPos.x >= e.x && newPos.y >= e.y
							&& newPos.x <= er.x && newPos.y <= er.y)
						continue;

					firstDefencePositions[newPos] = nullptr;
				}
			}
		}

		radius += 2.5;
	}

	for (auto t : allTanks.getUnits()) {
		if (t->getState() == "Idle")
		{
			if (firstDefencePositions.find(t->getIdlePosition()) != firstDefencePositions.end())
				firstDefencePositions[t->getIdlePosition()] = t;
			else // 좌표 없어졌으면 다시 받아
				setIdleState(t);
		}
		else if (t->getState() == "Defence" || t->getState() == "BaseDefence")
		{
			if (firstDefencePositions.find(t->getIdlePosition()) != firstDefencePositions.end())
				firstDefencePositions[t->getIdlePosition()] = t;
		}
	}


	if (firstDefencePositions.size() == 0)
	{
		Position pos = (INFO.getFirstExpansionLocation(S)->getPosition() + INFO.getSecondChokePosition(S)) / 2;
		firstDefencePositions[pos] = nullptr;
	}
}

// DefencPosition이 없는 경우 Positions::None으로 Return 된다.
Position TankManager::getPositionOnTheHill()
{
	if (defencePositionOnTheHill != Positions::Unknown)
		return defencePositionOnTheHill;

	// 역언덕은 안함.
	if (INFO.getAltitudeDifference() < 0)
	{
		defencePositionOnTheHill = Positions::None;
		return defencePositionOnTheHill;
	}

	Position standard = INFO.getSecondChokePosition(S);

	vector<Position> pos;

	int distance = 6;

	while (distance < 13)
	{
		int angle = 10;
		int angle_sum = 0;

		Position basicPos = getDirectionDistancePosition(standard, MYBASE, distance * TILE_SIZE);

		while (angle_sum <= 360)
		{
			basicPos = getCirclePosFromPosByDegree(standard, basicPos, angle);

			if (bw->isWalkable(WalkPosition(basicPos)) && isSameArea(basicPos, MYBASE) &&
					basicPos.getApproxDistance(INFO.getFirstChokePosition(S)) <= 12 * TILE_SIZE &&
					basicPos.getApproxDistance(INFO.getFirstChokePosition(S)) > 6 * TILE_SIZE)
				pos.push_back(basicPos);

			angle_sum += angle;
		}

		distance++;
	}

	if (pos.size() == 0)
		defencePositionOnTheHill = Positions::None;
	else
	{
		defencePositionOnTheHill = Position(0, 0);

		for (auto p : pos)
			defencePositionOnTheHill += p;

		defencePositionOnTheHill /= pos.size();
	}

	return defencePositionOnTheHill;
}

Position TankManager::getPositionNearMineral()
{
	if (defencePositionNearMineral != Positions::Unknown)
		return defencePositionNearMineral;

	if (!INFO.getFirstExpansionLocation(S))
		return Positions::None;

	Position bottomOfFirstExansion = INFO.getFirstExpansionLocation(S)->Center() + (Position)Terran_Command_Center.tileSize() - 1;
	Position topOfFirstExansion = INFO.getFirstExpansionLocation(S)->Center() - (Position)Terran_Command_Center.tileSize() + 1;

	int dist1 = topOfFirstExansion.getApproxDistance(INFO.getSecondAverageChokePosition(S));
	int dist2 = bottomOfFirstExansion.getApproxDistance(INFO.getSecondAverageChokePosition(S));

	return dist1 > dist2 ? topOfFirstExansion : bottomOfFirstExansion;
	/*
		Unitset minerals = bw->getUnitsInRadius(INFO.getFirstExpansionLocation(S)->Center(), 8 * TILE_SIZE, Filter::IsMineralField);

		if (minerals.size() == 0)
			return Positions::None;
		else
		{
			int dist = INT_MAX;
			Position closestFromFc = Positions::None;

			for (auto m : minerals)
			{
				if (m->getPosition().getApproxDistance(INFO.getFirstChokePosition(S)) < dist)
				{
					dist = m->getPosition().getApproxDistance(INFO.getFirstChokePosition(S));
					closestFromFc = m->getPosition();
				}
			}

			if (closestFromFc != Positions::None) {
				if (INFO.getFirstExpansionLocation(S)) {
					Position firstExpansion = INFO.getFirstExpansionLocation(S)->Center();
					defencePositionNearMineral = (closestFromFc * 4 + firstExpansion * 1) / 5;
				}
			}

			return defencePositionNearMineral;
		}*/
}

void TankManager::setStateToTank(UnitInfo *t, State *state)
{
	if (t == nullptr) {
		if (state != nullptr)
			delete state;
	}
	else if (state == nullptr)
		return;

	string oldState = t->getState();
	string newState = state->getName();

	/* 이전 스테이트 제거*/

	if (oldState == "Idle") {
		// Do nothing
	}
	else if (oldState == "AttackMove") {
		// Do nothing
	}
	else if (oldState == "Defence") {
		defenceTankSet.del(t);
	}
	else if (oldState == "BaseDefence") {
		baseDefenceTankSet.del(t);
	}
	else if (oldState == "BackMove") {
		// Do nothing
	}
	else if (oldState == "Gathering") {
		gatheringSet.del(t);
	}
	else if (oldState == "SiegeLineState") {
		siegeLineSet.del(t);
	}
	else if (oldState == "SiegeMovingState") {
		// Do nothing
	}
	else if (oldState == "FirstExpansionSecure") {
		tankFirstExpansionSecureSet.del(t);
	}
	else if (oldState == "ExpandState") {
		expandTankSet.del(t);
	}
	else if (oldState == "MultiBreak") {
		multiBreakSet.del(t);
	}
	else if (oldState == "KeepMulti") {
		keepMultiSet.del(t);
		keepMultiSet2.del(t);
	}
	else if (oldState == "Dropship") {
		dropshipSet.del(t);
	}
	else if (oldState == "Fight") {
		//
	}
	else if (oldState == "ProtectAdditionalMulti") {

	}

	/* 새로운 스테이트 부여 */
	if (newState == "Idle") {
		delete state;
		setIdleState(t);
		return;
	}
	else if (newState == "AttackMove") {
	}
	else if (newState == "Defence") {
		defenceTankSet.add(t);
	}
	else if (newState == "BaseDefence") {
		baseDefenceTankSet.add(t);
	}
	else if (newState == "BackMove") {
		// Do nothing
	}
	else if (newState == "Gathering") {
		gatheringSet.add(t);
	}
	else if (newState == "SiegeLineState") {
		siegeLineSet.add(t);
	}
	else if (newState == "SiegeMovingState") {
		// Do nothing
	}
	else if (newState == "FirstExpansionSecure") {
		tankFirstExpansionSecureSet.add(t);

		if (firstDefencePositions.find(t->getIdlePosition()) != firstDefencePositions.end())
			firstDefencePositions[t->getIdlePosition()] = nullptr;
	}
	else if (newState == "ExpandState") {
		expandTankSet.add(t);
	}
	else if (newState == "MultiBreak") {
		multiBreakSet.add(t);
	}
	else if (newState == "KeepMulti") {
		keepMultiSet.add(t);
	}
	else if (newState == "Dropship") {
		dropshipSet.add(t);
	}
	else if (newState == "Fight") {
		//
	}
	else if (newState == "ProtectAdditionalMulti") {

	}

	t->setState(state);

	return;
}

void TankManager::onUnitCompleted(Unit u)
{
	UnitInfo *unit = INFO.getUnitInfo(u, S);

	if (unit != nullptr) 	allTanks.add(unit);
}

void TankManager::onUnitDestroyed(Unit u)
{
	allTanks.del(u);
	defenceTankSet.del(u);
	baseDefenceTankSet.del(u);
	tankFirstExpansionSecureSet.del(u);

	UnitInfo *tInfo = INFO.getUnitInfo(u, S);

	if (INFO.enemyRace != Races::Terran)
	{
		gatheringSet.del(u);

		if (tInfo)
		{
			if (firstDefencePositions.find(tInfo->getIdlePosition()) != firstDefencePositions.end())
				firstDefencePositions[tInfo->getIdlePosition()] = nullptr;
		}
	}

	if (INFO.enemyRace == Races::Terran)
	{
		if (tInfo && tInfo->getState() == "KeepMulti")
			SM.setMultiDeathPos(tInfo->pos());

		if (tInfo && tInfo->getState() == "MultiBreak")
			SM.setMultiBreakDeathPos(tInfo->pos());

		expandTankSet.del(u);

		siegeLineSet.del(u);
		dropshipSet.del(u);
	}

	multiBreakSet.del(u);
	keepMultiSet.del(u);
	keepMultiSet2.del(u);

}

bool TankManager::notAttackableByTank(UnitInfo *targetInfo) {
	return targetInfo->unit()->isFlying() && targetInfo->unit()->getType() != Protoss_Shuttle && targetInfo->unit()->getType() != Terran_Dropship;
}

//
void TankManager::closeUnitAttack(Unit unit) {

	uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() + TILE_SIZE, true, false);
	uList unitsInMinRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 3 * TILE_SIZE, true, false);

	// 공격할 수 있는 적이 없는 경우
	if (unitsInMaxRange.size() == 0) {
		if (unit->isSieged())
			TM.unsiege(unit);
		else
			CommandUtil::attackMove(unit, waitingPosition);

		return;
	}

	// 너무 밖으로 나가면..
	if (getGroundDistance(unit->getPosition(), MYBASE) > 1500) {
		if (unitsInMaxRange.size() == 0) {
			if (unit->isSieged())
				TM.unsiege(unit);
			else
				CommandUtil::attackMove(unit, waitingPosition);
		}
		else {
			TM.siege(unit);
		}

		return;
	}

	if (unit->isSieged()) {
		// 시즈 상태인데, 공격 가능한 유닛이 없으면 Unsieged.
		if (unitsInMaxRange.size() == unitsInMinRange.size()) {
			TM.unsiege(unit);
			return;
		}

		uList nearUnits = INFO.getAllInRadius(E, unit->getPosition(), 2 * TILE_SIZE, true, false);

		if (!nearUnits.empty()) {
			if (INFO.getTypeUnitsInRadius(Terran_Vulture, S, unit->getPosition(), 15 * TILE_SIZE, false).empty()
					&& INFO.getTypeUnitsInRadius(Terran_Goliath, S, unit->getPosition(), 15 * TILE_SIZE, false).empty()) {
				TM.unsiege(unit);
				return;
			}
		}

		TM.attackFirstTargetOfTank(unit, unitsInMaxRange);
		return;
	}
	else {

		// 모든 적이 너무 가까이 있을 때.
		UnitInfo *target = INFO.getClosestUnit(E, unit->getPosition(), GroundCombatKind, 0, false);

		if (!target)
			return;

		if (unitsInMaxRange.size() == unitsInMinRange.size()) {
			// 카이팅
			if (unit->getDistance(target->pos()) <= 2 * TILE_SIZE)
				moveBackPostion(INFO.getUnitInfo(unit, S), target->pos(), 3 * TILE_SIZE);
			else {
				if (unit->getGroundWeaponCooldown() == 0)
					CommandUtil::attackMove(unit, target->pos());
			}
		}
		else {
			if (isSameArea(unit->getPosition(), MYBASE) && isSameArea(target->pos(), MYBASE))
				CommandUtil::attackMove(unit, target->pos());
			else
				TM.siege(unit);
		}
	}

	return;
}

void TankManager::commonAttackActionByTank(const Unit &unit, const UnitInfo *targetInfo) {

	Unit target = targetInfo->unit();
	uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false);
	uList unitsInMinRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 3 * TILE_SIZE, true, false);

	// 공격할 수 있는 적이 없는 경우
	if (unitsInMaxRange.size() == 0) {
		if (unit->isSieged())
			TM.unsiege(unit, false);
		else
			CommandUtil::attackMove(unit, target->getPosition());

		return;
	}

	if (unit->isSieged()) {
		// 시즈 상태인데, 공격 가능한 유닛이 없으면 Unsieged.
		if (unitsInMaxRange.size() == unitsInMinRange.size()) {
			TM.unsiege(unit, false);
			return;
		}

		uList nearUnits = INFO.getAllInRadius(E, unit->getPosition(), 2 * TILE_SIZE, true, false);

		if (!nearUnits.empty()) {
			if (INFO.getTypeUnitsInRadius(Terran_Vulture, S, unit->getPosition(), 15 * TILE_SIZE, false).empty()
					&& INFO.getTypeUnitsInRadius(Terran_Goliath, S, unit->getPosition(), 15 * TILE_SIZE, false).empty()) {
				TM.unsiege(unit, false);
				return;
			}
		}

		// 대형 유닛 공격
		UnitInfo *targetFound = TM.getFirstAttackTargetOfTank(unit, unitsInMaxRange);

		if (targetFound != nullptr) {
			// 공격 대상이 다크템플러이고 본진에 있으면 시즈모드 해제
			if (targetFound->type() == Protoss_Dark_Templar && isSameArea(targetFound->pos(), MYBASE))
				TM.unsiege(unit);
			else {
				CommandUtil::attackUnit(unit, targetFound->unit());

				if (unit->getGroundWeaponCooldown() == 0)
					targetFound->setDamage(unit);
			}
		}

		return;

	}
	else {

		// 모든 적이 너무 가까이 있을 때.
		if (unitsInMaxRange.size() == unitsInMinRange.size()) {

			int distance = unit->getDistance(target->getPosition());

			if (unit->getGroundWeaponCooldown() == 0) {
				CommandUtil::attackMove(unit, target->getPosition());
			}
			else {
				if (distance <= 2 * TILE_SIZE)
					moveBackPostion(INFO.getUnitInfo(unit, S), target->getPosition(), 3 * TILE_SIZE);
				else
					CommandUtil::attackMove(unit, target->getPosition());
			}

		}
		else {

			// 드라군 두마리 vs 탱크 1대와 같은 상황에서 내가 시즈를 해버리면 적이 접근해서 죽을 수 있다.
			uList nearMyUnits = INFO.getUnitsInRadius(S, unit->getPosition(), 15 * TILE_SIZE, true, false, false);
			uList nearEUnits = INFO.getUnitsInRadius(E, targetInfo->pos(), 15 * TILE_SIZE, true, false, false);

			if ((nearEUnits.size() <= nearMyUnits.size()
					|| INFO.getAltitudeDifference(unit->getTilePosition(), (TilePosition)targetInfo->pos()) == 1)
					&& !checkZeroAltitueAroundMe(unit->getPosition(), 6)) {
				TM.siege(unit);
				return;
			}
			else {
				// 퉁퉁포
				if (unit->getGroundWeaponCooldown() == 0) {
					CommandUtil::attackUnit(unit, targetInfo->unit());
				}
				else {
					if (unit->getDistance(targetInfo->unit()) > 6 * TILE_SIZE)
						CommandUtil::attackMove(unit, targetInfo->pos());
					else {
						//CommandUtil::move(unit, INFO.getFirstExpansionLocation(S)->Center());
						moveBackPostion(INFO.getUnitInfo(unit, S), targetInfo->vPos(), 3 * TILE_SIZE);
					}
				}
			}
		}
	}
}

// 가장 가까운 적을 찾음.(셔틀,다크 포함)
UnitInfo *TankManager::getDefenceTargetUnit(const uList &enemy, const Unit &unit, int defenceRange) {

	UnitInfo *targetInfo = nullptr;
	int dist = INT_MAX;
	int distFromMain = 0;
	bool needDefence = false;
	defenceRange = defenceRange == 0 ? INT_MAX : defenceRange;

	// 다크와 같은 히든 유닛을 가져오기 위해서 getClosest안씀
	for (auto eu : enemy)
	{
		theMap.GetPath(MYBASE, eu->pos(), &distFromMain);

		if (isInMyArea(eu) || (distFromMain >= 0 && distFromMain < defenceRange)) {
			needDefence = true;
		}
		else {

			if (eu->type().groundWeapon().targetsGround() && eu->type().groundWeapon().maxRange() != 0) {
				uList nearMyUnits = INFO.getAllInRadius(S, eu->pos(), eu->type().groundWeapon().maxRange(), true, true, false);

				for (auto nmu : nearMyUnits) {
					if (isInMyArea(nmu)) {
						needDefence = true;
						break;
					}
				}
			}
		}

		if (needDefence)
		{
			if (eu->getLift()) {
				// 날아다니는 유닛의 경우, 셔틀이나 드랍쉽만 따라감
				if (targetInfo == nullptr &&
						(eu->type() == Protoss_Shuttle || eu->type() == Terran_Dropship)) {
					targetInfo = eu;
					dist = unit->getDistance(eu->unit());
				}

			}
			else if (eu->type().isBuilding()) {

				if (!targetInfo) {
					targetInfo = eu;
					dist = unit->getDistance(eu->unit());
					continue;
				}

				if (eu->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace)) {
					if (targetInfo->unit()->isFlying() || targetInfo->type().groundWeapon().targetsGround()) {
						targetInfo = eu;
						dist = unit->getDistance(eu->unit());
					}
					// 기존 타겟이 공격이 가능한 지상 유닛이면, 거리가 가까울때만 타겟 변경
					else if (unit->getDistance(eu->unit()) < dist) {
						targetInfo = eu;
						dist = unit->getDistance(eu->unit());
					}
				}

			}
			else {
				// 지상유닛의 경우, 기존 Target이 셔틀이나 드랍쉽이면 변경
				if (unit->getDistance(eu->unit()) < dist || (targetInfo != nullptr && targetInfo->unit()->isFlying())) {
					targetInfo = eu;
					dist = unit->getDistance(eu->unit());
				}
			}

		}
	}

	return targetInfo;
}

void TankManager::defenceInvisibleUnit(const Unit &unit, const UnitInfo *targetInfo) {

	// 가장 가까운 유닛이 다크나 럴커라면 탱크 퉁퉁포로 잡는다.
	if (targetInfo->unit()->isDetected()) {
		// 눈에 보이는 경우.
		if (unit->isSieged()) {
			// target 유닛이 시즈모드로 공격 불가
			if (unit->getDistance(targetInfo->pos()) < WeaponTypes::Arclite_Shock_Cannon.minRange())
			{
				uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false);

				if (!TM.attackFirstTargetOfTank(unit, unitsInMaxRange))
					TM.unsiege(unit); // 공격 가능 유닛 없으면, 다크나 럴커가 가까이 있으므로 UnSieze

				return;
			}
			else {
				CommandUtil::attackUnit(unit, targetInfo->unit());
				return;
			}

		}
		else {
			// case 1. 카이팅이 필요한 경우
			if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) +
					INFO.getCompletedCount(Terran_Goliath, S) +
					INFO.getCompletedCount(Terran_Vulture, S) < 7) {
				if (unit->getDistance(targetInfo->unit()) > 3 * TILE_SIZE)
					CommandUtil::attackMove(unit, targetInfo->pos());
				else {
					//CommandUtil::move(unit, INFO.getFirstExpansionLocation(S)->Center());
					moveBackPostion(INFO.getUnitInfo(unit, S), targetInfo->vPos(), 3 * TILE_SIZE);
				}

			}
			else {
				// case 2. 카이팅이 필요없는 경우
				CommandUtil::attackUnit(unit, targetInfo->unit());
			}

			return;

		}

	}
	else {

		// 안보이는 적의 경우, Unsieze 후 퉁퉁포로 잡자.
		if (unit->isSieged()) {
			// 기왕 시즈상태라면 다른 공격 가능한 유닛이 있는지 찾아본다.
			uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false);

			if (!TM.attackFirstTargetOfTank(unit, unitsInMaxRange))
				TM.unsiege(unit); // 공격 가능 유닛 없으면, 다크나 럴커가 가까이 있으므로 UnSieze

		}
		else {

			int threshold = 8 * TILE_SIZE;

			if (ComsatStationManager::Instance().getAvailableScanCount() > 0)
				threshold = 5 * TILE_SIZE;

			// 거리 유지
			if (unit->getDistance(targetInfo->unit()) > threshold)
				CommandUtil::attackMove(unit, targetInfo->pos());
			else {
				//CommandUtil::move(unit, INFO.getFirstExpansionLocation(S)->Center());
				moveBackPostion(INFO.getUnitInfo(unit, S), targetInfo->vPos(), 3 * TILE_SIZE);
			}

		}

	}

	return ;
}

void TankManager::defenceOnFixedPosition(UnitInfo *t, Position defencePosition)
{
	uList enemy = INFO.getUnitsInRadius(E, t->pos(), 12 * TILE_SIZE, true, false, true, false);
	uList enemyBuilding = INFO.getBuildingsInRadius(E, t->pos(), 12 * TILE_SIZE, true, false);
	enemy.insert(enemy.end(), enemyBuilding.begin(), enemyBuilding.end());

	// posOnHill 로 이동 후 시즈
	if (enemy.empty()) {
		if (t->unit()->getDistance(defencePosition) > 50) {
			if (t->unit()->isSieged())
				TM.unsiege(t->unit());
			else
				CommandUtil::move(t->unit(), defencePosition, true);
		}
		else {
			if (!t->unit()->isSieged()) {
				TM.siege(t->unit());
			}
		}
	}
	else {

		// 적이 있는 경우
		if (t->unit()->isSieged()) {
			// 적이 있는데 내 주변에 아군이 아무도 없고
			// 적이 나한테 붙어있으면 언시즈
			uList closeEnemy = INFO.getUnitsInRadius(E, t->pos(), 3 * TILE_SIZE, true, false, true, false);

			if (closeEnemy.empty()) {

				uList myUnits = INFO.getTypeUnitsInRadius(Terran_Vulture, S, t->pos(), 20 * TILE_SIZE, false);
				uList myUnits2 = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, t->pos(), 20 * TILE_SIZE, false);

				if (myUnits.empty() && myUnits2.empty())
					TM.unsiege(t->unit());
			}
			else
				TM.attackFirstTargetOfTank(t->unit(), enemy);
		}
		else {

			// Do kiting
			UnitInfo *closeUnitInfo = INFO.getClosestUnit(E, t->pos(), TypeKind::GroundUnitKind, 12 * TILE_SIZE, false, false, true);

			if (closeUnitInfo == nullptr) {
				// 적이 근처에 없고 defencePosition 에 근접했으면 시즈(언덕이나 미네랄)
				if (t->unit()->getDistance(defencePosition) < 50) {
					TM.siege(t->unit());
				}
				else {
					CommandUtil::attackMove(t->unit(), defencePosition);
				}

			}
			else {
				int distance = t->pos().getApproxDistance(closeUnitInfo->pos());
				attackFirstkiting(t, closeUnitInfo, distance, 3 * TILE_SIZE);
			}
		}
	}

	return;
}

UnitInfo *TankManager::getFrontTankFromPos(Position pos)
{
	if (INFO.enemyRace == Races::Terran)
	{
		if (siegeLineSet.size() == 0)
			return nullptr;

		return siegeLineSet.getFrontUnitFromPosition(pos);
	}
	else
	{
		return notDefenceTankList.getFrontUnitFromPosition(pos);
	}
}

// Main Attack Position 까지 갈 때, 모든 병력이 같은 Choke로 지나도록
// 중간에 거쳐가는 Choke를 구해서 해당 위치로 병력들을 보낸다.
void TankManager::setNextMovingPoint()
{
	if (nextMovingPoint == Positions::Unknown)
		nextMovingPoint = SM.getMainAttackPosition();

	if (!frontTankOfNotDefenceTank) {
		nextMovingPoint = SM.getMainAttackPosition();
		return;
	}

	CPPath path = theMap.GetPath(frontTankOfNotDefenceTank->pos(), SM.getMainAttackPosition());

	if (path.size() == 0) {
		nextMovingPoint = SM.getMainAttackPosition();
		return;
	}
	else {

		const ChokePoint *nextChoke = path.at(0);
		nextMovingPoint = (Position)nextChoke->Center();

		// 목표 지점보다 5타일 이하면 그냥 계속 이동
		if (frontTankOfNotDefenceTank->pos().getApproxDistance(nextMovingPoint) > 3 * TILE_SIZE) {
			return;
		} else {
			nextMovingPoint = SM.getMainAttackPosition();
		}

	}

	return;
}

void TankManager::setSiegeNeedTank(uList &tanks)
{
	int buffer = INFO.enemyRace == Races::Protoss ? 2 * TILE_SIZE : 0;

	for (auto t : tanks) {

		uList eUnit = INFO.getUnitsInRadius(E, t->pos(), 12 * TILE_SIZE + buffer, true, false, false, false);
		uList eCloseUnit = INFO.getUnitsInRadius(E, t->pos(), 5 * TILE_SIZE, true, false, false, false);
		uList eDBUnit = INFO.getDefenceBuildingsInRadius(E, t->pos(), 12 * TILE_SIZE, true, false);

		uList eBUnit = INFO.getBuildingsInRadius(E, t->pos(), 8 * TILE_SIZE + buffer, true, false);
		uList neerTanks = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, t->pos(), 6 * TILE_SIZE);
		uList nexus = INFO.getTypeBuildingsInRadius(INFO.getBasicResourceDepotBuildingType(INFO.enemyRace), E, t->pos(), 12 * TILE_SIZE, true);

		// 내가 시즈를 해야 하는가?
		// 방어건물/ 일반 건물 3개이상, 넥서스가 있을때 길막이 아니면
		if (!eDBUnit.empty() || // 적 방어 건물이 있거나,
				((eBUnit.size() > 3 || !nexus.empty()) && eCloseUnit.empty() // 근접 적이 없으면서 건물이 많으면서 길막이 아니면
				 && !checkZeroAltitueAroundMe(t->pos(), 6)))
			siegeAllSet.insert(t);
		else if (eUnit.size() == eCloseUnit.size()) {
			continue;
		}

		// 공격 가능한 유닛이 있는 경우 주변 5타일 이내의 시즈는 모두 시즈모드(나 포함)
		if (eUnit.size() > 1 && eUnit.size() > neerTanks.size()) {
			for (auto nt : neerTanks) {
				eCloseUnit = INFO.getUnitsInRadius(E, t->pos(), 6 * TILE_SIZE, true, false, false, false);

				if (eCloseUnit.empty())
					siegeAllSet.insert(nt);
			}
		}
	}
}

void TankManager::setWaitAtChokePoint() {
	waitAtChokePoint = false;

	if (frontTankOfNotDefenceTank) {

		CPPath path = theMap.GetPath(frontTankOfNotDefenceTank->pos(), SM.getMainAttackPosition());

		if (!path.empty()) {

			WalkPosition end1 = path.at(0)->Pos(ChokePoint::end1);
			WalkPosition end2 = path.at(0)->Pos(ChokePoint::end2);

			// 주의!! WalkPosition 단위 임!!(Tile / 8)
			if (end1.getApproxDistance(end2) < 20) {
				if (frontTankOfNotDefenceTank->pos().getApproxDistance(nextMovingPoint) < 10 * TILE_SIZE) {

					if (!INFO.getUnitsInRadius(E, nextMovingPoint, 12 * TILE_SIZE, true, false, false, true).empty()) {
						waitAtChokePoint = true;
					}
				}
			}
		}
	}

	return;
}

void TankManager::removeEgg()
{
	bool exist = false;

	for (auto n : bw->getStaticNeutralUnits())
	{
		if (n->getType() == Zerg_Egg && n->getPosition().getApproxDistance(INFO.getFirstExpansionLocation(S)->getPosition()) < 20 * TILE_SIZE && n->exists())
		{
			exist = true;
			break;
		}
	}

	if (!exist)
		eggRemoved = true;
	else
		eggRemoved = false;

	if (!eggRemoved)
	{
		for (auto t : notDefenceTankList.getUnits())
		{
			uList unitsInMaxRange = INFO.getAllInRadius(E, t->pos(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false);

			if (unitsInMaxRange.empty())
			{
				Unit egg = nullptr;

				for (auto n : bw->getStaticNeutralUnits())
				{
					if (n->getType() == Zerg_Egg && n->getPosition().getApproxDistance(INFO.getFirstExpansionLocation(S)->getPosition()) < 20 * TILE_SIZE && n->exists())
					{
						egg = n;
						break;
					}
				}

				if (egg == nullptr)
				{
					eggRemoved = true;
					return;
				}

				if (t->unit()->isSieged())
				{
					if (egg->getPosition().getDistance(t->pos()) < WeaponTypes::Arclite_Shock_Cannon.maxRange() && INFO.getUnitsInRadius(S, egg->getPosition(), 40).size() == 0)
					{
						CommandUtil::attackUnit(t->unit(), egg);
						continue;
					}
				}
				else
				{
					if (egg->getPosition().getDistance(t->pos()) < 8 * TILE_SIZE)
					{
						CommandUtil::attackUnit(t->unit(), egg);
					}
					else
					{
						CommandUtil::attackMove(t->unit(), egg->getPosition());
					}

					continue;
				}
			}
		}
	}

	return;
}

void TankGatheringSet::init() {
	firstTank = nullptr;

	//for (auto t : getUnits())
	//	TM.setStateToTank(t, new TankIdleState());

	//clear();
}

void TankGatheringSet::action() {

	if (getUnits().empty())
		return;

	firstTank = getFrontUnitFromPosition(TM.getNextMovingPoint());

	if (!firstTank) {
		cout << "MainAttackPosition : " << SM.getMainAttackPosition() << " TankGatheringSet size : " << getUnits().size() << endl;
		return;
	}

	bw->drawCircleMap(firstTank->pos(), 10, Colors::Red, true);

	uList eList;
	uList eDList;

	for (auto t : getUnits()) {
		if (isStuckOrUnderSpell(t))
			continue;

		eList = INFO.getUnitsInRadius(E, t->pos(), 14 * TILE_SIZE, true, false, false);
		eDList = INFO.getDefenceBuildingsInRadius(E, t->pos(), 12 * TILE_SIZE, true, false);
		eList.insert(eList.end(), eDList.begin(), eDList.end());
		int tankNearMeCnt = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, t->pos(), 12 * TILE_SIZE, false).size();

		if (t->unit() == firstTank->unit()) {

			if (t->unit()->isSieged()) {
				if (!eList.empty())
					TM.attackFirstTargetOfTank(t->unit(), eList);
				else if (checkZeroAltitueAroundMe(t->pos(), 6))
					TM.unsiege(t->unit());

			}
			else {

				if ((int)eList.size() > tankNearMeCnt / 6)
					TM.siege(t->unit());
				else if (checkZeroAltitueAroundMe(t->pos(), 6)) {
					if (t->unit()->getGroundWeaponCooldown() == 0)
						CommandUtil::attackMove(t->unit(), TM.getNextMovingPoint());
					else
						CommandUtil::move(t->unit(), TM.getNextMovingPoint());

				}
				else
					// 길막도 아니고 적도 없으면 홀드
					CommandUtil::hold(t->unit());
			}
		}
		else if (t->unit()->isSieged()) {
			if (eList.empty())
				TM.unsiege(t->unit(), false);
			else
				TM.attackFirstTargetOfTank(t->unit(), eList);
		}
		else {
			// target으로 이동하면 first tank와 가까워지거나
			// 자신이 first tank가 됨.
			if (eList.empty()) {
				if (t->unit()->getGroundWeaponCooldown() == 0)
					CommandUtil::attackMove(t->unit(), TM.getNextMovingPoint());
				else
					CommandUtil::move(t->unit(), TM.getNextMovingPoint());
			}
			else if ((int)eList.size() > tankNearMeCnt / 6)
				TM.siege(t->unit());
		}
	}
}

void TankFirstExpansionSecureSet::init()
{
	for (auto t : getUnits())
		TM.setStateToTank(t, new TankIdleState());

	//clear();
}

void TankFirstExpansionSecureSet::action()
{
	bool commandLanded = false;

	if (INFO.getFirstExpansionLocation(S) != nullptr) {
		uList commands = INFO.getTypeBuildingsInArea(Terran_Command_Center, S, INFO.getFirstExpansionLocation(S)->Center(), true);

		for (auto c : commands) {
			if (!c->unit()->isLifted())
				commandLanded =  true;
		}
	}

	if (INFO.isTimeToMoveCommandCenterToFirstExpansion() || commandLanded) {

		uList tanks = TM.getTankFirstExpansionSecureSet().getUnits();

		Position posOnHill = TM.getPositionOnTheHill();
		Position posOnMineral = TM.getPositionNearMineral();

		if (tanks.size() == 1) {

			if (posOnHill != Positions::None) {

				// 언덕 시즈가 base defence로 바뀌면,
				// 미네랄에 잇는 시즈가 언덕으로 올라가는것을 방지하기 위해서.
				if (posOnMineral != Positions::None &&
						tanks.at(0)->pos().getDistance(posOnMineral) > 50) {
					TM.defenceOnFixedPosition(tanks.at(0), posOnHill);
				}

			}
			else if (posOnMineral != Positions::None) {
				TM.defenceOnFixedPosition(tanks.at(0), posOnMineral);
			}

		}
		else if (tanks.size() >= 2) {

			UnitInfo *firstTank = tanks.at(0)->unit()->getID() > tanks.at(1)->unit()->getID() ?
								  tanks.at(0) : tanks.at(1);

			UnitInfo *secondTank = tanks.at(0)->unit()->getID() > tanks.at(1)->unit()->getID() ?
								   tanks.at(1) : tanks.at(0);

			// position 의 valid 체크는 setFirstExpansionSecure 함수에서
			TM.defenceOnFixedPosition(firstTank, posOnHill);
			TM.defenceOnFixedPosition(secondTank, posOnMineral);

		}

	}
	else {
		// 시즈업이 안되면, 첫번째 초크를 사수하면서 내려가지 않는다.
		for (auto t : getUnits()) {

			// 시즈업 안된 상황에서 피가 너무 적으면 뒤로 빠지자.
			if (t->unit()->getHitPoints() < 20) {
				CommandUtil::move(t->unit(), MYBASE, true);
				continue;
			}

			uList eList = INFO.getUnitsInRadius(S, t->pos(), 10 * TILE_SIZE, true, false, true);

			if (eList.empty()) {
				if (t->unit()->getDistance(waitingPositionAtFirstChoke) < 1 * TILE_SIZE) {
					CommandUtil::hold(t->unit());
				}
				else {
					CommandUtil::attackMove(t->unit(), waitingPositionAtFirstChoke, true);
				}
			}
			else {
				if (t->unit()->getGroundWeaponCooldown() == 0) {
					UnitInfo *closeUnit = INFO.getClosestUnit(E, t->pos(), GroundCombatKind, 10 * TILE_SIZE, true);

					if (closeUnit) {
						CommandUtil::attackMove(t->unit(), closeUnit->pos());
						continue;
					}
				}

				CommandUtil::move(t->unit(), waitingPositionAtFirstChoke, true);

			}


		}
	}

}

void TankFirstExpansionSecureSet::moveStepByStepAsOneTeam(vector<UnitInfo *> &tanks, Position targetPosition)
{
	// 1. 적이 보이면 시즈모드 하고 공격한다.
	// 2. 본진에서 가까운 순서대로 TM.waitingPosition(앞마당) 으로 이동한다.
	// 3. 탱크들은 서로 5타일 이상 떨어지지 않는다.
	for (word i = 0; i < tanks.size(); i++) {

		UnitInfo *t = tanks.at(i);
		uList enemy = INFO.getUnitsInRadius(E, t->pos(), (12 + tanks.size() * 2 - i) * TILE_SIZE, true, false, true, false);
		uList enemyBuilding = INFO.getBuildingsInRadius(E, t->pos(), 12 * TILE_SIZE, true, false);
		enemy.insert(enemy.end(), enemyBuilding.begin(), enemyBuilding.end());

		if (t->unit()->isSieged()) {
			if (!enemy.empty()) {
				// 적이 보이니 공격한다.
				TM.attackFirstTargetOfTank(t->unit(), enemy);
			}
			else {
				// 적이 없으면서
				// 내가 맨 뒤에 있거나, 입구를 막고 있으면, 적이 없으면 시즈를 푼다.
				if (i == 0) {
					bool allSiezed = true;

					for (word i = 1; i < tanks.size(); i++) {
						UnitInfo *t = tanks.at(i);

						if (!t->unit()->isSieged()) {
							allSiezed = false;
							break;
						}
					}

					if (allSiezed)
						TM.unsiege(t->unit());


				}
				else if (checkZeroAltitueAroundMe(t->pos(), 6)) {
					TM.unsiege(t->unit());
				}

			}

		}
		else {

			if (getGroundDistance(t->pos(), targetPosition) < 4 * TILE_SIZE) {
				TM.siege(t->unit());
				continue;
			}

			// 내가 맨 뒤에 있어서 이동중이거나, 길막 상태에서 이동중인 경우.
			// 내가 맨뒤에서 이동 중인 애인가?
			if (!enemy.empty()) {
				// 시즈를 한 뒤 공격
				TM.siege(t->unit());
			}
			else {

				if (checkZeroAltitueAroundMe(t->pos(), 6))
					CommandUtil::attackMove(t->unit(), targetPosition, true);
				else {

					// 내가 선두 탱크보다 5타일 이상 앞에 나가있는 경우 시즈한다.

					if (getGroundDistance(tanks.at(0)->pos(), targetPosition) - getGroundDistance(t->pos(), targetPosition) >= 3 * TILE_SIZE) {
						TM.siege(t->unit());
					}
					else {
						CommandUtil::attackMove(t->unit(), targetPosition);
					}

				}
			}
		}

	}
}

void TankExpandSet::action()
{
	if (size() == 0)
		return;

	//	if (!gathering((3 + size() / 4) * TILE_SIZE))
	//		return;

	int ready = 0;
	int safe = 0;

	for (auto t : getUnits())
	{
		uList unitsInMaxRange = INFO.getUnitsInRadius(E, t->pos(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false, false);
		uList buildings = INFO.getBuildingsInRadius(E, t->pos(), WeaponTypes::Arclite_Shock_Cannon.maxRange() - 2 * TILE_SIZE, true, false);
		unitsInMaxRange.insert(unitsInMaxRange.end(), buildings.begin(), buildings.end());
		uList unitsInMinRange = INFO.getUnitsInRadius(E, t->pos(), WeaponTypes::Arclite_Shock_Cannon.minRange() + TILE_SIZE, true, false, false);

		if (unitsInMaxRange.size())
		{
			bw->drawTextMap(t->pos() + Position(0, 20), "UnitsInRange");

			if (t->unit()->isSieged()) {
				// 시즈 상태인데, 공격 가능한 유닛이 없으면 Unsieged.
				if (unitsInMaxRange.size() == unitsInMinRange.size()) {
					TM.unsiege(t->unit());

					continue;
				}

				TM.attackFirstTargetOfTank(t->unit(), unitsInMaxRange);
				continue;
			}
			else { // 시즈 아님

				// 모든 적이 너무 가까이 있을 때.
				if (unitsInMaxRange.size() == unitsInMinRange.size()) {

					// target이 유닛이 아니라 포지션으로 주어지면, 가장 가까운 유닛을 찾아야 한다.
					UnitInfo *closeUnitInfo = INFO.getClosestUnit(E, t->pos(), TypeKind::GroundUnitKind, WeaponTypes::Arclite_Shock_Cannon.maxRange(), false);

					if (closeUnitInfo == nullptr)
						continue;

					int distance = t->pos().getApproxDistance(closeUnitInfo->pos());

					kiting(t, closeUnitInfo, distance, 3 * TILE_SIZE);
				}
				else {
					TM.siege(t->unit());
				}
			}
		}
		else
		{
			int Threshold = 1 * TILE_SIZE;

			if (INFO.getBuildingsInRadius(E, t->pos(), 12 * TILE_SIZE).size() || INFO.getUnitsInRadius(E, t->pos(), 10 * TILE_SIZE).size())
			{
				Threshold = int(2.5 * TILE_SIZE);
			}

			int dangerPoint = 0;
			UnitInfo *dangerUnit = getDangerUnitNPoint(t->pos(), &dangerPoint, false);

			bw->drawTextMap(t->pos() + Position(0, 40), "danger : %d", dangerPoint);

			if (dangerUnit == nullptr)
			{
				safe++;

				if (!t->unit()->isSieged())
					CommandUtil::attackMove(t->unit(), TM.getNextMovingPoint());
			}
			else if (dangerUnit->type() == Terran_Siege_Tank_Siege_Mode)
			{
				if (t->unit()->isSieged()) // 시즈 모드
				{
					if (dangerPoint > Threshold)
						safe++;
				}
				else // 시즈모드 아님.
				{
					if (dangerPoint <= Threshold)
					{
						//if (t->unit()->isMoving())
						t->unit()->stop();

						ready++;
					}
					else
						safe++;

					CommandUtil::attackMove(t->unit(), SM.getMainAttackPosition());
				}
			}
			else
			{
				safe++;

				if (!t->unit()->isSieged())
					CommandUtil::attackMove(t->unit(), SM.getMainAttackPosition());
			}
		}
	}

	if (ready == size())
		siegeAll();

	if (safe == size())
		unSiegeAll();
}

void KeepMultiSet::action(int num, bool needScv)
{
	if (size() == 0)
		return;

	Position targetPos = Positions::None;

	if (num == 1)
	{
		targetPos = getDirectionDistancePosition(INFO.getSecondExpansionLocation(S)->Center(), theMap.Center(), 5 * TILE_SIZE);
	}
	else if (num == 2 && needScv)
	{
		targetPos = getDirectionDistancePosition(INFO.getThirdExpansionLocation(S)->Center(), theMap.Center(), 5 * TILE_SIZE);
	}
	else if (num == 2 && !needScv)
	{
		targetPos = middlePos;
	}

	if (!targetPos.isValid()) return;

	//커맨드센터가 있으면 needScv를 false처리해줌..
	if (!INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, targetPos, 10 * TILE_SIZE, false).empty())
	{
		needScv = false;
	}

	for (auto t : getUnits())
	{
		Unit unit = t->unit();

		// 목적지에서 그냥 대기
		if (t->pos().getApproxDistance(targetPos) < 3 * TILE_SIZE && ( !INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, targetPos, 10 * TILE_SIZE, true).empty() || !needScv))
		{
			if (!unit->isSieged())
				unit->siege();

			continue;
		}

		uList unitsInMaxRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() + TILE_SIZE, true, false, false);
		uList buildings = INFO.getBuildingsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() + TILE_SIZE, true, false);

		for (auto b : buildings)
		{
			if (b->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
				unitsInMaxRange.push_back(b);
		}

		uList unitsInMinRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 2 * TILE_SIZE, true, false, false);

		// 공격할 수 있는 적이 없는 경우
		if (unitsInMaxRange.size() == 0) {
			if (unit->isSieged())
				TM.unsiege(unit);
			else if (needScv)
			{
				//데려가야되는 SCV가 5타일 안에 있을 때 멀티 지역으로 이동
				if (t->pos().getApproxDistance(targetPos) > 3 * TILE_SIZE && myScv != nullptr )
				{
					if (myScv->getPosition().getApproxDistance(t->pos()) < 5 * TILE_SIZE)
					{
						CommandUtil::move(unit, targetPos);
					}
					else if (t->pos().getApproxDistance(targetPos) > 8 * TILE_SIZE)
					{
						CommandUtil::move(unit, myScv->getPosition());
					}

				}
				//SCV가 없으면 SecondChokepoint로 돌아갑니다
				else if (myScv == nullptr || !myScv->exists())
					CommandUtil::move(unit, (Position)INFO.getSecondChokePoint(S)->Center());
			}
			else //SCV를 데리고 다니는 상황 아닌 경우(가운데포지션가는경우)
			{
				CommandUtil::move(unit, targetPos);
			}

			continue;
		}

		if (unit->isSieged()) {
			// 시즈 상태인데, 공격 가능한 유닛이 없으면 Unsieged.
			if (unitsInMaxRange.size() == unitsInMinRange.size()) {
				TM.unsiege(unit);

				continue;
			}
			else
			{
				TM.attackFirstTargetOfTank(unit, unitsInMaxRange);
			}
		}
		else {
			// 모든 적이 너무 가까이 있을 때.
			if (unitsInMaxRange.size() == unitsInMinRange.size()) {

				// target이 유닛이 아니라 포지션으로 주어지면, 가장 가까운 유닛을 찾아야 한다.
				UnitInfo *closeUnitInfo = INFO.getClosestUnit(E, unit->getPosition(), TypeKind::GroundUnitKind, WeaponTypes::Arclite_Shock_Cannon.maxRange(), false);

				if (closeUnitInfo == nullptr)
					continue;

				kiting(t, closeUnitInfo, t->pos().getApproxDistance(closeUnitInfo->pos()), 3 * TILE_SIZE);
			}
			else {
				TM.siege(unit);
			}
		}
	}
}
