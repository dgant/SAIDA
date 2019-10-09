#include "BasicBuildStrategy.h"
#include "EnemyStrategyManager.h"
#include "TrainManager.h"

using namespace MyBot;

BasicBuildStrategy &BasicBuildStrategy::Instance()
{
	static BasicBuildStrategy instance;
	return instance;
}
//构造函数=========================================================================
BasicBuildStrategy::BasicBuildStrategy()
{
	setFrameCountFlag(24 * 420);
	canceledBunker = false;
	notCancel = false;
}
//析解函数=========================================================================
BasicBuildStrategy::~BasicBuildStrategy()
{
}
//获取第二个基地的位置=========================================================================
TilePosition MyBot::BasicBuildStrategy::getSecondCommandPosition(bool force)
{

	if (secondCommandPosition != TilePositions::None) {

		if (INFO.getFirstExpansionLocation(S) && INFO.getFirstExpansionLocation(S)->getTilePosition() == secondCommandPosition)
			return secondCommandPosition;

		TilePosition tp = ConstructionPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Command_Center, (TilePosition)secondCommandPosition, false, MYBASE);
		//test
		if (tp != TilePositions::None) {
			return tp;
		}

	}

	secondCommandPosition = TilePositions::None;
	//如果没有第一次扩张的位置，返回none
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
//获取山上的防空塔位置=========================================================================
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
		}
		else {
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

//获取游戏帧数标志
int BasicBuildStrategy::getFrameCountFlag()
{
	return frameCountFlag;
}
//设置游戏帧数标志
void BasicBuildStrategy::setFrameCountFlag(int frameCountFlag_)
{
	frameCountFlag = frameCountFlag_;
}

//能建造第一个兵营===================================================================================
bool BasicBuildStrategy::canBuildFirstBarrack(int supplyUsed)
{
	// 兵营已经造完或者正在建造的情况下，不造
	if (isExistOrUnderConstruction(Terran_Barracks))
	{
		return false;
	}
	else {
		// 没有足够资源的时候，不造
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
//能建造精炼厂===================================================================================
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
//能建造精炼厂===================================================================================
bool BasicBuildStrategy::canBuildRefinery(int supplyUsed, Base *base)
{
	if (CM.existConstructionQueueItem(Terran_Refinery) || BM.buildQueue.existItem(Terran_Refinery)) {
		return false;
	}
	else {//没有足够资源就不建造
		if (!hasEnoughResourcesToBuild(Terran_Refinery)) {
			return false;
		}

		if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag()))
		{	// 您的位置是否有Refinery？如果有，就不建造
			if (hasRefinery(base))
				return false;

			// 目前，如果有气体充足，就不建设
			if (S->gas() > 1000)
				return false;

			// 检查是否有矿物
			uList commandList = INFO.getBuildings(Terran_Command_Center, S);

			for (auto center : commandList)
			{
				if (!center->isComplete())
					continue;

				if (!isSameArea(center->pos(), base->Center()))
					continue;

				// 如果工人人数多于矿物数量，refinery将被建造
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
//能建造第二个堵路地堡==================================================================================
bool BasicBuildStrategy::canBuildSecondChokePointBunker(int supplyUsed)
{
	if (!INFO.getTypeBuildingsInRadius(Terran_Bunker, S, INFO.getSecondChokePosition(S), 6 * TILE_SIZE).empty())
	{
		return false;
	}

	for (auto b : INFO.getBuildings(Terran_Bunker, S))
	{
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
//能建造基地防守地堡==================================================================================
bool BasicBuildStrategy::canBuildMainBaseDefenceBunker(int supplyUsed)
{
	if (BM.buildQueue.existItem(Terran_Bunker))
		return false;

	for (auto b : INFO.getBuildings(Terran_Bunker, S))
	{
		if (!b->isComplete())
			return false;

		if (isSameArea(b->pos(), INFO.getMainBaseLocation(S)->Center()))
			return false;

	}






	if (!hasEnoughResourcesToBuild(Terran_Bunker))
	{
		return false;
	}

	if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag()))
	{
		return true;
	}
	else
	{
		return false;
	}


	return false;
}
//存在精炼厂===================================================================================
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
//存在精炼厂===================================================================================
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
//能制造第一个工厂===================================================================================
bool BasicBuildStrategy::canBuildFirstFactory(int supplyUsed)
{
	if (isExistOrUnderConstruction(Terran_Factory)//如果有建成的工厂或者正在被建造的工厂
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
//能建造第二个工厂================================================================================
bool BasicBuildStrategy::canBuildSecondFactory(int supplyUsed)
{
	
	if (isUnderConstruction(Terran_Armory) || INFO.getAllCount(Terran_Factory, S) != 1)
		{
			return false;
		}
		
	if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) 
		{

				if (!hasEnoughResourcesToBuild(Terran_Factory)) 
				{
					return false;
				}

				return true;
		}

	else 
		return false;
		
		
	
}
//能建造第二个兵营================================================================================
bool BasicBuildStrategy::canBuildSecondBarrack(int supplyUsed)
{
	
	if ((INFO.getAllCount(Terran_Barracks, S) != 1) || isUnderConstruction(Terran_Barracks))
		{
			return false;
		}
		else
		{

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
//能建造额外工厂==================================================================================
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
	else if (INFO.enemyRace == Races::Protoss)
	{
		if (SM.getMyBuild() == MyBuildTypes::Protoss_DragoonKiller)
		{
			if (!isMyCommandAtFirstExpansion())
				maxCount = 1;
			else if (INFO.getCompletedCount(Terran_Academy, S) < 1)
				maxCount = 2;

			else if (INFO.getAllCount(Terran_Armory, S) < 2)
				maxCount = 3;

			else if (INFO.getAllCount(Terran_Science_Facility, S) < 1)
				maxCount = 4;


			else if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 1)
				maxCount = 5;

			else maxCount = 6;
		}
		else if (SM.getMyBuild() == MyBuildTypes::Protoss_CannonKiller)
		{
			if (!isMyCommandAtFirstExpansion())
				maxCount = 1;
			else if (INFO.getCompletedCount(Terran_Academy, S) < 1)
				maxCount = 2;

			else if (INFO.getAllCount(Terran_Science_Facility, S) < 1)
				maxCount = 3;

			else if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 1)
				maxCount = 5;

			else maxCount = 6;
		}
		else if (SM.getMyBuild() == MyBuildTypes::Protoss_CannonKiller)
		{
			if (INFO.getAllCount(Terran_Armory, S) < 1)
				maxCount = 2;
			else if (INFO.getCompletedCount(Terran_Armory, S) < 2)
				maxCount = 4;
			else if (INFO.getAllCount(Terran_Science_Facility, S) < 1)
				maxCount = 5;
			else
				maxCount = 6;

		}
		else if	(SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller)
		{
			if (INFO.getAllCount(Terran_Armory, S) < 1)
				maxCount = 2;
			else if (INFO.getCompletedCount(Terran_Armory, S) < 2)
				maxCount = 4;
			else if (INFO.getAllCount(Terran_Science_Facility, S) < 1)
				maxCount = 5;
			else 
				maxCount = 6;

		}
		else if (SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller)
		{
			if (INFO.getAllCount(Terran_Barracks, S) < 2)
				maxCount = 1;
			
			else if (INFO.getAllCount(Terran_Barracks, S)>=2 &&INFO.getDestroyedCount(Protoss_Nexus,E)<1)
				maxCount = 2;

			else maxCount = 3;
		}
		else if (SM.getMyBuild() == MyBuildTypes::Protoss_TemplarKiller)
		{
			if (TIME<8*24*60)
				maxCount = 2;
			else if (TIME>=8 * 24 * 60 && TIME<9 * 24 * 60)
				maxCount = 3;

			else if (TIME>9 * 24 * 60 && TIME<11 * 24 * 60)
				maxCount = 5;

			else maxCount = 7;
		}
		else maxCount = 6;
		
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
			maxCount += 1;

			// 테란전 후반에 자원 많이 남을 경우 팩토리 추가로 건설해놓자
			if (INFO.enemyRace == Races::Terran && S->minerals() >= 4000 && S->gas() >= 1500)
				maxCount += 2;
		}


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
	else if (INFO.enemyRace == Races::Protoss)
	{
		if (SM.getMyBuild() != MyBuildTypes::Protoss_MineKiller && SM.getMyBuild() != MyBuildTypes::Protoss_TemplarKiller)
		{


			if (!(EIB == Toss_2g_zealot || EIB == Toss_1g_forward || EIB == Toss_2g_forward || EMB == Toss_1base_fast_zealot)
				&& !(S->hasResearched(TechTypes::Tank_Siege_Mode) || S->isResearching(TechTypes::Tank_Siege_Mode)))
				return false;


			if (EMB == Toss_dark && INFO.getAllCount(Terran_Missile_Turret, S) == 0)
			{
				return false;
			}


			else if (EMB == Toss_range_up_dra_push && !HasEnemyFirstExpansion())
			{
				if (INFO.getBuildings(Terran_Engineering_Bay, S).empty()) {
					return false;
				}
			}
			else if (INFO.getAllCount(Terran_Factory, S) > 1 && INFO.getBuildings(Terran_Engineering_Bay, S).empty()) {
				return false;
			}
		}
	}

	if (!hasEnoughResourcesToBuild(Terran_Factory))
	{
		return false;
	}

	// 기존 지어진 팩토리들에서 병력들이 모두 생산중인 경우만
	int canTrainTankCnt = 0;
	int canTrainGoliathCnt = 0;

	for (auto f : INFO.getBuildings(Terran_Factory, S))
	{
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
bool BasicBuildStrategy::canBuildadditionalBarrack(int supplyUsed)
{
	
	
	
	
	
	if (isUnderConstruction(Terran_Barracks))
	{
		return false;
	}
	else 
	{
		// 没有足够资源的时候，不造
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
//能建造防空塔===================================================================================
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
//能建造第一个分基地===================================================================================
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
//能建造第二个分基地===================================================================================
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
//能建造第三个分基地===================================================================================
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
//能建造分基地===================================================================================
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
//能建造第一个飞机场===================================================================================
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
//能建造第一个防空塔===================================================================================
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
//能建造第一个机械研究院===================================================================================
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
//能建造第二个机械研究院===================================================================================
bool BasicBuildStrategy::canBuildSecondArmory(int supplyUsed) {
	if (isUnderConstruction(Terran_Armory)||INFO.getAllCount(Terran_Armory, S) != 1) {
		return false;
	}

	
	if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag())) 
	{
		if (hasEnoughResourcesToBuild(Terran_Armory)) 
		{
			return true;
		}
		else 
			return false;
	}
	return false;
	
}
//能建造第三个机械研究院===================================================================================
bool BasicBuildStrategy::canBuildThirdArmory(int supplyUsed) {
	if (isUnderConstruction(Terran_Armory) || INFO.getAllCount(Terran_Armory, S) != 2) {
		return false;
	}


	if (checkSupplyUsedForInitialBuild(supplyUsed, getFrameCountFlag()))
	{
		if (hasEnoughResourcesToBuild(Terran_Armory))
		{
			return true;
		}
		else
			return false;
	}
	return false;

}
//能建造第一个地堡===================================================================================
bool BasicBuildStrategy::canBuildFirstBunker(int supplyUsed)
{
	if (isExistOrUnderConstruction(Terran_Bunker)) {
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
//能建造第二个地堡===================================================================================
bool BasicBuildStrategy::canBuildSecondBunker(int supplyUsed)
{
	if (isUnderConstruction(Terran_Bunker)||INFO.getCompletedCount(Terran_Bunker,S)!=1)
	{
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
//能建造第一个步兵升级所===================================================================================
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
//能建造第一个步兵学院===================================================================================
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
//能建造第一个科学院===================================================================================
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
//有足够的资源可以建造===================================================================================
bool BasicBuildStrategy::hasEnoughResourcesToBuild(UnitType unitType)
{
	return BM.hasEnoughResources(unitType);
}
//有第二个基地===================================================================================
bool BasicBuildStrategy::hasSecondCommand()
{
	return INFO.getBuildings(Terran_Command_Center, S).size() > 1;
}
//是时候建造工厂附属建筑==================================================================================
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
//是时候建造飞机场附属建筑==================================================================================
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
//是时候进行科学院的研究==================================================================================
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
//是时候进行学院的研究了=================================================================================
bool BasicBuildStrategy::isTimeToAcademyUpgrade(TechType techType, UpgradeType upType)
{
	if (INFO.getCompletedCount(Terran_Academy, S) == 0) {
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
//是时候进行机械研究院的研究==================================================================================
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
//是时候进行工程湾的研究==================================================================================
bool BasicBuildStrategy::isTimeToEngineeringBayUpgrade(UpgradeType upgradeType, int level)
{
	// 아머리가 없으면 return false;
	if (INFO.getCompletedCount(Terran_Engineering_Bay, S) == 0)
	{
		return false;
	}

	if (level == 1 && (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) <1))
		// 2렙 이상 업그레이드는 Terran_Science_Facility 필요
	if ((level > 1 && INFO.getCompletedCount(Terran_Science_Facility, S) == 0) || (S->minerals()<500 || S->gas()<400))
		return false;

	if (S->isUpgrading(upgradeType) == true
		|| BM.buildQueue.getItemCount(upgradeType) != 0
		|| S->getUpgradeLevel(upgradeType) >= level)
	{
		return false;
	}

	// canUpgrade 로 체크하면 자원까지 포함해서 체크하기 때문에 미리 큐에 넣기 위해 아래와 같이 체크함.
	for (auto a : INFO.getBuildings(Terran_Engineering_Bay, S))
	if (a->unit()->isCompleted() && !a->unit()->isUpgrading())
		return true;

	return false;
}
//存在或者正在被建造==================================================================================
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
//正在被建造==================================================================================
bool BasicBuildStrategy::isUnderConstruction(UnitType unitType)
{
	if (CM.existConstructionQueueItem(unitType)
		|| BM.buildQueue.existItem(unitType)) {
		return true;
	}

	return false;
}
//核实初始建造的supply==================================================================================
bool BasicBuildStrategy::checkSupplyUsedForInitialBuild(int supplyUsed, int until_frameCount)
{
	if (until_frameCount == 0) {
		return true;
	}

	// 到until_frameCount为止， SupplyUsed用Flag来判断执行特定逻辑的时机。
	if (TIME < until_frameCount) {
		if (S->supplyUsed() >= supplyUsed) {
			return true;
		}
		else {
			return false;
		}
	}
	else {
		// 需要其他逻辑 
		return true;
	}
}

//建造地堡防御==================================================================================
void BasicBuildStrategy::buildBunkerDefence()
{
	if (isExistOrUnderConstruction(Terran_Bunker))
	{

		// 이미 취소되었었다면, 위치가 변경된 채로 큐에 다시 들어가 있을테니,
		// 한번더 취소하지 않는다.
		if (canceledBunker)
			return;

		bool cancelFlag = false;

		for (auto b : INFO.getBuildings(Terran_Bunker, S))
		{
			// 본진에 이미 지어진 벙커가 있으면 리턴
			if (b->isComplete() && theMap.GetArea((WalkPosition)b->pos()) == theMap.GetArea(INFO.getMainBaseLocation(S)->Location()))
			{
				return;
			}
		}

		if (CM.existConstructionQueueItem(Terran_Bunker))
		{
			const vector<ConstructionTask> *constructionQueue = CM.getConstructionQueue();

			for (const auto &b : *constructionQueue)
			{

				if (b.type == Terran_Bunker)
				{
					if (b.desiredPosition != TilePositions::None &&
						(theMap.GetArea(b.desiredPosition) != theMap.GetArea(INFO.getMainBaseLocation(S)->Location())
						|| theMap.GetArea(b.finalPosition) != theMap.GetArea(INFO.getMainBaseLocation(S)->Location())))
					{
						CM.cancelConstructionTask(b);
						cancelFlag = true;
					}
				}
			}
		}



		if (cancelFlag)
		{
			canceledBunker = true;

			BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, false);
		}
	}
	else
	{

		if (!hasEnoughResourcesToBuild(Terran_Bunker)/* && INFO.getUnitsInRadius(E, INFO.getMainBaseLocation(S)->Center(), 10 * TILE_SIZE, true, true, false).size() != 0*/)
		{
			// 没有足够钱造地堡就把在建中的BS爆掉


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
//分基地建造地堡防御

//已经研究或正在被研究==================================================================================
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
//获取正在建造的计数==================================================================================
int BasicBuildStrategy::getUnderConstructionCount(UnitType u, TilePosition p) {
	return BM.buildQueue.getItemCount(u, p) + CM.getConstructionQueueItemCount(u, p);
}
//获取存在或正在建造的计数==================================================================================
int BasicBuildStrategy::getExistOrUnderConstructionCount(UnitType unitType, Position position) {
	return INFO.getTypeBuildingsInRadius(unitType, S, position, 6 * TILE_SIZE).size() + getUnderConstructionCount(unitType, (TilePosition)position);
}
//建造防空塔==================================================================================
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
void BasicBuildStrategy::setmyBasicBuildStrategy()
{
	if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
			TerranVulutreTankWraithBuild();
		else if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
			TerranTankGoliathBuild();
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		
		if (SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller)
			ProtossMineKillerBuild();
		else if (SM.getMyBuild() == MyBuildTypes::Protoss_DragoonKiller)
			ProtossDragoonKillerBuild();
		else if (SM.getMyBuild() == MyBuildTypes::Protoss_CarrierKiller)
			ProtossCarrierKillerBuild();
		else if (SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller)
			ProtossZealotKillerBuild();
		else if (SM.getMyBuild() == MyBuildTypes::Protoss_TemplarKiller)
			ProtossTemplarKillerBuild();
		else if (SM.getMyBuild() == MyBuildTypes::Protoss_CannonKiller)
			ProtossCannonKillerBuild();
		
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		if (SM.getMyBuild() == MyBuildTypes::Zerg_MMtoMachanic)
			ZergMMtoMachanicBuild();
	}
}

//执行基本建造策略********************===============================================================================
void BasicBuildStrategy::executeBasicBuildStrategy()
{
	if (TIME % 12 != 0)//12帧跟新一次，也就是0.5s
		return;

	//如果没有建造基地==============================================
	if (INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, MYBASE, 5 * TILE_SIZE, true).empty()
		&& INFO.getUnitsInArea(E, MYBASE, true, true, false).empty()
		&& !CM.existConstructionQueueItem(Terran_Command_Center)
		&& !BM.buildQueue.existItem(Terran_Command_Center)
		)
	{
		int remainingM = 0;
		//将所有mineral加入remainingM
		for (auto m : INFO.getMainBaseLocation(S)->Minerals())
			remainingM += m->Amount();

		if (remainingM >= 2000)//如果家里的mineral有2000多，建造一个cc
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::MainBaseLocation);
	}
	
	setmyBasicBuildStrategy();

	
	if (INFO.enemyRace == Races::Zerg&& SM.getMyBuild() != MyBuildTypes::Zerg_MMtoMachanic)
	{
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

		}

		if (canBuildThirdExpansion(100) && SM.getNeedThirdExpansion())
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());

		}
	}
	else
	{

		//兵营的建造=======================================================
		if (canBuildFirstBarrack(22))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
		}

		//瓦斯场的建造=======================================================
		if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation))  //如果12人口可以造精炼厂
		{
			TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

			if (refineryPos.isValid())
				BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
		}
		//工厂的建造=======================================================
		if (canBuildFirstFactory(32))  //如果16人口可以造Factory
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
		}


	}
	// 공통 체크 (엘리가 되면 공중유닛을 뽑는다.)
	if (SM.getMainStrategy() == Eliminate)
	{
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
//获取第二个堵路防空塔位置==================================================================================
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

//取消主基地的堡垒==================================================================================
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
//取消第一次扩张==================================================================================
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
//取消第一个工厂附属建筑的建造==================================================================================
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

/// 从a到c建造三个防空塔
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
		if (!bw->canBuildHere(newPositionOfC, Terran_Missile_Turret) || dist > 7) {
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
//初始建造的改变==================================================================================
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
//主要建造的改变==================================================================================
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
//雷车坦克维京的建造==================================================================================
void BasicBuildStrategy::TerranVulutreTankWraithBuild()
{
	// 1배럭 + 1팩 + 앞마당 + 스타포트
	if (canBuildFirstBarrack(20))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
	}

	if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation))
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	if (canBuildFirstFactory(28)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}

	if (canBuildFirstExpansion(40)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
	}

	if (canBuildFirstBunker(40)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
	}

	// 이후의 로직은 앞마당을 먹은 뒤에 수행되도록 체크
	if (!hasSecondCommand())
	{
		return;
	}

	if (EIB == Terran_1fac_1star ||
		EIB == Terran_1fac_2star ||
		EIB == Terran_2fac_1star ||
		EMB == Terran_1fac_1star ||
		EMB == Terran_1fac_2star ||
		EMB == Terran_2fac_1star ||
		SM.getMainStrategy() == AttackAll)
	{
		if (canBuildFirstStarport(48))
		{
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
//坦克机器人的建造==================================================================================
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
void BasicBuildStrategy::ProtossMineKillerBuild()
{
	//兵营的建造=======================================================
	if (canBuildFirstBarrack(22))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);

	}

	if (canBuildSecondBarrack(0))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);

	}

	//瓦斯场的建造=======================================================
	if (canBuildRefinery(40, BuildOrderItem::SeedPositionStrategy::MainBaseLocation))  //如果12人口可以造精炼厂
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}


	if (canBuildFirstFactory(32)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}


	if (canBuildSecondBarrack(0))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);

	}
	if (canBuildadditionalFactory())

	{
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);

	}

	if (canBuildFirstBunker(0))//立即建造地堡（1堵路点）
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::EnemySecondChokePoint, false);
	}

	if (isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode))
	{
		BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, true);
	}
	if (INFO.getAllCount(Terran_Factory, S) < 1)
		return;

	if (canBuildFirstAcademy(50))
		BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);

	if (isTimeToAcademyUpgrade(TechTypes::None, UpgradeTypes::U_238_Shells) && isResearchingOrResearched(TechTypes::Tank_Siege_Mode))
		BM.buildQueue.queueAsHighestPriority(UpgradeTypes::U_238_Shells);

	//if (isTimeToAcademyUpgrade(TechTypes::Stim_Packs) && isResearchingOrResearched(TechTypes::Tank_Siege_Mode))
	//	BM.buildQueue.queueAsHighestPriority(TechTypes::Stim_Packs);
	

}
void BasicBuildStrategy::ProtossDragoonKillerBuild()
{
	//兵营的建造=======================================================
	if (canBuildFirstBarrack(22))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
	}

	//瓦斯场的建造=======================================================
	if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation))  //如果12人口可以造精炼厂
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}
	//工厂的建造=======================================================
	if (canBuildFirstFactory(32))  //如果16人口可以造Factory
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}
	
		if (canBuildFirstBunker(42))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
		}

		if (canBuildFirstExpansion(44))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
		}

		//坦克立即升级
		if (isExistOrUnderConstruction(Terran_Siege_Tank_Tank_Mode) && isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode))
		{
			BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, true);
		}
	
		if (INFO.getAllCount(Terran_Command_Center, S) < 2)
		{
			return;
		}
