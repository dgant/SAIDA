#include "BuildStrategy.h"
#include "../HostileManager.h"
#include "../TrainManager.h"

using namespace MyBot;

BasicBuildStrategy &BasicBuildStrategy::Instance()
{
	static BasicBuildStrategy instance;
	return instance;
}

BasicBuildStrategy::BasicBuildStrategy()
{
	setFrameCountFlag(24 * 420);
	canceledBunker = false;
	notCancel = false;
}

BasicBuildStrategy::~BasicBuildStrategy()
{
}

TilePosition MyBot::BasicBuildStrategy::getSecondCommandPosition(bool force)
{

	if (secondCommandPosition != TilePositions::None) {

		if (INFO.getFirstExpansionLocation(S) && INFO.getFirstExpansionLocation(S)->getTilePosition() == secondCommandPosition)
			return secondCommandPosition;

		TilePosition tp = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Command_Center, (TilePosition)secondCommandPosition, false, MYBASE);

		if (tp != TilePositions::None) {
			return tp;
		}

	}

	secondCommandPosition = TilePositions::None;

	if (!INFO.getFirstExpansionLocation(S))
		return secondCommandPosition;

	if (!force && INFO.getAltitudeDifference() <= 0) {
		secondCommandPosition = INFO.getFirstExpansionLocation(S)->getTilePosition();
		return secondCommandPosition;
	}

	const Area *myArea = INFO.getMainBaseLocation(S)->GetArea();
	vector<Position> vertices = myArea->getBoundaryVertices();

	int dist = 10000000;

	Position firstExpansion = (Position)INFO.getFirstExpansionLocation(S)->getTilePosition();
	TilePosition tempPosition;
	int tempDist;

	for (auto p : vertices) {

		tempDist = (int)p.getDistance(firstExpansion);

		if (tempDist < dist) {
			tempPosition = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Command_Center, (TilePosition)p, false, MYBASE);

			if (tempPosition != TilePositions::None
					&& getGroundDistance(INFO.getWaitingPositionAtFirstChoke(3, 6), (Position)p) > 5 * TILE_SIZE
					&& INFO.getBuildingsInRadius(S, p, 5 * TILE_SIZE, true).empty()
					&& getGroundDistance(INFO.getFirstChokePosition(S), (Position)p) > 10 * TILE_SIZE) {
				secondCommandPosition = tempPosition;
				dist = tempDist;
			}
		}

	}

	if (secondCommandPosition == TilePositions::None) {
		secondCommandPosition = INFO.getFirstExpansionLocation(S)->getTilePosition();
	}

	return secondCommandPosition;
}

TilePosition MyBot::BasicBuildStrategy::getTurretPositionOnTheHill()
{
	if (turretposOnTheHill != TilePositions::None)
		return turretposOnTheHill;

	const Area *myArea = INFO.getMainBaseLocation(S)->GetArea();
	vector<Position> vertices = myArea->getBoundaryVertices();

	int dist = 10000000;
	turretposOnTheHill = TilePositions::None;
	Position firstChoke = (Position)INFO.getFirstChokePoint(S)->Center();
	TilePosition tempPosition;
	int tempDist;

	for (auto p : vertices) {

		tempDist = (int)p.getDistance(firstChoke);

		if (tempDist > 5 * TILE_SIZE)
			continue;

		if (!bw->canBuildHere((TilePosition)p, Terran_Missile_Turret)) {

			Position p1 = Position(p.x, p.y + 50);

			if (bw->canBuildHere((TilePosition)p1, Terran_Missile_Turret))
				p = p1;

			Position p2 = Position(p.x, p.y - 50);

			if (bw->canBuildHere((TilePosition)p2, Terran_Missile_Turret))
				p = p2;

		}

		if (!bw->canBuildHere((TilePosition)p, Terran_Missile_Turret)) {
			tempPosition = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch
						   (Terran_Missile_Turret, (TilePosition)p, false, MYBASE);

			tempDist = (int)firstChoke.getDistance((Position)tempPosition);
		} else {
			tempPosition = (TilePosition)p;
		}

		if (tempDist < dist && (INFO.getAltitudeDifference() > 0 || tempDist > 3 * TILE_SIZE)) {

			if (tempPosition != TilePositions::None) {
				turretposOnTheHill = tempPosition;
				dist = tempDist;
			}
		}
	}

	if (dist > 6 * TILE_SIZE) {
		turretposOnTheHill = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(
								 Terran_Missile_Turret, (TilePosition)INFO.getFirstChokePoint(S)->Center(), false, MYBASE);
	}

	if (turretposOnTheHill == TilePositions::None) {
		turretposOnTheHill = (TilePosition)INFO.getFirstChokePoint(S)->Center();
	}

	return turretposOnTheHill;
}


int BasicBuildStrategy::getFrameCountFlag()
{
	return frameCountFlag;
}

void BasicBuildStrategy::setFrameCountFlag(int frameCountFlag_)
{
	frameCountFlag = frameCountFlag_;
}


bool BasicBuildStrategy::canBuildFirstBarrack(int supplyUsed)
{
	if (isExistOrUnderConstruction(Terran_Barracks)) {
		return false;
	}
	else {
		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			if (!hasEnoughResourcesToBuild(Terran_Barracks))
				return false;

			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildRefinery(int supplyUsed, BuildOrderItem::SeedPositionStrategy seedPositionStrategy) {
	if (CM.existConstructionQueueItem(Terran_Refinery) || BM.buildQueue.existItem(Terran_Refinery))
		return false;
	else {
		if (!hasEnoughResourcesToBuild(Terran_Refinery))
			return false;

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag()))
		{	
			if (hasRefinery(seedPositionStrategy))
				return false;

			if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation) {
				if (INFO.getFirstExpansionLocation(S) == nullptr)
					return false;

				uList commandList = INFO.getBuildings(Terran_Command_Center, S);

				for (auto center : commandList)
				{
					if (!center->isComplete())
						continue;

					if (!isSameArea(center->pos(), INFO.getFirstExpansionLocation(S)->Center()))
						continue;

					if (SoldierManager::Instance().getDepotMineralSize(center->unit())
							<= SoldierManager::Instance().getAssignedScvCount(center->unit()))
					{
						return true;
					}
				}

				return false;
			}
			else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation) {
				if (INFO.getSecondExpansionLocation(S) == nullptr)
					return false;

				uList commandList = INFO.getBuildings(Terran_Command_Center, S);

				for (auto center : commandList)
				{
					if (!center->isComplete())
						continue;

					if (!isSameArea(center->pos(), INFO.getSecondExpansionLocation(S)->Center()))
						continue;

					return true;
				}

				return false;
			}
			else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation) {
				if (INFO.getThirdExpansionLocation(S) == nullptr)
					return false;

				uList commandList = INFO.getBuildings(Terran_Command_Center, S);

				for (auto center : commandList)
				{
					if (!center->isComplete())
						continue;

					if (!isSameArea(center->pos(), INFO.getThirdExpansionLocation(S)->Center()))
						continue;

					return true;
				}

				return false;
			}

			return true;
		}
	}

	return false;
}
bool BasicBuildStrategy::canBuildRefinery(int supplyUsed, Base *base)
{
	if (CM.existConstructionQueueItem(Terran_Refinery) || BM.buildQueue.existItem(Terran_Refinery)) {
		return false;
	}
	else {
		if (!hasEnoughResourcesToBuild(Terran_Refinery)) {
			return false;
		}

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag()))
		{	
			if (hasRefinery(base))
				return false;

			if (S->gas() > 1000)
				return false;

			uList commandList = INFO.getBuildings(Terran_Command_Center, S);

			for (auto center : commandList)
			{
				if (!center->isComplete())
					continue;

				if (!isSameArea(center->pos(), base->Center()))
					continue;

				if (SoldierManager::Instance().getDepotMineralSize(center->unit())
						<= SoldierManager::Instance().getAssignedScvCount(center->unit()))
				{
					return true;
				}
			}
		}
	}

	return false;
}
bool BasicBuildStrategy::canBuildSecondChokePointBunker(int supplyUsed)
{
	if (!INFO.getTypeBuildingsInRadius(Terran_Bunker, S, INFO.getSecondChokePosition(S), 6 * TILE_SIZE).empty())
	{
		return false;
	}

	for (auto b : INFO.getBuildings(Terran_Bunker, S)) {
		if (isSameArea(b->unit()->getTilePosition(), INFO.getFirstExpansionLocation(S)->getTilePosition()))
			return false;
	}

	if (isUnderConstruction(Terran_Bunker)
			|| INFO.getCompletedCount(Terran_Barracks, S) == 0) {
		return false;
	}
	else {
		if (!hasEnoughResourcesToBuild(Terran_Bunker)) {
			return false;
		}

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::hasRefinery(BuildOrderItem::SeedPositionStrategy seedPositionStrategy)
{
	vector<Geyser *> geysers;

	if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::MainBaseLocation)
		geysers = INFO.getMainBaseLocation(S)->Geysers();
	else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation) {
		if (INFO.getFirstExpansionLocation(S) == nullptr)
			return true;

		geysers = INFO.getFirstExpansionLocation(S)->Geysers();
	}
	else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation) {
		if (INFO.getSecondExpansionLocation(S) == nullptr)
			return true;

		geysers = INFO.getSecondExpansionLocation(S)->Geysers();
	}
	else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation) {
		if (INFO.getThirdExpansionLocation(S) == nullptr)
			return true;

		geysers = INFO.getThirdExpansionLocation(S)->Geysers();
	}

	for (auto geyser : geysers)
		for (auto unit : bw->getUnitsOnTile(geyser->Unit()->getInitialTilePosition())) {
			if (unit->getType() == Resource_Vespene_Geyser)
				return false;
		}

	return true;
}

