#include "ConstructionManager.h"
#include "UnitManager/TankManager.h"

using namespace MyBot;

ConstructionManager::ConstructionManager()
	: reservedMinerals(0)
	, reservedGas(0)
{
	memset(time, 0, sizeof(time));
	constructionQueue.reserve(20);
}

// add a new building to be constructed
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


// ConstructionTask 하나를 삭제한다. 건설중인 것은 건설 취소. for 문 안에서 삭제하는 경우 주의해서 사용해야 함.
void ConstructionManager::cancelConstructionTask(ConstructionTask ct) {
	// 건설중이 아닌것 삭제.
	if (ct.status != ConstructionStatus::UnderConstruction) {
		cout << "cancelConstructionTask" << endl;
		cancelNotUnderConstructionTasks({ ct });
	}
	// 건설중인것 삭제.
	else {
		if (ct.buildingUnit && ct.buildingUnit->exists() && ct.buildingUnit->canCancelConstruction())
			ct.buildingUnit->cancelConstruction();

		removeCompletedConstructionTasks({ ct });
	}
}

// ConstructionTask 여러개를 삭제한다
// 건설을 시작했었던 ConstructionTask 이기 때문에 reservedMinerals, reservedGas 는 건드리지 않는다
void ConstructionManager::removeCompletedConstructionTasks(const vector<ConstructionTask> &toRemove)
{
	for (auto &b : toRemove)
	{
		auto &it = find(constructionQueue.begin(), constructionQueue.end(), b);

		if (it != constructionQueue.end())
		{
			if (S->getRace() == Races::Terran && it->constructionWorker)
				ScvManager::Instance().setScvIdleState(it->constructionWorker);

			ReserveBuilding::Instance().unassignTiles(it->finalPosition, it->type);

			constructionQueue.erase(it);
		}
	}
}

// constructionQueue 에서 건설 시작하지 않은 ConstructionTask 를 취소합니다
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

			if (!desiredPosition.isValid())
				Logger::info(Config::Files::LogFilename, true, "error : desiredPosition is not valid (%d, %d) %s", desiredPosition.x, desiredPosition.y, it->type, toRemove.size());

			if (S->getRace() == Races::Terran && it->constructionWorker) {
				ScvManager::Instance().setScvIdleState(it->constructionWorker);
			}

			// 미리 위치를 예약하는 건물들은 unassign 하고, 실시간으로 계산하는 건물들은 free 해준다.
			if (it->type == Terran_Factory || it->type == Terran_Supply_Depot || it->type == Terran_Barracks || it->type == Terran_Armory || it->type == Terran_Academy || it->type == Terran_Starport || it->type == Terran_Science_Facility)
				ReserveBuilding::Instance().unassignTiles(desiredPosition, it->type);
			else
				ReserveBuilding::Instance().freeTiles(desiredPosition, it->type, true);

			constructionQueue.erase(it);
		}
	}
}

