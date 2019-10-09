#include "ScoutManager.h"

using namespace MyBot;

ScoutManager::ScoutManager()
	: currentScoutUnitTime(0)
	, currentScoutUnit(nullptr)
	, currentScoutUnitInfo(nullptr)
	, dualScoutUnit(nullptr)
	, currentScoutStatus(ScoutStatus::NoScout)
	, isToFinishScvScout(false)
	, scoutDeadCount(0)
	, startHidingframe(0)
	, isPassThroughChokePoint(0)
	, isExistDangerUnit(false)
	, scouterNextPosition(Positions::None)
	, currentScoutTargetDistance(0)
{
	memset(cardinalPoints, 0, sizeof(cardinalPoints));
	memset(explorePoints, 0, sizeof(explorePoints));
	checkCardinalPoints = false;
	closestBaseLocation = nullptr;
}

ScoutManager &ScoutManager::Instance()
{
	static ScoutManager instance;
	return instance;
}

void ScoutManager::update() {

	// scoutUnit 지정
	if (assignScoutIfNeeded()) {
		// scoutUnit 컨트롤
		moveScoutUnit();
	}

	// 동시 출발하는 정찰유닛 컨트롤 현재는 적의 base 를 첫번째 일꾼과 반대 방향으로 서칭하는 로직만 존재.
	if (ESM.getEnemyInitialBuild() > Zerg_5_Drone && dualScoutUnit && dualScoutUnit->exists())
		moveScoutToEnemBaseArea(INFO.getUnitInfo(dualScoutUnit, S), INFO.getNextSearchBase(true)->Center());
}

