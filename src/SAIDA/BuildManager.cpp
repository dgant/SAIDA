#include "BuildManager.h"

using namespace MyBot;

BuildManager::BuildManager()
{
}

// 对内部订单队列执行生产/建设/调研/升级 
void BuildManager::update()
{
	// 只在1秒（24帧）上运行4次就足够了 
	if (Broodwar->getFrameCount() % 6 != 0) return;

	if (buildQueue.isEmpty()) {
		return;
	}

	clock_t str = clock();
	// //deadLock已被检出并删除 
	checkBuildOrderQueueDeadlockAndAndFixIt();

	// deadlock移除后可能为空 

	if (buildQueue.isEmpty()) {
		return;
	}

	// 要使用的当前项目 
	BuildOrderItem currentItem = buildQueue.getHighestPriorityItem();

	//cout << "current HighestPriorityItem" << currentItem.metaType.getName() << endl;

	// 在buildqueue中还存在一些东西的时候 
	while (!buildQueue.isEmpty())
	{
		bool isOkToRemoveQueue = true;

		// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
		// 빌드 실행 유닛 (일꾼/건물) 결정 로직이 seedLocation 이나 seedLocationStrategy 를 잘 반영하도록 수정

		// seedPosition 을 도출한다
		Position seedPosition = Positions::None;

		if (currentItem.seedLocation != TilePositions::None && currentItem.seedLocation != TilePositions::Invalid
				&& currentItem.seedLocation != TilePositions::Unknown && currentItem.seedLocation.isValid()) {
			seedPosition = Position(currentItem.seedLocation);
		}
		else {
			seedPosition = (Position)ConstructionPlaceFinder::Instance().getSeedPositionFromSeedLocationStrategy(currentItem.seedLocationStrategy);
		}

		// this is the unit which can produce the currentItem
		Unit producer = getProducer(currentItem.metaType, seedPosition, currentItem.producerID);

		// BasicBot 1.1 Patch End //////////////////////////////////////////////////

		/*
		if (currentItem.metaType.isUnit() && currentItem.metaType.getUnitType().isBuilding() ) {
		if (producer) {
		cout << "Build " << currentItem.metaType.getName() << " producer : " << producer->getType().getName() << " ID : " << producer->getID() << endl;
		}
		else {
		cout << "Build " << currentItem.metaType.getName() << " producer nullptr" << endl;
		}
		}
		*/

		Unit secondProducer = nullptr;
		bool canMake;

		// 건물을 만들수 있는 유닛(일꾼)이나, 유닛을 만들수 있는 유닛(건물 or 유닛)이 있으면
		if (producer != nullptr) {
			// check to see if we can make it right now
			// 지금 해당 유닛을 건설/생산 할 수 있는지에 대해 자원, 서플라이, 테크 트리, producer 만을 갖고 판단한다
			canMake = canMakeNow(producer, currentItem.metaType);

			/*
			if (currentItem.metaType.isUnit() && currentItem.metaType.getUnitType().isBuilding() ) {
			cout << "Build " << currentItem.metaType.getName() << " canMakeNow : " << canMake << endl;
			}
			*/
		}

		// if we can make the current item, create it
		if (producer != nullptr && canMake == true)
		{
			MetaType t = currentItem.metaType;

			if (t.isUnit())
			{
				if (t.getUnitType().isBuilding()) {

					// 테란 Addon 건물의 경우 (Addon 건물을 지을수 있는지는 getProducer 함수에서 이미 체크완료)
					// 모건물이 Addon 건물 짓기 전에는 canBuildAddon = true, isConstructing = false, canCommand = true 이다가
					// Addon 건물을 짓기 시작하면 canBuildAddon = false, isConstructing = true, canCommand = true 가 되고 (Addon 건물 건설 취소는 가능하나 Train 등 커맨드는 불가능)
					// 완성되면 canBuildAddon = false, isConstructing = false 가 된다
					if (t.getUnitType().isAddon()) {

						//cout << "addon build start " << endl;

						producer->buildAddon(t.getUnitType());

						// 테란 Addon 건물의 경우 정상적으로 buildAddon 명령을 내려도 SCV가 모건물 근처에 있을 때 한동안 buildAddon 명령이 취소되는 경우가 있어서
						// 모건물이 isConstructing = true 상태로 바뀐 것을 확인한 후 buildQueue 에서 제거해야한다
						if (producer->isConstructing() == false) {
							isOkToRemoveQueue = false;
						}

						//cout << "8";
					}
					// 그외 대부분 건물의 경우
					else
					{
						// ConstructionPlaceFinder 를 통해 건설 가능 위치 desiredPosition 를 알아내서
						// ConstructionManager 의 ConstructionTask Queue에 추가를 해서 desiredPosition 에 건설을 하게 한다.
						// ConstructionManager 가 건설 도중에 해당 위치에 건설이 어려워지면 다시 ConstructionPlaceFinder 를 통해 건설 가능 위치를 desiredPosition 주위에서 찾을 것이다
						TilePosition desiredPosition = getDesiredPosition(t.getUnitType(), currentItem.seedLocation, currentItem.seedLocationStrategy);

						// cout << "BuildManager "
						//	<< currentItem.metaType.getUnitType().getName().c_str() << " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << endl;

						if (desiredPosition != TilePositions::None) {
							// Send the construction task to the construction manager
							ConstructionManager::Instance().addConstructionTask(t.getUnitType(), desiredPosition);
						}
						else {
							// 건물 가능 위치가 없는 경우는, Protoss_Pylon 가 없거나, Creep 이 없거나, Refinery 가 이미 다 지어져있거나, 정말 지을 공간이 주위에 없는 경우인데,
							// 대부분의 경우 Pylon 이나 Hatchery가 지어지고 있는 중이므로, 다음 frame 에 건물 지을 공간을 다시 탐색하도록 한다.
							cout << "There is no place to construct " << currentItem.metaType.getUnitType().getName().c_str()
								 << " strategy " << currentItem.seedLocationStrategy
								 << " seedPosition " << currentItem.seedLocation.x << "," << currentItem.seedLocation.y
								 << " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << endl;

							isOkToRemoveQueue = false;
						}
					}
				}
				// 지상유닛 / 공중유닛의 경우
				else {
					// 테란 지상유닛 / 공중유닛
					producer->train(t.getUnitType());
				}
			}
			// if we're dealing with a tech research
			else if (t.isTech())
			{
				producer->research(t.getTechType());
			}
			else if (t.isUpgrade())
			{
				producer->upgrade(t.getUpgradeType());
			}

			//cout << endl << " build " << t.getName() << " started " << endl;

			// remove it from the buildQueue
			if (isOkToRemoveQueue) {
				buildQueue.removeCurrentItem();
			}

			// don't actually loop around in here
			break;
		}
		// otherwise, if we can skip the current item
		else if (buildQueue.canSkipCurrentItem())
		{
			// skip it and get the next one
			buildQueue.skipCurrentItem();
			currentItem = buildQueue.getNextItem();
		}
		else
		{
			// so break out
			break;
		}
	}
}