// gets called every frame from GameCommander
void ConstructionManager::update()
{
	// 1초에 1번만 실행한다
	//if (TIME % 24 != 0) return;

	// constructionQueue 에 들어있는 ConstructionTask 들은
	// Unassigned -> Assigned (buildCommandGiven=false) -> Assigned (buildCommandGiven=true) -> UnderConstruction -> (Finished) 로 상태 변화된다
	int i = 0;
	time[i] = clock();

	try {
		// UnderConstruction 중 파괴된 건물 큐에서 제거. 부서지기 일보직전 건물 큐에서 제거. 일꾼 할당 해제
		validateWorkersAndBuildings();
	}
	catch (SAIDA_Exception e) {
		Logger::error("validateWorkersAndBuildings Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("validateWorkersAndBuildings Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("validateWorkersAndBuildings Unknown Error.\n");
		throw;
	}

	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "validateWorkersAndBuildings Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// Unassigned 인 경우 일꾼 할당
		assignWorkersToUnassignedBuildings();
	}
	catch (SAIDA_Exception e) {
		Logger::error("assignWorkersToUnassignedBuildings Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("assignWorkersToUnassignedBuildings Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("assignWorkersToUnassignedBuildings Unknown Error.\n");
		throw;
	}

	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "assignWorkersToUnassignedBuildings Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 건물 짓기 시작하는 경우, Assigned => UnderConstruction 상태 변경 및 자원 reserve, 일꾼 할당 해제.
		checkForStartedConstruction();
	}
	catch (SAIDA_Exception e) {
		Logger::error("checkForStartedConstruction Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("checkForStartedConstruction Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("checkForStartedConstruction Unknown Error.\n");
		throw;
	}

	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "checkForStartedConstruction Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// Assigned 인 상태 컨트롤. (일꾼이 건물 짓기 시작하기 전까지 상황 처리)
		constructAssignedBuildings();
	}
	catch (SAIDA_Exception e) {
		Logger::error("constructAssignedBuildings Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("constructAssignedBuildings Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("constructAssignedBuildings Unknown Error.\n");
		throw;
	}

	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "constructAssignedBuildings Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 건설 중 일꾼이 죽은 경우 재할당해준다.
		checkForDeadTerranBuilders();
	}
	catch (SAIDA_Exception e) {
		Logger::error("checkForDeadTerranBuilders Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("checkForDeadTerranBuilders Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("checkForDeadTerranBuilders Unknown Error.\n");
		throw;
	}

	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "checkForDeadTerranBuilders Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 완료된 빌딩 큐에서 제거. 일꾼 할당 해제
		checkForCompletedBuildings();
	}
	catch (SAIDA_Exception e) {
		Logger::error("checkForCompletedBuildings Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("checkForCompletedBuildings Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("checkForCompletedBuildings Unknown Error.\n");
		throw;
	}

	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "checkForCompletedBuildings Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 데드락 요소 제거
		checkForDeadlockConstruction();
	}
	catch (SAIDA_Exception e) {
		Logger::error("checkForDeadlockConstruction Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("checkForDeadlockConstruction Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("checkForDeadlockConstruction Unknown Error.\n");
		throw;
	}

	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "checkForDeadlockConstruction Timeout (%dms)\n", time[i] - time[i - 1]);
}