// 정찰 일꾼이 선정되어있지 않거나 죽은 경우 새로 지정한다.
bool ScoutManager::assignScoutIfNeeded() {
	// 4, 5 드론, 벙커러시인 경우 정찰하지 않는다. (기존 정찰 유닛도 취소) +Toss 캐논러쉬, 2배럭포워드도 정찰일꾼 할당X
	if (((ESM.getEnemyInitialBuild() == Toss_4probes || ESM.getEnemyInitialBuild() == Terran_4scv || ESM.getEnemyInitialBuild() == Zerg_4_Drone_Real) && ESM.getEnemyMainBuild() == UnknownMainBuild)
			|| ESM.getEnemyInitialBuild() <= Zerg_5_Drone || ESM.getEnemyInitialBuild() == Terran_bunker_rush || ESM.getEnemyInitialBuild() == Toss_cannon_rush || ESM.getEnemyInitialBuild() == Terran_2b_forward || SM.getMainStrategy() == AttackAll)
	{
		// 4, 5드론인 경우에 돌아올때는 돌아오다가 길에 막히는 경우 미네랄 right click 이 필요하나
		// mineral 상태에서는 commandutil의 gather 사용 (area 가 많을 경우 이동 후 right click) 해서 저글링을 통과 못함.
		// 또한 mineral 상태에서 공격받을 시에 counter attack 이 동작하므로 본진까지 강제 이동.
		if (currentScoutUnit != nullptr) {
			if (ESM.getEnemyInitialBuild() <= Zerg_5_Drone && !isSameArea(currentScoutUnit->getPosition(), MYBASE)) {
				Unit m = ScvManager::Instance().getTemporaryMineral(currentScoutUnit->getPosition());
				CommandUtil::rightClick(currentScoutUnit, m, false, true);
			}
			else {
				ScvManager::Instance().setScvIdleState(currentScoutUnit);
				currentScoutUnit = nullptr;
				currentScoutUnitInfo = nullptr;
				currentScoutStatus = ScoutStatus::NoScout;
			}
		}

		if (dualScoutUnit != nullptr) {
			if (ESM.getEnemyInitialBuild() <= Zerg_5_Drone && !isSameArea(dualScoutUnit->getPosition(), MYBASE)) {
				Unit m = ScvManager::Instance().getTemporaryMineral(dualScoutUnit->getPosition());
				CommandUtil::rightClick(dualScoutUnit, m, false, true);
			}
			else {
				ScvManager::Instance().setScvIdleState(dualScoutUnit);
				dualScoutUnit = nullptr;
			}
		}

		return false;
	}

	// 두마리가 정찰을 하다가 적 기지를 발견한 경우
	if (INFO.isEnemyBaseFound && currentScoutUnit && currentScoutUnit->exists() && dualScoutUnit && dualScoutUnit->exists()) {
		int enemyCount = currentScoutUnitInfo->getEnemiesTargetMe().size() + INFO.getUnitInfo(dualScoutUnit, S)->getEnemiesTargetMe().size();

		if (enemyCount < 1) {
			if (getGroundDistance(INFO.getMainBaseLocation(E)->Center(), currentScoutUnit->getPosition()) >
					getGroundDistance(INFO.getMainBaseLocation(E)->Center(), dualScoutUnit->getPosition())) {
				ScvManager::Instance().setScvIdleState(currentScoutUnit);
				currentScoutUnit = dualScoutUnit;
				currentScoutUnitInfo = INFO.getUnitInfo(currentScoutUnit, S);
				dualScoutUnit = nullptr;
			}
			else {
				ScvManager::Instance().setScvIdleState(dualScoutUnit);
				dualScoutUnit = nullptr;
			}

			return true;
		}
	}

	// 정찰일꾼이 살아있으면 정찰 진행
	if (currentScoutUnit && currentScoutUnit->exists()) {
		// 3, 4 인용 맵인 경우 적이 저그이거나 알지못하는 상태이면 두번째 정찰을 출발시킨다.
		if (!INFO.isEnemyBaseFound && INFO.getMapPlayerLimit() >= 3 && (INFO.enemyRace == Races::Zerg || INFO.enemyRace == Races::Unknown)) {
			// 두번째 정찰은 배럭이 완성된 시점.
			if (dualScoutUnit == nullptr && INFO.getCompletedCount(Terran_Barracks, S) == 1) {
				// 적 베이스와 가장 가까이에 있는 Worker 를 정찰유닛으로 지정한다
				UnitInfo *scouter = ScvManager::Instance().chooseScouterScvClosestTo(INFO.getMainBaseLocation(E)->Center());

				// 정찰 나갈 일꾼이 있으면 정찰 일꾼으로 지정하고,
				// 정찰 나갈 일꾼이 없으면 (일꾼이 없다든지), 아무것도 하지 않는다
				if (scouter)
					dualScoutUnit = scouter->unit();
			}
		}

		return true;
	}

	currentScoutUnit = nullptr;
	currentScoutUnitInfo = nullptr;
	currentScoutStatus = ScoutStatus::NoScout;

	// 정찰 일꾼이 죽은 시점부터 30초 까지는 새로 지정하지 않는다.
	if (currentScoutUnitTime + 24 * 30 * scoutDeadCount > TIME)
		return false;

	// 섬인 경우 정찰일꾼 지정하지 않는다.
	if (INFO.getFirstChokePosition(S) == Positions::None)
		return false;

	// 적이 앞마당 멀티 지은것을 확인하지 못한 경우, 일꾼을 지정해야함.
	bool hasFirstExpansion = false;

	if (INFO.enemyRace != Races::Terran && INFO.getFirstExpansionLocation(E)) {
		uList buildings = INFO.getBuildingsInArea(E, INFO.getFirstExpansionLocation(E)->Center(), true, false, true);

		for (auto b : buildings) {
			if (b->type().isResourceDepot()) {
				hasFirstExpansion = true;
				break;
			}
		}
	}

	// 앞마당 멀티 한것을 발견한 경우
	if (hasFirstExpansion || INFO.enemyRace == Races::Terran) {
		// 적군 Base 를 알면서 정찰도 한 경우 (2인용 고려) 새로 지정 안함.
		if (INFO.isEnemyBaseFound && INFO.getMainBaseLocation(E)->GetLastVisitedTime())
			return false;

		// 팩토리 완성되었으면 새로 지정 안함.
		if (!INFO.getBuildings(Terran_Factory, S).empty())
			return false;
	}
	// 앞마당 멀티 한것을 발견 못한 경우
	else {
		// 벌쳐가 있으면 새로 지정 안함.
		if (INFO.getCompletedCount(Terran_Vulture, S) + INFO.getDestroyedCount(Terran_Vulture, S) > 0)
			return false;
	}

	// 정찰 출발 조건 정의
	int checkSupplyUsed =10 * 2;
	bool checkSupplyDepotConstructionStarted = false;
	bool checkSupplyDepotConstructionFinished = false;
	bool checkBarrackConstructionStarted = false;
	bool checkRefineryConstructionStarted = false;

	// 2인용 맵 = 적의 위치를 안다 + 러시 거리가 멀다 + 한번만에 도달 가능
	if (INFO.getMapPlayerLimit() == 2) {
		// vs 프로토스 : (일반적) 서플, 배럭, 가스 건설 시작 후 인구수 14 에 정찰 출발
		if (INFO.enemyRace == Races::Protoss) {
			checkSupplyUsed = 11 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = false;
			checkBarrackConstructionStarted = false;
			checkRefineryConstructionStarted = false;
		}
		// vs 테란 : (일반적) 서플, 배럭, 가스 건설 시작 후 인구수 14 에 정찰 출발
		else if (INFO.enemyRace == Races::Terran) {
			checkSupplyUsed = 10 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = false;
			checkBarrackConstructionStarted = false;
			checkRefineryConstructionStarted = false;
		}
		// vs 저그 : 서플, 배럭, 가스 건설 시작 후 인구수 13 에 정찰 출발 (4드론 저글링을 중간에 만나기때문에 괜찮다)
		else if (INFO.enemyRace == Races::Zerg) 
		{
			checkSupplyUsed = 10 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = false;
			checkBarrackConstructionStarted = false;
			checkRefineryConstructionStarted = false;
		}
		// vs 랜덤 : 서플, 배럭 건설 시작 후 인구수 12 에 정찰 출발
		else {
			checkSupplyUsed = 9 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = false;
			checkBarrackConstructionStarted = false;
			checkRefineryConstructionStarted = false;
		}
	}
	// 3인용 / 4인용 맵 = 적의 위치를 모른다
	else {
		// vs 프로토스 : 서플, 배럭, 가스 건설 시작 후 인구수 13 에 정찰 출발
		if (INFO.enemyRace == Races::Protoss) {
			checkSupplyUsed = 9 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = true;
		}
		// vs 테란 : 서플, 배럭, 가스 건설 시작 후 인구수 13 에 정찰 출발
		else if (INFO.enemyRace == Races::Terran) {
			checkSupplyUsed = 10 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = true;
		}
		// vs 저그 : 서플, 배럭 건설 시작 후 인구수 12 에 정찰 출발
		else if (INFO.enemyRace == Races::Zerg) {
			checkSupplyUsed = 14 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = false;
		}
		// vs 랜덤 : 서플, 배럭 건설 시작 후 인구수 12 에 정찰 출발
		else {
			checkSupplyUsed = 12 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = false;
		}
	}

	if (INFO.enemyRace == Races::Terran)
	{
		if (S->supplyUsed() < checkSupplyUsed)
			return false;
	}
	else
	{
		// 정찰 출발 조건 체크
		if (S->supplyUsed() < checkSupplyUsed)
			return false;

		if (checkSupplyDepotConstructionStarted && S->allUnitCount(Terran_Supply_Depot) < 1)
			return false;

		if (checkSupplyDepotConstructionFinished && S->completedUnitCount(Terran_Supply_Depot) < 1)
			return false;

		if (checkBarrackConstructionStarted && S->allUnitCount(Terran_Barracks) < 1)
			return false;

		if (checkRefineryConstructionStarted && S->allUnitCount(Terran_Refinery) < 1)
			return false;
	}

	// 적 베이스와 가장 가까이에 있는 Worker 를 정찰유닛으로 지정한다
	UnitInfo *scouter = ScvManager::Instance().chooseScouterScvClosestTo(INFO.getMainBaseLocation(E)->Center());

	// 정찰 나갈 일꾼이 있으면 정찰 일꾼으로 지정하고,
	// 정찰 나갈 일꾼이 없으면 (일꾼이 없다든지), 아무것도 하지 않는다
	if (scouter) {
		// set unit as scout unit
		currentScoutUnitTime = TIME;
		currentScoutUnit = scouter->unit();
		currentScoutUnitInfo = scouter;
		currentScoutStatus = ScoutStatus::AssignScout;

		return true;
	}

	return false;
}