Unit BuildManager::getProducer(MetaType t, Position closestTo, int producerID)
{
	// get the type of unit that builds this
	UnitType producerType = t.whatBuilds();

	// make a set of all candidate producers
	Unitset candidateProducers;

	for (auto &unit : S->getUnits())
	{
		if (unit == nullptr) continue;

		// reasons a unit can not train the desired type
		if (unit->getType() != producerType) {
			continue;
		}

		if (!unit->exists()) {
			continue;
		}

		if (!unit->isCompleted()) {
			continue;
		}

		if (unit->isTraining()) {
			continue;
		}

		if (!unit->isPowered()) {
			continue;
		}

		// if unit is lifted, unit should land first
		if (unit->isLifted()) {
			continue;
		}

		if (producerID != -1 && unit->getID() != producerID) {
			continue;
		}

		if (t.isTech())
		{
			if (!unit->canResearch(t.getTechType()))
				continue;
		}
		else if (t.isUpgrade())
		{
			if (!unit->canUpgrade(t.getUpgradeType()))
				continue;
		}

		// if the type is an addon
		if (t.getUnitType().isAddon())
		{
			// if the unit already has an addon, it can't make one
			if (unit->getAddon() != nullptr) {
				continue;
			}

			// 모건물은 건설되고 있는 중에는 isCompleted = false, isConstructing = true, canBuildAddon = false 이다가
			// 건설이 완성된 후 몇 프레임동안은 isCompleted = true 이지만, canBuildAddon = false 인 경우가 있다
			if (!unit->canBuildAddon()) {
				continue;
			}

			// if we just told this unit to build an addon, then it will not be building another one
			// this deals with the frame-delay of telling a unit to build an addon and it actually starting to build
			if (unit->getLastCommand().getType() == UnitCommandTypes::Build_Addon
					&& (Broodwar->getFrameCount() - unit->getLastCommandFrame() < 10))
			{
				continue;
			}

			bool isBlocked = false;

			// if the unit doesn't have space to build an addon, it can't make one
			TilePosition addonPosition(
				unit->getTilePosition().x + unit->getType().tileWidth(),
				unit->getTilePosition().y + unit->getType().tileHeight() - t.getUnitType().tileHeight());

			for (int i = 0; i < t.getUnitType().tileWidth(); ++i)
			{
				for (int j = 0; j < t.getUnitType().tileHeight(); ++j)
				{
					TilePosition tilePos(addonPosition.x + i, addonPosition.y + j);

					// if the map won't let you build here, we can't build it.
					// 맵 타일 자체가 건설 불가능한 타일인 경우 + 기존 건물이 해당 타일에 이미 있는경우
					if (!Broodwar->isBuildable(tilePos, true))
					{
						isBlocked = true;
						break;
					}

					// if there are any units on the addon tile, we can't build it
					// 아군 유닛은 Addon 지을 위치에 있어도 괜찮음. (적군 유닛은 Addon 지을 위치에 있으면 건설 안되는지는 아직 불확실함)
					Unitset uot = Broodwar->getUnitsOnTile(tilePos.x, tilePos.y);

					for (auto &u : uot) {
						//cout << endl << "Construct " << t.getName()
						//	<< " beside "<< unit->getType().getName() << "(" << unit->getID() <<")"
						//	<< ", units on Addon Tile " << tilePos.x << "," << tilePos.y << " is " << u->getType().getName() << "(ID : " << u->getID() << " Player : " << u->getPlayer()->getName() << ")"
						//	<< endl;
						if (u->getType() == Terran_Siege_Tank_Siege_Mode) {
							isBlocked = true;
							break;
						}
					}
				}

				if (isBlocked)
					break;
			}

			if (isBlocked)
			{
				continue;
			}
		}

		// if we haven't cut it, add it to the set of candidates
		candidateProducers.insert(unit);
	}

	return getClosestUnitToPosition(candidateProducers, closestTo);
}

