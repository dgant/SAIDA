#include "GoliathManager.h"
#include "../InformationManager.h"
#include "../StrategyManager.h"

using namespace MyBot;

GoliathManager::GoliathManager()
{

}

GoliathManager &GoliathManager::Instance()
{
	static GoliathManager instance;
	return instance;
}


void GoliathManager::update()
{
	if (TIME < 300 || TIME % 2 != 0)
		return;

	uList goliathList = INFO.getUnits(Terran_Goliath, S);

	if (goliathList.empty())
		return;

	setDefenceGoliath(goliathList);
	checkDropship();//
	setKeepMultiGoliath();

	if (INFO.enemyRace == Races::Terran)
	{
		setKeepMultiGoliath2(); // 일단은 Terran전만...
		setMultiBreakGoliath(); // 일단은 Terran전만...
	}

	for (auto g : goliathList)
	{
		if (isStuckOrUnderSpell(g))
			continue;

		string state = g->getState();

		// 테란전
		if (INFO.enemyRace == Races::Terran)
		{
			if (state == "New" && g->isComplete()) {
				g->setState(new GoliathIdleState());
			}

			if (state == "Idle")
			{
				if (S->hasResearched(TechTypes::Tank_Siege_Mode))
					setStateToGoliath(g, new GoliathTerranState());
				else
					setStateToGoliath(g, new GoliathFightState());
			}

			if (state == "VsTerran")
			{
				if (INFO.getCompletedCount(Terran_Wraith, E) != 0 &&
						goliathTerranSet.size() > 2 &&
						goliathProtectTankSet.size() < 2 && TM.getUsableTankCnt())
				{
					setStateToGoliath(g, new GoliathProtectTankState());
				}
			}

			g->action();
			continue;
		}

		// 저그 프로
		if (SM.getMainStrategy() == AttackAll || SM.getMainStrategy() == Eliminate)
		{
			if (INFO.enemyRace == Races::Zerg)
			{
				if (allAttackFrame == 0)
					allAttackFrame = TIME;

				if (state != "Attack" && state != "Defense" && state != "KeepMulti")
				{
					g->setState(new GoliathAttackState());
				}

				if (allAttackFrame + 20 * 24 < TIME && removeOurTurret(g))
					continue;
			}
			else // (INFO.enemyRace == Races::Protoss)
			{
				if (state != "Attack" && state != "CarrierDefence" && state != "CarrierAttack" && state != "KeepMulti") {
					g->setState(new GoliathProtossAttackState());
				}
			}
		}
		else if (SM.getMainStrategy() == BackAll) {

			if (INFO.enemyRace == Races::Zerg)
				allAttackFrame = 0;

			if (g->getState() == "Attack") {
				g->setState(new GoliathMoveState(INFO.getSecondChokePosition(S)));
			}
		}
		else {
			if (INFO.enemyRace == Races::Zerg)
			{
				allAttackFrame = 0;

				if (state == "New") {
					g->setState(new GoliathIdleState(INFO.getSecondChokePosition(S)));
				}
			}
			else //(INFO.enemyRace == Races::Protoss)
			{
				if (state == "New") {
					g->setState(new GoliathIdleState(INFO.getSecondChokePosition(S)));
				}
				else if (state == "ProtossAttack")
				{
					g->setState(new GoliathMoveState(INFO.getSecondChokePosition(S)));
				}
				else if (state == "CarrierAttack") {
					g->setState(new GoliathIdleState(INFO.getSecondChokePosition(S)));
				}
			}
		}

		g->action();
	}

	goliathTerranSet.action();
}