bool BasicBuildStrategy::hasRefinery(Base *base)
{
	if (base == nullptr)
		return false;

	vector<Geyser *> geysers;

	geysers = base->Geysers();

	for (auto geyser : geysers)
		for (auto unit : bw->getUnitsOnTile(geyser->Unit()->getTilePosition()))
			if (unit->getType() == Resource_Vespene_Geyser)
				return false;

	return false;
}

bool BasicBuildStrategy::canBuildFirstFactory(int supplyUsed)
{
	if (isExistOrUnderConstruction(Terran_Factory)
			|| INFO.getCompletedCount(Terran_Barracks, S) == 0)
	{
		return false;
	}
	else {

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			if (!hasEnoughResourcesToBuild(Terran_Factory)) {
				return false;
			}

			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildadditionalFactory()
{
	bool machineShopFirstFlag = true;

	if (INFO.enemyRace == Races::Protoss && EIB <= Toss_2g_forward) {
		machineShopFirstFlag = false;
	}

	int maxCount = 0;

	if (INFO.enemyRace == Races::Zerg)
		maxCount = 5;
	else if (INFO.enemyRace == Races::Protoss) {
		if (!isMyCommandAtFirstExpansion())
			maxCount = 2;
		else
			maxCount = 6;
	}
	else if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
			maxCount = 4;
		else
			maxCount = 6;
	}

	if (INFO.getActivationMineralBaseCount() >= 3)
	{
		if (INFO.getActivationGasBaseCount() >= 3)
		{
			maxCount += 3;

			if (INFO.enemyRace == Races::Terran && S->minerals() >= 4000 && S->gas() >= 1500)
				maxCount += 2;
		}

		if (S->minerals() >= 1200)
			maxCount += 2;
	}

	if ((machineShopFirstFlag && INFO.getCompletedCount(Terran_Machine_Shop, S) == 0)
			|| (INFO.getAllCount(Terran_Factory, S) + CM.getConstructionQueueItemCount(Terran_Factory) + BM.buildQueue.getItemCount(Terran_Factory)) >= maxCount
			|| BM.buildQueue.existItem(Terran_Factory))
	{
		return false;
	}

	if (INFO.enemyRace == Races::Terran) {
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath
				&& INFO.getAllCount(Terran_Factory, S) == 2
				&& (!(S->hasResearched(TechTypes::Tank_Siege_Mode) || S->isResearching(TechTypes::Tank_Siege_Mode))
					|| !(INFO.getAllCount(Terran_Academy, S) > 0 && INFO.getAllCount(Terran_Armory, S) > 0)
					|| INFO.getCompletedCount(Terran_Machine_Shop, S) < 2))
		{
			return false;
		}
	}
	else if (INFO.enemyRace == Races::Protoss) {

		if (!(EIB == Toss_2g_zealot || EIB == Toss_1g_forward || EIB == Toss_2g_forward || EMB == Toss_1base_fast_zealot)
				&& !(S->hasResearched(TechTypes::Tank_Siege_Mode) || S->isResearching(TechTypes::Tank_Siege_Mode)))
			return false;

		if (EMB == Toss_dark && INFO.getAllCount(Terran_Missile_Turret, S) == 0) {
			return false;
		}
		else if (EMB == Toss_range_up_dra_push && !HasEnemyFirstExpansion()) {
			if (INFO.getBuildings(Terran_Engineering_Bay, S).empty()) {
				return false;
			}
		}
		else if (INFO.getAllCount(Terran_Factory, S) > 1 && INFO.getBuildings(Terran_Engineering_Bay, S).empty()) {
			return false;
		}

		if (INFO.getAllCount(Terran_Factory, S) == 2
				&& !(INFO.getCompletedCount(Terran_Academy, S) > 0 && INFO.getCompletedCount(Terran_Armory, S) > 0))
			return false;
	}

	if (!hasEnoughResourcesToBuild(Terran_Factory)) {
		return false;
	}

	int canTrainTankCnt = 0;
	int canTrainGoliathCnt = 0;

	for (auto f : INFO.getBuildings(Terran_Factory, S)) {
		if (!f->isComplete())
			continue;

		if (f->unit()->getAddon()) {
			if (f->unit()->getAddon()->isCompleted()) {
				if (f->unit()->getTrainingQueue().empty() || (f->unit()->isTraining() && f->unit()->getRemainingTrainTime() <= 130))
					canTrainTankCnt++;
			}
			else if (!f->unit()->getAddon()->isCompleted() && f->unit()->getAddon()->getRemainingBuildTime() <= 130)
				canTrainTankCnt++;
		}
		else {
			if (f->unit()->getTrainingQueue().empty())
				canTrainGoliathCnt++;
			else if (f->unit()->isTraining() && f->unit()->getRemainingTrainTime() <= 130)
				canTrainGoliathCnt++;
		}
	}

	if (canTrainTankCnt > 0 || canTrainGoliathCnt > 0) {
		UnitType combatUnit = INFO.enemyRace == Races::Protoss && EMB != Toss_fast_carrier ? Terran_Vulture : Terran_Goliath;

		double mineralRate = 1.0;
		double gasRate = 1.0;

		mineralRate -= 0.01 * SoldierManager::Instance().getAllMineralScvCount();
		gasRate -= 0.02 * SoldierManager::Instance().getAllRefineryScvCount();

		int needMineral = Terran_Siege_Tank_Tank_Mode.mineralPrice() * canTrainTankCnt + combatUnit.mineralPrice() * canTrainGoliathCnt + int(Terran_Factory.mineralPrice() * mineralRate);
		int needGas = Terran_Siege_Tank_Tank_Mode.gasPrice() * canTrainTankCnt + combatUnit.gasPrice() * canTrainGoliathCnt + int(Terran_Factory.gasPrice() * gasRate);

		if (S->minerals() < needMineral && S->gas() < needGas)
			return false;
	}

	return true;
}

bool BasicBuildStrategy::canBuildMissileTurret(TilePosition p, int buildCnt)
{
	if (INFO.getCompletedCount(Terran_Engineering_Bay, S) == 0)
		return false;

	if (p == TilePositions::None)
		return false;
	else {
		if (getExistOrUnderConstructionCount(Terran_Missile_Turret, (Position)p) >= buildCnt)
			return false;
	}

	if (!hasEnoughResourcesToBuild(Terran_Missile_Turret))
		return false;

	return true;
}

