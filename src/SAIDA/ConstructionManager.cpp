#include "ConstructionManager.h"
#include "../UnitManager/TankManager.h"

using namespace MyBot;

ConstructionManager::ConstructionManager()
	: reservedMinerals(0)
	, reservedGas(0)
{
	memset(time, 0, sizeof(time));
	constructionQueue.reserve(20);
}

void ConstructionManager::addConstructionTask(UnitType type, TilePosition desiredPosition, Unit builder)
{
	if (type == None || type == UnitTypes::Unknown || !desiredPosition.isValid())
		return ;

	ReserveBuilding::Instance().assignTiles(desiredPosition, type);

	ConstructionTask b(type, desiredPosition, builder);

	// reserve resources
	reservedMinerals += type.mineralPrice();
	reservedGas += type.gasPrice();

	constructionQueue.push_back(b);
}


void ConstructionManager::cancelConstructionTask(ConstructionTask ct) {
	if (ct.status != ConstructionStatus::UnderConstruction) {
		cout << "cancelConstructionTask" << endl;
		cancelNotUnderConstructionTasks({ ct });
	}
	else {
		if (ct.buildingUnit && ct.buildingUnit->exists() && ct.buildingUnit->canCancelConstruction())
			ct.buildingUnit->cancelConstruction();

		removeCompletedConstructionTasks({ ct });
	}
}

void ConstructionManager::removeCompletedConstructionTasks(const vector<ConstructionTask> &toRemove)
{
	for (auto &b : toRemove)
	{
		auto &it = find(constructionQueue.begin(), constructionQueue.end(), b);

		if (it != constructionQueue.end())
		{
			if (S->getRace() == Races::Terran && it->constructionWorker)
				SoldierManager::Instance().setScvIdleState(it->constructionWorker);

			ReserveBuilding::Instance().unassignTiles(it->finalPosition, it->type);

			constructionQueue.erase(it);
		}
	}
}

void ConstructionManager::cancelNotUnderConstructionTasks(const vector<ConstructionTask> &toRemove) {
	for (auto &b : toRemove)
	{
		auto &it = find(constructionQueue.begin(), constructionQueue.end(), b);

		if (it != constructionQueue.end())
		{
			reservedMinerals -= it->type.mineralPrice();
			reservedGas -= it->type.gasPrice();

			TilePosition desiredPosition = it->status == ConstructionStatus::Assigned ? it->finalPosition : it->desiredPosition;

			cout << "[" << TIME << "] Cancel Construction " << it->type << " at " << desiredPosition << endl;


			if (S->getRace() == Races::Terran && it->constructionWorker) {
				SoldierManager::Instance().setScvIdleState(it->constructionWorker);
			}

			if (it->type == Terran_Factory || it->type == Terran_Supply_Depot || it->type == Terran_Barracks || it->type == Terran_Armory || it->type == Terran_Academy || it->type == Terran_Starport || it->type == Terran_Science_Facility)
				ReserveBuilding::Instance().unassignTiles(desiredPosition, it->type);
			else
				ReserveBuilding::Instance().freeTiles(desiredPosition, it->type, true);

			constructionQueue.erase(it);
		}
	}
}

void ConstructionManager::update()
{
	int i = 0;
	time[i] = clock();

	try {
		validateWorkersAndBuildings();
	}
	catch (...) {
	}

	time[++i] = clock();


	try {
		assignWorkersToUnassignedBuildings();
	}
	catch (...) {
	}

	time[++i] = clock();


	try {
		checkForStartedConstruction();
	}
	
	catch (...) {
	}

	time[++i] = clock();


	try {
		constructAssignedBuildings();
	}
	
	catch (...) {;
	}

	time[++i] = clock();


	try {
		checkForDeadTerranBuilders();
	}
	
	catch (...) {
	}

	time[++i] = clock();


	try {
		checkForCompletedBuildings();
	}
	
	catch (...) {
	}

	time[++i] = clock();


	try {
		checkForDeadlockConstruction();
	}
	
	catch (...) {;
	}

	time[++i] = clock();

}

