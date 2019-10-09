#include "ExploreManager.h"

using namespace MyBot;

ExploreManager::ExploreManager()
	: currentScoutUnitTIME(0)
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

ExploreManager &ExploreManager::Instance()
{
	static ExploreManager instance;
	return instance;
}

void ExploreManager::update() {

	if (assignScoutIfNeeded()) {

		moveScoutUnit();
	}

	if (ESM.getEnemyInitialBuild() > Zerg_5_Drone && dualScoutUnit && dualScoutUnit->exists())
		moveScoutToEnemBaseArea(INFO.getUnitInfo(dualScoutUnit, S), INFO.getNextSearchBase(true)->Center());
}

bool ExploreManager::assignScoutIfNeeded() {
	if (((ESM.getEnemyInitialBuild() == Toss_4probes || ESM.getEnemyInitialBuild() == Terran_4scv || ESM.getEnemyInitialBuild() == Zerg_4_Drone_Real) && ESM.getEnemyMainBuild() == UnknownMainBuild)
			|| ESM.getEnemyInitialBuild() <= Zerg_5_Drone || ESM.getEnemyInitialBuild() == Terran_bunker_rush || ESM.getEnemyInitialBuild() == Toss_cannon_rush || ESM.getEnemyInitialBuild() == Terran_2b_forward || SM.getMainStrategy() == AttackAll)
	{

		if (currentScoutUnit != nullptr) {
			if (ESM.getEnemyInitialBuild() <= Zerg_5_Drone && !isSameArea(currentScoutUnit->getPosition(), MYBASE)) {
				Unit m = SoldierManager::Instance().getTemporaryMineral(currentScoutUnit->getPosition());
				CommandUtil::rightClick(currentScoutUnit, m, false, true);
			}
			else {
				SoldierManager::Instance().setScvIdleState(currentScoutUnit);
				currentScoutUnit = nullptr;
				currentScoutUnitInfo = nullptr;
				currentScoutStatus = ScoutStatus::NoScout;
			}
		}

		if (dualScoutUnit != nullptr) {
			if (ESM.getEnemyInitialBuild() <= Zerg_5_Drone && !isSameArea(dualScoutUnit->getPosition(), MYBASE)) {
				Unit m = SoldierManager::Instance().getTemporaryMineral(dualScoutUnit->getPosition());
				CommandUtil::rightClick(dualScoutUnit, m, false, true);
			}
			else {
				SoldierManager::Instance().setScvIdleState(dualScoutUnit);
				dualScoutUnit = nullptr;
			}
		}

		return false;
	}


	if (INFO.isEnemyBaseFound && currentScoutUnit && currentScoutUnit->exists() && dualScoutUnit && dualScoutUnit->exists()) {
		int enemyCount = currentScoutUnitInfo->getEnemiesTargetMe().size() + INFO.getUnitInfo(dualScoutUnit, S)->getEnemiesTargetMe().size();

		if (enemyCount < 1) {
			if (getGroundDistance(INFO.getMainBaseLocation(E)->Center(), currentScoutUnit->getPosition()) >
					getGroundDistance(INFO.getMainBaseLocation(E)->Center(), dualScoutUnit->getPosition())) {
				SoldierManager::Instance().setScvIdleState(currentScoutUnit);
				currentScoutUnit = dualScoutUnit;
				currentScoutUnitInfo = INFO.getUnitInfo(currentScoutUnit, S);
				dualScoutUnit = nullptr;
			}
			else {
				SoldierManager::Instance().setScvIdleState(dualScoutUnit);
				dualScoutUnit = nullptr;
			}

			return true;
		}
	}


	if (currentScoutUnit && currentScoutUnit->exists()) {
	
		if (!INFO.isEnemyBaseFound && INFO.getMapPlayerLimit() >= 3 && (INFO.enemyRace == Races::Zerg || INFO.enemyRace == Races::Unknown)) {
	
			if (dualScoutUnit == nullptr && INFO.getCompletedCount(Terran_Barracks, S) == 1) {
			
				UnitInfo *scouter = SoldierManager::Instance().chooseScouterScvClosestTo(INFO.getMainBaseLocation(E)->Center());

	
				if (scouter)
					dualScoutUnit = scouter->unit();
			}
		}

		return true;
	}

	currentScoutUnit = nullptr;
	currentScoutUnitInfo = nullptr;
	currentScoutStatus = ScoutStatus::NoScout;

	if (currentScoutUnitTIME + 24 * 30 * scoutDeadCount > TIME)
		return false;

	if (INFO.getFirstChokePosition(S) == Positions::None)
		return false;

	
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
	/*
	const BWAPI::Unitset & squadUnits = defenseSquad.getUnits();

	// NOTE Defenders can be double-counted as both air and ground defenders. It can be a mistake.
	size_t flyingDefendersInSquad = std::count_if(squadUnits.begin(), squadUnits.end(), UnitUtil::CanAttackAir);
	size_t groundDefendersInSquad = std::count_if(squadUnits.begin(), squadUnits.end(), UnitUtil::CanAttackGround);


	}
	*/
	
	if (hasFirstExpansion || INFO.enemyRace == Races::Terran) {
		
		if (INFO.isEnemyBaseFound && INFO.getMainBaseLocation(E)->GetLastVisitedTime())
			return false;

		
		if (!INFO.getBuildings(Terran_Factory, S).empty())
			return false;
	}
	/*
	// add flying defenders if we still need them
	size_t flyingDefendersAdded = 0;
	BWAPI::Unit defenderToAdd;
	while (flyingDefendersNeeded > flyingDefendersInSquad + flyingDefendersAdded &&
	(defenderToAdd = findClosestDefender(defenseSquad, defenseSquad.getSquadOrder().getPosition(), true, false)))
	{
	UAB_ASSERT(!defenderToAdd->getType().isWorker(), "flying worker defender");
	_squadData.assignUnitToSquad(defenderToAdd, defenseSquad);
	++flyingDefendersAdded;
	*/
	/*

	if (INFO.isEnemyBaseFound && INFO.getMainBaseLocation(E)->GetLastVisitedTime())
	return false;


	if (!INFO.getBuildings(Terran_Factory, S).empty())
	return false;
	*/
	else {
		
		if (INFO.getCompletedCount(Terran_Vulture, S) + INFO.getDestroyedCount(Terran_Vulture, S) > 0)
			return false;
	}

	/*
				checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			else if (INFO.enemyRace == Races::Zerg) {
			checkSupplyUsed = 14 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = false;
			
			checkRefineryConstructionStarted = true;
	*/
	int checkSupplyUsed = 10 * 2;
	bool checkSupplyDepotConstructionStarted = false;
	bool checkSupplyDepotConstructionFinished = false;
	bool checkBarrackConstructionStarted = false;
	bool checkRefineryConstructionStarted = false;


	if (INFO.getMapPlayerLimit() == 2) {
	
		if (INFO.enemyRace == Races::Protoss) {
			checkSupplyUsed = 12 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = true;
		}
		
		else if (INFO.enemyRace == Races::Terran) {
			checkSupplyUsed = 14 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = true;
		}
		
		else if (INFO.enemyRace == Races::Zerg) {
			checkSupplyUsed = 14 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = true;
		}
	
		else {
			checkSupplyUsed = 12 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = false;
		}
	}
	
	else {
		
		if (INFO.enemyRace == Races::Protoss) {
			checkSupplyUsed = 12 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = true;
		}
		
		else if (INFO.enemyRace == Races::Terran) {
			checkSupplyUsed = 13 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = true;
		}
		
		else if (INFO.enemyRace == Races::Zerg) {
			checkSupplyUsed = 14 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = false;
		}
		/*
			if (BWAPI::Broodwar->getFrameCount() < 10 && surveySquad.isEmpty() && _squadData.squadExists("Idle")) {
		Squad & idleSquad = _squadData.getSquad("Idle");
		BWAPI::Unit myOverlord = nullptr;
		for (const auto unit : idleSquad.getUnits())
		{
			if (unit->getType() == BWAPI::UnitTypes::Zerg_Overlord &&
				_squadData.canAssignUnitToSquad(unit, surveySquad))
			{
				myOverlord = unit;
				break;
			}
		}
		*/
		else {
			checkSupplyUsed = 12 * 2;
			checkSupplyDepotConstructionStarted = false;
			checkSupplyDepotConstructionFinished = true;
			checkBarrackConstructionStarted = true;
			checkRefineryConstructionStarted = false;
		}
	}

	
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
	/*
	if (unit->isVisible() &&
	(!unit->isDetected() || unit->getOrder() == BWAPI::Orders::Burrowing) &&
	unit->getPosition().isValid())
	{
	// At most one scan per call. We don't check whether it succeeds.
	(void) Micro::SmartScan(unit->getPosition());
	break;
	}
	*/
	
	UnitInfo *scouter = SoldierManager::Instance().chooseScouterScvClosestTo(INFO.getMainBaseLocation(E)->Center());

	
	if (scouter) {
	
		currentScoutUnitTIME = TIME;
		currentScoutUnit = scouter->unit();
		currentScoutUnitInfo = scouter;
		currentScoutStatus = ScoutStatus::AssignScout;

		return true;
	}

	return false;
}
/*

Unit ExploreManager::getScoutUnit()
{
return currentScoutUnit;
}
*/
void ExploreManager::waitTillFindFirstExpansion() {

	if (INFO.getSecondExpansionLocation(E))
		hidingPosition = INFO.getSecondExpansionLocation(E)->Center();

	if (hidingPosition == Positions::Unknown && INFO.getMainBaseLocation(E)) {
		int tmp = 0;

		
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
	theMap.GetPath(hidingPosition, currentScoutUnitInfo->pos(), &dst);

	if (INFO.getFirstExpansionLocation(E) == nullptr) return;

	
	if (TIME - INFO.getFirstExpansionLocation(E)->GetLastVisitedTime() > 15 * 24) {
		if (goWithoutDamage(currentScoutUnit, INFO.getFirstExpansionLocation(E)->getMineralBaseCenter(), direction) == false)
			direction *= -1;
	}
	else if (dst > TILE_SIZE) {
		if (goWithoutDamage(currentScoutUnit, hidingPosition, direction) == false)
			direction *= -1;
	}
}


void ExploreManager::moveScoutUnit() {
	

	
	if (currentScoutStatus == ScoutStatus::AssignScout) {
		
		if (INFO.enemyRace == Races::Protoss && INFO.getMapPlayerLimit() == 2 && scoutDeadCount == 0)
			currentScoutStatus = ScoutStatus::LookAroundMyMainBase;
	
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
		
		else
			currentScoutStatus = ScoutStatus::MovingToAnotherBaseLocation;
	}

	
	if (currentScoutStatus == ScoutStatus::MovingToAnotherBaseLocation) {
		
		checkEnemyBuildingsFromMyPos(currentScoutUnitInfo);
		
		checkEnemyRangedUnitsFromMyPos(currentScoutUnitInfo);

		
		if (scouterNextPosition != Positions::None)
			if (bw->isVisible((TilePosition)scouterNextPosition))
				scouterNextPosition = Positions::None;

		moveScoutToEnemBaseArea(currentScoutUnitInfo, scouterNextPosition == Positions::None ? INFO.getMainBaseLocation(E)->getPosition() : scouterNextPosition);

		if (INFO.isEnemyBaseFound && isSameArea(currentScoutUnitInfo->pos(), INFO.getMainBaseLocation(E)->getPosition()))
			currentScoutStatus = ScoutStatus::MoveAroundEnemyBaseLocation;
	}
	
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
		else 
		{
			direction *= -1;
		}

		if ((gasPos.isValid() && bw->isExplored((TilePosition)gasPos)) && (firstMineralPos.isValid() && bw->isExplored((TilePosition)firstMineralPos)) && (secondMineralPos.isValid() && bw->isExplored((TilePosition)secondMineralPos)))
		{
			
			checkEnemyBuildingsFromMyPos(currentScoutUnitInfo);

			
			checkEnemyRangedUnitsFromMyPos(currentScoutUnitInfo);

			
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
	

		if (currentScoutUnit->getPosition().getApproxDistance(INFO.getFirstExpansionLocation(E)->getMineralBaseCenter()) < 2 * TILE_SIZE)
			currentScoutStatus = ScoutStatus::MoveAroundEnemyBaseLocation;
	}
	
	else if (currentScoutStatus == ScoutStatus::WaitAtEnemyFirstExpansion) {
		if (startHidingframe == 0)
			startHidingframe = TIME;
		else if (TIME - startHidingframe > 24 * 80)
			currentScoutStatus = ScoutStatus::MoveAroundEnemyBaseLocation;

		
		if (isSameArea(currentScoutUnitInfo->pos(), INFO.getMainBaseLocation(E)->Center()))
			currentScoutStatus = ScoutStatus::MoveAroundEnemyBaseLocation;;

		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(currentScoutUnitInfo->pos(), &dangerPoint, false);

		
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
/*
void ExploreManager::waitTillFindFirstExpansion() {

if (INFO.getSecondExpansionLocation(E))
hidingPosition = INFO.getSecondExpansionLocation(E)->Center();

if (hidingPosition == Positions::Unknown && INFO.getMainBaseLocation(E)) {
int tmp = 0;


hidingPosition = targetBaseLocation->Center();
break;
}

}

if (hidingPosition == Positions::Unknown)
}


*/
Unit ExploreManager::getScoutUnit()
{
	return currentScoutUnit;
}

int ExploreManager::getScoutStatus()
{
	return currentScoutStatus;
}

void ExploreManager::moveScoutToEnemBaseArea(UnitInfo     *scouter, Position targetPos)
{
	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(scouter->pos(), &dangerPoint, false);

	if (dangerUnit != nullptr && dangerPoint < 5 * TILE_SIZE) 
	{
		if (dangerPoint > 1 * TILE_SIZE) 
		{
			Position tempTargetPos = Positions::None;
			int tempDist = INT_MAX;

			for (auto &base : INFO.getBaseLocations())
			{
				

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

void ExploreManager::moveScoutInEnemyBaseArea(UnitInfo *scouter)
{
	for (auto geyser : INFO.getMainBaseLocation(E)->Geysers()) {
		
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

void ExploreManager::checkEnemyBuildingsFromMyPos(UnitInfo *scouter)
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

void ExploreManager::drawScoutInformation(int x, int y)
{

	BWAPI::Broodwar->drawTextScreen(x, y, "ScoutInfo: %s", _scoutStatus.c_str());
	BWAPI::Broodwar->drawTextScreen(x, y + 10, "GasSteal: %s", _gasStealStatus.c_str());
	for (size_t i(0); i < _enemyRegionVertices.size(); ++i)
	{
		BWAPI::Broodwar->drawCircleMap(_enemyRegionVertices[i], 4, BWAPI::Colors::Green, false);
		BWAPI::Broodwar->drawTextMap(_enemyRegionVertices[i], "%d", i);
	}
}

void ExploreManager::checkEnemyRangedUnitsFromMyPos(UnitInfo *scouter)
{
	int maxDist = 0;

	isExistDangerUnit = false;

	
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
void ExploreManager::moveScout()
{
	int scoutHP = _workerScout->getHitPoints() + _workerScout->getShields();


	// get the enemy base location, if we have one
	// Note: In case of an enemy proxy or weird map, this might be our own base. Roll with it.
	//BWTA::BaseLocation enemyBaseLocation = InformationManager::Instance().getBaseLocation();

	int scoutDistanceThreshold = 30;

	// if we initiated a gas steal and the worker isn't idle, 
	bool finishedConstructingGasSteal = _workerScout->isIdle() || _workerScout->isCarryingGas();
	if (!_gasStealFinished && _didGasSteal && !finishedConstructingGasSteal)
	{
		return;
	}
	// check to see if the gas steal is completed
	else if (_didGasSteal && finishedConstructingGasSteal)
	{
		_gasStealFinished = true;
	}

	// if we know where the enemy region is and where our scout is
	if (_workerScout )
	{
	//	int scoutDistanceToEnemy = MapTools::Instance().getGroundTileDistance(_workerScout->getPosition(), enemyBaseLocation->getPosition());
		//bool scoutInRangeOfenemy = scoutDistanceToEnemy <= scoutDistanceThreshold;

		// we only care if the scout is under attack within the enemy region
		// this ignores if their scout worker attacks it on the way to their base
		if (scoutHP < _previousScoutHP)
		{
			_scoutUnderAttack = true;
		}

		if (!_workerScout->isUnderAttack() && !enemyWorkerInRadius())
		{
			_scoutUnderAttack = false;
		}

		// if the scout is in the enemy region
		bool scoutInRangeOfenemy = true;
		if (scoutInRangeOfenemy)
		{
			if (_scoutUnderAttack)
			{
				_scoutStatus = "Under attack, fleeing";
				followPerimeter();
			}
			else
			{
			//	BWAPI::Unit closestWorker = enemyWorkerToHarass();

				// If configured and reasonable, harass an enemy worker.
				/*
				if (Config::Strategy::ScoutHarassEnemy && closestWorker && (!_tryGasSteal || _gasStealFinished))
				{
				_scoutStatus = "Harass enemy worker";
				_currentRegionVertexIndex = -1;
				Micro::SmartAttackUnit(_workerScout, closestWorker);
				}
				*/

				// otherwise keep circling the enemy region
				/*
				else
				{
				_scoutStatus = "Following perimeter";
				followPerimeter();
				}
				*/

			}
		}
		// if the scout is not in the enemy region
		else if (_scoutUnderAttack)
		{
			_scoutStatus = "Under attack, fleeing";

			followPerimeter();
		}
		else
		{
			_scoutStatus = "Enemy region known, going there";

			// move to the enemy region
			followPerimeter();
		}

	}
	BWAPI::Position center(BWAPI::Broodwar->mapWidth() * 16, BWAPI::Broodwar->mapHeight() * 16);
	BWAPI::Position baseP1, baseP2;
	int dx1 = baseP1.x - center.x, dy1 = baseP1.y - center.y;
	int dx2 = baseP2.x - center.x, dy2 = baseP2.y - center.y;
	double theta1 = std::atan2(dy1, dx1);
	double theta2 = std::atan2(dy2, dx2);
	/*
	//降序列->逆时针
	return theta1 > theta2;
	// for each start location in the level
	if (!enemyBaseLocation)
	{
	_scoutStatus = "Enemy base unknown, exploring";

	//逆时针探路
	std::vector<BWAPI::TilePosition> starts;
	for (const auto start : BWEM::Map::Instance().StartingLocations())
	{
	starts.push_back(start);
	}
	auto cmp = [](const BWAPI::TilePosition baseTP1, const BWAPI::TilePosition baseTP2) {
	BWAPI::Position center(BWAPI::Broodwar->mapWidth() * 16, BWAPI::Broodwar->mapHeight() * 16);
	BWAPI::Position baseP1(baseTP1), baseP2(baseTP2);
	int dx1 = baseP1.x - center.x, dy1 = baseP1.y - center.y;
	int dx2 = baseP2.x - center.x, dy2 = baseP2.y - center.y;
	double theta1 = std::atan2(dy1, dx1);
	double theta2 = std::atan2(dy2, dx2);
	//降序列->逆时针
	return theta1 > theta2;
	};
	std::sort(starts.begin(), starts.end(), cmp);
	auto iter = std::find(starts.begin(), starts.end(), BWAPI::Broodwar->self()->getStartLocation());
	std::reverse(starts.begin(), iter);
	std::reverse(iter++, starts.end());
	std::reverse(starts.begin(), starts.end());
	for (const auto start : starts)
	{
	// if we haven't explored it yet
	for (int x = start.x - 2; x <= start.x + 2; ++x)
	{
	for (int y = start.y - 2; y <= start.y + 2; ++y)
	{
	if (!BWAPI::Broodwar->isExplored(x, y))
	{
	Micro::SmartMove(_workerScout, BWAPI::Position(x * 32, y * 32));
	return;
	}
	}
	}
	}
	}

	_previousScoutHP = scoutHP;
	*/
	
}

void ExploreManager::onUnitDestroyed(Unit scouter) {
	if (currentScoutUnit == scouter) {
		currentScoutUnitTIME = TIME;
		scoutDeadCount++;

		if (dualScoutUnit && dualScoutUnit->exists()) {
			currentScoutUnit = dualScoutUnit;
			currentScoutUnitInfo = INFO.getUnitInfo(currentScoutUnit, S);
			dualScoutUnit = nullptr;
		}
	}
}
void ExploreManager::followPerimeter()
{
	BWAPI::Position fleeTo;
	bool DrawScoutInfo = false;
	if (DrawScoutInfo)
	{
		BWAPI::Broodwar->drawCircleMap(fleeTo, 5, BWAPI::Colors::Red, true);
	}

}
bool ExploreManager::enemyWorkerInRadius()
{
	for (const auto unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		if (unit->getType().isWorker() && (unit->getDistance(_workerScout) < 300))
		{
			return true;
		}
	}

	return false;
}
int ExploreManager::getClosestVertexIndex(BWAPI::Unit unit)
{
	int closestIndex = -1;
	int closestDistance = 10000000;

	for (size_t i(0); i < _enemyRegionVertices.size(); ++i)
	{
		int dist = unit->getDistance(_enemyRegionVertices[i]);
		if (dist < closestDistance)
		{
			closestDistance = dist;
			closestIndex = i;
		}
	}

	return closestIndex;
}