bool GoliathManager::removeOurTurret(UnitInfo *g) {

	if (INFO.getSecondAverageChokePosition(S) != Positions::None) {

		int outsideG = INFO.getCompletedCount(Terran_Goliath, S) - INFO.getTypeUnitsInArea(Terran_Goliath, S, MYBASE).size() - INFO.getTypeUnitsInArea(Terran_Goliath, S, INFO.getFirstExpansionLocation(S)->Center()).size();

		// 50%가 나가지 못한 상황
		if ((double) outsideG / INFO.getCompletedCount(Terran_Goliath, S) < 0.5) {
			// 내가 초크 근처인가
			if (g->pos().getApproxDistance(INFO.getSecondAverageChokePosition(S)) < 10 * TILE_SIZE) {

				// 근처에 적이 없는가?
				if (!INFO.getClosestUnit(E, g->pos(), AllUnitKind, 15 * TILE_SIZE, false, false, false)) {

					// 초크 근처에 터렛이 두개인가?
					uList turrets = INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, S, INFO.getSecondAverageChokePosition(S), 10 * TILE_SIZE, true);

					//아무거나 하나 깨자.
					if (turrets.size() > 1 && turrets.at(0) && turrets.at(0)->unit()) {
						CommandUtil::attackUnit(g->unit(), turrets.at(0)->unit());
						return true;
					}
				}

			}
		}


	}

	return false;
}

Position GoliathManager::getDefaultMovePosition() {
	if (defaultMovePositionFrame < TIME) {
		defaultMovePositionFrame = TIME;

		//Position targetPosition = INFO.getMainBaseLocation(E)->Center();
		Position targetPosition = SM.getMainAttackPosition();
		firstTankPosition = Positions::Unknown;

		UnitInfo *firstTank = TM.getFrontTankFromPos(targetPosition);

		if (firstTank != nullptr)
			firstTankPosition = firstTank->pos();
	}

	return firstTankPosition;
}

// 저그인 경우만 실행됨.
// 0 : nothing, 1 : wait, 2 : move back, 3 : force attack
int GoliathManager::getWaitOrMoveOrAttack() {
	if (isNeedToWaitAttackFrame < TIME) {
		isNeedToWaitAttackFrame = TIME;

		if (!SM.getMainAttackPosition().isValid()) {
			isNeedToWaitAttack = 0;
			return isNeedToWaitAttack;
		}

		CPPath path = theMap.GetPath(SM.getMainAttackPosition(), INFO.getMainBaseLocation(S)->Center());

		if (path.empty()) {
			isNeedToWaitAttack = 0;
			return isNeedToWaitAttack;
		}

		const ChokePoint *cp = path.at(0);

		// mainAttackPosition 과 나 사이에 있는 기지 찾기.
		for (word i = 1; i < path.size() && cp == path.at(i - 1); i++) {
			const pair<const Area *, const Area *> &p1 = cp->GetAreas();
			const pair<const Area *, const Area *> &p2 = path.at(i)->GetAreas();

			const Area *a = nullptr;

			if (p1.first == p2.first || p1.first == p2.second)
				a = p1.first;
			else if (p1.second == p2.first || p1.second == p2.second)
				a = p1.second;

			for (auto b : a->Bases()) {
				if (b.GetOccupiedInfo() == enemyBase) {
					cp = path.at(i);
					break;
				}
			}
		}

		uList sunken = INFO.getTypeBuildingsInRadius(Zerg_Sunken_Colony, E, (Position)cp->Center(), 14 * TILE_SIZE);

		if (sunken.empty()) {
			isNeedToWaitAttack = 0;
			return isNeedToWaitAttack;
		}

		if (sunken.size() == 1) {
			if (INFO.getTypeUnitsInRadius(Terran_Goliath, S, sunken.at(0)->pos(), 10 * TILE_SIZE).size() > 6) {
				isNeedToWaitAttack = 0;
				return isNeedToWaitAttack;
			}
		}

		if (INFO.getOccupiedBaseLocations(E).size() == 1) {
			// 내 병력이 적의 2배가 넘고 내 병력이 성큰 갯수의 3배가 넘거나, 인구수가 190 이상이면 공격
			if (S->supplyUsed() > 380 ||
					(INFO.getCompletedCount(Terran_Goliath, S) + INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S)
					 + INFO.getCompletedCount(Terran_Vulture, S)
					 > INFO.getCompletedCount(Zerg_Sunken_Colony, E) * 3
					 &&
					 INFO.getUnitsInRadius(S, sunken.at(0)->pos(), 20 * TILE_SIZE, true, false, false).size()
					 > INFO.getUnitsInRadius(E, sunken.at(0)->pos(), 8 * TILE_SIZE, true, true, false, true).size() * 2)) {
				isNeedToWaitAttack = 0;
				return isNeedToWaitAttack;
			}
		}

		// 성큰이 있고 공격 시즈가 있으면 일단 대기
		uList tanks = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);

		bool isExistBlockingTank = false;

		for (auto t : tanks) {
			if (t->getState() == "SiegeMovingState") {
				// 탱크가 길막을 당하고 있고 적 본진 근처에 있다.
				if (t->isBlocked() && t->pos().getApproxDistance((Position)cp->Center()) < 10 * TILE_SIZE)
					isExistBlockingTank = true;

				// 일단 탱크가 있으면 기다린다.
				isNeedToWaitAttack = 1;
			}
		}

		// 입구가 매우 좁고 (WalkPosition 길이로 20) 성큰이 4개 이상이면 위험.
		if (cp->Pos(ChokePoint::end1).getApproxDistance(cp->Pos(ChokePoint::end1)) < 20) {
			if (sunken.size() >= 4)
				isNeedToWaitAttack = 1;
		}
		// 입구가 넓고 성큰이 6개 이상이면 위험.
		else if (sunken.size() >= 6)
			isNeedToWaitAttack = 1;

		// 기다리는데 isExistBlockingTank 이면
		if (isNeedToWaitAttack && isExistBlockingTank) {
			if (INFO.getCompletedCount(Terran_Goliath, S) > 30 && sunken.size() < 4)
				isNeedToWaitAttack = 3;
			else
				isNeedToWaitAttack = 2;
		}

		isNeedToWaitAttackFrame = TIME + 24 * 5;
	}

	return isNeedToWaitAttack;
}