void ConstructionManager::validateWorkersAndBuildings()
{
	vector<Unit> cancelBuildings;

	for (auto &b : INFO.getBuildings(S)) {
		if (!b.second->isComplete())
		{
			int damage = 1;

			for (auto enemy : b.second->getEnemiesTargetMe()) {
				if (enemy->isInWeaponRange(b.second->unit()) && enemy->isAttacking())
					damage += getDamage(enemy, b.second->unit());
				else if (!enemy->isDetected() && enemy->isAttacking() && enemy->getType().groundWeapon().targetsGround()
						 && getAttackDistance(enemy, b.second->unit()) <= UnitUtil::GetAttackRange(enemy, false))
					damage += getDamage(enemy->getType(), b.second->type(), E, S);
			}

			if (b.second->hp() <= damage) {
				b.second->unit()->cancelConstruction();
				cancelBuildings.push_back(b.first);
			}
		}
	}

	vector<ConstructionTask> toRemove;

	for (auto &b : constructionQueue)
	{
		if (b.status == ConstructionStatus::UnderConstruction)
		{
			if (b.buildingUnit == nullptr || !b.buildingUnit->getType().isBuilding() || !b.buildingUnit->exists())
			{
				cout << "Construction Failed case -> remove ConstructionTask " << b.type.getName() << endl;

				toRemove.push_back(b);
			}

			auto &iter = find(cancelBuildings.begin(), cancelBuildings.end(), b.buildingUnit);

			if (iter != cancelBuildings.end()) {
				toRemove.push_back(b);
			}
		}
	}

	removeCompletedConstructionTasks(toRemove);
}

void ConstructionManager::assignWorkersToUnassignedBuildings()
{
	vector<ConstructionTask> toRemove;

	for (ConstructionTask &b : constructionQueue)
	{
		if (b.status != ConstructionStatus::Unassigned) {
			if (b.status == ConstructionStatus::WaitToAssign && b.startWaitingFrame + 24 * 30 < TIME)
				b.status = ConstructionStatus::Unassigned;

			continue;
		}

		UnitInfo *closestScv = nullptr;
		Unit workerToAssign = nullptr;

		if (b.desiredPosition.isValid()) {
			closestScv = SoldierManager::Instance().chooseScvClosestTo((Position)b.desiredPosition, b.lastConstructionWorkerID);

			if (closestScv != nullptr) {
				workerToAssign = closestScv->unit();
			}
		}

		TilePosition testLocation;

		if (!SelfBuildingPlaceFinder::Instance().isFixedPositionOrder(b.type)) {
		
			testLocation = BuildingPlaceFinder::Instance().getBuildLocationNear(b.type, b.desiredPosition, false, workerToAssign);

	
			if (!testLocation.isValid()) {
				bool forceAssign = b.type.isResourceDepot();

				if (forceAssign) {
					for (auto u : bw->getUnitsInRectangle((Position)b.desiredPosition, (Position)(b.desiredPosition + b.type.tileSize()))) {
						if (u->getPlayer() != S || u->getType() != Terran_Vulture_Spider_Mine) {
							forceAssign = false;
							break;
						}
					}
				}

				if (forceAssign)
					testLocation = b.desiredPosition;
				else {
					cout << "assignWorkersToUnassignedBuildings - testLocation" << endl;
					cancelNotUnderConstructionTasks({ b });
					continue;
				}
			}
		}
		else
			testLocation = b.desiredPosition;

		if (closestScv == nullptr) {
			closestScv = SoldierManager::Instance().chooseScvClosestTo((Position)testLocation, b.lastConstructionWorkerID);
		}

		workerToAssign = SoldierManager::Instance().setStateToSCV(closestScv, new ScvBuildState());

		if (workerToAssign)
		{
			for (auto &cQueue : constructionQueue) {
				if (cQueue.status == ConstructionStatus::Assigned && cQueue.type == b.type && cQueue.finalPosition == testLocation) {
					toRemove.push_back(cQueue);
					break;
				}
			}


			b.constructionWorker = workerToAssign;

			b.finalPosition = testLocation;

			b.status = ConstructionStatus::Assigned;

			b.lastConstructionWorkerID = b.constructionWorker->getID();

			b.buildCommandGiven = false;
			b.assignFrame = TIME;
			b.retryCnt = 0;

			b.startWaitingFrame = 0;

			b.minDist = INT_MAX;
			b.minDistTime = 0;
			b.minDistPosition = Positions::None;
			b.minDistVisitCnt = 0;

			ReserveBuilding::Instance().assignTiles(b.finalPosition, b.type, b.constructionWorker);


			if (INFO.getSecondExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getSecondExpansionLocation(S)->getTilePosition()) < 4) {
				TM.assignSCV(1, b.constructionWorker);
			}
			else if (INFO.getThirdExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getThirdExpansionLocation(S)->getTilePosition()) < 4) {
				TM.assignSCV(2, b.constructionWorker);
			}
		}
	}

	if (!toRemove.empty()) {
		cout << "assignWorkersToUnassignedBuildings" << endl;
		cancelNotUnderConstructionTasks(toRemove);
	}
}