Unit BuildManager::getClosestUnitToPosition(const Unitset &units, Position closestTo)
{
	if (units.size() == 0)
	{
		return nullptr;
	}

	// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
	// 빌드 실행 유닛 (일꾼/건물) 결정 로직이 seedLocation 이나 seedLocationStrategy 를 잘 반영하도록 수정

	// if we don't care where the unit is return the first one we have
	if (closestTo == Positions::None || closestTo == Positions::Invalid || closestTo == Positions::Unknown || closestTo.isValid() == false)
	{
		return *(units.begin());
	}

	// BasicBot 1.1 Patch End //////////////////////////////////////////////////

	Unit closestUnit = nullptr;
	double minDist(1000000000);

	for (auto &unit : units)
	{
		if (unit == nullptr) continue;

		double distance = unit->getDistance(closestTo);

		//cout << "distance to " << unit->getType().getName() << " is " << distance << endl;

		if (!closestUnit || distance < minDist)
		{
			closestUnit = unit;
			minDist = distance;
		}
	}

	return closestUnit;
}

// 지금 해당 유닛을 건설/생산 할 수 있는지에 대해 자원, 서플라이, 테크 트리, producer 만을 갖고 판단한다
// 해당 유닛이 건물일 경우 건물 지을 위치의 적절 여부 (탐색했었던 타일인지, 건설 가능한 타일인지, 주위에 Pylon이 있는지, Creep이 있는 곳인지 등) 는 판단하지 않는다
bool BuildManager::canMakeNow(Unit producer, MetaType t)
{
	if (producer == nullptr) {
		return false;
	}

	bool canMake = hasEnoughResources(t);

	if (canMake)
	{
		if (t.isBuilding() && !t.getUnitType().isAddon())
		{
			// 건물의 경우 자원이 충족하기 전에 미리 출발하기 위해 테크트리만 체크한다.
			for (auto &val : t.getUnitType().requiredUnits()) {
				if (!INFO.getAllCount(val.first, S)) {
					canMake = false;
					break;
				}
			}
		}
		else if (t.isUnit())
		{
			// Broodwar->canMake : Checks all the requirements include resources, supply, technology tree, availability, and required units
			canMake = Broodwar->canMake(t.getUnitType(), producer);
		}
		else if (t.isTech())
		{
			canMake = Broodwar->canResearch(t.getTechType(), producer);
		}
		else if (t.isUpgrade())
		{
			canMake = Broodwar->canUpgrade(t.getUpgradeType(), producer);
		}
	}

	return canMake;
}