//================2矿分界线========================================================================
	//28人口造学院
	if (canBuildFirstAcademy(56))
		BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);


	//蜘蛛雷升级==================================================================
	if (isTimeToMachineShopUpgrade(TechTypes::Spider_Mines) && isResearchingOrResearched(TechTypes::Tank_Siege_Mode))
		BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines);

//	if (isTimeToAcademyUpgrade(TechTypes::Stim_Packs) && isResearchingOrResearched(TechTypes::Spider_Mines))
	//	BM.buildQueue.queueAsHighestPriority(TechTypes::Stim_Packs);

	//if (isTimeToAcademyUpgrade(TechTypes::None, UpgradeTypes::U_238_Shells) && isResearchingOrResearched(TechTypes::EMP_Shockwave) )
	//	BM.buildQueue.queueAsHighestPriority(UpgradeTypes::U_238_Shells);


	//雷车速度建造==================================================================
	if (isResearchingOrResearched(TechTypes::Spider_Mines)&& isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters)&&INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode,S)>=4)
		BM.buildQueue.queueAsLowestPriority(UpgradeTypes::Ion_Thrusters);



	//第二个工厂=======================================================
//if (canBuildSecondFactory(64))//32人口造第二个工厂
	//{
//		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	//}
	string enemyName = BWAPI::Broodwar->enemy()->getName();


	//EngineeringBay的建造=============================================================
	if (EMB == Toss_Scout || EMB == Toss_dark || EMB == Toss_dark_drop || EMB == Toss_drop || EMB == Toss_reaver_drop)
	{
		if (canBuildFirstEngineeringBay(72))
			BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);

	}

	else
	{
		
		 if (canBuildFirstEngineeringBay(82))
			BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);
	}



	//防空塔的建造================================================================================================
	if (EMB == Toss_dark || (EIB != Toss_cannon_rush && EMB == UnknownMainBuild  && TIME > 24 * 240))
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
		if (canBuildMissileTurret((TilePosition)MYBASE, 2))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
		}

		if (canBuildMissileTurret(getSecondChokePointTurretPosition()))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);
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

	


	if (isResearchingOrResearched(TechTypes::Tank_Siege_Mode) && canBuildFirstTurret(80))
		BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);

	

	if (EMB >= Toss_fast_carrier && EMB != UnknownMainBuild)
	{
		if (INFO.getCompletedCount(Terran_Goliath, S) >= 1 && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Charon_Boosters))
			BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Charon_Boosters, false);
	}

	// 分矿瓦斯建造==================================================================
	if (canBuildRefinery(60, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation) && INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 3)
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	// 三基地建造===========================================================


		//if (canBuildSecondExpansion(70)&&INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode,S)>=3)
	//	{
	//		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
	//	}

	
	
	//四基地建造============================================================
	//	if (canBuildThirdExpansion(72)&&INFO.getCompletedCount(Terran_Command_Center,S)==3)
	//	{
	//		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());