void ConstructionManager::constructAssignedBuildings()
{
	for (auto &b : constructionQueue)
	{
		if (b.status != ConstructionStatus::Assigned)
		{
			continue;
		}


		if (b.constructionWorker == nullptr || b.constructionWorker->exists() == false)
		{
			SoldierManager::Instance().setScvIdleState(b.constructionWorker);

			b.constructionWorker = nullptr;

			b.buildCommandGiven = false;

			b.finalPosition = TilePositions::None;

			b.startWaitingFrame = TIME;
			b.assignFrame = 0;

			b.status = ConstructionStatus::WaitToAssign;

			if (INFO.getSecondExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getSecondExpansionLocation(S)->getTilePosition()) < 4) {
				TM.assignSCV(1, nullptr);
			}
			else if (INFO.getThirdExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getThirdExpansionLocation(S)->getTilePosition()) < 4) {
				TM.assignSCV(2, nullptr);
			}

			continue;
		}
		else if ((b.buildCommandGiven && b.lastBuildCommandGivenFrame + 24 * 60 < TIME)
		
				 || (b.assignFrame + 24 * 120 < TIME && b.constructionWorker->getPosition().getApproxDistance((Position)b.finalPosition) > 4 * TILE_SIZE)) {
			SoldierManager::Instance().setScvIdleState(b.constructionWorker);

			b.constructionWorker = nullptr;

			b.buildCommandGiven = false;
			b.assignFrame = 0;

			b.finalPosition = TilePositions::None;

			b.status = ConstructionStatus::Unassigned;

			if (INFO.getSecondExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getSecondExpansionLocation(S)->getTilePosition()) < 4) {
				TM.assignSCV(1, nullptr);
			}
			else if (INFO.getThirdExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getThirdExpansionLocation(S)->getTilePosition()) < 4) {
				TM.assignSCV(2, nullptr);
			}

			continue;
		}

	
		Position destination = (Position)b.finalPosition + (Position)b.type.tileSize() / 2;
		double dist = b.constructionWorker->getPosition().getDistance(destination);

		if (b.minDist > dist) {
			b.minDist = dist;
			b.minDistTime = TIME;
			b.minDistPosition = destination;
			b.minDistVisitCnt = 0;
		}
		else if (b.minDist == dist && b.minDistPosition == destination) {
			if (b.minDistTime + 10 < TIME) {
			
				if (++b.minDistVisitCnt >= 3) {
					
					TilePosition topLeft = ReserveBuilding::Instance().getReservedPositionAtWorker(b.type, b.constructionWorker);

					
					if (topLeft.isValid() && bw->canBuildHere(topLeft, b.type, b.constructionWorker)) {
						b.finalPosition = topLeft;

						
						b.buildCommandGiven = CommandUtil::build(b.constructionWorker, b.type, b.finalPosition);

						b.lastBuildCommandGivenFrame = TIME;

						continue;
					}

					SoldierManager::Instance().setStateToSCV(INFO.getUnitInfo(b.constructionWorker, S), new ScvPrisonedState());

		
					b.constructionWorker = nullptr;

					b.finalPosition = TilePositions::None;

					b.buildCommandGiven = false;
					b.assignFrame = 0;

			
					b.status = ConstructionStatus::Unassigned;

				
					if (INFO.getSecondExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getSecondExpansionLocation(S)->getTilePosition()) < 4) {
						TM.assignSCV(1, nullptr);
					}
					else if (INFO.getThirdExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getThirdExpansionLocation(S)->getTilePosition()) < 4) {
						TM.assignSCV(2, nullptr);
					}

					continue;
				}

			
			}

			b.minDistTime = TIME;
		}

	
		if (INFO.enemyRace == Races::Terran) {

			if (INFO.getSecondExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getSecondExpansionLocation(S)->getTilePosition()) < 4
					&& b.constructionWorker->getPosition().getApproxDistance((Position)b.desiredPosition) > 15 * TILE_SIZE) {
				UnitInfo *farthest = nullptr;
				UnitInfo *closest = nullptr;
				int distMax = INT_MIN;
				int distMin = INT_MAX;

				for (auto tank : TM.getKeepMultiTanks(1)) {
					int tmpDist = tank->unit()->getDistance(b.constructionWorker);


					if (distMax < tmpDist) {
						distMax = tmpDist;
						farthest = tank;
					}

					if (distMin > tmpDist && tmpDist > 3 * TILE_SIZE) {
						distMin = tmpDist;
						closest = tank;
					}
				}

				closest = closest ? closest : farthest;

				
				if (closest)
				{
					CommandUtil::rightClick(b.constructionWorker, closest->unit(), TIME % 24 == 0);
				}
				
				else {
					Position pos = INFO.getFirstExpansionLocation(S) ? INFO.getSecondChokePosition(S) : INFO.getMainBaseLocation(S)->Center();

					CommandUtil::move(b.constructionWorker, pos);
				}

				continue;
			}
			else if (INFO.getThirdExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getThirdExpansionLocation(S)->getTilePosition()) < 4
					 && b.constructionWorker->getPosition().getApproxDistance((Position)b.desiredPosition) > 15 * TILE_SIZE) {
				UnitInfo *farthest = nullptr;
				UnitInfo *closest = nullptr;
				int distMax = INT_MIN;
				int distMin = INT_MAX;

				for (auto tank : TM.getKeepMultiTanks(2)) {
					int tmpDist = tank->unit()->getDistance(b.constructionWorker);

				
					if (distMax < tmpDist) {
						distMax = tmpDist;
						farthest = tank;
					}

					if (distMin > tmpDist && tmpDist > 3 * TILE_SIZE) {
						distMin = tmpDist;
						closest = tank;
					}
				}

				closest = closest ? closest : farthest;

		
				if (closest)
					CommandUtil::rightClick(b.constructionWorker, closest->unit(), TIME % 24 == 0);
			
				else {
					Position pos = INFO.getFirstExpansionLocation(S) ? INFO.getSecondChokePosition(S) : INFO.getMainBaseLocation(S)->Center();

					CommandUtil::rightClick(b.constructionWorker, pos);
				}

				continue;
			}
		}


		if (b.type.isResourceDepot()) {
		
			if (b.removeBurrowedUnit) {
				UnitInfo *burrowed = INFO.getClosestTypeUnit(E, b.constructionWorker->getPosition(), Terran_Vulture_Spider_Mine, 5 * TILE_SIZE);

				if (burrowed)
					CommandUtil::attackUnit(b.constructionWorker, burrowed->unit());
				else if (b.lastBuildCommandGivenFrame + 60 < TIME) {
					
					b.removeBurrowedUnit = false;
					b.buildCommandGiven = CommandUtil::build(b.constructionWorker, b.type, b.finalPosition);
					b.lastBuildCommandGivenFrame = TIME;
				}

				continue;
			}


			if (b.desiredPosition.isValid()) {
				const Area *area = theMap.GetArea(b.desiredPosition);

				Base *nearestBase = nullptr;
				int minDist = INT_MAX;

				for (auto &base : area->Bases()) {
					int dist = base.getTilePosition().getApproxDistance(b.desiredPosition);

					if (minDist > dist) {
						minDist = dist;
						nearestBase = (Base *)&base;
					}
				}

				if (nearestBase->getTilePosition() != b.desiredPosition && bw->canBuildHere(nearestBase->getTilePosition(), b.type, b.constructionWorker)) {
					if (b.buildCommandGiven && b.constructionWorker->isInterruptible()) {
						ReserveBuilding::Instance().modifyReservePosition(b.type, b.desiredPosition, nearestBase->getTilePosition());
						b.finalPosition = nearestBase->getTilePosition();
						b.desiredPosition = nearestBase->getTilePosition();
						b.buildCommandGiven = CommandUtil::build(b.constructionWorker, b.type, b.finalPosition);
						b.lastBuildCommandGivenFrame = TIME;

					}

					continue;
				}
			}

	
			Unitset us = bw->getUnitsInRectangle((Position)b.finalPosition, (Position)(b.finalPosition + b.type.tileSize()));

			for (auto u : us) {
				if (u->getPlayer() == S && u->getType() == Terran_Vulture_Spider_Mine) {
					CommandUtil::attackUnit(b.constructionWorker, u);
					break;
				}
			}
		}


		if (b.constructionWorker->isConstructing() == false)
		{

			if (!isBuildingPositionExplored(b))
			{
				CommandUtil::rightClick(b.constructionWorker, Position(b.finalPosition) + (Position)b.type.tileSize() / 2);
			}
			else if (b.buildCommandGiven == false)
			{

				b.buildCommandGiven = CommandUtil::build(b.constructionWorker, b.type, b.finalPosition);

				b.lastBuildCommandGivenFrame = TIME;

				b.lastConstructionWorkerID = b.constructionWorker->getID();
			}

			else if (b.lastBuildCommandGivenFrame + 24 < TIME)
			{
		
				if (b.type.mineralPrice() > S->minerals() || b.type.gasPrice() > S->gas()) {
		
					if (b.constructionWorker->getDistance((Position)b.finalPosition) > TILE_SIZE) {
						continue;
					}
				}

				bool skip = false;

	
				Unitset us = Broodwar->getUnitsInRectangle((Position)b.finalPosition + Position(1, 1), (Position)(b.finalPosition + b.type.tileSize()) - Position(1, 1));

				for (auto u : us) {
					if (u->getID() != b.constructionWorker->getID() && u->getType() != Resource_Vespene_Geyser && !u->isFlying()) {
						if (b.startWaitingFrame == 0) {
							b.startWaitingFrame = TIME;
							
						}

						if (!u->getType().isBuilding()) {
							UnitInfo *ui = INFO.getUnitInfo(u, S);

							if (ui == nullptr) {
							
								if (u->getType().isWorker() && b.startWaitingFrame + 24 * 10 >= TIME) {
									skip = true;
									break;
								}
								else
									continue;
							}

							
							if (ui->type() == Terran_Vulture_Spider_Mine)
								ui->setKillMe(MustKill);
							
							else if (ui->getState() == "Build" || ui->getState() == "Mineral") {
								if (b.startWaitingFrame + 24 * 10 >= TIME) {
									skip = true;
									break;
								}
							}
						}
					}
				}

				if (skip)
					continue;

				
				if (b.retryCnt > 5) {
					TilePosition topLeft = ReserveBuilding::Instance().getReservedPositionAtWorker(b.type, b.constructionWorker);

				
					if (topLeft.isValid() && bw->canBuildHere(topLeft, b.type, b.constructionWorker)) {
						
						b.finalPosition = topLeft;  
						b.buildCommandGiven = CommandUtil::build(b.constructionWorker, b.type, b.finalPosition);

						b.lastBuildCommandGivenFrame = TIME;

						continue;
					}

					if (b.type.isResourceDepot() && INFO.enemyRace == Races::Terran) {
						Unitset uSet = bw->getUnitsInRectangle((Position)b.finalPosition, (Position)(b.finalPosition + b.type.tileSize()));

						if (uSet.size() == 1) {
							for (auto u : uSet) {
								if (u->getID() == b.constructionWorker->getID()) {
									b.removeBurrowedUnit = true;

									if (INFO.getAvailableScanCount() > 0)
										ComsatStationManager::Instance().useScan((Position)b.finalPosition + ((Position)b.type.tileSize() / 2));

									break;
								}
							}
						}

						if (b.removeBurrowedUnit) {
							b.retryCnt = 0;
							b.buildCommandGiven = false;
							b.lastBuildCommandGivenFrame = TIME;
							continue;
						}
					}

					SoldierManager::Instance().setScvIdleState(b.constructionWorker);

			
					b.constructionWorker = nullptr;

		
					b.finalPosition = TilePositions::None;

					b.buildCommandGiven = false;
					b.assignFrame = 0;

				
					b.status = ConstructionStatus::Unassigned;

				
					if (INFO.getSecondExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getSecondExpansionLocation(S)->getTilePosition()) < 4) {
						TM.assignSCV(1, nullptr);
					}
					else if (INFO.getThirdExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getThirdExpansionLocation(S)->getTilePosition()) < 4) {
						TM.assignSCV(2, nullptr);
					}
				}
				else if (b.constructionWorker->isInterruptible()) {
					b.retryCnt++;
					b.buildCommandGiven = CommandUtil::build(b.constructionWorker, b.type, b.finalPosition);

					b.lastBuildCommandGivenFrame = TIME;

					
				}
			}
		}
	}
}

