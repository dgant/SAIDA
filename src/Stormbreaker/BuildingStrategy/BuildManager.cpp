#include "BuildManager.h"
#include "../ActorCriticForProductionStrategy.h"
using namespace MyBot;

BuildManager::BuildManager()
{
}

void BuildManager::update()
{
	if (Broodwar->getFrameCount() % 6 != 0) return;
	try{
		string chosenaction = ActorCriticForProductionStrategy::Instance().getTargetUnit();
//		Broodwar << "chosenaction:" << chosenaction.c_str() << endl;

	}
	catch (...)
	{
//		Broodwar << "No chosenaction" << endl;
	}

	if (buildQueue.isEmpty()) {
		return;
	}
	clock_t str = clock();
	checkBuildOrderQueueDeadlockAndAndFixIt();

	if (buildQueue.isEmpty()) {
		return;
	}
	BuildOrderItem currentItem = buildQueue.getHighestPriorityItem();

	while (!buildQueue.isEmpty())
	{
		bool isOkToRemoveQueue = true;

		Position seedPosition = Positions::None;

		if (currentItem.seedLocation != TilePositions::None && currentItem.seedLocation != TilePositions::Invalid
				&& currentItem.seedLocation != TilePositions::Unknown && currentItem.seedLocation.isValid()) {
			seedPosition = Position(currentItem.seedLocation);
		}
		else {
			seedPosition = (Position)BuildingPlaceFinder::Instance().getSeedPositionFromSeedLocationStrategy(currentItem.seedLocationStrategy);
		}

		Unit producer = getProducer(currentItem.metaType, seedPosition, currentItem.producerID);



		Unit secondProducer = nullptr;
		bool canMake;


		if (producer != nullptr) {
		
			canMake = canMakeNow(producer, currentItem.metaType);
		}

		// if we can make the current item, create it
		if (producer != nullptr && canMake == true)
		{
			MetaType t = currentItem.metaType;

			if (t.isUnit())
			{
				if (t.getUnitType().isBuilding()) {

					if (t.getUnitType().isAddon()) {


						producer->buildAddon(t.getUnitType());
						if (producer->isConstructing() == false) {
							isOkToRemoveQueue = false;
						}

					}
					else
					{
						
						TilePosition desiredPosition = getDesiredPosition(t.getUnitType(), currentItem.seedLocation, currentItem.seedLocationStrategy);

		

						if (desiredPosition != TilePositions::None) {
				
							ConstructionManager::Instance().addConstructionTask(t.getUnitType(), desiredPosition);
						}
						else {

							cout << "There is no place to construct " << currentItem.metaType.getUnitType().getName().c_str()
								 << " strategy " << currentItem.seedLocationStrategy
								 << " seedPosition " << currentItem.seedLocation.x << "," << currentItem.seedLocation.y
								 << " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << endl;

							isOkToRemoveQueue = false;
						}
					}
				}
			
				else {
			
					producer->train(t.getUnitType());
				}
			}

			else if (t.isTech())
			{
				producer->research(t.getTechType());
			}
			else if (t.isUpgrade())
			{
				producer->upgrade(t.getUpgradeType());
			}

			if (isOkToRemoveQueue) {
				buildQueue.removeCurrentItem();
			}

			break;
		}
		else if (buildQueue.canSkipCurrentItem())
		{
		
			buildQueue.skipCurrentItem();
			currentItem = buildQueue.getNextItem();
		}
		else
		{
		
			break;
		}
	}
}