//}
	//军械库的建造============================================================
	if (canBuildFirstArmory(74))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}
	//军械库的升级==================================================================
	if (!S->isUpgrading(UpgradeTypes::Terran_Vehicle_Weapons) || !S->isUpgrading(UpgradeTypes::Terran_Vehicle_Plating))
	{
		// 공업 먼저
		UpgradeType highPriority = UpgradeTypes::Terran_Vehicle_Weapons;
		UpgradeType lowPriority = UpgradeTypes::Terran_Vehicle_Plating;
		UpgradeType upgradeType = S->isUpgrading(highPriority) ? lowPriority : S->isUpgrading(lowPriority) ? highPriority : S->getUpgradeLevel(highPriority) > S->getUpgradeLevel(lowPriority) ? lowPriority : highPriority;

		// 메카닉 공방업
		if (isTimeToArmoryUpgrade(upgradeType, S->getUpgradeLevel(upgradeType) + 1) )
		{
			BM.buildQueue.queueAsHighestPriority(upgradeType, false);
		}
	}
	// 第二个军械库的建造===========================================================
	if (canBuildSecondArmory(86))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}



	// 飞机场建造===========================================================
	  if (canBuildFirstStarport(104))
		BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);

	 	

	// 科学院的建造===========================================================
	  if (canBuildFirstScienceFacility(106))
		BM.buildQueue.queueAsHighestPriority(Terran_Science_Facility, false);





	
	// EMP Shockwave ===========================================================
	if (isTimeToScienceFacilityUpgrade(TechTypes::EMP_Shockwave) && (INFO.getAllCount(Terran_Science_Vessel, S) > 0) && isResearchingOrResearched(TechTypes::Tank_Siege_Mode))
	{
		BM.buildQueue.queueAsHighestPriority(TechTypes::EMP_Shockwave, false);
	}

	// 额外工厂建造===========================================================
	if (canBuildadditionalFactory())
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}
	int TankAllCountinESC = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Siege_Mode, S, INFO.getSecondChokePosition(E),25 * TILE_SIZE).size() + INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, INFO.getSecondChokePosition(E), 25 * TILE_SIZE).size();
	int TankAllCountinMC = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Siege_Mode, S, theMap.Center(), 10 * TILE_SIZE).size() + INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, theMap.Center(), 10 * TILE_SIZE).size();

	


	if (canBuildSecondExpansion(160) && TankAllCountinESC >= 3 &&(INFO.getCompletedCount(Terran_Command_Center,S)==2))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
		
	}
	else if (TIME > 15.5 * 60 * 24 && SM.getMainStrategy() == AttackAll && canBuildSecondExpansion(160) && (INFO.getCompletedCount(Terran_Command_Center, S) == 2))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
	}

	if (canBuildThirdExpansion(180)  && (INFO.getAllCount(Terran_Command_Center, S) == 3))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());

	}
}
void BasicBuildStrategy::ProtossZealotKillerBuild()
{	
	

	//兵营的建造=======================================================
	if (canBuildFirstBarrack(22))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);

	}

	//瓦斯场的建造=======================================================
	if (canBuildRefinery(40, BuildOrderItem::SeedPositionStrategy::MainBaseLocation))  //如果12人口可以造精炼厂
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}




	if (canBuildFirstFactory(32)) {
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}

	//军械库的建造============================================================
	if (canBuildFirstArmory(40))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}



	//蜘蛛雷升级
	if (isTimeToMachineShopUpgrade(TechTypes::Spider_Mines))
		BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines);

	Position backPos1 = (INFO.getFirstChokePosition(S) + INFO.getMainBaseLocation(S)->getPosition()) / 2;
	if (canBuildFirstBunker(0))//立即建造地堡（1堵路点）
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, false);
	} 



	if (INFO.getCompletedCount(Terran_Vulture, S)>=2)
	{
		if (canBuildSecondBunker(0))//立即建造地堡（1堵路点）
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
		}
	}
	
	
	if (isResearchingOrResearched(TechTypes::Spider_Mines) && isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode))
	{
		BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, true);
	}
	


	if (INFO.getCompletedCount(Terran_Bunker, S) >= 2)
	{
		if (canBuildFirstExpansion(0))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
		}
	}

	if (INFO.getAllCount(Terran_Command_Center, S) < 2)
	{
		return;
	}

	// 分矿瓦斯建造==================================================================
	if (canBuildRefinery(60, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation))
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}


	if (canBuildSecondArmory(64))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}


	if (isResearchingOrResearched(TechTypes::Tank_Siege_Mode) && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters))
	{
		BM.buildQueue.queueAsHighestPriority( UpgradeTypes::Ion_Thrusters, true);
	}

	if (canBuildFirstAcademy(70))
		BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);

	//EngineeringBay的建造=============================================================
	if (EMB == Toss_Scout || EMB == Toss_dark || EMB == Toss_dark_drop || EMB == Toss_drop || EMB == Toss_reaver_drop)
	{
		if (canBuildFirstEngineeringBay(120))
			BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);

	}

	else
	{

		if (canBuildFirstEngineeringBay(180))
			BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);
	}

	
	//军械库的升级==================================================================
	if (!S->isUpgrading(UpgradeTypes::Terran_Vehicle_Weapons) || !S->isUpgrading(UpgradeTypes::Terran_Vehicle_Plating))
	{
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

	
	// 飞机场建造===========================================================
	if (canBuildFirstStarport(88))
		BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);



	// 科学院的建造===========================================================
	if (canBuildFirstScienceFacility(92))
		BM.buildQueue.queueAsHighestPriority(Terran_Science_Facility, false);




	if(canBuildadditionalFactory())
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}



}
void BasicBuildStrategy::ProtossTemplarKillerBuild()
{
	//兵营的建造=======================================================
	if (canBuildFirstBarrack(22))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
	}

	//瓦斯场的建造=======================================================
	if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation))  //如果12人口可以造精炼厂
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}




	//工厂的建造=======================================================
	if (canBuildFirstFactory(32))  //如果16人口可以造Factory
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}


	if (canBuildFirstExpansion(42))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
	}


	if (canBuildFirstBunker(46))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
	}


	//坦克立即升级
	if (isExistOrUnderConstruction(Terran_Siege_Tank_Tank_Mode) && isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode))
	{
		BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, true);
	}

	if (INFO.getAllCount(Terran_Command_Center, S) < 2)
	{
		return;
	}






	//================2矿分界线========================================================================
	//28人口造学院
	if (canBuildFirstAcademy(50))
		BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);


	if (canBuildSecondFactory(66))
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);


	//蜘蛛雷升级==================================================================
	if (isTimeToMachineShopUpgrade(TechTypes::Spider_Mines) && isResearchingOrResearched(TechTypes::Tank_Siege_Mode))
		BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines);

	// 分矿瓦斯建造==================================================================
	if (canBuildRefinery(72, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation) && INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 3)
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}


	// 飞机场建造===========================================================
	if (canBuildFirstStarport(90))
		BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);



	// 科学院的建造===========================================================
	if (canBuildFirstScienceFacility(96))
		BM.buildQueue.queueAsHighestPriority(Terran_Science_Facility, false);

	if (canBuildSecondExpansion(110) && TIME>=7.9 * 60*24)
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());

	}

	if (canBuildadditionalFactory())
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}


	if (canBuildFirstArmory(0) && TIME>11 * 24 * 60)
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}


	if (canBuildSecondArmory(0) && TIME>11.2 * 24 * 60)
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}

	if (canBuildThirdExpansion(0) && TIME >= 11.5 * 60 * 24 && S->minerals()>700)
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());

	}



	//军械库的升级==================================================================
	if (!S->isUpgrading(UpgradeTypes::Terran_Vehicle_Weapons) || !S->isUpgrading(UpgradeTypes::Terran_Vehicle_Plating))
	{
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




}
void BasicBuildStrategy::ProtossCannonKillerBuild()
{
	//兵营的建造=======================================================
	if (canBuildFirstBarrack(22))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
	}

	//瓦斯场的建造=======================================================
	if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation))  //如果12人口可以造精炼厂
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}
	//工厂的建造=======================================================
	if (canBuildFirstFactory(32))  //如果16人口可以造Factory
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}

	if (canBuildFirstExpansion(40))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
	}

	if (canBuildFirstBunker(44))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
	}
	if (INFO.getAllCount(Terran_Command_Center, S) < 2)
		return;


	//坦克立即升级
	if (isExistOrUnderConstruction(Terran_Siege_Tank_Tank_Mode) && isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode))
	{
		BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, true);
	}


	//================2矿分界线========================================================================
	//28人口造学院
	if (canBuildFirstAcademy(56))
		BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);


	//蜘蛛雷升级==================================================================
	if (isTimeToMachineShopUpgrade(TechTypes::Spider_Mines) && isResearchingOrResearched(TechTypes::Tank_Siege_Mode))
		BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines);

	//	if (isTimeToAcademyUpgrade(TechTypes::Stim_Packs) && isResearchingOrResearched(TechTypes::Spider_Mines))
	//	BM.buildQueue.queueAsHighestPriority(TechTypes::Stim_Packs);

	//if (isTimeToAcademyUpgrade(TechTypes::None, UpgradeTypes::U_238_Shells) && isResearchingOrResearched(TechTypes::EMP_Shockwave) )
	//	BM.buildQueue.queueAsHighestPriority(UpgradeTypes::U_238_Shells);


	//雷车速度建造==================================================================
	if (isResearchingOrResearched(TechTypes::Spider_Mines) && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Ion_Thrusters) && INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 4)
		BM.buildQueue.queueAsLowestPriority(UpgradeTypes::Ion_Thrusters);



	//第二个工厂=======================================================
	//if (canBuildSecondFactory(64))//32人口造第二个工厂
	//{
	//		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	//}
	string enemyName = BWAPI::Broodwar->enemy()->getName();


	//EngineeringBay的建造=============================================================
	if (EMB == Toss_Scout || EMB == Toss_dark || EMB == Toss_dark_drop || EMB == Toss_drop || EMB == Toss_reaver_drop)
	{
		if (canBuildFirstEngineeringBay(72))
			BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);

	}

	else
	{

		if (canBuildFirstEngineeringBay(82))
			BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);
	}



	//防空塔的建造================================================================================================
	if (EMB == Toss_dark || (EIB != Toss_cannon_rush && EMB == UnknownMainBuild  && TIME > 24 * 240))
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
		if (canBuildMissileTurret((TilePosition)MYBASE, 2))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
		}

		if (canBuildMissileTurret(getSecondChokePointTurretPosition()))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);
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




	if (isResearchingOrResearched(TechTypes::Tank_Siege_Mode) && canBuildFirstTurret(80))
		BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);



	if (EMB >= Toss_fast_carrier && EMB != UnknownMainBuild)
	{
		if (INFO.getCompletedCount(Terran_Goliath, S) >= 1 && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Charon_Boosters))
			BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Charon_Boosters, false);
	}

	// 分矿瓦斯建造==================================================================
	if (canBuildRefinery(60, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation) && INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 3)
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}

	// 三基地建造===========================================================


	//if (canBuildSecondExpansion(70)&&INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode,S)>=3)
	//	{
	//		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
	//	}



	//四基地建造============================================================
	//	if (canBuildThirdExpansion(72)&&INFO.getCompletedCount(Terran_Command_Center,S)==3)
	//	{
	//		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());
	//}
	//军械库的建造============================================================
	if (canBuildFirstArmory(74))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}
	//军械库的升级==================================================================
	if (!S->isUpgrading(UpgradeTypes::Terran_Vehicle_Weapons) || !S->isUpgrading(UpgradeTypes::Terran_Vehicle_Plating))
	{
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
	// 飞机场建造===========================================================
	if (canBuildFirstStarport(80))
		BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);

	if (canBuildSecondArmory(90))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}

	// 科学院的建造===========================================================
	if (canBuildFirstScienceFacility(100))
		BM.buildQueue.queueAsHighestPriority(Terran_Science_Facility, false);






	// EMP Shockwave ===========================================================
	if (isTimeToScienceFacilityUpgrade(TechTypes::EMP_Shockwave) && (INFO.getAllCount(Terran_Science_Vessel, S) > 0) && isResearchingOrResearched(TechTypes::Tank_Siege_Mode))
	{
		BM.buildQueue.queueAsHighestPriority(TechTypes::EMP_Shockwave, false);
	}

	// 额外工厂建造===========================================================
	if (canBuildadditionalFactory())
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}
	
	else if (TIME > 16 * 60 * 24 && SM.getMainStrategy() == AttackAll && canBuildSecondExpansion(160) && (INFO.getCompletedCount(Terran_Command_Center, S) == 2))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
	}

	if (canBuildThirdExpansion(180) && (INFO.getAllCount(Terran_Command_Center, S) == 3))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());

	}

}
void BasicBuildStrategy::ProtossCarrierKillerBuild()
{
	//兵营的建造=======================================================
	if (canBuildFirstBarrack(22))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Barracks, false);
	}

	//瓦斯场的建造=======================================================
	if (canBuildRefinery(24, BuildOrderItem::SeedPositionStrategy::MainBaseLocation))  //如果12人口可以造精炼厂
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}
	//工厂的建造=======================================================
	if (canBuildFirstFactory(32))  //如果16人口可以造Factory
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}



	if (canBuildFirstExpansion(0))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation, false);
	}



	if (canBuildFirstBunker(46))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Bunker, BuildOrderItem::SeedPositionStrategy::SecondChokePoint, false);
	}


	//坦克立即升级
	if (isExistOrUnderConstruction(Terran_Siege_Tank_Tank_Mode) && isTimeToMachineShopUpgrade(TechTypes::Tank_Siege_Mode))
	{
		BM.buildQueue.queueAsHighestPriority(TechTypes::Tank_Siege_Mode, true);
	}

	if (INFO.getAllCount(Terran_Command_Center, S) < 2)
	{
		return;
	}
	//================2矿分界线========================================================================
	//28人口造学院
	if (canBuildFirstAcademy(58))
		BM.buildQueue.queueAsHighestPriority(Terran_Academy, false);

	// 分矿瓦斯建造==================================================================
	if (canBuildRefinery(60, BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation))
	{
		TilePosition refineryPos = ConstructionPlaceFinder::Instance().getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);

		if (refineryPos.isValid())
			BM.buildQueue.queueAsHighestPriority(Terran_Refinery, refineryPos, false);
	}
	//蜘蛛雷升级==================================================================
	if (isTimeToMachineShopUpgrade(TechTypes::Spider_Mines) && isResearchingOrResearched(TechTypes::Tank_Siege_Mode))
		BM.buildQueue.queueAsHighestPriority(TechTypes::Spider_Mines);

	//	if (isTimeToAcademyUpgrade(TechTypes::Stim_Packs) && isResearchingOrResearched(TechTypes::Spider_Mines))
	//	BM.buildQueue.queueAsHighestPriority(TechTypes::Stim_Packs);

	if (isTimeToAcademyUpgrade(TechTypes::None, UpgradeTypes::U_238_Shells) && isResearchingOrResearched(TechTypes::Spider_Mines))
		BM.buildQueue.queueAsHighestPriority(UpgradeTypes::U_238_Shells);


	




	//EngineeringBay的建造=============================================================
	if (EMB == Toss_Scout || EMB == Toss_dark || EMB == Toss_dark_drop || EMB == Toss_drop || EMB == Toss_reaver_drop)
	{
		if (canBuildFirstEngineeringBay(62))
			BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);

	}

	else
	{

		if (canBuildFirstEngineeringBay(70))
			BM.buildQueue.queueAsHighestPriority(Terran_Engineering_Bay, false);
	}


	//防空塔的建造================================================================================================
	if (EMB == Toss_dark || (EIB != Toss_cannon_rush && EMB == UnknownMainBuild  && TIME > 24 * 240))
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
		if (canBuildMissileTurret((TilePosition)MYBASE, 2))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, BuildOrderItem::SeedPositionStrategy::MainBaseLocation, true);
		}

		if (canBuildMissileTurret(getSecondChokePointTurretPosition()))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);
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




	if (isResearchingOrResearched(TechTypes::Tank_Siege_Mode) && canBuildFirstTurret(80))
		BM.buildQueue.queueAsHighestPriority(Terran_Missile_Turret, getSecondChokePointTurretPosition(), true);



	if (EMB >= Toss_fast_carrier && EMB != UnknownMainBuild)
	{
		if (INFO.getCompletedCount(Terran_Goliath, S) >= 1 && isTimeToMachineShopUpgrade(TechTypes::None, UpgradeTypes::Charon_Boosters))
			BM.buildQueue.queueAsHighestPriority(UpgradeTypes::Charon_Boosters, false);
	}



	if (EMB == Toss_fast_carrier || EMB == Toss_Scout || EMB == Toss_arbiter_carrier)
	{
		if (canBuildFirstStarport(70))
			BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);
		//军械库的建造============================================================
		if (canBuildFirstArmory(82))
		{
			BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
		}
	}
	else
		if (canBuildFirstStarport(104))
			BM.buildQueue.queueAsHighestPriority(Terran_Starport, false);
	    if (canBuildFirstArmory(76))
	     {
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	     }


	//军械库的升级==================================================================
		if (!S->isUpgrading(UpgradeTypes::Terran_Ship_Weapons) || !S->isUpgrading(UpgradeTypes::Terran_Vehicle_Weapons) || !S->isUpgrading(UpgradeTypes::Terran_Vehicle_Plating))
	{
		// 공업 먼저
		UpgradeType highPriority = UpgradeTypes::Terran_Vehicle_Weapons;
		UpgradeType lowPriority = UpgradeTypes::Terran_Vehicle_Plating;
		UpgradeType lowestPriority = UpgradeTypes::Terran_Ship_Weapons;
		UpgradeType upgradeType = UpgradeTypes::None;
		 upgradeType = S->isUpgrading(highPriority) ? lowPriority : S->isUpgrading(lowPriority) ? highPriority : S->getUpgradeLevel(highPriority) > S->getUpgradeLevel(lowPriority) ? lowPriority : highPriority;
		 if ((S->isUpgrading(highPriority) && S->isUpgrading(lowPriority)))
			 upgradeType = lowestPriority;

		 if (S->getUpgradeLevel(highPriority) >= 3 && S->getUpgradeLevel(lowPriority) >= 3)
			 upgradeType = lowestPriority;




		// 메카닉 공방업
		if (isTimeToArmoryUpgrade(upgradeType, S->getUpgradeLevel(upgradeType) + 1))
		{
			BM.buildQueue.queueAsHighestPriority(upgradeType, false);
		}
	}
	// 第二个军械库的建造===========================================================
	if (canBuildSecondArmory(90))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}

	// 科学院的建造===========================================================
	if (canBuildFirstScienceFacility(110))
		BM.buildQueue.queueAsHighestPriority(Terran_Science_Facility, false);

	if (canBuildThirdArmory(120))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Armory, false);
	}



	// EMP Shockwave ===========================================================
	if (isTimeToScienceFacilityUpgrade(TechTypes::EMP_Shockwave) && (INFO.getAllCount(Terran_Science_Vessel, S) > 0) && (EMB != Toss_fast_carrier && EMB != Toss_arbiter_carrier))
	{
		BM.buildQueue.queueAsHighestPriority(TechTypes::EMP_Shockwave, false);
	}

	// 额外工厂建造===========================================================
	if (canBuildadditionalFactory())
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Factory, false);
	}
	int TankAllCountinESC = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Siege_Mode, S, INFO.getSecondChokePosition(E), 25 * TILE_SIZE).size() + INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, INFO.getSecondChokePosition(E), 25 * TILE_SIZE).size();
	int TankAllCountinMC = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Siege_Mode, S, theMap.Center(), 10 * TILE_SIZE).size() + INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, theMap.Center(), 10 * TILE_SIZE).size();




	if (canBuildSecondExpansion(160) && TankAllCountinESC >= 3 && (INFO.getCompletedCount(Terran_Command_Center, S) == 2))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());

	}

	else if (TIME > 14.5* 60 * 24 && SM.getMainStrategy() == AttackAll && canBuildSecondExpansion(160) && (INFO.getCompletedCount(Terran_Command_Center, S) == 2))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getSecondExpansionLocation(S)->getTilePosition());
	}

	if (canBuildThirdExpansion(180) && (INFO.getAllCount(Terran_Command_Center, S) == 3))
	{
		BM.buildQueue.queueAsHighestPriority(Terran_Command_Center, INFO.getThirdExpansionLocation(S)->getTilePosition());

	}
	
}
void BasicBuildStrategy::ZergMMtoMachanicBuild()
{

}