// STEP 4: UPDATE DATA STRUCTURES FOR BUILDINGS STARTING CONSTRUCTION
void ConstructionManager::checkForStartedConstruction()
{
	// for each building unit which is being constructed
	for (auto &buildingThatStartedConstruction : S->getUnits())
	{
		// filter out units which aren't buildings under construction
		if (!buildingThatStartedConstruction->getType().isBuilding() || !buildingThatStartedConstruction->isBeingConstructed())
		{
			continue;
		}

		for (auto &b : constructionQueue)
		{
			if (b.status != ConstructionStatus::Assigned)
			{
				continue;
			}

			if (b.finalPosition == buildingThatStartedConstruction->getTilePosition() && b.type == buildingThatStartedConstruction->getType())
			{

				reservedMinerals -= buildingThatStartedConstruction->getType().mineralPrice();
				reservedGas      -= buildingThatStartedConstruction->getType().gasPrice();

				b.buildingUnit = buildingThatStartedConstruction;

				if (S->getRace() == Races::Zerg)
				{
					b.constructionWorker = nullptr;
				}
				else if (S->getRace() == Races::Protoss)
				{
					SoldierManager::Instance().setScvIdleState(b.constructionWorker);

					b.constructionWorker = nullptr;
				}

				b.status = ConstructionStatus::UnderConstruction;

				break;
			}
		}
	}
}