void ScoutManager::waitTillFindFirstExpansion() {

	if (INFO.getSecondExpansionLocation(E))
		hidingPosition = INFO.getSecondExpansionLocation(E)->Center();

	if (hidingPosition == Positions::Unknown && INFO.getMainBaseLocation(E)) {
		int tmp = 0;

		// 적 본진에서 2번째로 가까운 베이스
		for (Base *targetBaseLocation : INFO.getBaseLocations()) {

			tmp = getGroundDistance(INFO.getMainBaseLocation(E)->Center(), targetBaseLocation->Center());

			if (tmp > 1200 && tmp < 2800) {
				hidingPosition = targetBaseLocation->Center();
				break;
			}

		}

		if (hidingPosition == Positions::Unknown)
			return;
	}

	int dst = 0;
	theMap.GetPath(hidingPosition, currentScoutUnitInfo->pos(), &dst); // 숨어있는 포인트 와의 거리

	if (INFO.getFirstExpansionLocation(E) == nullptr) return;

	// 15초동안 대기하다가 앞마당을 다시 확인
	if (TIME - INFO.getFirstExpansionLocation(E)->GetLastVisitedTime() > 15 * 24) {
		if (goWithoutDamage(currentScoutUnit, INFO.getFirstExpansionLocation(E)->getMineralBaseCenter(), direction) == false)
			direction *= -1;
	}
	else if (dst > TILE_SIZE) {
		if (goWithoutDamage(currentScoutUnit, hidingPosition, direction) == false)
			direction *= -1;
	}
}