bool BasicBuildStrategy::canBuildFirstExpansion(int supplyUsed)
{
	if (CM.existConstructionQueueItem(Terran_Command_Center)
			|| INFO.getAllCount(Terran_Command_Center, S) >= 2
			|| BM.buildQueue.existItem(Terran_Command_Center)) {
		return false;
	}
	else {

		if (!INFO.getTypeBuildingsInArea(Terran_Command_Center, S, INFO.getFirstExpansionLocation(S)->Center(), true).empty()) {
			return false;
		}

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			if (!hasEnoughResourcesToBuild(Terran_Command_Center)) {
				return false;
			}

			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildSecondExpansion(int supplyUsed)
{
	if (CM.existConstructionQueueItem(Terran_Command_Center)
			|| INFO.getAllCount(Terran_Command_Center, S) >= 3
			|| BM.buildQueue.existItem(Terran_Command_Center)) {
		return false;
	}
	else {
		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			if (INFO.enemyRace != Races::Protoss && !hasEnoughResourcesToBuild(Terran_Command_Center)) {
				return false;
			}

			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildThirdExpansion(int supplyUsed)
{
	if (CM.existConstructionQueueItem(Terran_Command_Center)
			|| INFO.getAllCount(Terran_Command_Center, S) >= 4
			|| BM.buildQueue.existItem(Terran_Command_Center)) {
		return false;
	}
	else {
		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			if (INFO.enemyRace != Races::Protoss && !hasEnoughResourcesToBuild(Terran_Command_Center)) {
				return false;
			}

			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildAdditionalExpansion(int supplyUsed)
{
	if (CM.existConstructionQueueItem(Terran_Command_Center)
			|| BM.buildQueue.existItem(Terran_Command_Center)) {
		return false;
	}
	else {
		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			if (!hasEnoughResourcesToBuild(Terran_Command_Center)) {
				return false;
			}

			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildFirstStarport(int supplyUsed)
{
	if (isExistOrUnderConstruction(Terran_Starport)
			|| INFO.getCompletedCount(Terran_Factory, S) == 0) {
		return false;
	}
	else {

		if (!hasEnoughResourcesToBuild(Terran_Starport)) {
			return false;
		}

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildFirstTurret(int supplyUsed)
{
	if (isExistOrUnderConstruction(Terran_Missile_Turret)
			|| INFO.getCompletedCount(Terran_Engineering_Bay, S) == 0) {
		return false;
	}
	else {
		if (!hasEnoughResourcesToBuild(Terran_Missile_Turret)) {
			return false;
		}

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildFirstArmory(int supplyUsed)
{
	if (isExistOrUnderConstruction(Terran_Armory)
			|| INFO.getCompletedCount(Terran_Factory, S) == 0) {
		return false;
	}
	else {
		if (!hasEnoughResourcesToBuild(Terran_Armory)) {
			return false;
		}

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildSecondArmory(int supplyUsed) {
	if (isUnderConstruction(Terran_Armory)
			|| INFO.getCompletedCount(Terran_Factory, S) == 0 || INFO.getAllCount(Terran_Armory, S) != 1) {
		return false;
	}

	if (!hasEnoughResourcesToBuild(Terran_Armory)) {
		return false;
	}

	if (!checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
		return false;
	}

	if (!isExistOrUnderConstruction(Terran_Science_Facility))
		return false;

	if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) >= 2 || S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Plating) >= 1)
		return true;

	for (auto a : INFO.getBuildings(Terran_Armory, S)) {
		if (a->isComplete() && a->unit()->isUpgrading() && a->unit()->getRemainingUpgradeTime() < Terran_Armory.buildTime() + 5 * 24)
			return true;
	}

	return false;
}

bool BasicBuildStrategy::canBuildFirstBunker(int supplyUsed)
{
	if (isExistOrUnderConstruction(Terran_Bunker)
			|| INFO.getCompletedCount(Terran_Barracks, S) == 0) {
		return false;
	}
	else {
		if (!hasEnoughResourcesToBuild(Terran_Bunker)) {
			return false;
		}

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildFirstEngineeringBay(int supplyUsed)
{
	if (isExistOrUnderConstruction(Terran_Engineering_Bay)) {
		return false;
	}
	else {
		if (!hasEnoughResourcesToBuild(Terran_Engineering_Bay)) {
			return false;
		}

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildFirstAcademy(int supplyUsed)
{
	if (isExistOrUnderConstruction(Terran_Academy)
			|| INFO.getCompletedCount(Terran_Barracks, S) == 0) {
		return false;
	}
	else {
		if (!hasEnoughResourcesToBuild(Terran_Academy)) {
			return false;
		}

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::canBuildFirstScienceFacility(int supplyUsed)
{
	if (isExistOrUnderConstruction(Terran_Science_Facility)
			|| INFO.getCompletedCount(Terran_Starport, S) == 0) {
		return false;
	}
	else {
		if (!hasEnoughResourcesToBuild(Terran_Science_Facility)) {
			return false;
		}

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) {
			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

bool BasicBuildStrategy::hasEnoughResourcesToBuild(UnitType unitType)
{
	return BM.hasEnoughResources(unitType);
}

bool BasicBuildStrategy::hasSecondCommand()
{
	return INFO.getBuildings(Terran_Command_Center, S).size() > 1;
}

bool BasicBuildStrategy::isTimeToMachineShopUpgrade(TechType techType, UpgradeType upType)
{
	if (INFO.getCompletedCount(Terran_Machine_Shop, S) == 0) {
		return false;
	}

	if (techType != TechTypes::None) {
		if (S->isResearching(techType)
				|| BM.buildQueue.existItem(techType)
				|| S->hasResearched(techType))
		{
			return false;
		}
	}
	else {
		if (S->isUpgrading(upType)
				|| BM.buildQueue.existItem(upType)
				|| S->getUpgradeLevel(upType) == 1)
		{
			return false;
		}
	}

	return true;
}

bool BasicBuildStrategy::isTimeToControlTowerUpgrade(TechType techType)
{
	if (INFO.getCompletedCount(Terran_Control_Tower, S) == 0) {
		return false;
	}

	if (S->isResearching(techType) == true
			|| BM.buildQueue.getItemCount(techType) != 0
			|| S->hasResearched(techType))
	{
		return false;
	}

	return true;
}

bool BasicBuildStrategy::isTimeToScienceFacilityUpgrade(TechType techType)
{
	if (INFO.getCompletedCount(Terran_Science_Facility, S) == 0) {
		return false;
	}

	if (S->isResearching(techType) == true
			|| BM.buildQueue.getItemCount(techType) != 0
			|| S->hasResearched(techType))
	{
		return false;
	}

	return true;
}

bool BasicBuildStrategy::isTimeToArmoryUpgrade(UpgradeType upgradeType, int level)
{
	if (INFO.getCompletedCount(Terran_Armory, S) == 0) {
		return false;
	}

	if (level > 1 && INFO.getCompletedCount(Terran_Science_Facility, S) == 0)
		return false;

	if (S->isUpgrading(upgradeType) == true
			|| BM.buildQueue.getItemCount(upgradeType) != 0
			|| S->getUpgradeLevel(upgradeType) >= level)
	{
		return false;
	}
	for (auto a : INFO.getBuildings(Terran_Armory, S))
		if (a->unit()->isCompleted() && !a->unit()->isUpgrading())
			return true;

	return false;
}

bool BasicBuildStrategy::isExistOrUnderConstruction(UnitType unitType)
{
	if (CM.existConstructionQueueItem(unitType)
			|| BM.buildQueue.existItem(unitType)
			|| INFO.getAllCount(unitType, S) >= 1) {
		return true;
	}

	if (unitType.isAddon()) {
		for (auto &unit : INFO.getUnits(S))
		{
			if (unit.first->getType() == unitType.whatBuilds().first) {
				if (unit.first->getLastCommand().getType() == UnitCommandTypes::Build_Addon
						&& (TIME - unit.first->getLastCommandFrame() < 10))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool BasicBuildStrategy::isUnderConstruction(UnitType unitType)
{
	if (CM.existConstructionQueueItem(unitType)
			|| BM.buildQueue.existItem(unitType)) {
		return true;
	}

	return false;
}

bool BasicBuildStrategy::checkSupplyUsedForInitialBuild(int supplyUsed, int until_frameCount)
{
	if (until_frameCount == 0) {
		return true;
	}

	if (TIME < until_frameCount) {
		if (S->supplyUsed() >= supplyUsed) {
			return true;
		}
		else {
			return false;
		}
	}
	else {
		// 추가 로직 필요
		return true;
	}
}


void BasicBuildStrategy::buildBunkerDefence()
{
	if (isExistOrUnderConstruction(Terran_Bunker)) {


		if (canceledBunker)
			return;

		bool cancelFlag = false;

		for (auto b : INFO.getBuildings(Terran_Bunker, S))
		{

			if (b->isComplete() && theMap.GetArea((WalkPosition)b->pos()) == theMap.GetArea(INFO.getMainBaseLocation(S)->Location())) {
				return;
			}
		}

		if (CM.existConstructionQueueItem(Terran_Bunker)) {
			const vector<ConstructionTask> *constructionQueue = CM.getConstructionQueue();

			for (const auto &b : *constructionQueue) {

				if (b.type == Terran_Bunker) {
					if (b.desiredPosition != TilePositions::None &&
							(theMap.GetArea(b.desiredPosition) != theMap.GetArea(INFO.getMainBaseLocation(S)->Location())
							 || theMap.GetArea(b.finalPosition) != theMap.GetArea(INFO.getMainBaseLocation(S)->Location()))) {
						CM.cancelConstructionTask(b);
						cancelFlag = true;
					}
				}
			}
		}

		cancelFlag = cancelFlag || BM.buildQueue.removeQueue(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, false);

		if (cancelFlag) {
			canceledBunker = true;

			BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, false);
		}
	}
	else {

		if (!hasEnoughResourcesToBuild(Terran_Bunker) && INFO.getUnitsInRadius(E, INFO.getMainBaseLocation(S)->Center(), 10 * TILE_SIZE, true, true, false).size() != 0) {


		
			for (auto s : INFO.getBuildings(Terran_Supply_Depot, S))
			{
				if (!s->isComplete())
				{
					s->unit()->cancelConstruction();
					break;
				}
			}

		}

		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, false);
	}
}

bool BasicBuildStrategy::isResearchingOrResearched(TechType techType, UpgradeType upType)
{
	if (techType != TechTypes::None) {
		if (S->isResearching(techType) == true
				|| S->hasResearched(techType))
		{
			return true;
		}
	}
	else if (upType != UpgradeTypes::None) {
		if (S->isUpgrading(upType) == true
				|| S->getUpgradeLevel(upType) == 1)
		{
			return true;
		}
	}

	return false;
}

int BasicBuildStrategy::getUnderConstructionCount(UnitType u, TilePosition p) {
	return BM.buildQueue.getItemCount(u, p) + CM.getConstructionQueueItemCount(u, p);
}

int BasicBuildStrategy::getExistOrUnderConstructionCount(UnitType unitType, Position position) {
	return INFO.getTypeBuildingsInRadius(unitType, S, position, 6 * TILE_SIZE).size() + getUnderConstructionCount(unitType, (TilePosition)position);
}

void BasicBuildStrategy::buildMissileTurrets(int count)
{
	if (INFO.enemyRace == Races::Terran && S->minerals() > 2000)
		count = 2;

	const Base *mainBase = INFO.getMainBaseLocation(S);
	Base *firstExpansion = INFO.getFirstExpansionLocation(S);

	if (getExistOrUnderConstructionCount(Terran_Missile_Turret, MYBASE) < count)
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation);
	}

	if (firstExpansion && !INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, firstExpansion->getPosition(), 5 * TILE_SIZE).empty())
	{
		if (getExistOrUnderConstructionCount(Terran_Missile_Turret, firstExpansion->Center()) < count)
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);
		}
	}

	Base *secondExpansion = INFO.getSecondExpansionLocation(S);

	if (secondExpansion == nullptr)
		return;

	if (!INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, secondExpansion->getPosition(), 5 * TILE_SIZE).empty())
	{
		if (getExistOrUnderConstructionCount(Terran_Missile_Turret, secondExpansion->Center()) < count)
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation);
		}
	}

	Base *thirdExpansion = INFO.getThirdExpansionLocation(S);

	if (thirdExpansion == nullptr)
		return;

	if (!INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, thirdExpansion->getPosition(), 5 * TILE_SIZE).empty())
	{
		if (getExistOrUnderConstructionCount(Terran_Missile_Turret, thirdExpansion->Center()) < count)
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation);
		}
	}

	for (auto b : INFO.getAdditionalExpansions())
	{
		if (!INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, b->getPosition(), 5 * TILE_SIZE).empty())
		{
			if (getExistOrUnderConstructionCount(Terran_Missile_Turret, b->Center()) < count)
			{
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, b->getTilePosition());
			}
		}
	}
}

void BasicBuildStrategy::executeBasicBuildStrategy()
{
	if (TIME % 12 != 0)
		return;


	if (INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, MYBASE, 5 * TILE_SIZE, true).empty()
			&& INFO.getUnitsInArea(E, MYBASE, true, true, false).empty()
			&& !CM.existConstructionQueueItem(Terran_Command_Center)
			&& !BM.buildQueue.existItem(Terran_Command_Center)
	   ) {
		int remainingM = 0;

		for (auto m : INFO.getMainBaseLocation(S)->Minerals())
			remainingM += m->Amount();

		if (remainingM >= 2000)
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::MainBaseLocation);
	}

	if (INFO.enemyRace == Races::Terran) {
		if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
			TerranVulutreTankWraithBuild();
		else if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
			TerranTankGoliathBuild();
	}
	else if (INFO.enemyRace == Races::Protoss) {

		if (canBuildFirstBarrack(20)) {
			BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
		}

		if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation)) {
			TilePosition refineryPos = BuildingPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

			if (refineryPos.isValid())
				BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
		}

		if (canBuildFirstFactory(32)) {
			BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
		}

		int baseVultureCount = 0;
		int baseZealotCount = 0;

		if (EIB == Toss_1g_forward)
		{
			baseVultureCount = 2;
			baseZealotCount = 3;
		}
		else if (EIB == Toss_2g_zealot || EIB == Toss_2g_forward || EMB == Toss_1base_fast_zealot)
		{
			baseVultureCount = 4;
			baseZealotCount = 5;
		}

		if (EIB == Toss_2g_zealot || EIB == Toss_1g_forward || EIB == Toss_2g_forward
				|| EMB == Toss_1base_fast_zealot)
		{
			if (INFO.getCompletedCount(Terran_Factory, S) == 0)
				buildBunkerDefence();

			if ((INFO.getCompletedCount(Terran_Vulture, S) >= baseVultureCount || INFO.getDestroyedCount(Protoss_Zealot, E) >= baseZealotCount)
					&& canBuildFirstExpansion(42))
				BM.buildQueue.queueAsLowestPriority(Terran_Command_Center, getSecondCommandPosition(), false);
			else if (S->minerals() >= 500 && canBuildFirstExpansion(42))
				BM.buildQueue.queueAsLowestPriority(Terran_Command_Center, getSecondCommandPosition(true), false);
			if (INFO.getCompletedCount(Terran_Vulture, S) >= 1 && isTimeToMachineShopUpgrade(TechTypes::Spider_Mines))
				BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines);

		}
		else if (EIB == Toss_pure_double || EIB == Toss_forge)
		{
			if (canBuildFirstExpansion(42)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
			}

		
			if (INFO.getAltitudeDifference() <= 0 && hasSecondCommand() && canBuildFirstBunker(42)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
			}

		}
		else if (EIB == Toss_cannon_rush) {
			uList cannons = INFO.getBuildings(Protoss_Photon_Cannon, E);
			bool isDestroyComplete = true;

			for (auto c : cannons) {
			
				if (isSameArea(c->pos(), INFO.getMainBaseLocation(S)->Center())
						|| (INFO.getFirstExpansionLocation(S) &&
							(isSameArea(c->pos(), INFO.getFirstExpansionLocation(S)->Center())
							 || getAttackDistance(c->type(), c->pos(), Terran_Command_Center, INFO.getFirstExpansionLocation(S)->Center()) <= Protoss_Photon_Cannon.groundWeapon().maxRange() + 2 * TILE_SIZE))) {
					isDestroyComplete = false;
					break;
				}
			}

			
			if (!isDestroyComplete && isExistOrUnderConstruction(Terran_Siege_Tank_Tank_Mode) && isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, true);
			}

			if (canBuildFirstExpansion(42)) {
				
				if (isDestroyComplete)
					BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
				
				else if (S->isResearching(TechTypes::Tank_Siege_Mode) || BM.buildQueue.existItem(TechTypes::Tank_Siege_Mode)
						 || S->hasResearched(TechTypes::Tank_Siege_Mode))
					BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, getSecondCommandPosition(true), false);
			}
		}
		else
		{
			if (canBuildFirstBunker(38)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
			}

			if (canBuildFirstExpansion(42)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
			}


		}


		if (!hasSecondCommand()) {
			return;
		}

		if (canBuildSecondChokePointBunker(62)/* && !HasEnemyFirstExpansion()*/
				&& isMyCommandAtFirstExpansion()) {
			BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
		}

		if (EMB == Toss_Scout || EMB == Toss_dark || EMB == Toss_dark_drop || EMB == Toss_drop || EMB == Toss_reaver_drop) {
		
			if (canBuildFirstEngineeringBay(0) && INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0) {
				int completeTime = INT_MAX;

				if (INFO.getAllCount(Protoss_Templar_Archives, E) + INFO.getAllCount(Protoss_Dark_Templar, E) + INFO.getAllCount(Protoss_Shuttle, E) + INFO.getAllCount(Protoss_Scout, E) == 0) {
					uList aduns = INFO.getBuildings(Protoss_Citadel_of_Adun, E);
					uList robotics = INFO.getBuildings(Protoss_Robotics_Facility, E);
					uList roboticsSupportBay = INFO.getBuildings(Protoss_Robotics_Support_Bay, E);

					
					if (!aduns.empty()) {
						for (auto a : aduns) {
							if (completeTime > a->getCompleteTime())
								completeTime = a->getCompleteTime();
						}
					}
					
					else if (!roboticsSupportBay.empty()) {
						for (auto r : roboticsSupportBay) {
							if (completeTime > r->getCompleteTime())
								completeTime = r->getCompleteTime();
						}
					}
				
					else if (!robotics.empty()) {
						for (auto r : robotics) {
							if (completeTime > r->getCompleteTime() + 450) {
								completeTime = r->getCompleteTime() + 450;
							}
						}
					}
				}
				else {
					completeTime = TIME;
				}

				if (completeTime <= TIME)
					BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, true);
			}
		}
		
		else if (EIB != Toss_cannon_rush && EMB == UnknownMainBuild && !HasEnemyFirstExpansion()) {
			
			if (TIME > 24 * 240 && canBuildFirstEngineeringBay(0) && INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0)
				BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, true);
		}
		
		else if (EMB == Toss_range_up_dra_push && !HasEnemyFirstExpansion()) {
			if (canBuildFirstEngineeringBay(0) && INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0) {
				int completeTime = INT_MAX;

				uList cyberneticsCores = INFO.getBuildings(Protoss_Cybernetics_Core, E);

				if (cyberneticsCores.empty()) {
					uList gateways = INFO.getBuildings(Protoss_Gateway, E);

					for (auto g : gateways)
						if (g->getCompleteTime() < completeTime)
							completeTime = g->getCompleteTime();

					completeTime += Protoss_Cybernetics_Core.buildTime();
				}
				else {
					for (auto c : cyberneticsCores)
						if (c->getCompleteTime() < completeTime)
							completeTime = c->getCompleteTime();
				}

				completeTime += Protoss_Citadel_of_Adun.buildTime();

				
				if (EMB == Toss_range_up_dra_push) {
				
					completeTime += 1080;
				}

				if (completeTime <= TIME)
					BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, true);
			}
		}
		else
		{
			
			if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) >= 2 && isResearchingOrResearched(TechTypes::Tank_Siege_Mode)
					&& canBuildFirstEngineeringBay(0))
				BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);
		}


	
		if (EMB == Toss_dark || (EIB != Toss_cannon_rush && EMB == UnknownMainBuild && !HasEnemyFirstExpansion() && TIME > 24 * 240))
		{
		
			if (INFO.getFirstExpansionLocation(S) && INFO.getSecondChokePoint(S)
					&& (isMyCommandAtFirstExpansion() || INFO.getCompletedCount(Terran_Bunker, S) != 0)
					&& INFO.getTypeUnitsInRadius(Protoss_Dark_Templar, E, (Position)INFO.getSecondChokePoint(S)->Center(), 8 * TILE_SIZE).empty()
					&& TIME < (24 * 270 + Protoss_Dark_Templar.buildTime() + 24 * 60)) {


				if (canBuildMissileTurret(getSecondChokePointTurretPosition())) {
					BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);
				}

			}
			else {
				if (INFO.getSecondChokePoint(S) && INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, S, (Position)INFO.getSecondChokePoint(S)->Center(), 8 * TILE_SIZE, true).empty())
				{
					if (canBuildMissileTurret(getTurretPositionOnTheHill()))
						BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, (TilePosition)getTurretPositionOnTheHill(), true);

					if (canBuildMissileTurret(getSecondChokePointTurretPosition()))
						BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, (TilePosition)getSecondChokePointTurretPosition(), true);
				}
			}

			if (canBuildMissileTurret((TilePosition)MYBASE) && S->supplyUsed() >= 80) {
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
			}
		}
		else if (EMB == Toss_dark_drop)
		{
			if (canBuildMissileTurret((TilePosition)MYBASE, 2)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
			}

			if (canBuildMissileTurret(getSecondChokePointTurretPosition())) {
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);
			}
		}
		else if (EMB == Toss_Scout || INFO.getCompletedCount(Protoss_Scout, E) > 0) {
		
			if (INFO.getCompletedCount(Terran_Goliath, S) <= INFO.getCompletedCount(Protoss_Scout, E)) {
				if (canBuildMissileTurret((TilePosition)MYBASE)) {
					BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
				}

				if (canBuildMissileTurret((TilePosition)INFO.getFirstExpansionLocation(S)->Center())) {
					BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, true);
				}
			}
		}
		else if (EMB == Toss_fast_carrier) {
			
			if (canBuildMissileTurret(getSecondChokePointTurretPosition())
					&& INFO.getCompletedCount(Terran_Factory, S) > 5 && S->supplyUsed() >= 100)
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);


			if (canBuildMissileTurret((TilePosition)MYBASE)
					&& INFO.getCompletedCount(Terran_Factory, S) > 5 && S->supplyUsed() >= 100)
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);

	
			if (TrainManager::Instance().getAvailableMinerals() > 800) {
				if (CM.getConstructionQueueItemCount(Terran_Missile_Turret)
						+ BM.buildQueue.getItemCount(Terran_Missile_Turret)
						+ INFO.getTypeBuildingsInArea(Terran_Missile_Turret, S, MYBASE).size() < 6) {

					BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, false);

				}
			}
		}
		else if (EMB == Toss_arbiter || EMB == Toss_arbiter_carrier) {
			buildMissileTurrets(INFO.getCompletedCount(Terran_Command_Center, S) >= 3 ? 2 : 1);
		}
		else {

			
			if (canBuildMissileTurret(getSecondChokePointTurretPosition())
					&& INFO.getCompletedCount(Terran_Factory, S) > 1 && (S->supplyUsed() >= 60 || INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) >= 3))
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);


			if (canBuildMissileTurret((TilePosition)MYBASE)
					&& INFO.getCompletedCount(Terran_Factory, S) > 1 && (S->supplyUsed() >= 70 || INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) >= 3))
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
		}

	
		if (EIB != Toss_cannon_rush && (EMB == UnknownMainBuild || EMB == Toss_dark) && !HasEnemyFirstExpansion()) {
			
			if (TIME > 24 * 240 && isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode)
					&& INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 1 && INFO.getAllCount(Terran_Engineering_Bay, S) > 0)
				BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, true);
		}
		else if (isExistOrUnderConstruction(Terran_Siege_Tank_Tank_Mode) && isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode)) {
			BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, true);
		}

		if (isResearchingOrResearched(TechTypes::Tank_Siege_Mode) && canBuildFirstTurret(60))
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);

		if (INFO.getCompletedCount(Terran_Vulture, S) >= 1 && isResearchingOrResearched(TechTypes::Tank_Siege_Mode) && isTimeToMachineShopUpgrade(TechTypes::Spider_Mines)) {
			BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines, false);
		}

		if (EMB >= Toss_fast_carrier && EMB != UnknownMainBuild)
		{
			if (INFO.getCompletedCount(Terran_Goliath, S) >= 1 && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Charon_Boosters))
				BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Charon_Boosters, false);
		}

		
		if (isResearchingOrResearched(TechTypes::Tank_Siege_Mode) && INFO.getAllCount(Terran_Factory, S) >= 2
				&& canBuildFirstAcademy(70)) {
			BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);
		}

		
		if (EMB == Toss_Scout || EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier || EMB == Toss_arbiter) {
			if (canBuildFirstArmory(0))
				BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
		}
		else if (INFO.getAllCount(Terran_Academy, S) > 0) {
			int amoryTiming = 70;

			if (EMB == Toss_dark || EMB == Toss_1base_fast_zealot) {
				amoryTiming = 80;
			}

			if (canBuildFirstArmory(amoryTiming))
				BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
		}

		if (canBuildSecondArmory(0))
			BM.buildQueue.queueAsHighestPriority(Terran_Armory, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);

		if (!S->isUpgrading(UpgradeTypes::Terran_Vehicle_Weapons) || !S->isUpgrading(UpgradeTypes::Terran_Vehicle_Plating)) {
			
			UpgradeType highPriority = UpgradeTypes::Terran_Vehicle_Weapons;
			UpgradeType lowPriority = UpgradeTypes::Terran_Vehicle_Plating;
			UpgradeType upgradeType = S->isUpgrading(highPriority) ? lowPriority : S->isUpgrading(lowPriority) ? highPriority : S->getUpgradeLevel(highPriority) > S->getUpgradeLevel(lowPriority) ? lowPriority : highPriority;

			
			if (isTimeToArmoryUpgrade(upgradeType, S->getUpgradeLevel(upgradeType) + 1))
			{
				BM.buildQueue.queueAsHighestPriority(upgradeType, false);
			}
		}

	
		if (canBuildRefinery(80, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)
				&& (INFO.getAllCount(Terran_Factory, S) >= 2 && INFO.getAllCount(Terran_Academy, S) && INFO.getAllCount(Terran_Armory, S) || S->minerals() > 400)) {
			TilePosition refineryPos = BuildingPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

			if (refineryPos.isValid())
				BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
		}

		if (INFO.getCompletedCount(Terran_Vulture, S) >= 6 &&
				isResearchingOrResearched(TechTypes::Tank_Siege_Mode) &&
				isResearchingOrResearched(TechTypes::Spider_Mines)
				&& isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters))
			BM.buildQueue.queueAsLowestPriority(UpgradeTypes::Ion_Thrusters);

	
		if (EMB == Toss_arbiter)
		{
			if (canBuildFirstStarport(60)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Starport);
			}
		}
		else
		{
			if (canBuildFirstStarport(80) && S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) > 0) {
				BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);
			}
		}

	
		if (EMB == Toss_arbiter || EMB == Toss_arbiter_carrier)
		{
			if (canBuildFirstScienceFacility(0)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Science_Facility);
			}
		}
		else
		{
			if (canBuildFirstScienceFacility(80)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Science_Facility, false);
			}
		}

	
		if (isTimeToScienceFacilityUpgrade(TechTypes::EMP_Shockwave) && INFO.getAllCount(Terran_Science_Vessel, S) > 0) {
			BM.buildQueue.queueAsHighestPriority(TechTypes::EMP_Shockwave, false);
		}

		if (canBuildadditionalFactory()) {
			BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
		}

		if (canBuildSecondExpansion(100) && SM.getNeedSecondExpansion())
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
		}

		if (canBuildThirdExpansion(100) && SM.getNeedThirdExpansion())
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());
		}

	}
	else {
		if (canBuildFirstBarrack(20)) {
			BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
		}

		if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation)) {
			TilePosition refineryPos = BuildingPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

			if (refineryPos.isValid())
				BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
		}

		if (EIB <= Zerg_5_Drone)
		{
			if (canBuildFirstBunker(0))
				BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
		}
		else if (EIB <= Zerg_9_Balup || EMB == Zerg_main_zergling)
		{
			int baseVultureCount = 2;

			if (E->getUpgradeLevel(UpgradeTypes::Metabolic_Boost) || E->isUpgrading(UpgradeTypes::Metabolic_Boost))
				baseVultureCount = 4;

			if (canBuildSecondChokePointBunker(38) && (isMyCommandAtFirstExpansion() || getEnemyInMyYard(1000, Zerg_Zergling).empty())
					&& (INFO.getCompletedCount(Terran_Vulture, S) >= baseVultureCount || INFO.getDestroyedCount(Zerg_Zergling, E) >= 10))
			{
				BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
			}
		}
		else
		{
			if (canBuildSecondChokePointBunker(38)
					&& (INFO.getCompletedCount(Terran_Vulture, S) > 0 || INFO.getCompletedCount(Terran_Goliath, S) > 0 || INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 0))
			{
				BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
			}
		}

		if (canBuildFirstFactory(32)) {
			if (!(EIB == Zerg_4_Drone && INFO.getAllCount(Terran_Bunker, S) == 0))
				BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
		}

		if (EMB == Zerg_main_lurker || INFO.getAllCount(Zerg_Lurker, E))
		{
			if (canBuildFirstEngineeringBay(0)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay);
			}

			if (canBuildFirstBunker(0)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint);
			}


			int turretCount = StrategyManager::Instance().getMainStrategy() == AttackAll || INFO.getAllCount(Terran_Science_Vessel, S) ? 1 : 2;

			if (canBuildMissileTurret(getSecondChokePointTurretPosition(), turretCount) && INFO.getAllCount(Terran_Bunker, S) > 0) {
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::SecondChokePoint);
			}

			if (isTimeToMachineShopUpgrade(TechTypes::Spider_Mines)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines, false);
			}
		}

		if (EIB <= Zerg_9_Balup || EMB == Zerg_main_zergling)
		{
			int baseVultureCount = 2;

			if (E->getUpgradeLevel(UpgradeTypes::Metabolic_Boost) || E->isUpgrading(UpgradeTypes::Metabolic_Boost))
				baseVultureCount = 4;

			if ((INFO.getCompletedCount(Terran_Vulture, S) >= baseVultureCount || INFO.getDestroyedCount(Zerg_Zergling, E) >= 10 || S->minerals() > 500)
					&& canBuildFirstExpansion(42))
				
				BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, getSecondCommandPosition(true), false);
		}
		else if (EMB == Zerg_main_lurker) 
		{
		
			if (canBuildFirstExpansion(42) && (S->hasResearched(TechTypes::Spider_Mines) || S->isResearching(TechTypes::Spider_Mines))
					&& INFO.getAllCount(Terran_Engineering_Bay, S) > 0) {
				BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
			}
		}
		else
		{
			if (INFO.getAllCount(Terran_Vulture, S) && canBuildFirstExpansion(42)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
			}
		}

		int completeTime = INT_MAX;

	
		if (EMB >= Zerg_main_maybe_mutal && EMB <= Zerg_main_hydra_mutal)
		{
			if (canBuildFirstEngineeringBay(0)) {
				
				if (HasEnemyFirstExpansion()) {

					uList spires = INFO.getBuildings(Zerg_Spire, E);

					if (INFO.getAllCount(Zerg_Lurker, E) + INFO.getAllCount(Zerg_Lurker_Egg, E) + INFO.getAllCount(Zerg_Mutalisk, E) > 0)
						completeTime = TIME;
					else if (spires.empty()) {
						uList lairs = INFO.getBuildings(Zerg_Lair, E);

					
						if (lairs.empty()) {
							uList hatcherys = INFO.getBuildings(Zerg_Hatchery, E);

							for (auto h : hatcherys) {
								if (h->isComplete() && completeTime > h->getLastPositionTime()) {
									completeTime = h->getLastPositionTime();
								}
							}

							completeTime += Zerg_Lair.buildTime();
						}
						
						else {
							for (auto l : lairs) {
								if (completeTime > l->getCompleteTime()) {
									completeTime = l->getCompleteTime();
								}
							}
						}

						completeTime += Zerg_Spire.buildTime() / 2;
					}
					
					else {
						for (auto s : spires) {
							if (completeTime > s->getCompleteTime()) {
								completeTime = s->getCompleteTime();
							}
						}

						completeTime -= Zerg_Spire.buildTime() / 2;
					}

					if (completeTime <= TIME)
						BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay);
				}
			
				else {
					BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay);
				}
			}
		}
		else
		{
			if (INFO.getAllCount(Terran_Vulture, S) >= 2 && INFO.getAllCount(Terran_Armory, S) && canBuildFirstEngineeringBay(0)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay);
			}
		}

		if ((EMB == Zerg_main_mutal
				|| EMB == Zerg_main_maybe_mutal
				|| EMB == Zerg_main_fast_mutal
				|| EMB == Zerg_main_hydra_mutal)
				&& INFO.getCompletedCount(Terran_Engineering_Bay, S) > 0)
		{
			if (completeTime != INT_MAX && completeTime <= TIME)
			{
				if (TIME % (3 * 24) == 0)
				{
					buildMissileTurrets();
				}
			}
			else
			{
				if (TIME % (10 * 24) == 0)
				{
					buildMissileTurrets();
				}
			}
		}
		else if (EMB == UnknownMainBuild)
		{
			
			if (INFO.getCompletedCount(Terran_Engineering_Bay, S) > 0)
			{
				int enemyHatcheryCount = INFO.getAllCount(Zerg_Hatchery, E);

				
				if (enemyHatcheryCount < 3)
				{
					if (TIME % (5 * 24) == 0)
					{
						buildMissileTurrets();
					}
				}
				else
				{
					
					if (TIME >= (24 * 60 * 6 + 10))
					{
						if (TIME % (10 * 24) == 0)
						{
							buildMissileTurrets();
						}
					}
				}
			}
		}

	
		if (!hasSecondCommand()) {
			return;
		}

	
		if (EMB == Zerg_main_zergling)
		{
	
			if (S->getUpgradeLevel(UpgradeTypes::Ion_Thrusters) == 1 && isTimeToMachineShopUpgrade(TechTypes::Spider_Mines)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines, false);
			}
		}
		else if (EMB == Zerg_main_hydra)
		{
		
			if (S->hasResearched(TechTypes::Tank_Siege_Mode) == true && isTimeToMachineShopUpgrade(TechTypes::Spider_Mines)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines, false);
			}
		}
		else
		{
			
			if (INFO.getAllCount(Terran_Engineering_Bay, S) && isTimeToMachineShopUpgrade(TechTypes::Spider_Mines)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines, false);
			}
		}

		
		if (EMB == Zerg_main_zergling)
		{
			
			if (S->getUpgradeLevel(UpgradeTypes::Ion_Thrusters) == 0 && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters))
				BM.buildQueue.queueAsLowestPriority(UpgradeTypes::Ion_Thrusters, false);
		}
		else
		{
			
			if (S->hasResearched(TechTypes::Tank_Siege_Mode) &&
					S->hasResearched(TechTypes::Spider_Mines) &&
					S->getUpgradeLevel(UpgradeTypes::Charon_Boosters) == 1 &&
					isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters)) {
				BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Ion_Thrusters, false);
			}
		}

	
		if (EMB == Zerg_main_hydra
				|| EMB == Zerg_main_hydra_mutal)
		{
			if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0 && isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, false);
			}
		}
		else if (EMB == Zerg_main_lurker)
		{
			if (S->getUpgradeLevel(UpgradeTypes::Charon_Boosters) && INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0
					&& isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, false);
			}
		}
		else
		{
			if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0
					&& INFO.getCompletedCount(Terran_Goliath, S) >= 8
					&& isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, false);
			}
		}

		
		if (EMB >= Zerg_main_mutal && EMB <= Zerg_main_hydra_mutal)
		{
			if (S->getUpgradeLevel(UpgradeTypes::Charon_Boosters) == 0 && INFO.getCompletedCount(Terran_Armory, S) > 0)
			{
				if (isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Charon_Boosters))
				{
					BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Charon_Boosters, false);

					uList machineShopList = INFO.getBuildings(Terran_Machine_Shop, S);

				
					bool canUpgrade = false;

					for (auto m : machineShopList) {
						if (!m->unit()->isUpgrading() && m->unit()->canUpgrade()) {
							canUpgrade = true;
							break;
						}
					}

					if (!canUpgrade)
						machineShopList.front()->unit()->cancelUpgrade();
				}
			}
		}
		else
		{
			if (INFO.getCompletedCount(Terran_Goliath, S) >= 1 && S->hasResearched(TechTypes::Spider_Mines) && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Charon_Boosters)) {
				BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Charon_Boosters, false);
			}
		}

		if (canBuildadditionalFactory()) {
			BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
		}

	
		if (EMB == Zerg_main_mutal
				|| EMB == Zerg_main_fast_mutal
				|| EMB == Zerg_main_hydra_mutal)
		{
			if (canBuildFirstArmory(0) && INFO.getAllCount(Terran_Factory, S) >= 1) {
				BM.buildQueue.queueAsHighestPriority(Terran_Armory);
			}
		}
		else
		{
			if (canBuildFirstArmory(50) && INFO.getAllCount(Terran_Factory, S) >= 1
					&& (INFO.getAllCount(Terran_Vulture, S) + INFO.getDestroyedCount(Terran_Vulture, S))) {
				BM.buildQueue.queueAsHighestPriority(Terran_Armory);
			}
		}

		if (canBuildSecondArmory(0))
			BM.buildQueue.queueAsHighestPriority(Terran_Armory, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);

		bool doUpgrade = true;

		if (EMB == Zerg_main_fast_mutal)
		{
			uList goliathList = INFO.getUnits(Terran_Goliath, S);

			if (goliathList.size() < 6 && !notCancel)
			{
				uList amoryList = INFO.getBuildings(Terran_Armory, S);

				for (auto a : amoryList)
				{
					if (a->unit()->isUpgrading())
					{
						a->unit()->cancelUpgrade();
					}
				}

				doUpgrade = false;
			}
			else
			{
				notCancel = true;
			}
		}

	
		if (doUpgrade && (!S->isUpgrading(UpgradeTypes::Terran_Vehicle_Weapons) || !S->isUpgrading(UpgradeTypes::Terran_Vehicle_Plating))) {
			
			UpgradeType highPriority = UpgradeTypes::Terran_Vehicle_Weapons;
			UpgradeType lowPriority = UpgradeTypes::Terran_Vehicle_Plating;
			UpgradeType upgradeType = S->isUpgrading(highPriority) ? lowPriority : S->isUpgrading(lowPriority) ? highPriority : S->getUpgradeLevel(highPriority) > S->getUpgradeLevel(lowPriority) ? lowPriority : highPriority;

			if (isTimeToArmoryUpgrade(upgradeType, S->getUpgradeLevel(upgradeType) + 1) && INFO.getBuildings(Terran_Factory, S).size() > 1)
			{
				BM.buildQueue.queueAsHighestPriority(upgradeType, false);
			}
		}

		
		if (canBuildRefinery(80, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)) {
			TilePosition refineryPos = BuildingPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

			if (refineryPos.isValid())
				BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
		}

	
		if (INFO.getAllCount(Terran_Factory, S) >= 2)
		{
			if (EMB == Zerg_main_lurker)
			{
				if (canBuildFirstAcademy(0))
					BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);
			}
			else
			{
				if (canBuildFirstAcademy(100))
					BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);
			}
		}

	
		if (EMB == Zerg_main_lurker || (INFO.getCompletedCount(Zerg_Lurker, E) + INFO.getDestroyedCount(Zerg_Lurker, E)) >= 4)
		{
			if (canBuildFirstStarport(0) && INFO.getCompletedCount(Terran_Factory, S) >= 3) {
				BM.buildQueue.queueAsHighestPriority(Terran_Starport);
			}
		}
		else
		{
			if (canBuildFirstStarport(80) && S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) > 0) {
				BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);
			}
		}

	
		if (EMB == Zerg_main_lurker || (INFO.getCompletedCount(Zerg_Lurker, E) + INFO.getDestroyedCount(Zerg_Lurker, E)) >= 4)
		{
			if (canBuildFirstScienceFacility(0)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Science_Facility);
			}
		}
		else
		{
			if (canBuildFirstScienceFacility(80)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Science_Facility, false);
			}
		}

	
		if (isTimeToScienceFacilityUpgrade(TechTypes::Irradiate) && INFO.getAllCount(Terran_Science_Vessel, S) > 0) {
			BM.buildQueue.queueAsHighestPriority(TechTypes::Irradiate, false);
		}

		if (canBuildSecondExpansion(100) && SM.getNeedSecondExpansion())
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
			
		}

		if (canBuildThirdExpansion(100) && SM.getNeedThirdExpansion())
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());
			
		}
	}


	if (SM.getMainStrategy() == Eliminate) {
		if (canBuildFirstStarport(48)) {
			BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);
		}
	}


	if (SM.needAdditionalExpansion() && canBuildAdditionalExpansion(0))
	{
		Base *multiBase = nullptr;

		for (int i = 1; i < 4; i++)
		{
			multiBase = INFO.getFirstMulti(S, false, false, i);

			if (preBase == nullptr)
				break;

			if (multiBase == nullptr)
				break;

			if (preBase->getPosition() != multiBase->getPosition())
				break;
		}

		if (multiBase != nullptr)
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, multiBase->getTilePosition());
			preBase = multiBase;
			
		}
	}

	
	if (canBuildRefinery(0, BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation)) {
		TilePosition refineryPos = BuildingPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	
	if (canBuildRefinery(0, BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation)) {
		TilePosition refineryPos = BuildingPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	
	if (!INFO.getAdditionalExpansions().empty())
	{
		for (auto base : INFO.getAdditionalExpansions())
		{
			if (canBuildRefinery(0, base) && S->minerals() > 1000)
			{
				TilePosition refineryPos = BuildingPlaceFinder::Instance().getRefineryPositionNear(base);

				if (refineryPos.isValid())
					BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
			}
		}
	}

}

TilePosition BasicBuildStrategy::getSecondChokePointTurretPosition() {
	Unit bunkerUnit = nullptr;

	for (auto bunker : INFO.getBuildings(Terran_Bunker, S)) {
		if (isSameArea(bunker->unit()->getTilePosition(), INFO.getFirstExpansionLocation(S)->getTilePosition())) {
			bunkerUnit = bunker->unit();
		}
	}

	TilePosition mySecondChokePointBunker = TilePositions::None;

	
	if (bunkerUnit == nullptr)
	{
		mySecondChokePointBunker = (TilePosition)(INFO.getSecondAverageChokePosition(S) - ((Position)Terran_Bunker.tileSize() / 2));
	}
	else
	{
		mySecondChokePointBunker = bunkerUnit->getTilePosition();
	}

	return mySecondChokePointBunker;
}


void BasicBuildStrategy::cancelBunkerAtMainBase() {

	if (CM.existConstructionQueueItem(Terran_Bunker)) {
		const vector<ConstructionTask> *constructionQueue = CM.getConstructionQueue();

		for (const auto &b : *constructionQueue) {

			if (b.type == Terran_Bunker) {
				if (b.desiredPosition != TilePositions::None &&
						(isSameArea(b.desiredPosition, INFO.getMainBaseLocation(S)->getTilePosition())
						 || isSameArea(b.finalPosition, INFO.getMainBaseLocation(S)->getTilePosition()))) {
					CM.cancelConstructionTask(b);
					return;
				}
			}
		}
	}

	BM.buildQueue.removeQueue(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
	return;
}

void BasicBuildStrategy::cancelFirstExpansion()
{
	if (CM.existConstructionQueueItem(Terran_Command_Center)) {
		const vector<ConstructionTask> *constructionQueue = CM.getConstructionQueue();

		for (const auto &b : *constructionQueue) {

			if (b.type == Terran_Command_Center) {
				if (b.desiredPosition != TilePositions::None) {
					CM.cancelConstructionTask(b);
					return;
				}
			}
		}
	}

	BM.buildQueue.removeQueue(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
	BM.buildQueue.removeQueue(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, true);
	return;
}
void BasicBuildStrategy::cancelFirstMachineShop(InitialBuildType eib, MainBuildType emb)
{
	int baseVultureCount = TrainManager::Instance().getBaseVultureCount(eib, emb);

	if (INFO.getAllCount(Terran_Vulture, S) >= baseVultureCount)
		return;

	for (auto fac : INFO.getBuildings(Terran_Machine_Shop, S))
	{
		if (fac->unit()->canCancelConstruction())
			fac->unit()->cancelConstruction();
	}

	
	for (auto fac : INFO.getBuildings(Terran_Factory, S))
	{
		if (fac->unit()->isTraining() && fac->unit()->getTrainingQueue()[0] == Terran_Siege_Tank_Tank_Mode)
			fac->unit()->cancelTrain();
	}
}


void BasicBuildStrategy::buildTurretFromAtoB(Position a, Position c)
{

	UnitInfo *turretA = nullptr;
	int minDist = INT_MAX;

	for (auto t : INFO.getTypeBuildingsInArea(Terran_Missile_Turret, S, a)) {
		int tmpDist = t->pos().getApproxDistance(a);

		if (minDist > tmpDist) {
			minDist = tmpDist;
			turretA = t;
		}
	}

	if (turretA == nullptr || minDist > 9 * TILE_SIZE) {

		
		if (canBuildMissileTurret((TilePosition)a))
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, (TilePosition)a, true);

		return;
	}



	TilePosition newPosOfB = (TilePosition)c;
	int length = 1;

	while (true) {

		int dist = newPosOfB.getApproxDistance((TilePosition)turretA->pos());

	
		if (!bw->canBuildHere(newPosOfB, Terran_Missile_Turret) || dist > 7) {
			TilePosition tempB = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Missile_Turret, newPosOfB, false, (Position)newPosOfB);

			if (tempB != TilePositions::None)
				newPosOfB = tempB;
		}

		if (dist <= 7 && dist >= 5)
			break;

		newPosOfB = (TilePosition)((c * (50 - length) + turretA->pos() * (50 + length)) / 100);

		if (length == 49)
			break;

		length++;
	}

	UnitInfo *turretB = INFO.getClosestTypeUnit(S, (Position)newPosOfB, Terran_Missile_Turret, 5 * TILE_SIZE, false, false, false);

	if (turretB == nullptr) {

		if (newPosOfB != TilePositions::None && canBuildMissileTurret(newPosOfB)) {
			
			if (INFO.getFirstExpansionLocation(S) && INFO.getFirstExpansionLocation(S)->Center().getDistance((Position)newPosOfB) > 3 * TILE_SIZE)
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, newPosOfB, true);

		}

		return;
	}

	UnitInfo *turretC = INFO.getClosestTypeUnit(S, c, Terran_Missile_Turret, 5 * TILE_SIZE, false, false, false);

	if (turretC)
		return;

	TilePosition newPositionOfC = (TilePosition)c;
	length = 1;

	while (true) {

		int dist = newPositionOfC.getApproxDistance((TilePosition)turretB->pos());

	
		if (!bw->canBuildHere(newPositionOfC, Terran_Missile_Turret) || dist > 7 ) {
			TilePosition temp =
				BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Missile_Turret, newPositionOfC, false, (Position)newPositionOfC);

			if (temp != TilePositions::None)
				newPositionOfC = temp;
		}

		if (dist <= 7 && dist >= 5)
			break;

		newPositionOfC = (TilePosition)((c * (50 - length) + turretB->pos() * (50 + length)) / 100);

		if (length == 49)
			break;

		length++;
	}

	if (newPositionOfC != TilePositions::None && canBuildMissileTurret(newPositionOfC))
		BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, newPositionOfC);

	return;
}