void ConstructionManager::checkForDeadTerranBuilders()
{
	if (INFO.selfRace == Races::Terran) {

		if (S->completedUnitCount(Terran_SCV) <= 0) return;

		for (auto &b : constructionQueue)
		{

			if (b.status == ConstructionStatus::UnderConstruction) {

				if (b.buildingUnit->isCompleted()) continue;

				if (b.constructionWorker == nullptr || b.constructionWorker->exists() == false) {

					Unit workerToAssign = SoldierManager::Instance().chooseConstuctionScvClosestTo(b.finalPosition, b.lastConstructionWorkerID);

					if (workerToAssign)
					{
		

						b.constructionWorker = workerToAssign;



						CommandUtil::rightClick(b.constructionWorker, b.buildingUnit);

						b.buildCommandGiven = true;

						b.lastBuildCommandGivenFrame = TIME;

						b.lastConstructionWorkerID = b.constructionWorker->getID();
					}
				}
	
				else if (!b.constructionWorker->isConstructing() && b.lastBuildCommandGivenFrame + 10 < TIME) {
					CommandUtil::rightClick(b.constructionWorker, b.buildingUnit);

					b.lastBuildCommandGivenFrame = TIME;
				}
			}
		}
	}

}

void ConstructionManager::checkForCompletedBuildings()
{
	vector<ConstructionTask> toRemove;


	for (auto &b : constructionQueue)
	{
		if (b.status != ConstructionStatus::UnderConstruction)
		{
			continue;
		}


		if (b.buildingUnit->isCompleted())
		{

			if (b.constructionWorker == nullptr || !b.constructionWorker->exists() || INFO.getUnitInfo(b.constructionWorker, S)->getLastPositionTime() + 5 < TIME)
				toRemove.push_back(b);
		}
	}

	removeCompletedConstructionTasks(toRemove);
}