void GoliathManager::setDefenceGoliath(uList &gList) {
	word defenceUnitSize = 0;

	if (INFO.enemyRace == Races::Terran)
	{
		defenceUnitSize = INFO.enemyInMyArea().size();

		if (goliathDefenceSet.size() == defenceUnitSize)
			return;

		if (goliathDefenceSet.size() < defenceUnitSize)
		{
			uList sortedList;

			if (S->hasResearched(TechTypes::Tank_Siege_Mode))
				sortedList = goliathTerranSet.getSortedUnitList(MYBASE);
			else // 테란전 시즈업 되기 전까지는 Fight로 유지하기 때문에 여기서 Defence로 보내도록
			{
				UListSet allGol;

				for (auto g : gList)
					allGol.add(g);

				sortedList = allGol.getSortedUnitList(MYBASE);
			}

			for (auto g : sortedList)
			{
				setStateToGoliath(g, new GoliathDefenseState());

				if (goliathDefenceSet.size() == defenceUnitSize)
					break;
			}
		}
		else
		{
			uList sortedList = goliathDefenceSet.getSortedUnitList(MYBASE, true);

			for (auto g : sortedList)
			{
				setStateToGoliath(g, new GoliathIdleState());

				if (goliathDefenceSet.size() == defenceUnitSize)
					break;
			}
		}

		return;
	}

	double carrierCntRatio = 0;
	int carrcarrierCnt = 0;
	int interceptorCntInMyArea = 0;

	if (INFO.enemyRace == Races::Protoss) {
		uList interceptor = INFO.getUnits(Protoss_Interceptor, E);
		int interceptorCnt = interceptor.size();

		for (auto i : interceptor) {
			if (i->getLastSeenPosition().getApproxDistance(MYBASE) <= 50 * TILE_SIZE) {
				interceptorCntInMyArea++;
			}
		}

		carrcarrierCnt = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, MYBASE, 50 * TILE_SIZE).size();

		// 캐리어가 많으면 비율을 계산해서 방어에 할당
		if (interceptorCnt >= 20 && carrcarrierCnt != 0) {
			carrierCntRatio = ((double)interceptorCntInMyArea) / interceptorCnt;
			defenceUnitSize += (word)ceil(INFO.getCompletedCount(Terran_Goliath, S) * carrierCntRatio);
		}
		else {

			if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion) {
				defenceUnitSize += carrcarrierCnt * 8;
			}
			else {
				defenceUnitSize += carrcarrierCnt * 4;
			}

		}

		uList enemyInMyBase = INFO.enemyInMyArea();

		for (auto eu : enemyInMyBase)
		{
			if (eu->type() != Protoss_Observer && eu->type() && eu->type() != Protoss_Carrier &&
					eu->type() != Protoss_Interceptor)
				defenceUnitSize++;
		}

	}
	else { // 저그
		double zergUnits = 0.0;

		for (auto z : INFO.enemyInMyYard())
		{
			if (z->type() == Zerg_Zergling)
				zergUnits += 0.3;
			else if (z->type() == Zerg_Lurker)
				zergUnits += 2;
			else if (z->type() == Zerg_Hydralisk)
				zergUnits += 0.5;
			else if (z->type() == Zerg_Mutalisk)
				zergUnits += 0.8;
			else if (z->type() == Zerg_Guardian)
				zergUnits += 2;
			else
				zergUnits += 1;
		}

		defenceUnitSize = (word)ceil(zergUnits);
	}

	// 너무 많으니 제거
	if (defenceUnitSize < goliathDefenceSet.size()) {
		uList sorted = goliathDefenceSet.getSortedUnitList(INFO.getMainBaseLocation(S)->getPosition(), true);

		for (word i = 0, size = min(sorted.size(), goliathDefenceSet.size() - defenceUnitSize); i < size; i++) {
			setStateToGoliath(sorted.at(i), new GoliathIdleState());
		}
	}
	// 너무 적으니 추가
	else if (defenceUnitSize > goliathDefenceSet.size()) {
		vector<pair<int, UnitInfo * >> sortList;

		for (auto t : gList)
		{
			// 캐리어 있으면
			if (carrierCntRatio != 0.0) {
				if (t->getState() == "CarrierDefence" || t->getState() == "KeepMulti" || t->getState() == "Dropship" || t->getState() == "MultiBreak")
					continue;
			}
			else {
				if (t->getState() == "CarrierDefence" || t->getState() == "Defence" || t->getState() == "KeepMulti" || t->getState() == "Dropship" || t->getState() == "MultiBreak")
					continue;
			}

			int tempDist = 0;
			theMap.GetPath(t->pos(), INFO.getMainBaseLocation(S)->getPosition(), &tempDist);

			if (tempDist >= 0)
				sortList.push_back(pair<int, UnitInfo * >(tempDist, t));
		}

		sort(sortList.begin(), sortList.end(), [](pair<int, UnitInfo *> a, pair<int, UnitInfo *> b) {
			return a.first < b.first;
		});

		for (word i = 0, size = min(sortList.size(), defenceUnitSize - goliathDefenceSet.size()); i < size; i++) {

			UnitInfo *unit = sortList.at(i).second;

			if (INFO.enemyRace == Races::Protoss) {
				if ((carrcarrierCnt + interceptorCntInMyArea) != 0)
				{
					setStateToGoliath(unit, new GoliathCarrierDefenceState());
				}
				else {
					setStateToGoliath(unit, new GoliathDefenseState());

				}
			}
			else
			{
				setStateToGoliath(unit, new GoliathDefenseState());
			}

		}
	}
}