void BasicBuildStrategy::onInitialBuildChange(InitialBuildType pre, InitialBuildType cur)
{
	cout << "Build Change Pre : " << pre << ", cur : " << cur << endl;

	if (cur == Toss_2g_zealot || cur == Toss_1g_forward || cur == Toss_2g_forward)
	{
		cancelFirstExpansion();
		cancelFirstMachineShop(cur, EMB);
	}
	else if (cur == Terran_2b_forward) {
		if (INFO.getCompletedCount(Terran_Marine, E) > 3 && INFO.getCompletedCount(Terran_Vulture, S) < 2) {
			cancelFirstExpansion();
			cancelFirstMachineShop(cur, EMB);
		}
	}
	else if (cur <= Zerg_5_Drone)
	{
	
		if (INFO.getAllCount(Terran_Bunker, S) == 0 && S->minerals() < Terran_Bunker.mineralPrice() + 100) {
			bool isCancelConstruction = false;
			uList factoryList = INFO.getBuildings(Terran_Factory, S);

			for (auto f : factoryList)
			{
				if (!f->isComplete()) {
					f->unit()->cancelConstruction();
					isCancelConstruction = true;
					break;
				}
			}

			if (!isCancelConstruction) {
				uList supplyDepotList = INFO.getBuildings(Terran_Supply_Depot, S);

				for (auto s : supplyDepotList)
				{
					if (!s->isComplete()) {
						s->unit()->cancelConstruction();
						break;
					}
				}
			}
		}
	}
	else if (cur <= Zerg_9_Balup)
	{
		cancelFirstExpansion();
		cancelFirstMachineShop(cur, EMB);
	}
	else
	{
		cancelBunkerAtMainBase();
	}
}