void ConstructionManager::checkForDeadlockConstruction()
{
	vector<ConstructionTask> toCancel;

	for (auto &b : constructionQueue)
	{
		if (b.status != ConstructionStatus::UnderConstruction)
		{

			UnitType unitType = b.type;
			UnitType producerType = b.type.whatBuilds().first;
			Area *desiredPositionArea = (Area *)theMap.GetArea(b.desiredPosition);

			bool isDeadlockCase = false;


			if (BuildManager::Instance().isProducerWillExist(producerType) == false) {
				isDeadlockCase = true;
			}

	
			if (!isDeadlockCase && unitType.isRefinery())
			{
				TilePosition testLocation = b.desiredPosition;

		
				if (!testLocation.isValid()) {
					isDeadlockCase = true;
				}
				else {
			
					Unitset uot = Broodwar->getUnitsOnTile(testLocation);

					for (auto &u : uot) {
						if (u->getType().isRefinery() && u->exists() ) {
							isDeadlockCase = true;
							break;
						}
					}
				}
			}


			if (!isDeadlockCase
					&& INFO.getOccupiedAreas(S).find(desiredPositionArea) == INFO.getOccupiedAreas(S).end()
					&& INFO.getOccupiedAreas(E).find(desiredPositionArea) != INFO.getOccupiedAreas(E).end())
			{
				if (desiredPositionArea->Bases().empty())
					isDeadlockCase = true;
				else {
					for (auto &base : desiredPositionArea->Bases()) {
						if (base.GetEnemyGroundDefenseBuildingCount() + base.GetEnemyGroundDefenseUnitCount()) {
							isDeadlockCase = true;
							break;
						}
					}
				}
			}

			const map< UnitType, int > &requiredUnits = unitType.requiredUnits();


			if (!isDeadlockCase && requiredUnits.size() > 0)
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
									isDeadlockCase = true;
								}
							}
						}
					}
				}
			}

			if (isDeadlockCase) {
				toCancel.push_back(b);
			}
		}
	}

	if (!toCancel.empty()) {
		cout << "checkForDeadlockConstruction" << endl;
		cancelNotUnderConstructionTasks(toCancel);
	}
}