// 상대방 MainBaseLocation 위치를 모르는 상황이면, StartLocation 들에 대해 아군의 MainBaseLocation에서 가까운 것부터 순서대로 정찰
// 상대방 MainBaseLocation 위치를 아는 상황이면, 해당 BaseLocation 이 있는 Area의 가장자리를 따라 계속 이동함 (정찰 유닛이 죽을때까지)
void ScoutManager::moveScoutUnit() {
	//cout << "정찰상태:" << currentScoutStatus << endl;

	// 최초 정찰유닛이 할당됨.
	if (currentScoutStatus == ScoutStatus::AssignScout) {
		// 상대방이 저그일 경우 마린 한기 나올때까지는 Second Choke Point에 위치해 있는다.
		//if (INFO.enemyRace == Races::Zerg && INFO.getCompletedCount(Terran_Marine, S) == 0 && scoutDeadCount == 0)
		//	currentScoutStatus = ScoutStatus::WaitAtMySecondChokePoint;
		// 2인용 맵이고 프로토스 상대인 경우, 포토러시 확인을 위해 본진을 한바퀴 돌고 출발한다.
		if (INFO.enemyRace == Races::Protoss && INFO.getMapPlayerLimit() == 2 && scoutDeadCount == 0)
			currentScoutStatus = ScoutStatus::LookAroundMyMainBase;
		// 상대가 프로토스이고 적 위치를 찾은 후 두번째 정찰인 경우 다른 베이스를 둘러보고 간다.
		else if (INFO.enemyRace == Races::Protoss && INFO.isEnemyBaseFound && scoutDeadCount >= 1) {
			int dist = INT_MAX;

			for (auto base : INFO.getStartLocations()) {
				if (base->Center() != INFO.getMainBaseLocation(S)->Center() && base->Center() != INFO.getMainBaseLocation(E)->Center()) {
					int tmpDist = getGroundDistance(INFO.getMainBaseLocation(E)->Center(), base->Center());

					if (dist > tmpDist && tmpDist >= 0) {
						dist = tmpDist;
						scouterNextPosition = base->Center();
					}
				}
			}

			currentScoutStatus = ScoutStatus::MovingToAnotherBaseLocation;
		}
		// 그 외의 경우 적 기지를 찾아간다.
		else
			currentScoutStatus = ScoutStatus::MovingToAnotherBaseLocation;
	}

	// 적군 메인베이스 찾기
	if (currentScoutStatus == ScoutStatus::MovingToAnotherBaseLocation) {
		// 현재 위치에서 1000이내에 적 건물이 있는지 체크
		checkEnemyBuildingsFromMyPos(currentScoutUnitInfo);
		// 현재 위치에서 1000이내에 적 원거리 공격 유닛이 있는지 체크
		checkEnemyRangedUnitsFromMyPos(currentScoutUnitInfo);

		// 둘러볼 베이스가 시야 안에 들어오면 적 기지로 간다.
		if (scouterNextPosition != Positions::None)
			if (bw->isVisible((TilePosition)scouterNextPosition))
				scouterNextPosition = Positions::None;

		moveScoutToEnemBaseArea(currentScoutUnitInfo, scouterNextPosition == Positions::None ? INFO.getMainBaseLocation(E)->getPosition() : scouterNextPosition);

		if (INFO.isEnemyBaseFound && isSameArea(currentScoutUnitInfo->pos(), INFO.getMainBaseLocation(E)->getPosition()))
			currentScoutStatus = ScoutStatus::MoveAroundEnemyBaseLocation;
	}
	// 적군 메인베이스 돌아다니면서 정찰
	else if (currentScoutStatus == ScoutStatus::MoveAroundEnemyBaseLocation) {

		Position gasPos = Positions::None;

		if (!INFO.getMainBaseLocation(E)->Geysers().empty())
			gasPos = INFO.getMainBaseLocation(E)->Geysers().at(0)->Unit()->getInitialPosition();

		Position firstMineralPos = INFO.getMainBaseLocation(E)->getEdgeMinerals().first->getInitialPosition();
		Position secondMineralPos = INFO.getMainBaseLocation(E)->getEdgeMinerals().second->getInitialPosition();
		Position targetPos = Positions::None;

		Position movePos = getDirectionDistancePosition(INFO.getMainBaseLocation(E)->Center(), currentScoutUnitInfo->pos(), 10 * TILE_SIZE);


		for (int i = 10; i > 0; i--)
		{
			Position temp = getDirectionDistancePosition(INFO.getMainBaseLocation(E)->Center(), currentScoutUnitInfo->pos(), i * TILE_SIZE);

			if (temp.isValid() && bw->isWalkable((WalkPosition)temp))
			{
				movePos = temp;
				break;
			}
		}

		targetPos = getCirclePosFromPosByDegree(INFO.getMainBaseLocation(E)->Center(), movePos, 15 * direction);
		bool found = false;
		int minimumDist = 1 * TILE_SIZE;

		for (int i = 0; i < 10; i++)
		{
			Position temp = getDirectionDistancePosition(targetPos, INFO.getMainBaseLocation(E)->Center(), (-1) * i * TILE_SIZE);
			int dp = INT_MAX;
			UnitInfo *dangerUnit = getDangerUnitNPoint(temp, &dp, false);

			if (dangerUnit != nullptr && dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
			{
				minimumDist = 4 * TILE_SIZE;
			}

			if (temp.isValid() && bw->getUnitsInRadius(temp, 1 * TILE_SIZE, Filter::IsBuilding || Filter::IsNeutral).size() == 0 && dp > minimumDist && bw->isWalkable((WalkPosition)temp))
			{
				targetPos = temp;
				found = true;
				break;
			}

		}

		if (!found)
		{
			for (int i = 0; i < 5; i++)
			{
				Position temp = getDirectionDistancePosition(targetPos, INFO.getMainBaseLocation(E)->Center(), i * TILE_SIZE);
				int dp = INT_MAX;
				UnitInfo *dangerUnit = getDangerUnitNPoint(temp, &dp, false);

				if (dangerUnit != nullptr && dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
				{
					minimumDist = 4 * TILE_SIZE;
				}

				if (temp.isValid() && bw->getUnitsInRadius(temp, 1 * TILE_SIZE, Filter::IsBuilding || Filter::IsNeutral).size() == 0 && dp > minimumDist && bw->isWalkable((WalkPosition)temp))
				{
					targetPos = temp;
					found = true;
					break;
				}
			}
		}

		if (found)
		{
			bw->drawCircleMap(targetPos, 5, Colors::Orange, true);

			if (goWithoutDamage(currentScoutUnitInfo->unit(), targetPos, direction) == false)
			{
				CommandUtil::rightClick(currentScoutUnitInfo->unit(), targetPos, true);
			}
			else if (targetPos.x > (theMap.Size().x - 3) * 32 || targetPos.y > (theMap.Size().y - 2) * 32) {
				direction *= -1;
			}
		}
		else //다 돌았는데 없으면
		{
			direction *= -1;
		}

		if ((gasPos.isValid() && bw->isExplored((TilePosition)gasPos)) && (firstMineralPos.isValid() && bw->isExplored((TilePosition)firstMineralPos)) && (secondMineralPos.isValid() && bw->isExplored((TilePosition)secondMineralPos)))
		{
			// 현재 위치에서 1000이내에 적 건물이 있는지 체크
			checkEnemyBuildingsFromMyPos(currentScoutUnitInfo);

			// 현재 위치에서 1000이내에 적 원거리 공격 유닛이 있는지 체크
			checkEnemyRangedUnitsFromMyPos(currentScoutUnitInfo);

			// 위험 유닛을 만나면 앞마당을 확인한다.
			if (startHidingframe == 0 && isExistDangerUnit && ESM.getEnemyInitialBuild() != UnknownBuild) {
				if (INFO.getMainBaseLocation(E)->GetLastVisitedTime() && !HasEnemyFirstExpansion()) {
					currentScoutStatus = ScoutStatus::WaitAtEnemyFirstExpansion;
					return;
				}
			}
		}


	}
	else if (currentScoutStatus == ScoutStatus::CheckEnemyFirstExpansion) {
		if (INFO.getFirstExpansionLocation(E) == nullptr) return;

		moveScoutToEnemBaseArea(currentScoutUnitInfo, INFO.getFirstExpansionLocation(E)->getMineralBaseCenter());
		//CommandUtil::move(currentScoutUnit, INFO.getFirstExpansionLocation(E)->getMineralBaseCenter());

		if (currentScoutUnit->getPosition().getApproxDistance(INFO.getFirstExpansionLocation(E)->getMineralBaseCenter()) < 2 * TILE_SIZE)
			currentScoutStatus = ScoutStatus::MoveAroundEnemyBaseLocation;
	}
	// 적군 멀티에 숨어서 주기적으로 확장 짓는지 여부 확인
	else if (currentScoutStatus == ScoutStatus::WaitAtEnemyFirstExpansion) {
		if (startHidingframe == 0)
			startHidingframe = TIME;
		else if (TIME - startHidingframe > 24 * 80)
			currentScoutStatus = ScoutStatus::MoveAroundEnemyBaseLocation;

		//따돌리다가 메인베이스에 도착한 경우 다시 movearound한다
		if (isSameArea(currentScoutUnitInfo->pos(), INFO.getMainBaseLocation(E)->Center()))
			currentScoutStatus = ScoutStatus::MoveAroundEnemyBaseLocation;;

		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(currentScoutUnitInfo->pos(), &dangerPoint, false);

		//숨어서 대기하고 싶지만 적군이 계속 따라오는 경우 따돌리면서 메인베이스로 다시 간다
		if (dangerUnit != nullptr && dangerPoint < 5 * TILE_SIZE)
		{
			moveScoutToEnemBaseArea(currentScoutUnitInfo, INFO.getMainBaseLocation(E)->Center());
		}
		else
		{
			waitTillFindFirstExpansion();
		}

		return;
	}
	// 세컨드 초크포인트 대기
	else if (currentScoutStatus == ScoutStatus::WaitAtMySecondChokePoint) {
		CommandUtil::move(currentScoutUnit, INFO.getSecondChokePosition(S));

		if (INFO.getCompletedCount(Terran_Marine, S))
			currentScoutStatus = ScoutStatus::AssignScout;
	}
	else if (currentScoutStatus == ScoutStatus::LookAroundMyMainBase) {
		vector<Position> currentAreaVertices = INFO.getMainBaseLocation(S)->GetArea()->getBoundaryVertices();
		bool finishLookAround = true;

		for (auto v : currentAreaVertices) {
			if (!bw->isExplored((TilePosition)v)) {
				if (finishLookAround)
					CommandUtil::move(currentScoutUnit, v);

				finishLookAround = false;
				bw->drawCircleMap(v, 5, Colors::Green);
			}
		}

		if (!finishLookAround)
			return;

		currentScoutStatus = ScoutStatus::MovingToAnotherBaseLocation;
	}
}

Unit ScoutManager::getScoutUnit()
{
	return currentScoutUnit;
}

int ScoutManager::getScoutStatus()
{
	return currentScoutStatus;
}

void ScoutManager::moveScoutToEnemBaseArea(UnitInfo     *scouter, Position targetPos)
{
	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(scouter->pos(), &dangerPoint, false);

	if (dangerUnit != nullptr && dangerPoint < 5 * TILE_SIZE) //위험유닛이 있음
	{
		if (dangerPoint > 1 * TILE_SIZE) //약간거리가있어
		{
			Position tempTargetPos = Positions::None;
			int tempDist = INT_MAX;

			for (auto &base : INFO.getBaseLocations())
			{
				//if (getGroundDistance(base->Center(), scouter->pos()) < 8 * TILE_SIZE) continue;

				if (isSameArea(base->Center(), scouter->pos())) continue;

				if (getGroundDistance(scouter->pos(), base->Center()) < getGroundDistance(dangerUnit->pos(), base->Center()) && getGroundDistance(targetPos, base->Center()) < tempDist)
				{
					tempTargetPos = base->Center();
					tempDist = getGroundDistance(targetPos, base->Center());
				}
			}

			if (tempTargetPos == Positions::None)
			{
				moveBackPostion(scouter, dangerUnit->pos(), 3 * TILE_SIZE);
				return;
			}
			else
			{
				CommandUtil::rightClick(scouter->unit(), tempTargetPos, true);
				return;
			}
		}
		else
		{
			//TEST dp 1타일 내면 2타일거리만 본다
			moveBackPostion(scouter, dangerUnit->pos(), 2 * TILE_SIZE);
			return;
		}
	}
	else
	{
		CommandUtil::rightClick(scouter->unit(), targetPos);
	}

	return;
}

void ScoutManager::moveScoutInEnemyBaseArea(UnitInfo *scouter)
{
	for (auto geyser : INFO.getMainBaseLocation(E)->Geysers()) {
		// 아직 적 본진을 제대로 보지 못했다면 좀 맞더라도 일단 뚫고 들어간다
		if (!bw->isExplored((TilePosition)geyser->Pos())) {
			if (immediateRangedDamage == 0 || (scouter->hp() > immediateRangedDamage && INFO.getUnitsInRadius(E, scouter->pos(), 3 * TILE_SIZE, true, false, true, true).size() < 3))
			{
				CommandUtil::move(currentScoutUnit, geyser->Pos());
				return;
			}
		}
	}

	if (scouter->getVeryFrontEnemyUnit() != nullptr && scouter->unit()->getDistance(scouter->getVeryFrontEnemyUnit()->getPosition()) < 2 * TILE_SIZE)
	{
		if (enemAttackBuilding == nullptr || (enemAttackBuilding != nullptr && attackBuildingDist > enemAttackBuilding->type().groundWeapon().maxRange() + 3 * TILE_SIZE))
		{
			moveBackPostion(scouter, scouter->getVeryFrontEnemyUnit()->getPosition(), 3 * TILE_SIZE);
			return;
		}
	}

	Position scouterPos = INFO.getMainBaseLocation(E)->Center();

	if (isSameArea(scouter->pos(), scouterPos) &&
			!bw->getUnitsInRadius(scouter->pos(), 3 * TILE_SIZE, Filter::IsMineralField).empty()) {
		direction *= -1;
		currentScoutStatus = ScoutStatus::CheckEnemyFirstExpansion;
		return;
	}

	if (goWithoutDamageForSCV(scouter->unit(), scouterPos, direction) == false) {
		direction *= -1;

		if (direction == 1)
			currentScoutStatus = ScoutStatus::CheckEnemyFirstExpansion;
	}
	else {
		if (scouterPos.getApproxDistance(scouter->pos()) < 3 * TILE_SIZE) {
			direction *= -1;
			currentScoutStatus = ScoutStatus::CheckEnemyFirstExpansion;
		}
	}

	return;
}

void ScoutManager::checkEnemyBuildingsFromMyPos(UnitInfo *scouter)
{
	int maxDist = 0;

	enemAttackBuilding = nullptr;
	attackBuildingDist = INT_MAX;

	uList defenceBuildings = INFO.getTypeBuildingsInRadius(INFO.getAdvancedDefenseBuildingType(INFO.enemyRace), E, scouter->pos(), 1000, false, false);

	for (UnitInfo *ui : defenceBuildings) {
		int tempDist = scouter->pos().getApproxDistance(ui->pos());

		if (attackBuildingDist > tempDist)
		{
			attackBuildingDist = tempDist;
			enemAttackBuilding = ui;
		}
	}
}

void ScoutManager::checkEnemyRangedUnitsFromMyPos(UnitInfo *scouter)
{
	int maxDist = 0;

	isExistDangerUnit = false;

	// SCV 보다 이속이 빠른 유닛이 나오면 빠진다. 이속은 느리지만 사거리가 긴 유닛이나 유닛이 여러마리가 있으면 빠진다.
	if (INFO.enemyRace == Races::Zerg) {
		isExistDangerUnit = INFO.getAllCount(Zerg_Zergling, E) > 0 || INFO.getAllCount(Zerg_Hydralisk, E) > 1;
	}
	else if (INFO.enemyRace == Races::Protoss) {
		isExistDangerUnit = INFO.getAllCount(Protoss_Dragoon, E) > 0 || INFO.getAllCount(Protoss_Zealot, E) > 2;
	}
	else if (INFO.enemyRace == Races::Terran) {
		isExistDangerUnit = INFO.getAllCount(Terran_Vulture, E) > 0 || INFO.getAllCount(Terran_Marine, E) > 1;
	}

	immediateRangedDamage = 0;

	for (auto e : scouter->getEnemiesTargetMe())
		if (e->isInWeaponRange(scouter->unit()) && e->getGroundWeaponCooldown() < 10)
			immediateRangedDamage += getDamage(e, scouter->unit());
}

void ScoutManager::onUnitDestroyed(Unit scouter) {
	if (currentScoutUnit == scouter) {
		currentScoutUnitTime = TIME;
		scoutDeadCount++;

		if (dualScoutUnit && dualScoutUnit->exists()) {
			currentScoutUnit = dualScoutUnit;
			currentScoutUnitInfo = INFO.getUnitInfo(currentScoutUnit, S);
			dualScoutUnit = nullptr;
		}
	}
}