// STEP 1: DO BOOK KEEPING ON WORKERS WHICH MAY HAVE DIED
void ConstructionManager::validateWorkersAndBuildings()
{
	vector<Unit> cancelBuildings;

	// 내 건물중 완성되지 않고 나를 공격하고 있는 유닛 데미지가 hp 보다 큰 경우 건설 취소
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
			// 건설 진행 도중 (공격을 받아서) 건설하려던 건물이 파괴된 경우, constructionQueue 에서 삭제한다
			// 그렇지 않으면 (아마도 전투가 벌어지고있는) 기존 위치에 다시 건물을 지으려 할 것이기 때문.
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

// STEP 2: ASSIGN WORKERS TO BUILDINGS WITHOUT THEM
void ConstructionManager::assignWorkersToUnassignedBuildings()
{
	vector<ConstructionTask> toRemove;

	// for each building that doesn't have a builder, assign one
	for (ConstructionTask &b : constructionQueue)
	{
		if (b.status != ConstructionStatus::Unassigned) {
			if (b.status == ConstructionStatus::WaitToAssign && b.startWaitingFrame + 24 * 30 < TIME)
				b.status = ConstructionStatus::Unassigned;

			continue;
		}

		//cout << "find build place near desiredPosition " << b.desiredPosition << endl;

		// 최초 건설 위치가 지정된 경우 우선적으로 일꾼 할당
		UnitInfo *closestScv = nullptr;
		Unit workerToAssign = nullptr;

		if (b.desiredPosition.isValid()) {
			closestScv = ScvManager::Instance().chooseScvClosestTo((Position)b.desiredPosition, b.lastConstructionWorkerID);

			if (closestScv != nullptr) {
				//cout << b.lastConstructionWorkerID << " is changed to " << closestScv->unit()->getID() << endl;
				workerToAssign = closestScv->unit();
			}
		}

		TilePosition testLocation;

		// 반드시 해당 위치에 지어야 하는 경우가 아닌 경우만 탈것. (적군 방어 때문에 거의 없음.)
		if (!TerranConstructionPlaceFinder::Instance().isFixedPositionOrder(b.type)) {
			// 최초 건설 위치가 지정된 경우 건설 일꾼이 assigned, 그 외의 경우 건설 일꾼이 Unassigned 인 상태에서 getBuildLocationNear 로 건설할 위치를 다시 정한다. -> Assigned
			testLocation = ConstructionPlaceFinder::Instance().getBuildLocationNear(b.type, b.desiredPosition, false, workerToAssign);

			//cout << "ConstructionPlaceFinder Selected Location : " << testLocation << endl;

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
				// 취소한다.
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
			closestScv = ScvManager::Instance().chooseScvClosestTo((Position)testLocation, b.lastConstructionWorkerID);
		}

		workerToAssign = ScvManager::Instance().setStateToSCV(closestScv, new ScvBuildState());

		if (workerToAssign)
		{
			// 기존에 동일한 명령을 받은 queue 가 있다면 취소한다.
			for (auto &cQueue : constructionQueue) {
				if (cQueue.status == ConstructionStatus::Assigned && cQueue.type == b.type && cQueue.finalPosition == testLocation) {
					cout << "[" << TIME << "] 동일 Queue 안들어 가도록 삭제" << endl;
					toRemove.push_back(cQueue);
					break;
				}
			}

			//cout << "set ConstuctionWorker " << workerToAssign->getID() << b.type << " location : " << testLocation << endl;

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

			// keepmulti tank 에게 scv 할당
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

// STEP 3: ISSUE CONSTRUCTION ORDERS TO ASSIGN BUILDINGS AS NEEDED
void ConstructionManager::constructAssignedBuildings()
{
	for (auto &b : constructionQueue)
	{
		if (b.status != ConstructionStatus::Assigned)
		{
			continue;
		}

		/*
		if (b.constructionWorker == nullptr) {
			cout << b.type.c_str() << " constructionWorker null" << endl;
		}
		else {
			cout << b.type.c_str()
				<< " constructionWorker " << b.constructionWorker->getID()
				<< " exists " << b.constructionWorker->exists()
				<< " isIdle " << b.constructionWorker->isIdle()
				<< " isConstructing " << b.constructionWorker->isConstructing()
				<< " isMorphing " << b.constructionWorker->isMorphing() << endl;
		}
		*/

		// 일꾼에게 build 명령을 내리기 전에는 isConstructing = false 이다
		// 아직 탐색하지 않은 곳에 대해서는 build 명령을 내릴 수 없다
		// 일꾼에게 build 명령을 내리면, isConstructing = true 상태가 되어 이동을 하다가
		// build 를 실행할 수 없는 상황이라고 판단되면 isConstructing = false 상태가 된다
		// build 를 실행할 수 있으면, 프로토스 / 테란 종족의 경우 일꾼이 build 를 실행하고
		// 저그 종족 건물 중 Extractor 건물이 아닌 다른 건물의 경우 일꾼이 exists = true, isConstructing = true, isMorphing = true 가 되고, 일꾼 ID 가 건물 ID가 된다
		// 저그 종족 건물 중 Extractor 건물의 경우 일꾼이 exists = false, isConstructing = true, isMorphing = true 가 된 후, 일꾼 ID 가 건물 ID가 된다.
		//                  Extractor 건물 빌드를 도중에 취소하면, 새로운 ID 를 가진 일꾼이 된다

		// 일꾼이 Assigned 된 후, UnderConstruction 상태로 되기 전, 즉 일꾼이 이동 중에 일꾼이 죽은 경우, 건물을 WaitToAssign 상태로 되돌려 일꾼을 다시 Assign 하도록 한다
		if (b.constructionWorker == nullptr || b.constructionWorker->exists() == false)
		{
			//cout << "unassign " << b.type.getName().c_str() << " worker " << b.constructionWorker->getID() << ", because it is not exists" << endl;

			// Unassigned 된 상태로 되돌린다
			ScvManager::Instance().setScvIdleState(b.constructionWorker);

			b.constructionWorker = nullptr;

			b.buildCommandGiven = false;

			b.finalPosition = TilePositions::None;

			b.startWaitingFrame = TIME;
			b.assignFrame = 0;

			b.status = ConstructionStatus::WaitToAssign;

			// keepmulti tank 에게 scv 할당
			if (INFO.getSecondExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getSecondExpansionLocation(S)->getTilePosition()) < 4) {
				TM.assignSCV(1, nullptr);
			}
			else if (INFO.getThirdExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getThirdExpansionLocation(S)->getTilePosition()) < 4) {
				TM.assignSCV(2, nullptr);
			}

			continue;
		}
		// 건설 명령 후 1분간 건설을 시작하지 못하면 Unassigned 시킨다.
		else if ((b.buildCommandGiven && b.lastBuildCommandGivenFrame + 24 * 60 < TIME)
				 // build 명령이 없더라도 2분내에 목적지에 도달하지 못하면 Unassigned 시킨다.
				 || (b.assignFrame + 24 * 120 < TIME && b.constructionWorker->getPosition().getApproxDistance((Position)b.finalPosition) > 4 * TILE_SIZE)) {
			ScvManager::Instance().setScvIdleState(b.constructionWorker);

			b.constructionWorker = nullptr;

			b.buildCommandGiven = false;
			b.assignFrame = 0;

			b.finalPosition = TilePositions::None;

			b.status = ConstructionStatus::Unassigned;

			// keepmulti tank 에게 scv 할당
			if (INFO.getSecondExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getSecondExpansionLocation(S)->getTilePosition()) < 4) {
				TM.assignSCV(1, nullptr);
			}
			else if (INFO.getThirdExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getThirdExpansionLocation(S)->getTilePosition()) < 4) {
				TM.assignSCV(2, nullptr);
			}

			continue;
		}

		// 일꾼이 갇혀있는지 판단한다.
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
				// 3번 이상 동일한 위치 방문 시, 갇혀있다고 판단한다.
				if (++b.minDistVisitCnt >= 3) {
					// 현재 위치에 지을 수 있는 경우 짓는다.
					TilePosition topLeft = ReserveBuilding::Instance().getReservedPositionAtWorker(b.type, b.constructionWorker);

					// 갇힌 위치에 건물을 지을 수 있는 경우 건설위치 변경.
					if (topLeft.isValid() && bw->canBuildHere(topLeft, b.type, b.constructionWorker)) {
						cout << b.type << " id : " << b.constructionWorker->getID() << " SCV 위치 : " << (TilePosition)b.constructionWorker->getPosition() << " " << b.finalPosition << " to " << topLeft << " 현재 위치에 짓기" << endl;
						b.finalPosition = topLeft;

						// set the buildCommandGiven flag to true
						b.buildCommandGiven = CommandUtil::build(b.constructionWorker, b.type, b.finalPosition);

						b.lastBuildCommandGivenFrame = TIME;

						continue;
					}

					// 지을 수 없는 경우 할당 해제한다.
					//cout << b.type << " id : " << b.constructionWorker->getID() << " " << b.finalPosition << " to " << topLeft << " prisoned!" << endl;

					// tell worker manager the unit we had is not needed now, since we might not be able
					// to get a valid location soon enough
					ScvManager::Instance().setStateToSCV(INFO.getUnitInfo(b.constructionWorker, S), new ScvPrisonedState());

					// nullify its current builder unit
					b.constructionWorker = nullptr;

					// nullify its current builder unit
					b.finalPosition = TilePositions::None;

					// reset the build command given flag
					b.buildCommandGiven = false;
					b.assignFrame = 0;

					// add the building back to be assigned
					b.status = ConstructionStatus::Unassigned;

					// keepmulti tank 에게 scv 할당
					if (INFO.getSecondExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getSecondExpansionLocation(S)->getTilePosition()) < 4) {
						TM.assignSCV(1, nullptr);
					}
					else if (INFO.getThirdExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getThirdExpansionLocation(S)->getTilePosition()) < 4) {
						TM.assignSCV(2, nullptr);
					}

					continue;
				}

				//cout << "ID : " << b.constructionWorker->getID() << " " << b.minDistPosition << " 에 " << b.minDistVisitCnt << " 번 방문." << endl;
			}

			b.minDistTime = TIME;
		}

		// 테란전의 경우 scv 를 keepmulti 와 함께 이동시킨다.
		if (INFO.enemyRace == Races::Terran) {
			// 일꾼이 앞마당이 아닌 다른 확장에 추가 확장을 지으러 가는 경우 처리..
			if (INFO.getSecondExpansionLocation(S) && b.desiredPosition.getApproxDistance(INFO.getSecondExpansionLocation(S)->getTilePosition()) < 4
					&& b.constructionWorker->getPosition().getApproxDistance((Position)b.desiredPosition) > 15 * TILE_SIZE) {
				UnitInfo *farthest = nullptr;
				UnitInfo *closest = nullptr;
				int distMax = INT_MIN;
				int distMin = INT_MAX;

				for (auto tank : TM.getKeepMultiTanks(1)) {
					int tmpDist = tank->unit()->getDistance(b.constructionWorker);

					// 탱크가 구석으로 가면 따라가는 일꾼때문에 길막현상 발생 가능. 가장 먼 탱크 따라가도록 수정.
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

				// TODO 탱크와 SCV 사이에 위험이 있을 수 있기 때문에 추가 처리 필요
				if (closest)
				{
					CommandUtil::rightClick(b.constructionWorker, closest->unit(), TIME % 24 == 0);
				}
				// 가드하는 탱크가 없으면 앞마당 미네랄로 가서 대기
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

					// 탱크가 구석으로 가면 따라가는 일꾼때문에 길막현상 발생 가능. 가장 먼 탱크 따라가도록 수정.
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

				// TODO 탱크와 SCV 사이에 위험이 있을 수 있기 때문에 추가 처리 필요
				if (closest)
					CommandUtil::rightClick(b.constructionWorker, closest->unit(), TIME % 24 == 0);
				// 가드하는 탱크가 없으면 앞마당 미네랄로 가서 대기
				else {
					Position pos = INFO.getFirstExpansionLocation(S) ? INFO.getSecondChokePosition(S) : INFO.getMainBaseLocation(S)->Center();

					CommandUtil::rightClick(b.constructionWorker, pos);
				}

				continue;
			}
		}

		// 일꾼이 베이스를 지으러 간다면
		if (b.type.isResourceDepot()) {
			// 일꾼이 burrowedunit 제거 모드이면
			if (b.removeBurrowedUnit) {
				UnitInfo *burrowed = INFO.getClosestTypeUnit(E, b.constructionWorker->getPosition(), Terran_Vulture_Spider_Mine, 5 * TILE_SIZE);

				if (burrowed)
					CommandUtil::attackUnit(b.constructionWorker, burrowed->unit());
				else if (b.lastBuildCommandGivenFrame + 60 < TIME) {
					cout << "[" << TIME << "] burrow 유닛 없음 할당 해제" << endl;
					b.removeBurrowedUnit = false;
					b.buildCommandGiven = CommandUtil::build(b.constructionWorker, b.type, b.finalPosition);
					b.lastBuildCommandGivenFrame = TIME;
				}

				continue;
			}

			// 1. 지으러 가는 위치가 기본 베이스 위치가 아니고
			// 2. 기본 베이스 위치에 건설이 가능하면
			// 위치를 변경한다.
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

			// 지으러 가는 곳에 우리편 마인이 심어져 있다면 마인을 먼저 제거한다.
			Unitset us = bw->getUnitsInRectangle((Position)b.finalPosition, (Position)(b.finalPosition + b.type.tileSize()));

			for (auto u : us) {
				if (u->getPlayer() == S && u->getType() == Terran_Vulture_Spider_Mine) {
					CommandUtil::attackUnit(b.constructionWorker, u);
					break;
				}
			}
		}

		// if that worker is not currently constructing
		// 일꾼이 build command 를 받으면 isConstructing = true 가 되고 건설을 하기위해 이동하는데,
		// isConstructing = false 가 되었다는 것은, build command 를 수행할 수 없어 게임에서 해당 임무가 취소되었다는 것이다
		if (b.constructionWorker->isConstructing() == false)
		{
			// if we haven't explored the build position, first we mush go there
			// 한번도 안가본 곳에는 build 커맨드 자체를 지시할 수 없으므로, 일단 그곳으로 이동하게 한다
			if (!isBuildingPositionExplored(b))
			{
				CommandUtil::rightClick(b.constructionWorker, Position(b.finalPosition) + (Position)b.type.tileSize() / 2);
			}
			else if (b.buildCommandGiven == false)
			{
				//cout << b.type.c_str() << " build commanded to " << b.constructionWorker->getID() << ", buildCommandGiven true " << endl;

				// set the buildCommandGiven flag to true
				b.buildCommandGiven = CommandUtil::build(b.constructionWorker, b.type, b.finalPosition);

				b.lastBuildCommandGivenFrame = TIME;

				b.lastConstructionWorkerID = b.constructionWorker->getID();
			}
			// if this is not the first time we've sent this guy to build this
			// 일꾼에게 build command 를 주었지만, 건설시작하기전에 도중에 자원이 미달하게 되었거나, 해당 장소에 다른 유닛들이 있어서 건설을 시작 못하게 되거나, Pylon 이나 Creep 이 없어진 경우 등이 발생할 수 있다
			// 이 경우, 해당 일꾼의 build command 를 해제하고, 건물 상태를 Unassigned 로 바꿔서, 다시 건물 위치를 정하고, 다른 일꾼을 지정하는 식으로 처리한다
			else if (b.lastBuildCommandGivenFrame + 24 < TIME)
			{
				// 자원이 모자란 경우
				if (b.type.mineralPrice() > S->minerals() || b.type.gasPrice() > S->gas()) {
					// 목적지까지의 거리가 먼 경우 계속 이동한다.
					if (b.constructionWorker->getDistance((Position)b.finalPosition) > TILE_SIZE) {
						continue;
					}
				}

				bool skip = false;

				// 다른 유닛이 있는 경우
				Unitset us = Broodwar->getUnitsInRectangle((Position)b.finalPosition + Position(1, 1), (Position)(b.finalPosition + b.type.tileSize()) - Position(1, 1));

				for (auto u : us) {
					if (u->getID() != b.constructionWorker->getID() && u->getType() != Resource_Vespene_Geyser && !u->isFlying()) {
						if (b.startWaitingFrame == 0) {
							b.startWaitingFrame = TIME;
							//cout << b.finalPosition << (b.finalPosition + b.type.tileSize()) << " 에 id : " << b.constructionWorker->getID() << " 이외의 유닛 존재 (id:" << u->getID() << u->getType() << ") " << u->getTilePosition() << endl;
						}

						if (!u->getType().isBuilding()) {
							UnitInfo *ui = INFO.getUnitInfo(u, S);

							if (ui == nullptr) {
								// 적 일꾼인 경우 기다린다.
								if (u->getType().isWorker() && b.startWaitingFrame + 24 * 10 >= TIME) {
									skip = true;
									break;
								}
								else
									continue;
							}

							// 마인이 있는 경우 kill me 로 바꿔줌.
							if (ui->type() == Terran_Vulture_Spider_Mine)
								ui->setKillMe(MustKill);
							// building 중이거나 미네랄 채취중일때 기다린다.
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

				// 그 외의 경우
				//cout << "[" << TIME << "]" << b.type
				//	 << " constructionWorker : " << b.constructionWorker->getID()
				//	 << " state : " << INFO.getUnitInfo(b.constructionWorker, S)->getState()
				//	 << " buildCommandGiven : " << b.buildCommandGiven
				//	 << " lastBuildCommandGivenFrame : " << b.lastBuildCommandGivenFrame
				//	 << " lastConstructionWorkerID : " << b.lastConstructionWorkerID
				//	 << " exists : " << b.constructionWorker->exists()
				//	 << " isIdle : " << b.constructionWorker->isIdle()
				//	 << " isInterruptible : " << b.constructionWorker->isInterruptible()
				//	 << " canCommand : " << b.constructionWorker->canCommand()
				//	 << " isConstructing : " << b.constructionWorker->isConstructing()
				//	 << " retryCnt : " << b.retryCnt
				//	 << " minerals : " << S->minerals() << "/" << b.type.mineralPrice()
				//	 << " gas : " << S->gas() << "/" << b.type.gasPrice()
				//	 << " isMorphing : " << b.constructionWorker->isMorphing() << endl;

				// 일꾼 등록 해제
				if (b.retryCnt > 5) {
					TilePosition topLeft = ReserveBuilding::Instance().getReservedPositionAtWorker(b.type, b.constructionWorker);

					// 갇힌 위치에 건물을 지을 수 있는 경우 건설위치 변경.
					if (topLeft.isValid() && bw->canBuildHere(topLeft, b.type, b.constructionWorker)) {
						cout << b.type << " id : " << b.constructionWorker->getID() << " SCV 위치 : " << (TilePosition)b.constructionWorker->getPosition() << " " << b.finalPosition << " to " << topLeft << " 현재 위치에 짓기2" << endl;
						b.finalPosition = topLeft;

						// set the buildCommandGiven flag to true
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

									cout << "[" << TIME << "] " << b.finalPosition << "여기는 burrow 된 유닛이 숨어있다?" << endl;
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

					//cout << b.type.c_str() << b.finalPosition << " buildCommandGiven -> but now Unassigned " << endl;

					// tell worker manager the unit we had is not needed now, since we might not be able
					// to get a valid location soon enough
					ScvManager::Instance().setScvIdleState(b.constructionWorker);

					// nullify its current builder unit
					b.constructionWorker = nullptr;

					// nullify its current builder unit
					b.finalPosition = TilePositions::None;

					// reset the build command given flag
					b.buildCommandGiven = false;
					b.assignFrame = 0;

					// add the building back to be assigned
					b.status = ConstructionStatus::Unassigned;

					// keepmulti tank 에게 scv 할당
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

					//cout << b.type << " retry Cnt : " << b.retryCnt << endl;
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

		//cout << "buildingThatStartedConstruction " << buildingThatStartedConstruction->getType().getName().c_str() << " isBeingConstructed at " << buildingThatStartedConstruction->getTilePosition().x << "," << buildingThatStartedConstruction->getTilePosition().y << endl;

		// check all our building status objects to see if we have a match and if we do, update it
		for (auto &b : constructionQueue)
		{
			if (b.status != ConstructionStatus::Assigned)
			{
				continue;
			}

			// check if the positions match.  Worker just started construction.
			if (b.finalPosition == buildingThatStartedConstruction->getTilePosition() && b.type == buildingThatStartedConstruction->getType())
			{
				//cout << "construction " << b.type.getName().c_str() << " started at " << b.finalPosition.x << "," << b.finalPosition.y << endl;

				// the resources should now be spent, so unreserve them
				reservedMinerals -= buildingThatStartedConstruction->getType().mineralPrice();
				reservedGas      -= buildingThatStartedConstruction->getType().gasPrice();

				b.buildingUnit = buildingThatStartedConstruction;

				// if we are zerg, make the buildingUnit nullptr since it's morphed or destroyed
				// Extractor 의 경우 destroyed 되고, 그외 건물의 경우 morphed 된다
				if (S->getRace() == Races::Zerg)
				{
					b.constructionWorker = nullptr;
				}
				// if we are protoss, give the worker back to worker manager
				else if (S->getRace() == Races::Protoss)
				{
					ScvManager::Instance().setScvIdleState(b.constructionWorker);

					b.constructionWorker = nullptr;
				}

				// put it in the under construction vector
				b.status = ConstructionStatus::UnderConstruction;

				// only one building will match
				break;
			}
		}
	}
}

// STEP 5: IF WE ARE TERRAN, THIS MATTERS
// 테란은 건설을 시작한 후, 건설 도중에 일꾼이 죽을 수 있다. 이 경우, 건물에 대해 다시 다른 SCV를 할당한다
// 참고로, 프로토스 / 저그는 건설을 시작하면 일꾼 포인터를 nullptr 로 만들기 때문에 (constructionWorker = nullptr) 건설 도중에 죽은 일꾼을 신경쓸 필요 없다
void ConstructionManager::checkForDeadTerranBuilders()
{
	if (INFO.selfRace == Races::Terran) {

		if (S->completedUnitCount(Terran_SCV) <= 0) return;

		// for each of our buildings under construction
		for (auto &b : constructionQueue)
		{
			// if a terran building whose worker died mid construction,
			// send the right click command to the buildingUnit to resume construction
			if (b.status == ConstructionStatus::UnderConstruction) {

				if (b.buildingUnit->isCompleted()) continue;

				if (b.constructionWorker == nullptr || b.constructionWorker->exists() == false) {

					//cout << "checkForDeadTerranBuilders - chooseConstuctionWorkerClosest for " << b.type.getName().c_str() << " to worker near " << b.finalPosition.x << "," << b.finalPosition.y << endl;

					// grab a worker unit from WorkerManager which is closest to this final position
					Unit workerToAssign = ScvManager::Instance().chooseConstuctionScvClosestTo(b.finalPosition, b.lastConstructionWorkerID);

					if (workerToAssign)
					{
						//cout << "set ConstuctionWorker " << workerToAssign->getID() << endl;

						b.constructionWorker = workerToAssign;

						//b.status 는 계속 UnderConstruction 로 둔다. Assigned 로 바꾸면, 결국 Unassigned 가 되어서 새로 짓게 되기 때문이다
						//b.status = ConstructionStatus::Assigned;

						CommandUtil::rightClick(b.constructionWorker, b.buildingUnit);

						b.buildCommandGiven = true;

						b.lastBuildCommandGivenFrame = TIME;

						b.lastConstructionWorkerID = b.constructionWorker->getID();
					}
				}
				// 할당은 되어 있으나 건설하고 있지 않은 경우, 건설 명령을 내린다.
				else if (!b.constructionWorker->isConstructing() && b.lastBuildCommandGivenFrame + 10 < TIME) {
					CommandUtil::rightClick(b.constructionWorker, b.buildingUnit);

					b.lastBuildCommandGivenFrame = TIME;
				}
			}
		}
	}

}

// STEP 6: CHECK FOR COMPLETED BUILDINGS
void ConstructionManager::checkForCompletedBuildings()
{
	vector<ConstructionTask> toRemove;

	// for each of our buildings under construction
	for (auto &b : constructionQueue)
	{
		if (b.status != ConstructionStatus::UnderConstruction)
		{
			continue;
		}

		// if the unit has completed
		if (b.buildingUnit->isCompleted())
		{
			//cout << "construction " << b.type.getName().c_str() << " completed at " << b.finalPosition.x << "," << b.finalPosition.y << endl;

			// remove this unit from the under construction vector
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
			// BuildManager가 판단했을때 Construction 가능조건이 갖춰져서 ConstructionManager의 ConstructionQueue 에 들어갔는데,
			// 선행 건물이 파괴되서 Construction을 수행할 수 없게 되었거나,
			// 일꾼이 다 사망하는 등 게임상황이 바뀌어서, 계속 ConstructionQueue 에 남아있게 되는 dead lock 상황이 된다
			// 선행 건물을 BuildQueue에 추가해넣을지, 해당 ConstructionQueueItem 을 삭제할지 전략적으로 판단해야 한다
			UnitType unitType = b.type;
			UnitType producerType = b.type.whatBuilds().first;
			Area *desiredPositionArea = (Area *)theMap.GetArea(b.desiredPosition);

			bool isDeadlockCase = false;

			// 건물을 생산하는 유닛이나, 유닛을 생산하는 건물이 존재하지 않고, 건설 예정이지도 않으면 dead lock
			if (BuildManager::Instance().isProducerWillExist(producerType) == false) {
				isDeadlockCase = true;
			}

			// Refinery 건물의 경우, 건물 지을 장소를 찾을 수 없게 되었거나, 건물 지을 수 있을거라고 판단했는데 이미 Refinery 가 지어져있는 경우, dead lock
			if (!isDeadlockCase && unitType.isRefinery())
			{
				TilePosition testLocation = b.desiredPosition;

				// Refinery 를 지으려는 장소를 찾을 수 없으면 dead lock
				if (!testLocation.isValid()) {
					cout << "Construction Dead lock case -> Cann't find place to construct at " << b.desiredPosition << b.type.getName() << endl;
					isDeadlockCase = true;
				}
				else {
					// Refinery 를 지으려는 장소에 Refinery 가 이미 건설되어 있다면 dead lock
					Unitset uot = Broodwar->getUnitsOnTile(testLocation);

					for (auto &u : uot) {
						if (u->getType().isRefinery() && u->exists() ) {
							cout << "Construction Dead lock case -> Refinery Building was built already at " << testLocation.x << ", " << testLocation.y << endl;
							isDeadlockCase = true;
							break;
						}
					}
				}
			}

			// 정찰결과 혹은 전투결과, 건설 장소가 아군 점령 Area 이 아니고, 적군이 점령한 Area 이 되었으면 일반적으로는 현실적으로 dead lock 이 된다
			// (포톤캐논 러시이거나, 적군 점령 Area 근처에서 테란 건물 건설하는 경우에는 예외일테지만..)
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

			// 선행 건물/유닛이 있는데
			if (!isDeadlockCase && requiredUnits.size() > 0)
			{
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

	// for each tile where the building will be built
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

// constructionQueue에 해당 type 의 Item 이 존재하는지 카운트한다. queryTilePosition 을 입력한 경우, 위치간 거리까지도 고려한다
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
		cout << "clearNotUnderConstruction" << endl;
		cancelNotUnderConstructionTasks(toRemove);
	}
}