// 건설 가능 위치를 찾는다
// seedLocationStrategy 가 SeedPositionSpecified 인 경우에는 그 근처만 찾아보고, SeedPositionSpecified 이 아닌 경우에는 seedLocationStrategy 를 조금씩 바꿔가며 계속 찾아본다.
// (MainBase -> MainBase 주위 -> MainBase 길목 -> MainBase 가까운 앞마당 -> MainBase 가까운 앞마당의 길목 -> 다른 멀티 위치 -> 탐색 종료)
TilePosition BuildManager::getDesiredPosition(UnitType unitType, TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy)
{
	TilePosition desiredPosition = ConstructionPlaceFinder::Instance().getBuildLocationWithSeedPositionAndStrategy(unitType, seedPosition, seedPositionStrategy);

	/*
	cout << "ConstructionPlaceFinder getBuildLocationWithSeedPositionAndStrategy "
	<< unitType.getName().c_str()
	<< " strategy " << seedPositionStrategy
	<< " seedPosition " << seedPosition.x << "," << seedPosition.y
	<< " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << endl;
	*/

	// desiredPosition 을 찾을 수 없는 경우
	bool findAnotherPlace = true;

	while (desiredPosition == TilePositions::None) {

		switch (seedPositionStrategy) {
			case BuildOrderItem::SeedPositionStrategy::MainBaseLocation:
				seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation;
				break;

			case BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation:
				seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation;
				break;

			case BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation:
				seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation;
				break;

			default:
				findAnotherPlace = false;
				break;
		}

		// 다른 곳을 더 찾아본다
		if (findAnotherPlace) {
			desiredPosition = ConstructionPlaceFinder::Instance().getBuildLocationWithSeedPositionAndStrategy(unitType, seedPosition, seedPositionStrategy);
			/*
			cout << "ConstructionPlaceFinder getBuildLocationWithSeedPositionAndStrategy "
			<< unitType.getName().c_str()
			<< " strategy " << seedPositionStrategy
			<< " seedPosition " << seedPosition.x << "," << seedPosition.y
			<< " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << endl;
			*/
		}
		// 다른 곳을 더 찾아보지 않고, 끝낸다
		else {
			break;
		}
	}

	return desiredPosition;
}

// 사용가능 미네랄 = 현재 보유 미네랄 - 사용하기로 예약되어있는 미네랄
int BuildManager::getAvailableMinerals()
{
	return S->minerals() - ConstructionManager::Instance().getReservedMinerals();
}

// 사용가능 가스 = 현재 보유 가스 - 사용하기로 예약되어있는 가스
int BuildManager::getAvailableGas()
{
	return S->gas() - ConstructionManager::Instance().getReservedGas();
}

// return whether or not we meet resources, including building reserves
bool BuildManager::hasEnoughResources(MetaType type)
{
	double mineralRate = 1.0;
	double gasRate = 1.0;

	if (!(type.isTech() || type.isUpgrade()))
	{
		mineralRate -= 0.01 * ScvManager::Instance().getAllMineralScvCount();
		gasRate -= 0.02 * ScvManager::Instance().getAllRefineryScvCount();
	}

	// return whether or not we meet the resources
	return (type.mineralPrice() * mineralRate <= getAvailableMinerals()) && (type.gasPrice() * gasRate <= getAvailableGas());
}

BuildManager &BuildManager::Instance()
{
	static BuildManager instance;
	return instance;
}

