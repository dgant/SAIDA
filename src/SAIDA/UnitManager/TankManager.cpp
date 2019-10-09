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

		if (firstDefencePositions.empty() || (SM.getMainStrategy() == WaitToFirstExpansion && TIME % (24 * 60) == 0 )) 
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
	
	catch (...) {
	}

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
		
		catch (...) {
		}

		bool gatherFlag = false;

		try {
			if (INFO.enemyRace == Races::Protoss)
				gatherFlag = needGathering();
		}
		
		catch (...) {
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
			catch (...) {

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
			
			catch (...) {
			}
		}

		
		for (auto t : tankList)
			if (!isStuckOrUnderSpell(t))
				t->action();
	}

	else {
		
		for (auto t : tankList)
			t->action();
	}
}

void TankManager::useScanForCannonOnTheHill()
{
	if (frontTankOfNotDefenceTank) {
		uList buildings = INFO.getBuildingsInRadius(E, frontTankOfNotDefenceTank->pos(), WeaponTypes::Arclite_Shock_Cannon.maxRange() - 2 * TILE_SIZE, true, false, true);

		for (auto b : buildings) {
			
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

	
	if (!SM.getNeedAttackMulti() || INFO.getFirstChokePosition(E) == Positions::None) {
		return false;
	}

	if (EMB == Toss_fast_carrier)
		return false;

	if (!INFO.getMainBaseLocation(E))
		return false;

	
	uList nexus = INFO.getTypeBuildingsInRadius(INFO.getBasicResourceDepotBuildingType(INFO.enemyRace), E, INFO.getMainBaseLocation(E)->Center(), 5 * TILE_SIZE, true);

	if (nexus.empty())
		return false;

	if (INFO.getFirstExpansionLocation(E)) {

		Position firstExpansionE = INFO.getFirstExpansionLocation(E)->Center();
		UnitInfo *firstTank = TM.getFrontTankFromPos(firstExpansionE);

		
		if (firstTank && isSameArea(firstTank->pos(), firstExpansionE)
				&& INFO.getTypeBuildingsInArea(INFO.getBasicResourceDepotBuildingType(INFO.enemyRace), E, firstExpansionE, true).empty()) {

			
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
				notDefenceTankList.del(t); 

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
					setStateToTank(t, new TankKeepMultiState()); 
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


void TankManager::setAllSiegeMode(const uList &tankList)
{



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

	
	for (auto &p : firstDefencePositions)
	{
		
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


		if (isSameArea(e->pos(), MYBASE)) {
			
			if (uType == Protoss_Shuttle || uType == Terran_Dropship)
				baseNeedCnt += ((uType.spaceProvided() - e->getSpaceRemaining()) / 2);
			else if (uType == Protoss_Reaver)
				baseNeedCnt += 2;
			else if (uType == Protoss_Dark_Templar)
				baseNeedCnt += 2;
			else if (!e->type().isFlyer() && !e->type().isWorker())
				baseNeedCnt += 1;
		}
	
		else {
			
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

	
	if (overloadCnt > 8)
		baseNeedCnt += (overloadCnt * 3 / 8);

	if (baseNeedCnt == 0) {
		
		if ( SM.getMainStrategy() != AttackAll &&
				(ESM.getWaitTimeForDrop() > 0 || E->getUpgradeLevel(UpgradeTypes::Ventral_Sacs))
				&& TIME - ESM.getWaitTimeForDrop() <= 24 * 60 * 3
				&& BasicBuildStrategy::Instance().hasSecondCommand()
				&& INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 2) {

			if (INFO.enemyRace != Races::Protoss || (EMB == Toss_drop || EMB == Toss_dark_drop))
			{
				
				if (INFO.getUnitsInRadius(E, INFO.getSecondChokePosition(S), 10 * TILE_SIZE, true).size() < 5)
					baseNeedCnt = 1;
			}

		}

	}

	if (needCnt + baseNeedCnt > 0) {
		
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

	if (INFO.enemyRace == Races::Terran) 
	{
		if (!S->hasResearched(TechTypes::Tank_Siege_Mode))
		{
			baseNeedCnt = 0;

			for (auto eu : INFO.enemyInMyArea())
				if (!eu->type().isFlyer())
					needCnt++;
		}

		
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

		
		if (defenceTankSet.size() < needCnt) {
			
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

	
	uList sortTank = notDefenceTankList.getSortedUnitList(targetPos, true);
	int tankSize = (int)sortTank.size();
	int moveFrontCount = (int)(tankSize * threshold);
	moveFrontCount = moveFrontCount == 0 ? 1 : moveFrontCount;

	

	uList eList;
	uList eDList;
	uList eBList;
	Unit me;
	int i = 0;


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

				siege(sortTank.at(i)->unit());
			}

			continue;
		}

		
		if (sortTank.at(i)->unit()->isSieged()) {

			eList = INFO.getUnitsInRadius(E, sortTank.at(i)->pos(), 12 * TILE_SIZE, true, false, false);

			if (eList.size() != 0) {
				attackFirstTargetOfTank(sortTank.at(i)->unit(), eList);
				continue;
			}
		}


		if (getGroundDistance(sortTank.at(i)->pos(), frontUnit->pos()) >= 30 * TILE_SIZE)
			moveFrontCount++;

		moveForward(sortTank.at(i)->unit(), targetPos);
	}


	if (threshold == 1) {
		if (!isStuckOrUnderSpell(frontUnit))
		{
			
			if (siegeAllSet.find(frontUnit) != siegeAllSet.end()) {
				if (frontUnit->unit()->isSieged()) {
					eList = INFO.getUnitsInRadius(E, frontUnit->pos(), 12 * TILE_SIZE, true, false, false);
					attackFirstTargetOfTank(frontUnit->unit(), eList);
				}
				else {

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

		
		if (siegeAllSet.find(sortTank.at(i)) != siegeAllSet.end()) {
			if (!me->isSieged()) {
				siege(me);

			}
			else {
				eList.insert(eList.end(), eDList.begin(), eDList.end());
				attackFirstTargetOfTank(me, eList);
			}

			continue;
		}

		if (eDList.empty() && (checkZeroAltitueAroundMe(sortTank.at(i)->pos(), 8) || (TIME - sortTank.at(i)->getLastPositionTime() > 20 * 24)))
		{
			if (me->isSieged()) {

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

	
		if (!me->isSieged()) {

			if (amIMoreCloseToTarget(me, targetPos, distanceFromtarget)
					&& INFO.getUnitsInRadius(E, me->getPosition(), 6 * TILE_SIZE, true, false, false, false).empty()) {

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
			
			if (!eList.empty()) {

				
				if (eList.size() == closeElist.size())
					unsiege(t->unit(), false);
				else {
					attackFirstTargetOfTank(t->unit(), eList);
				}
			}
			else {
				
				if (t->pos().getApproxDistance(waitingPosition) > 15 * TILE_SIZE)
					unsiege(t->unit(), false);
			}
		}
		else {

			
			if (eList.empty()) {
				
				if (t->pos().getApproxDistance(waitingPosition) > 15 * TILE_SIZE)
					CommandUtil::attackMove(t->unit(), waitingPosition);
				else if (!checkZeroAltitueAroundMe(t->pos(), 6))
					siege(t->unit());
			}
			else {
				
				if (eList.size() != closeElist.size()) {
					siege(t->unit());
				}
				else {
					
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

	
	for (auto b : br) {
		if (!b->unit()->canLand() && !b->unit()->canLift()
				|| (b->type() == Terran_Missile_Turret && !b->isComplete())) {
			return;
		}
	}


	Position c = (Position)INFO.getFirstExpansionLocation(S)->getTilePosition();
	Position cr = c + (Position)Terran_Command_Center.tileSize();

	
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

	float threashold = 15; 
	float threashold2 = 2; 

	if (INFO.getFirstExpansionLocation(S)) {
		
		if (getGroundDistance(INFO.getFirstExpansionLocation(S)->Center(), firstTank->pos()) < 40 * TILE_SIZE) {
			threashold = 10;
			threashold2 = 3;
		}
	}

	
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

	int distanceFromTarget = getGroundDistance(frontTankOfNotDefenceTank->pos(), targetPos); 
	int distanceFromMainBase = getGroundDistance(frontTankOfNotDefenceTank->pos(), MYBASE); 
	word enemyCnt = getEnemyNeerMyAllTanks(notDefenceTankList.getUnits());

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
			if (isSameArea(newPos, firstExpansion->Center()) && getAltitude(newPos) > 0) { 
				if (secondChokeMiddle.getApproxDistance(avgChoke) + TILE_SIZE <= secondChokeMiddle.getApproxDistance(newPos)
						&& secondChokeEnd1.getApproxDistance(newPos) > 3 * TILE_SIZE
						&& secondChokeEnd2.getApproxDistance(newPos) > 3 * TILE_SIZE
						&& firstChoke.getApproxDistance(newPos) > 4 * TILE_SIZE) { 

					
					Position c = (Position)firstExpansion->getTilePosition();
					Position cr = c + (Position)Terran_Command_Center.tileSize();

					if (newPos.x >= c.x - 16 && newPos.y >= c.y - 16
							&& newPos.x <= cr.x + 16 && newPos.y <= cr.y + 16)
						continue;

				
					Position cs = Position(cr.x, c.y + 32);
					Position csr = cs + (Position)Terran_Comsat_Station.tileSize();

					if (newPos.x >= cs.x - 16 && newPos.y >= cs.y - 16
							&& newPos.x <= csr.x + 16 && newPos.y <= csr.y + 16)
						continue;

					Position b = (Position)SelfBuildingPlaceFinder::Instance().getBarracksPositionInSCP();
					Position br = b + (Position)Terran_Barracks.tileSize();

					if (newPos.x >= b.x && newPos.y >= b.y
							&& newPos.x <= br.x && newPos.y <= br.y)
						continue;

					Position e = (Position)SelfBuildingPlaceFinder::Instance().getEngineeringBayPositionInSCP();
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
			else 
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


Position TankManager::getPositionOnTheHill()
{
	if (defencePositionOnTheHill != Positions::Unknown)
		return defencePositionOnTheHill;

	
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

	

	if (oldState == "Idle") {
		
	}
	else if (oldState == "AttackMove") {
		
	}
	else if (oldState == "Defence") {
		defenceTankSet.del(t);
	}
	else if (oldState == "BaseDefence") {
		baseDefenceTankSet.del(t);
	}
	else if (oldState == "BackMove") {
		
	}
	else if (oldState == "Gathering") {
		gatheringSet.del(t);
	}
	else if (oldState == "SiegeLineState") {
		siegeLineSet.del(t);
	}
	else if (oldState == "SiegeMovingState") {
		
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
		
	}
	else if (oldState == "ProtectAdditionalMulti") {

	}

	
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
		
	}
	else if (newState == "Gathering") {
		gatheringSet.add(t);
	}
	else if (newState == "SiegeLineState") {
		siegeLineSet.add(t);
	}
	else if (newState == "SiegeMovingState") {
		
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


void TankManager::closeUnitAttack(Unit unit) {

	uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() + TILE_SIZE, true, false);
	uList unitsInMinRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 3 * TILE_SIZE, true, false);

	
	if (unitsInMaxRange.size() == 0) {
		if (unit->isSieged())
			TM.unsiege(unit);
		else
			CommandUtil::attackMove(unit, waitingPosition);

		return;
	}

	
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

		
		UnitInfo *target = INFO.getClosestUnit(E, unit->getPosition(), GroundCombatKind, 0, false);

		if (!target)
			return;

		if (unitsInMaxRange.size() == unitsInMinRange.size()) {
			
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

	
	if (unitsInMaxRange.size() == 0) {
		if (unit->isSieged())
			TM.unsiege(unit, false);
		else
			CommandUtil::attackMove(unit, target->getPosition());

		return;
	}

	if (unit->isSieged()) {
		
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

		
		UnitInfo *targetFound = TM.getFirstAttackTargetOfTank(unit, unitsInMaxRange);

		if (targetFound != nullptr) {
			
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

			
			uList nearMyUnits = INFO.getUnitsInRadius(S, unit->getPosition(), 15 * TILE_SIZE, true, false, false);
			uList nearEUnits = INFO.getUnitsInRadius(E, targetInfo->pos(), 15 * TILE_SIZE, true, false, false);

			if ((nearEUnits.size() <= nearMyUnits.size()
					|| INFO.getAltitudeDifference(unit->getTilePosition(), (TilePosition)targetInfo->pos()) == 1)
					&& !checkZeroAltitueAroundMe(unit->getPosition(), 6)) {
				TM.siege(unit);
				return;
			}
			else {
				
				if (unit->getGroundWeaponCooldown() == 0) {
					CommandUtil::attackUnit(unit, targetInfo->unit());
				}
				else {
					if (unit->getDistance(targetInfo->unit()) > 6 * TILE_SIZE)
						CommandUtil::attackMove(unit, targetInfo->pos());
					else {
						
						moveBackPostion(INFO.getUnitInfo(unit, S), targetInfo->vPos(), 3 * TILE_SIZE);
					}
				}
			}
		}
	}
}


UnitInfo *TankManager::getDefenceTargetUnit(const uList &enemy, const Unit &unit, int defenceRange) {

	UnitInfo *targetInfo = nullptr;
	int dist = INT_MAX;
	int distFromMain = 0;
	bool needDefence = false;
	defenceRange = defenceRange == 0 ? INT_MAX : defenceRange;


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
					
					else if (unit->getDistance(eu->unit()) < dist) {
						targetInfo = eu;
						dist = unit->getDistance(eu->unit());
					}
				}

			}
			else {
				
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

	
	if (targetInfo->unit()->isDetected()) {
		
		if (unit->isSieged()) {
			
			if (unit->getDistance(targetInfo->pos()) < WeaponTypes::Arclite_Shock_Cannon.minRange())
			{
				uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false);

				if (!TM.attackFirstTargetOfTank(unit, unitsInMaxRange))
					TM.unsiege(unit); 

				return;
			}
			else {
				CommandUtil::attackUnit(unit, targetInfo->unit());
				return;
			}

		}
		else {
			
			if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) +
					INFO.getCompletedCount(Terran_Goliath, S) +
					INFO.getCompletedCount(Terran_Vulture, S) < 7) {
				if (unit->getDistance(targetInfo->unit()) > 3 * TILE_SIZE)
					CommandUtil::attackMove(unit, targetInfo->pos());
				else {
					
					moveBackPostion(INFO.getUnitInfo(unit, S), targetInfo->vPos(), 3 * TILE_SIZE);
				}

			}
			else {
				
				CommandUtil::attackUnit(unit, targetInfo->unit());
			}

			return;

		}

	}
	else {

		
		if (unit->isSieged()) {
			
			uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false);

			if (!TM.attackFirstTargetOfTank(unit, unitsInMaxRange))
				TM.unsiege(unit); 

		}
		else {

			int threshold = 8 * TILE_SIZE;

			if (ComsatStationManager::Instance().getAvailableScanCount() > 0)
				threshold = 5 * TILE_SIZE;

			
			if (unit->getDistance(targetInfo->unit()) > threshold)
				CommandUtil::attackMove(unit, targetInfo->pos());
			else {
				
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

		
		if (t->unit()->isSieged()) {
			
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

			
			UnitInfo *closeUnitInfo = INFO.getClosestUnit(E, t->pos(), TypeKind::GroundUnitKind, 12 * TILE_SIZE, false, false, true);

			if (closeUnitInfo == nullptr) {
				
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

		
		if (!eDBUnit.empty() || 
				((eBUnit.size() > 3 || !nexus.empty()) && eCloseUnit.empty()
				 && !checkZeroAltitueAroundMe(t->pos(), 6)))
			siegeAllSet.insert(t);
		else if (eUnit.size() == eCloseUnit.size()) {
			continue;
		}

		
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

			
			TM.defenceOnFixedPosition(firstTank, posOnHill);
			TM.defenceOnFixedPosition(secondTank, posOnMineral);

		}

	}
	else {
	
		for (auto t : getUnits()) {

			
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

	for (word i = 0; i < tanks.size(); i++) {

		UnitInfo *t = tanks.at(i);
		uList enemy = INFO.getUnitsInRadius(E, t->pos(), (12 + tanks.size() * 2 - i) * TILE_SIZE, true, false, true, false);
		uList enemyBuilding = INFO.getBuildingsInRadius(E, t->pos(), 12 * TILE_SIZE, true, false);
		enemy.insert(enemy.end(), enemyBuilding.begin(), enemyBuilding.end());

		if (t->unit()->isSieged()) {
			if (!enemy.empty()) {
				
				TM.attackFirstTargetOfTank(t->unit(), enemy);
			}
			else {
				
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

		
			if (!enemy.empty()) {
			
				TM.siege(t->unit());
			}
			else {

				if (checkZeroAltitueAroundMe(t->pos(), 6))
					CommandUtil::attackMove(t->unit(), targetPosition, true);
				else {

					

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
				
				if (unitsInMaxRange.size() == unitsInMinRange.size()) {
					TM.unsiege(t->unit());

					continue;
				}

				TM.attackFirstTargetOfTank(t->unit(), unitsInMaxRange);
				continue;
			}
			else { 
				if (unitsInMaxRange.size() == unitsInMinRange.size()) {

					
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

//			bw->drawTextMap(t->pos() + Position(0, 40), "danger : %d", dangerPoint);

			if (dangerUnit == nullptr)
			{
				safe++;

				if (!t->unit()->isSieged())
					CommandUtil::attackMove(t->unit(), TM.getNextMovingPoint());
			}
			else if (dangerUnit->type() == Terran_Siege_Tank_Siege_Mode)
			{
				if (t->unit()->isSieged()) 
				{
					if (dangerPoint > Threshold)
						safe++;
				}
				else 
				{
					if (dangerPoint <= Threshold)
					{
						
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


	if (!INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, targetPos, 10 * TILE_SIZE, false).empty())
	{
		needScv = false;
	}

	for (auto t : getUnits())
	{
		Unit unit = t->unit();

	
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

		
		if (unitsInMaxRange.size() == 0) {
			if (unit->isSieged())
				TM.unsiege(unit);
			else if (needScv)
			{
				
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
				
				else if (myScv == nullptr || !myScv->exists())
					CommandUtil::move(unit, (Position)INFO.getSecondChokePoint(S)->Center());
			}
			else 
			{
				CommandUtil::move(unit, targetPos);
			}

			continue;
		}

		if (unit->isSieged()) {
			
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
			
			if (unitsInMaxRange.size() == unitsInMinRange.size()) {

				
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