void GoliathManager::setKeepMultiGoliath()
{
	if (!SM.getNeedSecondExpansion())
		return;

	//base에 미네랄 다 캣으면 더이상 지키도록 할당하지 않습니다
	Base *base = INFO.getSecondExpansionLocation(S);

	if (base == nullptr || base->Minerals().size() == 0)
	{
		uList golsToIdle = goliahtKeepMultiSet.getUnits();

		for (auto g : golsToIdle)
			setStateToGoliath(g, new GoliathIdleState());

		return;
	}

	if (INFO.enemyRace == Races::Terran)
	{
		// 일단 4마리 안되면 Multi Goliath도 안간다.
		// Tank와 동일하게
		//if (goliathTerranSet.size() < 3)
		//	return;

		if (multiBase == Positions::Unknown)
			multiBase = INFO.getSecondExpansionLocation(S)->getPosition();

		if (multiBase == Positions::Unknown)
			return;

		if (goliahtKeepMultiSet.size() > 1)
			return;

		uList sortedGoliath = goliathTerranSet.getSortedUnitList(multiBase);

		for (auto g : sortedGoliath)
		{
			setStateToGoliath(g, new GoliathKeepMultiState(multiBase));

			if (goliahtKeepMultiSet.size() > 1)
				break;
		}
	}
	else
	{
		if (!SM.getNeedKeepSecondExpansion(Terran_Goliath) && !SM.getNeedKeepThirdExpansion(Terran_Goliath)) {
			uList golsToIdle = goliahtKeepMultiSet.getUnits();

			for (auto g : golsToIdle)
				setStateToGoliath(g, new GoliathIdleState());
		}
		else
		{
			for (auto g : INFO.getUnits(Terran_Goliath, S))
			{
				if (g->getState() == "New" && g->isComplete())
				{
					setStateToGoliath(g, new GoliathKeepMultiState());// action에서 target Position을 잡음.
				}
			}
		}
	}

	return;
}