BuildOrderQueue *BuildManager::getBuildQueue()
{
	return &buildQueue;
}

// 테란에게는 add on 건물을 지을 때 모건물이 존재하는가를 판단한다.
bool BuildManager::isProducerWillExist(UnitType producerType)
{
	if (producerType.isAddon()) {
		return true;
	}

	if (producerType == Terran_SCV) {
		if (S->completedUnitCount(Terran_SCV) < 1) {
			return false;
		}

		return true;
	}

	if (ConstructionManager::Instance().existConstructionQueueItem(producerType) ||
			S->incompleteUnitCount(producerType) >= 1 ||
			BuildManager::Instance().buildQueue.existItem(producerType)) {
		return true;
	}

	// 지어져있는 건물중에 addon 가능한 건물이 있나
	uList buildings = INFO.getTypeBuildingsInRadius(producerType, S, Positions::Origin, 0, false, false);

	for (auto b : buildings)
		if (b->unit()->canBuildAddon())
			return true;

	return false;
}

void BuildManager::checkBuildOrderQueueDeadlockAndAndFixIt()
{
	// 빌드오더를 수정할 수 있는 프레임인지 먼저 판단한다
	// this will be true if any unit is on the first frame if it's training time remaining
	// this can cause issues for the build order search system so don't plan a search on these frames
	bool canPlanBuildOrderNow = true;

	for (const auto &unit : S->getUnits()) {
		if (unit->getRemainingTrainTime() == 0) {
			continue;
		}

		//UnitType trainType = unit->getLastCommand().getUnitType();
		UnitCommand unitCommand = unit->getLastCommand();
		UnitCommandType unitCommandType = unitCommand.getType();

		if (unitCommandType != UnitCommandTypes::None) {
			if (unitCommand.getUnit() != nullptr) {
				UnitType trainType = unitCommand.getUnit()->getType();

				if (unit->getRemainingTrainTime() == trainType.buildTime()) {
					canPlanBuildOrderNow = false;
					break;
				}
			}
		}
	}

	if (!canPlanBuildOrderNow) {
		return;
	}

	// BuildQueue 의 HighestPriority 에 있는 BuildQueueItem 이 skip 불가능한 것인데, 선행조건이 충족될 수 없거나, 실행이 앞으로도 계속 불가능한 경우, dead lock 이 발생한다
	// 선행 건물을 BuildQueue에 추가해넣을지, 해당 BuildQueueItem 을 삭제할지 전략적으로 판단해야 한다
	BuildOrderQueue *buildQueue = BuildManager::Instance().getBuildQueue();

	if (!buildQueue->isEmpty())
	{
		BuildOrderItem currentItem = buildQueue->getHighestPriorityItem();
		//if (buildQueue->canSkipCurrentItem() == false)
		// blocking 이 false 이면 아무것도 안한다.
		bool isDeadlockCase = false;

		// producerType을 먼저 알아낸다
		UnitType producerType = currentItem.metaType.whatBuilds();

		// 건물이나 유닛의 경우, addon 포함
		if (currentItem.metaType.isUnit())
		{
			UnitType unitType = currentItem.metaType.getUnitType();
			TechType requiredTechType = unitType.requiredTech();
			const map< UnitType, int > &requiredUnits = unitType.requiredUnits();
			int requiredSupply = unitType.supplyRequired();

			// Refinery 건물의 경우, Refinery 가 건설되지 않은 Geyser가 있는 경우에만 가능
			if (unitType.isRefinery()) {
				bool hasAvailableGeyser = true;

				// Refinery가 지어질 수 있는 장소를 찾아본다
				TilePosition testLocation = getDesiredPosition(unitType, currentItem.seedLocation, currentItem.seedLocationStrategy);

				// Refinery 를 지으려는 장소를 찾을 수 없으면 dead lock
				if (!testLocation.isValid()) {
					cout << "Build Order Dead lock case -> Cann't find place to construct at " << currentItem.seedLocation << " Strategy " << currentItem.seedLocationStrategy << " " << unitType.getName() << endl;
					hasAvailableGeyser = false;
				}
				else {
					// Refinery 를 지으려는 장소에 Refinery 가 이미 건설되어 있다면 dead lock
					Unitset uot = Broodwar->getUnitsOnTile(testLocation);

					for (auto &u : uot) {
						if (u->getType().isRefinery() && u->exists()) {
							hasAvailableGeyser = false;

							cout << "Build Order Dead lock case -> Refinery Building was built already at " << testLocation.x << ", " << testLocation.y << endl;

							break;
						}
					}
				}

				if (!hasAvailableGeyser) {
					isDeadlockCase = true;
				}
			}

			// 선행 기술 리서치가 되어있지 않고, 리서치 중이지도 않으면 dead lock
			else if (requiredTechType != TechTypes::None)
			{
				if (S->hasResearched(requiredTechType) == false) {
					if (S->isResearching(requiredTechType) == false) {
						isDeadlockCase = true;
					}
				}
			}

			// 선행 건물/유닛이 있는데
			if (!isDeadlockCase && requiredUnits.size() > 0)
			{
				// # addon 건물일 경우 비어있는 모건물 체크
				// # addon 이면 모건물이 있어도 비어있는 모건물이어야 한다.
				// [TO DO] : 모건물이 공중에 떠있는 경우엔 어떻게 해야할지 정해야 한다.
				if (currentItem.metaType.getUnitType().isAddon()) {
					if (!isProducerWillExist(producerType)) {

						if (StrategyManager::Instance().decideToBuildWhenDeadLock(&producerType)) {
							cout << "producerType 강제 삽입 " << producerType.getName() << endl;
							buildQueue->queueAsHighestPriority(producerType, false);
						}
						else {
							isDeadlockCase = true;
						}
					}

				}

				// # 선행건물 체크
				// # addon 을 제외한 나머지 건물 + Comsat 및 Nuclear silo
				// # Comsat 과 nuclear silo 는 모건물 및 선행건물 둘다 필요한 특수 케이스
				if (!isDeadlockCase && !currentItem.metaType.getUnitType().isAddon()
						|| currentItem.metaType.getUnitType() == Terran_Comsat_Station
						|| currentItem.metaType.getUnitType() == Terran_Nuclear_Silo)
				{
					// nuclear silo의 경우, Covert ops 와 Facility 가 required Type이다.
					// 두 건물 모두 빌드 큐에 들어갈 필요가 없기 때문에 여기서 따로 처리한다.
					if (currentItem.metaType.getUnitType() == Terran_Nuclear_Silo)
					{
						if (S->completedUnitCount(Terran_Covert_Ops) == 0
								&& S->incompleteUnitCount(Terran_Covert_Ops) == 0)
						{
							// 선행 건물이 건설 예정이지도 않으면 dead lock
							if (!ConstructionManager::Instance().existConstructionQueueItem(Terran_Covert_Ops)) {
								UnitType unitType = Terran_Covert_Ops;

								if (StrategyManager::Instance().decideToBuildWhenDeadLock(&unitType)) {
									cout << "requiredUnitType 강제 삽입 " << Terran_Covert_Ops.getType << endl;
									buildQueue->queueAsHighestPriority(Terran_Covert_Ops, false);
								}
								else {
									isDeadlockCase = true;
								}
							}
						}
					}
					else
					{
						// Nuclear 를 제외한 나머지 건물들에 경우
						// 여기서 선행건물을 지을지 체크한다.
						for (auto &u : requiredUnits)
						{
							UnitType requiredUnitType = u.first;

							if (requiredUnitType != None) {

								// 선행 건물 / 유닛이 존재하지 않고, 생산 중이지도 않고
								if (S->completedUnitCount(requiredUnitType) == 0
										&& S->incompleteUnitCount(requiredUnitType) == 0)
								{
									// 선행 건물이 건설 예정이지도 않으면 dead lock
									if (requiredUnitType.isBuilding())
									{
										if (!ConstructionManager::Instance().existConstructionQueueItem(requiredUnitType)) {
											if (StrategyManager::Instance().decideToBuildWhenDeadLock(&requiredUnitType)) {
												cout << "requiredUnitType 강제 삽입 " << unitType.getName() << " : " << requiredUnitType.getName() << endl;
												buildQueue->queueAsHighestPriority(requiredUnitType, false);
											}
											else {
												isDeadlockCase = true;
											}
										}
									}
								}
							}
						}

					}

				}
			}

			// 건물이 아닌 지상/공중 유닛인 경우, 서플라이가 400 꽉 찼으면 dead lock
			if (!isDeadlockCase && !unitType.isBuilding()
					&& S->supplyTotal() == 400 && S->supplyUsed() + unitType.supplyRequired() > 400)
			{
				isDeadlockCase = true;
			}

			// 건물이 아닌 지상/공중 유닛인 경우, 서플라이가 부족하면 dead lock 이지만, 서플라이 부족하다고 지상/공중유닛 빌드를 취소하기보다는 빨리 서플라이를 짓도록 하기 위해,
			// 이것은 StrategyManager 등에서 따로 처리하도록 한다

		}
		// 테크의 경우, 해당 리서치를 이미 했거나, 이미 하고있거나, 리서치를 하는 건물 및 선행건물이 존재하지않고 건설예정이지도 않으면 dead lock
		else if (currentItem.metaType.isTech())
		{
			TechType techType = currentItem.metaType.getTechType();
			UnitType requiredUnitType = techType.requiredUnit();

			if (S->hasResearched(techType) || S->isResearching(techType)) {
				isDeadlockCase = true;
			}
			else if (S->completedUnitCount(producerType) == 0
					 && S->incompleteUnitCount(producerType) == 0)
			{

				if (!ConstructionManager::Instance().existConstructionQueueItem(producerType)) {

					// 테크 리서치의 producerType이 Addon 건물인 경우,
					// Addon 건물 건설이 명령 내려졌지만 시작되기 직전에는 getUnits, completedUnitCount, incompleteUnitCount 에서 확인할 수 없다
					// producerType의 producerType 건물에 의해 Addon 건물 건설의 명령이 들어갔는지까지 확인해야 한다
					if (producerType.isAddon()) {

						bool isAddonConstructing = false;

						UnitType producerTypeOfProducerType = producerType.whatBuilds().first;

						if (producerTypeOfProducerType != None) {

							for (auto &unit : S->getUnits())
							{
								if (unit == nullptr) continue;

								if (unit->getType() != producerTypeOfProducerType) continue;

								// 모건물이 완성되어있고, 모건물이 해당 Addon 건물을 건설중인지 확인한다
								if (unit->isCompleted() && unit->isConstructing() && unit->getBuildType() == producerType) {
									isAddonConstructing = true;
									break;
								}
							}
						}

						if (isAddonConstructing == false) {
							isDeadlockCase = true;
						}
					}
					else {
						isDeadlockCase = true;
					}

					if (isDeadlockCase) {
						if (StrategyManager::Instance().decideToBuildWhenDeadLock(&producerType)) {
							cout << "producerType 강제 삽입 " << currentItem.metaType.getName() << " : " << producerType.getName() << endl;
							buildQueue->queueAsHighestPriority(producerType, false);
							isDeadlockCase = false;
						}
					}

				}
			}
			else if (requiredUnitType != None) {

				if (S->completedUnitCount(requiredUnitType) == 0
						&& S->incompleteUnitCount(requiredUnitType) == 0) {
					if (!ConstructionManager::Instance().existConstructionQueueItem(requiredUnitType)) {
						isDeadlockCase = true;
					}
				}
			}
		}
		// 업그레이드의 경우, 해당 업그레이드를 이미 했거나, 이미 하고있거나, 업그레이드를 하는 건물 및 선행건물이 존재하지도 않고 건설예정이지도 않으면 dead lock
		else if (currentItem.metaType.isUpgrade())
		{
			UpgradeType upgradeType = currentItem.metaType.getUpgradeType();
			int maxLevel = S->getMaxUpgradeLevel(upgradeType);
			int currentLevel = S->getUpgradeLevel(upgradeType);
			UnitType requiredUnitType = upgradeType.whatsRequired();

			// 엔베와 아머리에서 진행되는 공방업의 경우, 사이언스 Facility가 있는지 체크해야한다.
			if ((producerType == Terran_Engineering_Bay ||
					producerType == Terran_Armory) &&
					currentLevel >= 1 ) {

				UnitType sf = Terran_Science_Facility;

				if (S->completedUnitCount(sf) == 0 &&
						S->incompleteUnitCount(sf) == 0 &&
						!BuildManager::Instance().buildQueue.existItem(sf) &&
						!ConstructionManager::Instance().existConstructionQueueItem(sf)) {
					// 사이언스 퍼실리티가 존재하지 않고, 현재 업그레이드가 블락킹 모드이면 데드락에 걸린다.
					if (StrategyManager::Instance().decideToBuildWhenDeadLock(&sf)) {
						cout << "Terran_Science_Facility 강제 삽입 " << endl;
						buildQueue->queueAsHighestPriority(sf, false);
					}
				}

			}

			if (currentLevel >= maxLevel || S->isUpgrading(upgradeType)) {
				isDeadlockCase = true;
			}
			else if (S->completedUnitCount(producerType) == 0
					 && S->incompleteUnitCount(producerType) == 0)  {
				if (!ConstructionManager::Instance().existConstructionQueueItem(producerType)) {
					// 업그레이드의 producerType이 Addon 건물인 경우, Addon 건물 건설이 시작되기 직전에는 getUnits, completedUnitCount, incompleteUnitCount 에서 확인할 수 없다
					// producerType의 producerType 건물에 의해 Addon 건물 건설이 시작되었는지까지 확인해야 한다
					if (producerType.isAddon()) {

						bool isAddonConstructing = false;

						UnitType producerTypeOfProducerType = producerType.whatBuilds().first;

						if (producerTypeOfProducerType != None) {

							for (auto &unit : S->getUnits())
							{
								if (unit == nullptr) continue;

								if (unit->getType() != producerTypeOfProducerType) {
									continue;
								}

								// 모건물이 완성되어있고, 모건물이 해당 Addon 건물을 건설중인지 확인한다
								if (unit->isCompleted() && unit->isConstructing() && unit->getBuildType() == producerType) {
									isAddonConstructing = true;
									break;
								}
							}
						}

						if (isAddonConstructing == false) {
							isDeadlockCase = true;
						}
					}
					else {
						isDeadlockCase = true;
					}

					if (isDeadlockCase) {
						if (StrategyManager::Instance().decideToBuildWhenDeadLock(&producerType)) {
							cout << "producerType 강제 삽입 " << producerType.getName() << endl;
							buildQueue->queueAsHighestPriority(producerType, false);
							isDeadlockCase = false;
						}
					}

				}
			}
			else if (requiredUnitType != None) {
				if (S->completedUnitCount(requiredUnitType) == 0
						&& S->incompleteUnitCount(requiredUnitType) == 0) {
					if (!ConstructionManager::Instance().existConstructionQueueItem(requiredUnitType)) {
						isDeadlockCase = true;
					}

				}
			}


		}

		if (!isDeadlockCase) {
			// producerID 를 지정했는데, 해당 ID 를 가진 유닛이 존재하지 않으면 dead lock
			if (currentItem.producerID != -1) {
				bool isProducerAlive = false;

				for (auto &unit : S->getUnits()) {
					if (unit != nullptr && unit->getID() == currentItem.producerID && unit->exists() && unit->getHitPoints() > 0) {
						isProducerAlive = true;
						break;
					}
				}

				if (isProducerAlive == false) {
					isDeadlockCase = true;
				}
			}
		}

		if (isDeadlockCase) {
			buildQueue->removeCurrentItem();
		}
	}

}

int BuildManager::getFirstItemMineral()
{
	int mineral = 0;

	if (!buildQueue.isEmpty())
	{
		BuildOrderItem currentItem = buildQueue.getHighestPriorityItem();

		// 커맨드, 업그레이드일때만 자원 세이브
		if (currentItem.metaType.isUpgrade() || currentItem.metaType.isTech()
				|| (currentItem.metaType.isUnit()
					&& (currentItem.metaType.getUnitType() == Terran_Command_Center || currentItem.metaType.getUnitType() == Terran_Engineering_Bay)))
			mineral = currentItem.metaType.mineralPrice();
	}

	return mineral;
}

int BuildManager::getFirstItemGas()
{
	int gas = 0;

	if (!buildQueue.isEmpty())
	{
		BuildOrderItem currentItem = buildQueue.getHighestPriorityItem();

		if (currentItem.metaType.isUpgrade() || currentItem.metaType.isTech())
			gas = currentItem.metaType.gasPrice();
	}

	return gas;
}