void BasicBuildStrategy::onMainBuildChange(MainBuildType pre, MainBuildType cur)
{
	cout << "MainBuild " << pre << " => " << cur << endl;

	if (cur == Zerg_main_zergling || (cur == Zerg_main_lurker && INFO.getAllCount(Zerg_Hatchery, E) < 2))
	{
		if (TIME < 24 * 60 * 4) {
			cancelFirstExpansion();
			cancelFirstMachineShop(EIB, cur);
		}
	}
}

void BasicBuildStrategy::TerranVulutreTankWraithBuild()
{

	if (canBuildFirstBarrack(20)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
	}

	if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation)) {
		TilePosition refineryPos = BuildingPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	if (canBuildFirstFactory(32)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}

	if (canBuildFirstExpansion(40)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
	}

	if (canBuildFirstBunker(40)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
	}


	if (!hasSecondCommand()) {
		return;
	}

	if (EIB == Terran_1fac_1star ||
			EIB == Terran_1fac_2star ||
			EIB == Terran_2fac_1star ||
			SM.getMainStrategy() == AttackAll)
	{
		if (canBuildFirstStarport(48)) {
			BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);
		}
	}

	if (canBuildadditionalFactory()) {
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}

	if (isTimeToMachineShopUpgrade(TechTypes::Spider_Mines))
	{
		BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines, false);
	}

	if (S->hasResearched(TechTypes::Spider_Mines) &&
			isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters))
	{
		BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Ion_Thrusters, false);
	}

	if (INFO.getAllCount(Terran_Starport, S) && INFO.getAllCount(Terran_Wraith, S) && isTimeToControlTowerUpgrade(TechTypes::Cloaking_Field)) {
		BM.buildQueue.queueAsHighestPriority(TechTypes::Cloaking_Field, false);
	}


	if (canBuildRefinery(100, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)) {
		TilePosition refineryPos = BuildingPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	if (canBuildFirstArmory(140)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}

	
	if (SM.getNeedTank() && isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode)) {
		BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, false);
	}

	if (isTimeToArmoryUpgrade(UpgradeTypes::Terran_Vehicle_Weapons)) {
		BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Terran_Vehicle_Weapons, false);
	}

	if (canBuildFirstAcademy(120)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);
	}
}