Unit BuildManager::getProducer(MetaType t, Position closestTo, int producerID)
{

	UnitType producerType = t.whatBuilds();

	Unitset candidateProducers;

	for (auto &unit : S->getUnits())
	{
		if (unit == nullptr) continue;

	
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


		if (t.getUnitType().isAddon())
		{
			
			if (unit->getAddon() != nullptr) {
				continue;
			}


			if (!unit->canBuildAddon()) {
				continue;
			}


			if (unit->getLastCommand().getType() == UnitCommandTypes::Build_Addon
					&& (Broodwar->getFrameCount() - unit->getLastCommandFrame() < 10))
			{
				continue;
			}

			bool isBlocked = false;

			TilePosition addonPosition(
				unit->getTilePosition().x + unit->getType().tileWidth(),
				unit->getTilePosition().y + unit->getType().tileHeight() - t.getUnitType().tileHeight());

			for (int i = 0; i < t.getUnitType().tileWidth(); ++i)
			{
				for (int j = 0; j < t.getUnitType().tileHeight(); ++j)
				{
					TilePosition tilePos(addonPosition.x + i, addonPosition.y + j);


					if (!Broodwar->isBuildable(tilePos, true))
					{
						isBlocked = true;
						break;
					}



					Unitset uot = Broodwar->getUnitsOnTile(tilePos.x, tilePos.y);

					for (auto &u : uot) {
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


		if (!closestUnit || distance < minDist)
		{
			closestUnit = unit;
			minDist = distance;
		}
	}

	return closestUnit;
}

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
			for (auto &val : t.getUnitType().requiredUnits()) {
				if (!INFO.getAllCount(val.first, S)) {
					canMake = false;
					break;
				}
			}
		}
		else if (t.isUnit())
		{
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


TilePosition BuildManager::getDesiredPosition(UnitType unitType, TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy)
{
	TilePosition desiredPosition = BuildingPlaceFinder::Instance().getBuildLocationWithSeedPositionAndStrategy(unitType, seedPosition, seedPositionStrategy);

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


		if (findAnotherPlace) {
			desiredPosition = BuildingPlaceFinder::Instance().getBuildLocationWithSeedPositionAndStrategy(unitType, seedPosition, seedPositionStrategy);

		}

		else {
			break;
		}
	}

	return desiredPosition;
}


int BuildManager::getAvailableMinerals()
{
	return S->minerals() - ConstructionManager::Instance().getReservedMinerals();
}


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
		mineralRate -= 0.01 * SoldierManager::Instance().getAllMineralScvCount();
		gasRate -= 0.02 * SoldierManager::Instance().getAllRefineryScvCount();
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

	
	uList buildings = INFO.getTypeBuildingsInRadius(producerType, S, Positions::Origin, 0, false, false);

	for (auto b : buildings)
		if (b->unit()->canBuildAddon())
			return true;

	return false;
}

void BuildManager::checkBuildOrderQueueDeadlockAndAndFixIt()
{
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

	BuildOrderQueue *buildQueue = BuildManager::Instance().getBuildQueue();

	if (!buildQueue->isEmpty())
	{
		BuildOrderItem currentItem = buildQueue->getHighestPriorityItem();
		bool isDeadlockCase = false;

		UnitType producerType = currentItem.metaType.whatBuilds();

		if (currentItem.metaType.isUnit())
		{
			UnitType unitType = currentItem.metaType.getUnitType();
			TechType requiredTechType = unitType.requiredTech();
			const map< UnitType, int > &requiredUnits = unitType.requiredUnits();
			int requiredSupply = unitType.supplyRequired();

			if (unitType.isRefinery()) {
				bool hasAvailableGeyser = true;

				TilePosition testLocation = getDesiredPosition(unitType, currentItem.seedLocation, currentItem.seedLocationStrategy);

				if (!testLocation.isValid()) {
					cout << "Build Order Dead lock case -> Cann't find place to construct at " << currentItem.seedLocation << " Strategy " << currentItem.seedLocationStrategy << " " << unitType.getName() << endl;
					hasAvailableGeyser = false;
				}
				else {
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

			else if (requiredTechType != TechTypes::None)
			{
				if (S->hasResearched(requiredTechType) == false) {
					if (S->isResearching(requiredTechType) == false) {
						isDeadlockCase = true;
					}
				}
			}

			if (!isDeadlockCase && requiredUnits.size() > 0)
			{
				if (currentItem.metaType.getUnitType().isAddon()) {
					if (!isProducerWillExist(producerType)) {

						if (StrategyManager::Instance().decideToBuildWhenDeadLock(&producerType)) {
							buildQueue->queueAsHighestPriority(producerType, false);
						}
						else {
							isDeadlockCase = true;
						}
					}

				}


				if (!isDeadlockCase && !currentItem.metaType.getUnitType().isAddon()
						|| currentItem.metaType.getUnitType() == Terran_Comsat_Station
						|| currentItem.metaType.getUnitType() == Terran_Nuclear_Silo)
				{

					if (currentItem.metaType.getUnitType() == Terran_Nuclear_Silo)
					{
						if (S->completedUnitCount(Terran_Covert_Ops) == 0
								&& S->incompleteUnitCount(Terran_Covert_Ops) == 0)
						{
							if (!ConstructionManager::Instance().existConstructionQueueItem(Terran_Covert_Ops)) {
								UnitType unitType = Terran_Covert_Ops;

								if (StrategyManager::Instance().decideToBuildWhenDeadLock(&unitType)) {
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
						for (auto &u : requiredUnits)
						{
							UnitType requiredUnitType = u.first;

							if (requiredUnitType != None) {

								
								if (S->completedUnitCount(requiredUnitType) == 0
										&& S->incompleteUnitCount(requiredUnitType) == 0)
								{
								
									if (requiredUnitType.isBuilding())
									{
										if (!ConstructionManager::Instance().existConstructionQueueItem(requiredUnitType)) {
											if (StrategyManager::Instance().decideToBuildWhenDeadLock(&requiredUnitType)) {
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

	
			if (!isDeadlockCase && !unitType.isBuilding()
					&& S->supplyTotal() == 400 && S->supplyUsed() + unitType.supplyRequired() > 400)
			{
				isDeadlockCase = true;
			}



		}
	
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

					if (producerType.isAddon()) {

						bool isAddonConstructing = false;

						UnitType producerTypeOfProducerType = producerType.whatBuilds().first;

						if (producerTypeOfProducerType != None) {

							for (auto &unit : S->getUnits())
							{
								if (unit == nullptr) continue;

								if (unit->getType() != producerTypeOfProducerType) continue;

				
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
		else if (currentItem.metaType.isUpgrade())
		{
			UpgradeType upgradeType = currentItem.metaType.getUpgradeType();
			int maxLevel = S->getMaxUpgradeLevel(upgradeType);
			int currentLevel = S->getUpgradeLevel(upgradeType);
			UnitType requiredUnitType = upgradeType.whatsRequired();

			if ((producerType == Terran_Engineering_Bay ||
					producerType == Terran_Armory) &&
					currentLevel >= 1 ) {

				UnitType sf = Terran_Science_Facility;

				if (S->completedUnitCount(sf) == 0 &&
						S->incompleteUnitCount(sf) == 0 &&
						!BuildManager::Instance().buildQueue.existItem(sf) &&
						!ConstructionManager::Instance().existConstructionQueueItem(sf)) {
					if (StrategyManager::Instance().decideToBuildWhenDeadLock(&sf)) {
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



