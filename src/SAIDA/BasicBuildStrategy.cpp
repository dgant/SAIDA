#include "BasicBuildStrategy.h"
#include "EnemyStrategyManager.h"
#include "TrainManager.h"

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

		TilePosition tp = ConstructionPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Command_Center, (TilePosition)secondCommandPosition, false, MYBASE);

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
			tempPosition = ConstructionPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Command_Center, (TilePosition)p, false, MYBASE);

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

		//y 축으로 위 아래로 조금 조정해봄
		if (!bw->canBuildHere((TilePosition)p, Terran_Missile_Turret)) {

			Position p1 = Position(p.x, p.y + 50);

			if (bw->canBuildHere((TilePosition)p1, Terran_Missile_Turret))
				p = p1;

			Position p2 = Position(p.x, p.y - 50);

			if (bw->canBuildHere((TilePosition)p2, Terran_Missile_Turret))
				p = p2;

		}

		// 못짓는 위치라면 spiral 하게 다시 찾는다.
		if (!bw->canBuildHere((TilePosition)p, Terran_Missile_Turret)) {
			tempPosition = ConstructionPlaceFinder::Instance().getBuildLocationBySpiralSearch
						   (Terran_Missile_Turret, (TilePosition)p, false, MYBASE);

			tempDist = (int)firstChoke.getDistance((Position)tempPosition);
		} else {
			tempPosition = (TilePosition)p;
		}

		// 평지맵에서 터렛이 입구를 막는다..ㅠ
		if (tempDist < dist && (INFO.getAltitudeDifference() > 0 || tempDist > 3 * TILE_SIZE)) {

			if (tempPosition != TilePositions::None) {
				turretposOnTheHill = tempPosition;
				dist = tempDist;
			}
		}
	}

	// 안드로메다 때문에 넣는 코드.
	if (dist > 6 * TILE_SIZE) {
		turretposOnTheHill = ConstructionPlaceFinder::Instance().getBuildLocationBySpiralSearch(
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
	// 현재 존재하거나 짓는 중인가?
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
		{	// 해당 위치에 Refinery가 있는가?
			if (hasRefinery(seedPositionStrategy))
				return false;

			if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation) {
				if (INFO.getFirstExpansionLocation(S) == nullptr)
					return false;

				// 만약 앞마당 이라면미네랄 개수만큼 앞마당 SCV가 있는지 체크
				uList commandList = INFO.getBuildings(Terran_Command_Center, S);

				for (auto center : commandList)
				{
					if (!center->isComplete())
						continue;

					if (!isSameArea(center->pos(), INFO.getFirstExpansionLocation(S)->Center()))
						continue;

					// 미네랄 숫자보다 일꾼수가 많아지면 가스 건설
					if (ScvManager::Instance().getDepotMineralSize(center->unit())
							<= ScvManager::Instance().getAssignedScvCount(center->unit()))
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
			// 3번째 Center 부터는 이부분에 추가 필요
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
		{	// 해당 위치에 Refinery가 있는가?
			if (hasRefinery(base))
				return false;

			// 현재 가스 여유가 있으면 건설하지 않음
			if (S->gas() > 1000)
				return false;

			// 미네랄 개수만큼 SCV가 있는지 체크,
			uList commandList = INFO.getBuildings(Terran_Command_Center, S);

			for (auto center : commandList)
			{
				if (!center->isComplete())
					continue;

				if (!isSameArea(center->pos(), base->Center()))
					continue;

				// 미네랄 숫자보다 일꾼수가 많아지면 가스 건설
				if (ScvManager::Instance().getDepotMineralSize(center->unit())
						<= ScvManager::Instance().getAssignedScvCount(center->unit()))
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

	// 해당 위치에 가스가 지어져있지 않으면 없는 것으로 간주. 가스가 지어졌다가 부서지면 갱신이 안돼서 아래와 같이 코딩함.
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

	// 해당 위치에 가스가 지어져있지 않으면 없는 것으로 간주. 가스가 지어졌다가 부서지면 갱신이 안돼서 아래와 같이 코딩함.
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

	// 상대가 빠른 질럿 러쉬라면 머신샵을 빠르게 갈 필요가 없다.
	if (INFO.enemyRace == Races::Protoss && EIB <= Toss_2g_forward) {
		machineShopFirstFlag = false;
	}

	int maxCount = 0;

	if (INFO.enemyRace == Races::Zerg)
		maxCount = 5;
	else if (INFO.enemyRace == Races::Protoss) {
		// 앞마당에 커맨드 없으면 2로!!
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

			// 테란전 후반에 자원 많이 남을 경우 팩토리 추가로 건설해놓자
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

		// 다크인 경우
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

		// 2팩에서 아카 아머리 건설 후 추가 팩토리 건설
		if (INFO.getAllCount(Terran_Factory, S) == 2
				&& !(INFO.getCompletedCount(Terran_Academy, S) > 0 && INFO.getCompletedCount(Terran_Armory, S) > 0))
			return false;
	}

	if (!hasEnoughResourcesToBuild(Terran_Factory)) {
		return false;
	}

	// 기존 지어진 팩토리들에서 병력들이 모두 생산중인 경우만
	int canTrainTankCnt = 0;
	int canTrainGoliathCnt = 0;

	for (auto f : INFO.getBuildings(Terran_Factory, S)) {
		if (!f->isComplete())
			continue;

		// addon 이 있는 경우
		if (f->unit()->getAddon()) {
			if (f->unit()->getAddon()->isCompleted()) {
				// 병력을 안뽑고 있거나 생산 곧 완료 되는 경우 병력 생산 가능
				if (f->unit()->getTrainingQueue().empty() || (f->unit()->isTraining() && f->unit()->getRemainingTrainTime() <= 130))
					canTrainTankCnt++;
			}
			// addon 이 곧 완성되는 경우 병력 생산 가능
			else if (!f->unit()->getAddon()->isCompleted() && f->unit()->getAddon()->getRemainingBuildTime() <= 130)
				canTrainTankCnt++;
		}
		// addon 이 없는 경우
		else {
			// 생산중이거나 업그레이드 중이 아닌 경우
			if (f->unit()->getTrainingQueue().empty())
				canTrainGoliathCnt++;
			// 생산 완료 전
			else if (f->unit()->isTraining() && f->unit()->getRemainingTrainTime() <= 130)
				canTrainGoliathCnt++;
		}
	}

	if (canTrainTankCnt > 0 || canTrainGoliathCnt > 0) {
		// 프로토스이고 캐리어 제외한 경우 벌쳐 그 외에는 골리앗
		UnitType combatUnit = INFO.enemyRace == Races::Protoss && EMB != Toss_fast_carrier ? Terran_Vulture : Terran_Goliath;

		double mineralRate = 1.0;
		double gasRate = 1.0;

		mineralRate -= 0.01 * ScvManager::Instance().getAllMineralScvCount();
		gasRate -= 0.02 * ScvManager::Instance().getAllRefineryScvCount();

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

	// Terran_Science_Facility 가 늦게 올라가기 때문에 아머리 하나로 업그레이드를 찍다가 올리도록 수정.
	if (!isExistOrUnderConstruction(Terran_Science_Facility))
		return false;

	// 업그레이드가 됐으면 짓는다.
	if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) >= 2 || S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Plating) >= 1)
		return true;

	// 업그레이드가 아머리 지을 시간 + 5초 남으면 짓는다.
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
	// 컨트롤타워가 없으면 return false;
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
	// 사이언스 퍼실러티가 없으면 return false;
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
	// 아머리가 없으면 return false;
	if (INFO.getCompletedCount(Terran_Armory, S) == 0) {
		return false;
	}

	// 2렙 이상 업그레이드는 Terran_Science_Facility 필요
	if (level > 1 && INFO.getCompletedCount(Terran_Science_Facility, S) == 0)
		return false;

	if (S->isUpgrading(upgradeType) == true
			|| BM.buildQueue.getItemCount(upgradeType) != 0
			|| S->getUpgradeLevel(upgradeType) >= level)
	{
		return false;
	}

	// canUpgrade 로 체크하면 자원까지 포함해서 체크하기 때문에 미리 큐에 넣기 위해 아래와 같이 체크함.
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

	// until_frameCount 까지는 SupplyUsed를 Flag로 해서 특정 로직 수행에 대한 타이밍을 판단한다.
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

		// 이미 취소되었었다면, 위치가 변경된 채로 큐에 다시 들어가 있을테니,
		// 한번더 취소하지 않는다.
		if (canceledBunker)
			return;

		bool cancelFlag = false;

		for (auto b : INFO.getBuildings(Terran_Bunker, S))
		{
			// 본진에 이미 지어진 벙커가 있으면 리턴
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
			// 돈이 없으면서 적 유닛이 가까이에 있으면 서플을 취소한다.

			// 서플 취소
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
	/*cout << "메인베이스 터렛 수:" << mainBaseTurretCount << endl;
	cout << "메인베이스 짓는 중인 터렛 수:" << getUnderConstructionCount(Terran_Missile_Turret, mainBase) << endl;*/

	if (getExistOrUnderConstructionCount(Terran_Missile_Turret, MYBASE) < count)
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation);
	}

	/*cout << "앞마당 터렛 수:" << firstExpansionTurretCount << endl;
	cout << "앞마당 짓는 중인 터렛 수:" << getUnderConstructionCount(Terran_Missile_Turret, firstExpansion) << endl;*/

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

	// 본진 커맨드 뿌개지면 다시 짓는 로직
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
		// 1배럭 + 1팩 + 앞마당 + 5 팩토리(골리앗) + 방1업 진출
		// 1배럭 + 1팩 + 앞마당 스타포트
		if (canBuildFirstBarrack(20)) {
			BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
		}

		if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation)) {
			TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

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
			// Factory 있으면 Vulture로 막는다.
			if (INFO.getCompletedCount(Terran_Factory, S) == 0)
				buildBunkerDefence();

			if ((INFO.getCompletedCount(Terran_Vulture, S) >= baseVultureCount || INFO.getDestroyedCount(Protoss_Zealot, E) >= baseZealotCount)
					&& canBuildFirstExpansion(42))
				BM.buildQueue.queueAsLowestPriority(Terran_Command_Center, getSecondCommandPosition(), false);
			else if (S->minerals() >= 500 && canBuildFirstExpansion(42))
				BM.buildQueue.queueAsLowestPriority(Terran_Command_Center, getSecondCommandPosition(true), false);

			// 벌쳐 속업 먼저
			/*if (INFO.getCompletedCount(Terran_Factory, S) >= 2 && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters))
				BM.buildQueue.queueAsLowestPriority(UpgradeTypes::Ion_Thrusters);
			*/
			// 벌쳐 마인 업
			if (INFO.getCompletedCount(Terran_Vulture, S) >= 1 && isTimeToMachineShopUpgrade(TechTypes::Spider_Mines))
				BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines);

		}
		else if (EIB == Toss_pure_double || EIB == Toss_forge)
		{
			if (canBuildFirstExpansion(42)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
			}

			// 역언덕 맵에서, 앞마당 짓고 벙커 짓기
			if (INFO.getAltitudeDifference() <= 0 && hasSecondCommand() && canBuildFirstBunker(42)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
			}

		}
		else if (EIB == Toss_cannon_rush) {
			uList cannons = INFO.getBuildings(Protoss_Photon_Cannon, E);
			bool isDestroyComplete = true;

			for (auto c : cannons) {
				// 본진이나 앞마당이나 앞마당 근처에 있는 경우.
				if (isSameArea(c->pos(), INFO.getMainBaseLocation(S)->Center())
						|| (INFO.getFirstExpansionLocation(S) &&
							(isSameArea(c->pos(), INFO.getFirstExpansionLocation(S)->Center())
							 || getAttackDistance(c->type(), c->pos(), Terran_Command_Center, INFO.getFirstExpansionLocation(S)->Center()) <= Protoss_Photon_Cannon.groundWeapon().maxRange() + 2 * TILE_SIZE))) {
					isDestroyComplete = false;
					break;
				}
			}

			// 우리 본진 + 초크포인트 + 포토 사거리 에 적 포토가 있는 경우 시즈모드 업 먼저 하고.
			if (!isDestroyComplete && isExistOrUnderConstruction(Terran_Siege_Tank_Tank_Mode) && isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, true);
			}

			if (canBuildFirstExpansion(42)) {
				// 커멘드는 포토캐논 다 부순 경우 멀티 위치에 짓는다.
				if (isDestroyComplete)
					BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
				// 포토가 있으면 탱크 있고 시즈업 시작했으면 짓는다.
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

			//if (canBuildFirstExpansion(42)) {
			//	BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, getSecondCommandPosition(), false);
			//}

			//// 앞마당에 건설 시에는 Bunker짓는다.
			//if (INFO.getFirstExpansionLocation(S) && getSecondCommandPosition() == INFO.getFirstExpansionLocation(S)->getTilePosition())
			//{
			//	if (canBuildFirstBunker(42)) {
			//		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, (TilePosition)(INFO.getSecondAverageChokePosition(S) - ((Position)Terran_Bunker.tileSize() / 2)), false);
			//	}
			//}
		}

		//만약 적이 앞마당을.. 점령했으면

		// 이후의 로직은 앞마당을 짓고나서 수행되도록
		if (!hasSecondCommand()) {
			return;
		}

		// 적 앞마당 있든 없든 벙커 짓자..
		if (canBuildSecondChokePointBunker(62)/* && !HasEnemyFirstExpansion()*/
				&& isMyCommandAtFirstExpansion()) {
			BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
		}

		// 엔지니어링 베이
		if (EMB == Toss_Scout || EMB == Toss_dark || EMB == Toss_dark_drop || EMB == Toss_drop || EMB == Toss_reaver_drop) {
			// 다크가 확실한 경우
			if (canBuildFirstEngineeringBay(0) && INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0) {
				int completeTime = INT_MAX;

				if (INFO.getAllCount(Protoss_Templar_Archives, E) + INFO.getAllCount(Protoss_Dark_Templar, E) + INFO.getAllCount(Protoss_Shuttle, E) + INFO.getAllCount(Protoss_Scout, E) == 0) {
					uList aduns = INFO.getBuildings(Protoss_Citadel_of_Adun, E);
					uList robotics = INFO.getBuildings(Protoss_Robotics_Facility, E);
					uList roboticsSupportBay = INFO.getBuildings(Protoss_Robotics_Support_Bay, E);

					// 아둔이 있는 경우(다크템플러)
					if (!aduns.empty()) {
						for (auto a : aduns) {
							if (completeTime > a->getCompleteTime())
								completeTime = a->getCompleteTime();
						}
					}
					// 로보틱스 서포트베이가 있는 경우 (리버드랍)
					else if (!roboticsSupportBay.empty()) {
						for (auto r : roboticsSupportBay) {
							if (completeTime > r->getCompleteTime())
								completeTime = r->getCompleteTime();
						}
					}
					// 로보틱스만 있는 경우
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
		// 적 빌드를 모른 상태에서 적 앞마당이 없이 4분이 지난 경우
		else if (EIB != Toss_cannon_rush && EMB == UnknownMainBuild && !HasEnemyFirstExpansion()) {
			// 원탱 찍고 엔베
			if (TIME > 24 * 240 && canBuildFirstEngineeringBay(0) && INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0)
				BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, true);
		}
		// 사업드라군 푸쉬 후 다크 공격 대비
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

				// 사업한 경우는 가스 캐는데 시간이 더 필요함. (720)
				if (EMB == Toss_range_up_dra_push) {
					// 사업 한 경우 드라군 최소 한기 뽑았다고 가정 (360)
					completeTime += 1080;
				}

				if (completeTime <= TIME)
					BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, true);
			}
		}
		else
		{
			// 시즈 생산, 시즈 모드 업 먼저
			if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) >= 2 && isResearchingOrResearched(TechTypes::Tank_Siege_Mode)
					&& canBuildFirstEngineeringBay(0))
				BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);
		}


		// [터렛]
		// 다크이거나 상대 앞마당이 없고 상대 전략을 파악하지 못했을 때.
		if (EMB == Toss_dark || (EIB != Toss_cannon_rush && EMB == UnknownMainBuild && !HasEnemyFirstExpansion() && TIME > 24 * 240))
		{
			// 앞마당이 있으면 앞마당 앞에
			// 이미 다크가 와버리면 그냥 언덕에서부터 차례로 지어서 내려가자..
			// 시간적으로 여유가 있는 경우 (템플러아카이브 건설 시간 + 다크 생산 + 오는 시간 + 터렛 지으러 가는 시간)
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
			// 본진 하나 앞마당 하나
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
			// 아무 전략이 없을때도 그냥 본진에 터렛 하나는 짓자
			if (canBuildMissileTurret(getSecondChokePointTurretPosition())
					&& INFO.getCompletedCount(Terran_Factory, S) > 5 && S->supplyUsed() >= 100)
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);


			if (canBuildMissileTurret((TilePosition)MYBASE)
					&& INFO.getCompletedCount(Terran_Factory, S) > 5 && S->supplyUsed() >= 100)
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);

			// 후반에 터렛 무작위로 짓기 로직
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

			// 아무 전략이 없을때도 그냥 본진에 터렛 하나는 짓자
			if (canBuildMissileTurret(getSecondChokePointTurretPosition())
					&& INFO.getCompletedCount(Terran_Factory, S) > 1 && (S->supplyUsed() >= 60 || INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) >= 3))
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);


			if (canBuildMissileTurret((TilePosition)MYBASE)
					&& INFO.getCompletedCount(Terran_Factory, S) > 1 && (S->supplyUsed() >= 70 || INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) >= 3))
				BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
		}

		// 시즈모드 업
		if (EIB != Toss_cannon_rush && (EMB == UnknownMainBuild || EMB == Toss_dark) && !HasEnemyFirstExpansion()) {
			// 적 빌드를 모른 상태에서 적 앞마당이 없이 4분이 지난 경우 탱-엔베-탱 찍고 시즈모드업
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

		// 아카데미
		if (isResearchingOrResearched(TechTypes::Tank_Siege_Mode) && INFO.getAllCount(Terran_Factory, S) >= 2
				&& canBuildFirstAcademy(70)) {
			BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);
		}

		// 아모리
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
			// 공업 먼저
			UpgradeType highPriority = UpgradeTypes::Terran_Vehicle_Weapons;
			UpgradeType lowPriority = UpgradeTypes::Terran_Vehicle_Plating;
			UpgradeType upgradeType = S->isUpgrading(highPriority) ? lowPriority : S->isUpgrading(lowPriority) ? highPriority : S->getUpgradeLevel(highPriority) > S->getUpgradeLevel(lowPriority) ? lowPriority : highPriority;

			// 메카닉 공방업
			if (isTimeToArmoryUpgrade(upgradeType, S->getUpgradeLevel(upgradeType) + 1))
			{
				BM.buildQueue.queueAsHighestPriority(upgradeType, false);
			}
		}

		// 두번째 가스
		if (canBuildRefinery(80, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)
				&& (INFO.getAllCount(Terran_Factory, S) >= 2 && INFO.getAllCount(Terran_Academy, S) && INFO.getAllCount(Terran_Armory, S) || S->minerals() > 400)) {
			TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

			if (refineryPos.isValid())
				BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
		}

		if (INFO.getCompletedCount(Terran_Vulture, S) >= 6 &&
				isResearchingOrResearched(TechTypes::Tank_Siege_Mode) &&
				isResearchingOrResearched(TechTypes::Spider_Mines)
				&& isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters))
			BM.buildQueue.queueAsLowestPriority(UpgradeTypes::Ion_Thrusters);

		// 스타포트
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

		// 사이언스 퍼실러티
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

		// EMP Shockwave 업그레이드
		if (isTimeToScienceFacilityUpgrade(TechTypes::EMP_Shockwave) && INFO.getAllCount(Terran_Science_Vessel, S) > 0) {
			BM.buildQueue.queueAsHighestPriority(TechTypes::EMP_Shockwave, false);
		}

		if (canBuildadditionalFactory()) {
			BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
		}

		if (canBuildSecondExpansion(100) && SM.getNeedSecondExpansion())
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
			cout << "2멀티 위치 : " << INFO.getSecondExpansionLocation(S)->getTilePosition() << endl;
		}

		if (canBuildThirdExpansion(100) && SM.getNeedThirdExpansion())
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());
			cout << "3멀티 위치 : " << INFO.getThirdExpansionLocation(S)->getTilePosition() << endl;
		}

		//// Factory 2개 완성되면 본진에 터렛 건설
		//if (canBuildMissileTurret(INFO.getMainBaseLocation(S)->getTilePosition()) && INFO.getCompletedCount(Terran_Factory, S) > 1)
		//	BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
	}
	// 저그 혹은 랜덤
	else {
		// 1배럭 + 1팩 + 앞마당 + 5팩(탱크) + 방1업 진출
		if (canBuildFirstBarrack(20)) {
			BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
		}

		if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation)) {
			TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

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
			// 4드론인 경우는 벙커 지어지기 시작한 후에 팩토리를 짓는다.
			if (!(EIB == Zerg_4_Drone && INFO.getAllCount(Terran_Bunker, S) == 0))
				BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
		}

		// 앞마당 건설 전에 상대가 럴커를 뽑는 초패스트 럴커 빌드인 경우, 히드라/뮤탈 이후 다시 럴커인 경우까지
		if (EMB == Zerg_main_lurker || INFO.getAllCount(Zerg_Lurker, E))
		{
			if (canBuildFirstEngineeringBay(0)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay);
			}

			if (canBuildFirstBunker(0)) {
				BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint);
			}

			// attack all 이거나 사이언스 베슬이 있는 경우에는 터렛 하나만 짓는다.
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
				//BM.buildQueue.queueAsLowestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
				BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, getSecondCommandPosition(true), false);
		}
		else if (EMB == Zerg_main_lurker) // 럴커는 Mine Up 하고 건설하자
		{
			// 마인업, 엔베까지 건설 후 커맨드 추가
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

		// 엔지니어링 베이
		if (EMB >= Zerg_main_maybe_mutal && EMB <= Zerg_main_hydra_mutal)
		{
			if (canBuildFirstEngineeringBay(0)) {
				// 멀티를 한 경우
				if (HasEnemyFirstExpansion()) {

					uList spires = INFO.getBuildings(Zerg_Spire, E);

					if (INFO.getAllCount(Zerg_Lurker, E) + INFO.getAllCount(Zerg_Lurker_Egg, E) + INFO.getAllCount(Zerg_Mutalisk, E) > 0)
						completeTime = TIME;
					else if (spires.empty()) {
						uList lairs = INFO.getBuildings(Zerg_Lair, E);

						// 레어가 없는 경우 가장 본지 오래된 해처리 기준으로 계산.
						if (lairs.empty()) {
							uList hatcherys = INFO.getBuildings(Zerg_Hatchery, E);

							for (auto h : hatcherys) {
								if (h->isComplete() && completeTime > h->getLastPositionTime()) {
									completeTime = h->getLastPositionTime();
								}
							}

							completeTime += Zerg_Lair.buildTime();
						}
						// 레어가 있는 경우 가장 빨리 지은 레어 기준.
						else {
							for (auto l : lairs) {
								if (completeTime > l->getCompleteTime()) {
									completeTime = l->getCompleteTime();
								}
							}
						}

						completeTime += Zerg_Spire.buildTime() / 2;
					}
					// 스파이어를 본 경우
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
				// 멀티를 안한 경우 바로 짓는다.
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

		// 멀티별 미사일 터렛 짓기
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
			// 정찰 실패해서 상대 빌드 분류 안되었을 때
			if (INFO.getCompletedCount(Terran_Engineering_Bay, S) > 0)
			{
				int enemyHatcheryCount = INFO.getAllCount(Zerg_Hatchery, E);

				// 2햇 미만 대비
				if (enemyHatcheryCount < 3)
				{
					if (TIME % (5 * 24) == 0)
					{
						buildMissileTurrets();
					}
				}
				else
				{
					// 3햇이면 6분 이후로 건설
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

		// 이후의 로직은 앞마당을 짓고나서 수행되도록
		if (!hasSecondCommand()) {
			return;
		}

		// 벌쳐 마인업
		if (EMB == Zerg_main_zergling)
		{
			// 벌쳐 속도업 후
			if (S->getUpgradeLevel(UpgradeTypes::Ion_Thrusters) == 1 && isTimeToMachineShopUpgrade(TechTypes::Spider_Mines)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines, false);
			}
		}
		else if (EMB == Zerg_main_hydra)
		{
			// 시즈모드업 후
			if (S->hasResearched(TechTypes::Tank_Siege_Mode) == true && isTimeToMachineShopUpgrade(TechTypes::Spider_Mines)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines, false);
			}
		}
		else
		{
			// 벌쳐 마인업
			if (INFO.getAllCount(Terran_Engineering_Bay, S) && isTimeToMachineShopUpgrade(TechTypes::Spider_Mines)) {
				BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines, false);
			}
		}

		// 벌쳐 속도업
		if (EMB == Zerg_main_zergling)
		{
			// 벌쳐 속업 먼저
			if (S->getUpgradeLevel(UpgradeTypes::Ion_Thrusters) == 0 && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters))
				BM.buildQueue.queueAsLowestPriority(UpgradeTypes::Ion_Thrusters, false);
		}
		else
		{
			// 저글링 이외 벌쳐 속업은 제일 마지막으로
			if (S->hasResearched(TechTypes::Tank_Siege_Mode) &&
					S->hasResearched(TechTypes::Spider_Mines) &&
					S->getUpgradeLevel(UpgradeTypes::Charon_Boosters) == 1 &&
					isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters)) {
				BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Ion_Thrusters, false);
			}
		}

		// 시즈모드업
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

		// 골리앗 사거리업, 벌쳐 마인업 후
		if (EMB >= Zerg_main_mutal && EMB <= Zerg_main_hydra_mutal)
		{
			if (S->getUpgradeLevel(UpgradeTypes::Charon_Boosters) == 0 && INFO.getCompletedCount(Terran_Armory, S) > 0)
			{
				if (isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Charon_Boosters))
				{
					BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Charon_Boosters, false);

					uList machineShopList = INFO.getBuildings(Terran_Machine_Shop, S);

					// 업그레이드 할 머신샵이 없는 경우 추가.
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

		// 아머리
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

		// 메카닉 공방업
		if (doUpgrade && (!S->isUpgrading(UpgradeTypes::Terran_Vehicle_Weapons) || !S->isUpgrading(UpgradeTypes::Terran_Vehicle_Plating))) {
			// 공업 먼저
			UpgradeType highPriority = UpgradeTypes::Terran_Vehicle_Weapons;
			UpgradeType lowPriority = UpgradeTypes::Terran_Vehicle_Plating;
			UpgradeType upgradeType = S->isUpgrading(highPriority) ? lowPriority : S->isUpgrading(lowPriority) ? highPriority : S->getUpgradeLevel(highPriority) > S->getUpgradeLevel(lowPriority) ? lowPriority : highPriority;

			// 메카닉 공방업
			if (isTimeToArmoryUpgrade(upgradeType, S->getUpgradeLevel(upgradeType) + 1) && INFO.getBuildings(Terran_Factory, S).size() > 1)
			{
				BM.buildQueue.queueAsHighestPriority(upgradeType, false);
			}
		}

		// 두번째 가스
		if (canBuildRefinery(80, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)) {
			TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

			if (refineryPos.isValid())
				BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
		}

		// 아카데미
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

		// 스타포트
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

		// 사이언스 퍼실러티
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

		// Irradiate 업그레이드
		if (isTimeToScienceFacilityUpgrade(TechTypes::Irradiate) && INFO.getAllCount(Terran_Science_Vessel, S) > 0) {
			BM.buildQueue.queueAsHighestPriority(TechTypes::Irradiate, false);
		}

		if (canBuildSecondExpansion(100) && SM.getNeedSecondExpansion())
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
			cout << "2멀티 위치 : " << INFO.getSecondExpansionLocation(S)->getTilePosition() << endl;
		}

		if (canBuildThirdExpansion(100) && SM.getNeedThirdExpansion())
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());
			cout << "3멀티 위치 : " << INFO.getThirdExpansionLocation(S)->getTilePosition() << endl;
		}
	}

	// 공통 체크 (엘리가 되면 공중유닛을 뽑는다.)
	if (SM.getMainStrategy() == Eliminate) {
		if (canBuildFirstStarport(48)) {
			BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);
		}
	}

	// 추가 멀티들..
	if (SM.needAdditionalExpansion() && canBuildAdditionalExpansion(0))
	{
		Base *multiBase = nullptr;

		// 이 전 타임에서 멀티 시도했던 지역인지 확인, 이전과 같다면 다음 랭크의 멀티로, 3번 확인
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
			cout << "추가 멀티 위치 : " << multiBase->getTilePosition() << endl;
		}
	}

	// 세번째 가스를 지을 수 있게 되면 가스를 짓는다.
	if (canBuildRefinery(0, BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation)) {
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	// 네번째 가스를 지을 수 있게 되면 가스를 짓는다.
	if (canBuildRefinery(0, BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation)) {
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	// 추가 멀티들 공통 가스 추가
	if (!INFO.getAdditionalExpansions().empty())
	{
		for (auto base : INFO.getAdditionalExpansions())
		{
			if (canBuildRefinery(0, base) && S->minerals() > 1000)
			{
				TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(base);

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

	// 벙커 없으면 SecondChokePoint 위치
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

	// 혹시나 이미 Add On이 달리고 Tank가 찍혔으면 취소한다.
	for (auto fac : INFO.getBuildings(Terran_Factory, S))
	{
		if (fac->unit()->isTraining() && fac->unit()->getTrainingQueue()[0] == Terran_Siege_Tank_Tank_Mode)
			fac->unit()->cancelTrain();
	}
}

/// 터렛 세개를 a - c에 이어서 짓는다.
void BasicBuildStrategy::buildTurretFromAtoB(Position a, Position c)
{
	/* Turret A */
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

		// 큐에 있는지, 근처에 이미 지은게 있는지 체크
		if (canBuildMissileTurret((TilePosition)a))
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, (TilePosition)a, true);

		return;
	}

	/* Turret B */
	// 터렛 A가 있는지 체크
	// turret A 와 c의 중간점에 터렛을 짓는다.

	TilePosition newPosOfB = (TilePosition)c;
	int length = 1;

	while (true) {

		int dist = newPosOfB.getApproxDistance((TilePosition)turretA->pos());

		// A하고 거리를 일정하게 유지.
		if (!bw->canBuildHere(newPosOfB, Terran_Missile_Turret) || dist > 7) {
			TilePosition tempB = ConstructionPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Missile_Turret, newPosOfB, false, (Position)newPosOfB);

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
			// 앞마당과 거리 잼
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

		// A하고 거리를 일정하게 유지.
		if (!bw->canBuildHere(newPositionOfC, Terran_Missile_Turret) || dist > 7 ) {
			TilePosition temp =
				ConstructionPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Missile_Turret, newPositionOfC, false, (Position)newPositionOfC);

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
		// 벙커를 짓고있지 않으면 서플라이 취소
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
	// 1배럭 + 1팩 + 앞마당 + 스타포트
	if (canBuildFirstBarrack(20)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
	}

	if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation)) {
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

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

	// 이후의 로직은 앞마당을 먹은 뒤에 수행되도록 체크
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

	// 두번째 가스
	if (canBuildRefinery(100, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)) {
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	if (canBuildFirstArmory(140)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}

	// 시즈모드업
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
	// 4팩 탱크 골리앗 드랍쉽 운영
	if (canBuildFirstBarrack(20)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
	}

	if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation)) {
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	if (canBuildFirstFactory(32)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}

	if (canBuildFirstExpansion(40)) {
		// 적이 2배럭 포워드이고 마린을 많이 뽑았는데 수비병력(벌쳐 or 탱크) 가 없으면 커멘드 센터를 짓지 않는다.
		if (!(EIB == Terran_2b_forward && INFO.getCompletedCount(Terran_Marine, E) > 3 && (INFO.getCompletedCount(Terran_Vulture, S) < 2 || INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) < 1)))
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
	}

	// 벌쳐 없는데 마린이 3마리 이상
	if (canBuildFirstBunker(0) && !INFO.getCompletedCount(Terran_Vulture, S) && INFO.getCompletedCount(Terran_Marine, E) >= 3) {
		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
	}
	// 적이 초반 러시인 경우
	else if (canBuildFirstBunker(0) && INFO.getCompletedCount(Terran_Barracks, E) >= 2
			 && (ESM.getEnemyInitialBuild() == Terran_bunker_rush || ESM.getEnemyInitialBuild() == Terran_2b_forward)) {
		// 적 배럭 위치에 따라 벙커 위치 정한다.
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

	// 이후의 로직은 앞마당을 먹은 뒤에 수행되도록 체크
	if (!hasSecondCommand()) {
		return;
	}

	// 두번째 가스
	if (canBuildRefinery(70, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)) {
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	// 팩토리 추가
	if (canBuildadditionalFactory()) {
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}

	// 아카데미
	if (canBuildFirstAcademy(50) && INFO.getAllCount(Terran_Factory, S) >= 2) {
		BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);
	}

	// 아모리
	if (canBuildFirstArmory(50) && INFO.getAllCount(Terran_Factory, S) >= 2) {
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}

	// 엔지니어링 베이
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

	// 스타포트
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

	// 사이언스 퍼실러티
	if (canBuildFirstScienceFacility(80)) {
		UnitInfo *armory = INFO.getClosestTypeUnit(S, MYBASE, Terran_Armory);

		// 공1업 완성됐거나 절반쯤 완성됐을 때
		if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) ||
				(armory != nullptr && armory->unit()->isUpgrading()
				 && (armory->unit()->getRemainingUpgradeTime() * 100) / UpgradeTypes::Terran_Vehicle_Weapons.upgradeTime() < 50))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Science_Facility, false);
		}
	}

	// 시즈모드업
	if (isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode)
			&& INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) >= 2
			&& INFO.getAllCount(Terran_Goliath, S) >= 4) {
		BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, false);
	}

	// 골리앗 사업
	if (INFO.getCompletedCount(Terran_Goliath, S) >= 1 && S->hasResearched(TechTypes::Tank_Siege_Mode) && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Charon_Boosters)) {
		if (INFO.getAllCount(Terran_Wraith, E) > 0 || INFO.getAllCount(Terran_Dropship, E) > 0 || INFO.getAllCount(Terran_Battlecruiser, E) > 0)
			BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Charon_Boosters, false);
		else if (INFO.getAllCount(Terran_Factory, S) >= 4)
			BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Charon_Boosters, false);
	}

	// 지상 메카닉 공업
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

	// 2번째 멀티, 가스 멀티를 우선 확인
	if (canBuildSecondExpansion(100) && SM.getNeedSecondExpansion() && INFO.getSecondExpansionLocation(S)
			&& (SM.getMainStrategy() == DrawLine || SM.getMainStrategy() == WaitLine || SM.getMainStrategy() == AttackAll))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
		cout << "멀티 위치 : " << INFO.getSecondExpansionLocation(S)->getTilePosition() << endl;
	}

	// 3번째 멀티, 가스 멀티를 우선 확인
	if (canBuildThirdExpansion(100) && SM.getNeedThirdExpansion() && INFO.getThirdExpansionLocation(S)
			&& (SM.getMainStrategy() == DrawLine || SM.getMainStrategy() == WaitLine || SM.getMainStrategy() == AttackAll))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());
		cout << "멀티 위치 : " << INFO.getThirdExpansionLocation(S)->getTilePosition() << endl;
	}

	if ((S->minerals() > 800 && INFO.getAllCount(Terran_Factory, S) >= 4) || (INFO.getAllCount(Terran_Command_Center, S) >= 3 && S->minerals() > 400))
	{
		// 벌쳐 마인업
		if (isTimeToMachineShopUpgrade(TechTypes::Spider_Mines)) {
			BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines, false);
		}

		// 벌쳐 속업 먼저
		if (isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters)) {
			BM.buildQueue.queueAsLowestPriority(UpgradeTypes::Ion_Thrusters, false);
		}
	}
}