void GoliathManager::setKeepMultiGoliath2()
{
	if (!SM.getNeedSecondExpansion())
		return;

	if (!SM.getNeedThirdExpansion())
	{
		if (!setMiddlePosition)
		{
			if (SM.getNeedSecondExpansion() && !goliahtKeepMultiSet.isEmpty())
			{
				int distanceFromSecondChokePoint = 0;
				int distanceFromKeepMultiSet = 0;
				Position firstGoliathPosition = (*goliahtKeepMultiSet.getSortedUnitList(INFO.getSecondExpansionLocation(S)->getPosition()).begin())->pos();
				theMap.GetPath(INFO.getSecondChokePosition(S), INFO.getSecondExpansionLocation(S)->getPosition(), &distanceFromSecondChokePoint);
				theMap.GetPath(firstGoliathPosition, INFO.getSecondExpansionLocation(S)->getPosition(), &distanceFromKeepMultiSet);

				if (distanceFromSecondChokePoint != -1 && distanceFromKeepMultiSet != -1
						&& distanceFromSecondChokePoint / 2 > distanceFromKeepMultiSet)
				{
					multiBase2 = firstGoliathPosition;
					setMiddlePosition = true;
					cout << "#### [Goliath] multiBase2 = " << multiBase2 / TILE_SIZE << " 구했어!" << endl;
				}
			}
			else
				return;
		}
	}
	else
	{
		//base에 미네랄 다 캣으면 더이상 지키도록 할당하지 않습니다
		Base *base = INFO.getThirdExpansionLocation(S);

		if (base == nullptr || base->Minerals().size() == 0)
		{
			uList golsToIdle = goliahtKeepMultiSet2.getUnits();

			for (auto g : golsToIdle)
				setStateToGoliath(g, new GoliathIdleState());

			return;
		}

		if (multiBase2 != INFO.getThirdExpansionLocation(S)->getPosition())
			multiBase2 = INFO.getThirdExpansionLocation(S)->getPosition();

		if (setMiddlePosition)
		{
			for (auto k : goliahtKeepMultiSet2.getUnits())
			{
				k->setState(new GoliathKeepMultiState(multiBase2));
			}

			setMiddlePosition = false;
			cout << "#### [Goliath] multiBase2 = " << multiBase2 / TILE_SIZE << " 로 리셋!!! 이동하자 이동" << endl;
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

	// 일단 4마리 안되면 Multi Goliath도 안간다.
	// Tank와 동일하게
	//if (goliathTerranSet.size() < 3)
	//	return;

	if (!multiBase2.isValid())
		return;

	if (goliahtKeepMultiSet2.size() > 1)
		return;

	uList sortedGoliath = goliathTerranSet.getSortedUnitList(multiBase2);

	for (auto g : sortedGoliath)
	{
		//setStateToGoliath(g, new GoliathKeepMultiState(multiBase2));
		goliathTerranSet.del(g);
		goliahtKeepMultiSet2.add(g);
		g->setState(new GoliathKeepMultiState(multiBase2));
		cout << "------- [Goliath] KeepMultiGoliath2 부여" << endl;

		if (goliahtKeepMultiSet2.size() > 1)
			break;
	}

	return;
}

void GoliathManager::setMultiBreakGoliath()
{
	if (SM.getNeedAttackMulti() == false)
	{
		if (goliathMultiBreakSet.size())
		{
			uList listToIdle = goliathMultiBreakSet.getUnits();

			for (auto g : listToIdle)
			{
				setStateToGoliath(g, new GoliathIdleState());
			}
		}

		return;
	}

	if (SM.getSecondAttackPosition() == Positions::Unknown || !SM.getSecondAttackPosition().isValid())
		return;

	int needGolCnt = needCountForBreakMulti(Terran_Goliath);

	// 이부분에서 Multi Break 병력을 빼준다.
	if (SM.getNeedAttackMulti() && (int)goliathMultiBreakSet.size() < needGolCnt)
	{
		uList sortedList;

		if (INFO.enemyRace == Races::Terran)
			sortedList = goliathTerranSet.getSortedUnitList(SM.getSecondAttackPosition());

		for (auto g : sortedList)
		{
			setStateToGoliath(g, new GoliathMultiBreakState());

			if (goliathMultiBreakSet.size() == needGolCnt || goliathTerranSet.size() < 5)
				break;
		}
	}
}

UnitInfo *GoliathManager::getFrontGoliathFromPos(Position pos)
{
	if (INFO.enemyRace == Races::Terran)
	{
		if (goliathTerranSet.size() == 0)
			return nullptr;

		return goliathTerranSet.getFrontUnitFromPosition(pos);
	}
	else
	{
		return INFO.getClosestTypeUnit(S, pos, Terran_Goliath, 0, false, true);
	}
}

void GoliathManager::onUnitDestroyed(Unit u)
{
	goliathDefenceSet.del(u);
	goliahtKeepMultiSet.del(u);
	goliahtKeepMultiSet2.del(u);

	if (INFO.enemyRace == Races::Terran)
	{
		UnitInfo *gInfo = INFO.getUnitInfo(u, S);

		if (gInfo && gInfo->getState() == "KeepMulti")
			SM.setMultiDeathPos(gInfo->pos());

		if (gInfo && gInfo->getState() == "MultiBreak")
			SM.setMultiBreakDeathPos(gInfo->pos());

		goliathTerranSet.del(u);
		goliathMultiBreakSet.del(u);
		dropshipSet.del(u);
		goliathProtectTankSet.del(u);
	}
}

void vsTerranSet::init()
{
	for (auto g : getUnits())
		g->setState(new GoliathIdleState());

	clear();
}

void vsTerranSet::action()
{
	if (size() == 0)
		return;

	// 탱크 시즈모드 푸는 시간을 기다린다.
	if (changedTime != 0 && changedTime + 100 > TIME)
		return;

	if (preStage != DrawLine && SM.getMainStrategy() == DrawLine)
	{
		changedTime = TIME;
		preStage = SM.getMainStrategy();
		return;
	}

	preStage = SM.getMainStrategy();

	for (auto g : getUnits())
	{
		if (isStuckOrUnderSpell(g))
			continue;

		//마인 공격로직 추가
		uList mines = INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, E, g->pos(), 6 * TILE_SIZE, false);

		if (!mines.empty())
		{
			CommandUtil::attackUnit(g->unit(), mines.at(0)->unit());
			return;
		}

		if (SM.getMainStrategy() == AttackAll)
		{
			CommandUtil::attackMove(g->unit(), SM.getMainAttackPosition());
		}
		else if (SM.getMainStrategy() == DrawLine)
		{
			vector<UnitType> types = { Terran_Wraith, Terran_Battlecruiser };
			UnitInfo *closestWraith = INFO.getClosestTypeUnit(E, g->pos(), types, 10 * TILE_SIZE, true, false, false);

			if (closestWraith == nullptr)
				CommandUtil::attackMove(g->unit(), SM.getDrawLinePosition());
			else
				CommandUtil::attackMove(g->unit(), closestWraith->pos());
		}
		else if (SM.getMainStrategy() == WaitLine)
		{
			int dangerPoint = 0;
			UnitInfo *dangerUnit = getDangerUnitNPoint(g->pos(), &dangerPoint, false);

			int myGDist, thresholdGDist;

			if (theMap.GetArea((WalkPosition)SM.getWaitLinePosition()) == nullptr)
			{
				myGDist = g->pos().getApproxDistance(INFO.getSecondChokePosition(S));
				thresholdGDist = SM.getWaitLinePosition().getApproxDistance(INFO.getSecondChokePosition(S));
			}
			else
			{
				myGDist = getGroundDistance(MYBASE, g->pos());
				thresholdGDist = getGroundDistance(MYBASE, SM.getWaitLinePosition());
			}

			if (myGDist > thresholdGDist - TILE_SIZE || (dangerUnit != nullptr && dangerUnit->type() == Terran_Siege_Tank_Siege_Mode && dangerPoint < 4 * TILE_SIZE))
			{
				CommandUtil::move(g->unit(), MYBASE);
				continue;
			}

			vector<UnitType> types = { Terran_Wraith, Terran_Battlecruiser };
			UnitInfo *closestWraith = INFO.getClosestTypeUnit(E, g->pos(), types, 10 * TILE_SIZE, true, false, false);

			if (closestWraith == nullptr)
				CommandUtil::attackMove(g->unit(), SM.getWaitLinePosition());
			else
				CommandUtil::attackMove(g->unit(), closestWraith->pos());
		}
		else if (SM.getMainStrategy() == WaitToFirstExpansion)
		{
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

			if (g->pos().getApproxDistance(waitPosition) > 3 * TILE_SIZE)
				CommandUtil::attackMove(g->unit(), waitPosition, true);
		}
	}

	return;
}

void GoliathManager::checkDropship()
{
	if (SM.getDropshipMode() == false)
		return;

	int GoliathSpace = Terran_Goliath.spaceRequired();
	word dropshipSetNum = 4;

	UnitInfo *availDropship = nullptr;

	for (auto ds : INFO.getUnits(Terran_Dropship, S))
	{
		if (ds->getSpaceRemaining() >= GoliathSpace &&
				(ds->getState() == "Boarding" || (ds->getState() == "Back" && DropshipManager::Instance().getBoardingState())))
		{
			availDropship = ds;
			break;
		}
	}

	if (availDropship != nullptr)
	{
		uList sortedGoliath = goliathTerranSet.getSortedUnitList(MYBASE);

		for (auto g : sortedGoliath)
		{
			//Dropship TEST
			if (dropshipSet.size() < dropshipSetNum)
			{
				setStateToGoliath(g, new GoliathDropshipState(availDropship->unit()));

				availDropship->delSpaceRemaining(GoliathSpace);

				if (availDropship->getSpaceRemaining() < GoliathSpace)
					break;
			}
		}
	}
}

bool GoliathManager::enoughGoliathForDrop()
{
	if (INFO.enemyRace == Races::Terran)
	{
		//bw->drawTextScreen(Position(200, 100), " GoliathSet : %d", goliathTerranSet.size());

		if (goliathTerranSet.size() + dropshipSet.size() >= 4)
		{
			return true;
		}
	}

	return false;
}

void GoliathManager::setStateToGoliath(UnitInfo *t, State *state)
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
	else if (oldState == "VsTerran") {
		goliathTerranSet.del(t);
	}
	else if (oldState == "KeepMulti") {
		goliahtKeepMultiSet.del(t);
		goliahtKeepMultiSet2.del(t);
	}
	else if (oldState == "Dropship") {
		dropshipSet.del(t);
	}
	else if (oldState == "Defense" || oldState == "CarrierDefence") {
		goliathDefenceSet.del(t);
	}
	else if (oldState == "MultiBreak") {
		goliathMultiBreakSet.del(t);
	}
	else if (oldState == "ProtectTank") {
		goliathProtectTankSet.del(t);
	}
	else if (oldState == "Fight") {
		//
	}

	///////////////////
	if (newState == "Idle") {
		// Do nothing
	}
	else if (newState == "AttackMove") {
		// Do nothing
	}
	else if (newState == "VsTerran") {
		goliathTerranSet.add(t);
	}
	else if (newState == "KeepMulti") {
		goliahtKeepMultiSet.add(t);
	}
	else if (newState == "Dropship") {
		dropshipSet.add(t);
	}
	else if (newState == "Defense" || newState == "CarrierDefence") {
		goliathDefenceSet.add(t);
	}
	else if (newState == "MultiBreak") {
		goliathMultiBreakSet.add(t);
	}
	else if (newState == "ProtectTank") {
		goliathProtectTankSet.add(t);
	}
	else if (newState == "Fight") {
		//
	}

	t->setState(state);

	return;
}