void BasicBuildStrategy::TerranTankGoliathBuild()
{
	
	if (canBuildFirstBarrack(20)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
	}

	if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation)) {
		TilePosition refineryPos = BuildingPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	if (canBuildFirstFactory(32)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}

	if (canBuildFirstExpansion(40)) {
		
		if (!(EIB == Terran_2b_forward && INFO.getCompletedCount(Terran_Marine, E) > 3 && (INFO.getCompletedCount(Terran_Vulture, S) < 2 || INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) < 1)))
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
	}

	
	if (canBuildFirstBunker(0) && !INFO.getCompletedCount(Terran_Vulture, S) && INFO.getCompletedCount(Terran_Marine, E) >= 3) {
		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
	}

	else if (canBuildFirstBunker(0) && INFO.getCompletedCount(Terran_Barracks, E) >= 2
			 && (ESM.getEnemyInitialBuild() == Terran_bunker_rush || ESM.getEnemyInitialBuild() == Terran_2b_forward)) {
		
		bool isEnemyBarracksNear = false;
		int dist1 = getGroundDistance(MYBASE, INFO.getSecondChokePosition(S)) + 5 * TILE_SIZE;

		for (auto b : INFO.getBuildings(Terran_Barracks, E)) {
			int dist2 = getGroundDistance(MYBASE, b->pos());

			if (dist1 > dist2) {
				isEnemyBarracksNear = true;
				break;
			}
		}

		if (isEnemyBarracksNear)
			BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::FirstChokePoint, false);
		else
			BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
	}
	else if (canBuildFirstBunker(40)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
	}

	
	if (!hasSecondCommand()) {
		return;
	}

	
	if (canBuildRefinery(70, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)) {
		TilePosition refineryPos = BuildingPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}


	if (canBuildadditionalFactory()) {
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}


	if (canBuildFirstAcademy(50) && INFO.getAllCount(Terran_Factory, S) >= 2) {
		BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);
	}


	if (canBuildFirstArmory(50) && INFO.getAllCount(Terran_Factory, S) >= 2) {
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}

	
	if (INFO.getCompletedCount(Terran_Machine_Shop, S) >= 2
			&& (S->hasResearched(TechTypes::Tank_Siege_Mode) || S->isResearching(TechTypes::Tank_Siege_Mode))) {
		if (INFO.getAllCount(Terran_Wraith, E) > 0)
		{
			if (canBuildFirstEngineeringBay(60))
				BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);
		}
		else
		{
			if (canBuildFirstEngineeringBay(100))
				BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);
		}
	}


	if (S->hasResearched(TechTypes::Tank_Siege_Mode) || S->isResearching(TechTypes::Tank_Siege_Mode)) {
		if (INFO.getAllCount(Terran_Command_Center, S) >= 3)
		{
			if (canBuildFirstStarport(60))
				BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);
		}
		else
		{
			if ((canBuildFirstStarport(240) && INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 12)
					&& INFO.getCompletedCount(Terran_Factory, S) >= 4)
				BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);
		}
	}


	if (canBuildFirstScienceFacility(80)) {
		UnitInfo *armory = INFO.getClosestTypeUnit(S, MYBASE, Terran_Armory);

	
		if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) ||
				(armory != nullptr && armory->unit()->isUpgrading()
				 && (armory->unit()->getRemainingUpgradeTime() * 100) / UpgradeTypes::Terran_Vehicle_Weapons.upgradeTime() < 50))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Science_Facility, false);
		}
	}


	if (isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode)
			&& INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) >= 2
			&& INFO.getAllCount(Terran_Goliath, S) >= 4) {
		BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, false);
	}


	if (INFO.getCompletedCount(Terran_Goliath, S) >= 1 && S->hasResearched(TechTypes::Tank_Siege_Mode) && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Charon_Boosters)) {
		if (INFO.getAllCount(Terran_Wraith, E) > 0 || INFO.getAllCount(Terran_Dropship, E) > 0 || INFO.getAllCount(Terran_Battlecruiser, E) > 0)
			BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Charon_Boosters, false);
		else if (INFO.getAllCount(Terran_Factory, S) >= 4)
			BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Charon_Boosters, false);
	}

	if (isTimeToArmoryUpgrade(UpgradeTypes::Terran_Vehicle_Weapons)) {
		if (INFO.getAllCount(Terran_Command_Center, S) >= 3)
			BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Terran_Vehicle_Weapons, false);
		else if (INFO.getCompletedCount(Terran_Factory, S) >= 4 && INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 12)
			BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Terran_Vehicle_Weapons, false);
	}

	if (isTimeToArmoryUpgrade(UpgradeTypes::Terran_Vehicle_Weapons, 2))
	{
		BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Terran_Vehicle_Weapons, false);
	}

	if (isTimeToArmoryUpgrade(UpgradeTypes::Terran_Vehicle_Weapons, 3))
	{
		BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Terran_Vehicle_Weapons, false);
	}

	if (INFO.getCompletedCount(Terran_Engineering_Bay, S) > 0)
	{
		if (TIME % (10 * 24) == 0)
		{
			buildMissileTurrets(1);
		}
	}


	if (canBuildSecondExpansion(100) && SM.getNeedSecondExpansion() && INFO.getSecondExpansionLocation(S)
			&& (SM.getMainStrategy() == DrawLine || SM.getMainStrategy() == WaitLine || SM.getMainStrategy() == AttackAll))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
	}

	
	if (canBuildThirdExpansion(100) && SM.getNeedThirdExpansion() && INFO.getThirdExpansionLocation(S)
			&& (SM.getMainStrategy() == DrawLine || SM.getMainStrategy() == WaitLine || SM.getMainStrategy() == AttackAll))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());
	}

	if ((S->minerals() > 800 && INFO.getAllCount(Terran_Factory, S) >= 4) || (INFO.getAllCount(Terran_Command_Center, S) >= 3 && S->minerals() > 400))
	{
		
		if (isTimeToMachineShopUpgrade(TechTypes::Spider_Mines)) {
			BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines, false);
		}

		
		if (isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters)) {
			BM.buildQueue.queueAsLowestPriority(UpgradeTypes::Ion_Thrusters, false);
		}
	}
}