bool ConstructionManager::isBuildingPositionExplored(const ConstructionTask &b) const
{
	TilePosition tile = b.finalPosition;

	
	for (int x = 0; x < b.type.tileWidth(); ++x)
	{
		for (int y = 0; y < b.type.tileHeight(); ++y)
		{
			if (!Broodwar->isExplored(tile.x + x, tile.y + y))
			{
				return false;
			}
		}
	}

	return true;
}

int ConstructionManager::getReservedMinerals()
{
	return reservedMinerals;
}

int ConstructionManager::getReservedGas()
{
	return reservedGas;
}


ConstructionManager &ConstructionManager::Instance()
{
	static ConstructionManager instance;
	return instance;
}

int ConstructionManager::getConstructionQueueItemCount(UnitType queryType, TilePosition queryTilePosition, int maxRange)
{
	int count = 0;

	bool checkTilePosition = queryType.isBuilding() && queryTilePosition != TilePositions::None;

	for (auto &b : constructionQueue)
	{
		if (b.type == queryType)
		{
			if (b.type.isBuilding() && b.buildingUnit != nullptr && b.buildingUnit->isCompleted())
				continue;

			if (checkTilePosition)
			{
				TilePosition p = b.status == ConstructionStatus::UnderConstruction ? b.finalPosition : b.desiredPosition;

				if (queryTilePosition.getDistance(p) <= maxRange) {
					count++;
				}
			}
			else {
				count++;
			}
		}
	}

	return count;
}

bool ConstructionManager::existConstructionQueueItem(UnitType queryType, TilePosition queryTilePosition, int maxRange) {
	bool checkTilePosition = queryType.isBuilding() && queryTilePosition != TilePositions::None;

	for (auto &b : constructionQueue)
	{
		if (b.type == queryType)
		{
			if (b.type.isBuilding() && b.buildingUnit != nullptr && b.buildingUnit->isCompleted())
				continue;

			if (checkTilePosition)
			{
				TilePosition p = b.status == ConstructionStatus::UnderConstruction ? b.finalPosition : b.desiredPosition;

				if (queryTilePosition.getDistance(p) <= maxRange) {
					return true;
				}
			}
			else {
				return true;
			}
		}
	}

	return false;
}

const vector<ConstructionTask> *ConstructionManager::getConstructionQueue()
{
	return & constructionQueue;
}

void ConstructionManager::clearNotUnderConstruction() {
	vector<ConstructionTask> toRemove;

	for (auto &b : constructionQueue)
	{
		if (b.status != ConstructionStatus::UnderConstruction)
		{
			toRemove.push_back(b);
		}
	}

	if (!toRemove.empty()) {
		cancelNotUnderConstructionTasks(toRemove);
	}
}