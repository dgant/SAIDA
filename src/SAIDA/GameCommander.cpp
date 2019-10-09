#include "GameCommander.h"

using namespace MyBot;

GameCommander::GameCommander() {
	isToFindError = false;
	memset(time, 0, sizeof(time));
}
GameCommander::~GameCommander() {
}

GameCommander &GameCommander::Instance()
{
	static GameCommander instance;
	return instance;
}

void GameCommander::onStart()
{
	TilePosition startLocation = Broodwar->self()->getStartLocation();

	if (startLocation == TilePositions::None || startLocation == TilePositions::Unknown) {
		return;
	}

	StrategyManager::Instance().onStart();
}

void GameCommander::onEnd(bool isWinner)
{
	if (!isWinner) {
		string loseLog = "DefeatResult_" + CommonUtil::getYYYYMMDDHHMMSSOfNow() + Broodwar->mapFileName() + Broodwar->enemy()->getName() + ".dat";

		Logger::info(loseLog, false, "Frame : %d %d분 %d초\n", TIME, (TIME / 24) / 60, (TIME / 24) % 60);
		Logger::info(loseLog, false, "Map : %s\n", bw->mapFileName().c_str());
		Logger::info(loseLog, false, "Enemy Name : %s\n", E->getName().c_str());
		Logger::info(loseLog, false, "My Starting Location (%d, %d)\n", S->getStartLocation().x, S->getStartLocation().y);
		Logger::info(loseLog, false, "Enemy Race : select-%s real-%s\n", INFO.enemySelectRace.c_str(), INFO.enemyRace.c_str());

		Logger::info(loseLog, false, "\nInitial Build History\n");

		for (auto &eib : EnemyStrategyManager::Instance().getEIBHistory())
			Logger::info(loseLog, false, "%d %d분 %d초 : %s\n", eib.second, (eib.second / 24) / 60, (eib.second / 24) % 60, eib.first.c_str());

		Logger::info(loseLog, false, "\nMain Build History\n");

		for (auto &emb : EnemyStrategyManager::Instance().getEMBHistory())
			Logger::info(loseLog, false, "%d %d분 %d초 : %s\n", emb.second, (emb.second / 24) / 60, (emb.second / 24) % 60, emb.first.c_str());

		Logger::info(loseLog, false, "\n● My Unit Count\n");
		map<UnitType, uList> &allSUnit = INFO.getUnitData(S).getUnitTypeMap();
		map<UnitType, uList> &allSBuild = INFO.getUnitData(S).getBuildingTypeMap();

		for (auto &u : allSUnit) {
			if (INFO.getCompletedCount(u.first, S) + INFO.getAllCount(u.first, S) == 0)
				continue;

			Logger::info(loseLog, false, "%-30s: %d/%d\n", u.first.c_str(), INFO.getCompletedCount(u.first, S), INFO.getAllCount(u.first, S));
		}

		for (auto &u : allSBuild) {
			if (INFO.getCompletedCount(u.first, S) + INFO.getAllCount(u.first, S) == 0)
				continue;

			Logger::info(loseLog, false, "%-30s: %d/%d\n", u.first.c_str(), INFO.getCompletedCount(u.first, S), INFO.getAllCount(u.first, S));
		}

		Logger::info(loseLog, false, "\n○ Enemy Unit Count\n");
		map<UnitType, uList> &allEUnit = INFO.getUnitData(E).getUnitTypeMap();
		map<UnitType, uList> &allEBuild = INFO.getUnitData(E).getBuildingTypeMap();

		for (auto &u : allEUnit) {
			if (INFO.getCompletedCount(u.first, E) + INFO.getAllCount(u.first, E) == 0)
				continue;

			Logger::info(loseLog, false, "%-30s: %d/%d\n", u.first.c_str(), INFO.getCompletedCount(u.first, E), INFO.getAllCount(u.first, E));
		}

		for (auto &u : allEBuild) {
			if (INFO.getCompletedCount(u.first, E) + INFO.getAllCount(u.first, E) == 0)
				continue;

			Logger::info(loseLog, false, "%-30s: %d/%d\n", u.first.c_str(), INFO.getCompletedCount(u.first, E), INFO.getAllCount(u.first, E));
		}

		Logger::info(loseLog, false, "\n● My Destroyed Unit Count\n");

		for (auto &destroyed : INFO.getDestroyedCountMap(S))
			Logger::info(loseLog, false, "%-30s: %d\n", destroyed.first.c_str(), destroyed.second);

		Logger::info(loseLog, false, "\n○ Enemy Destroyed Unit Count\n");

		for (auto &destroyed : INFO.getDestroyedCountMap(E))
			Logger::info(loseLog, false, "%-30s: %d\n", destroyed.first.c_str(), destroyed.second);
	}

	if (ML_ON_OFF) {
		string mapName = bw->mapFileName();

		if (INFO.enemyRace == Races::Protoss && mapName.find("CircuitBreaker") != string::npos) {
			// 호출 한번도 안한경우
			if (!SharedMemoryManager::Instance().communication)
				Logger::info("Not_Comm_" + string(isWinner ? "Win" : "Lose") + CommonUtil::getYYYYMMDDHHMMSSOfNow(), false, " ");
			// 한번이라도 호출 한 경우
			else
				Logger::info("Comm_" + string(isWinner ? "Win" : "Lose") + CommonUtil::getYYYYMMDDHHMMSSOfNow(), false, " ");
		}
	}
}

void GameCommander::onFrame()
{
	int i = 0;

	if (Broodwar->isPaused()
			|| Broodwar->self() == nullptr || Broodwar->self()->isDefeated() || Broodwar->self()->leftGame()
			|| Broodwar->enemy() == nullptr || Broodwar->enemy()->isDefeated() || Broodwar->enemy()->leftGame()) {
		return;
	}

	Logger::debug("0");
	time[i] = clock();

	try {
		// 아군 베이스 위치. 적군 베이스 위치. 각 유닛들의 상태정보 등을 Map 자료구조에 저장/업데이트
		InformationManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("InformationManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("InformationManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("InformationManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("1");
	time[++i] = clock();


	if (time[i] - time[i - 1] > 55)
		Logger::info(Config::Files::TimeoutFilename, true, "InformationManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		TerranConstructionPlaceFinder::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("TerranConstructionPlaceFinder Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("TerranConstructionPlaceFinder Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("TerranConstructionPlaceFinder Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("2");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "TerranConstructionPlaceFinder Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 빌드오더큐를 관리하며, 빌드오더에 따라 실제 실행(유닛 훈련, 테크 업그레이드 등)을 지시한다.
		BuildManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("BuildManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("BuildManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("BuildManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("3");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "BuildManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 빌드오더 중 건물 빌드에 대해서는, 일꾼유닛 선정, 위치선정, 건설 실시, 중단된 건물 빌드 재개를 지시한다
		ConstructionManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("ConstructionManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("ConstructionManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("ConstructionManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("4");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) {
		Logger::info(Config::Files::TimeoutFilename, true, "ConstructionManager Timeout (%dms, queue size : %d)\n", time[i] - time[i - 1], CM.getConstructionQueue()->size());

		for (auto c : *CM.getConstructionQueue())
			Logger::info(Config::Files::TimeoutFilename, true, "(%d, %d)\t%s\n", c.desiredPosition.x, c.desiredPosition.y, c.type.c_str());
	}

	try {
		// 전략적 판단 및 유닛 컨트롤
		StrategyManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("StrategyManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("StrategyManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("StrategyManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("5");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "StrategyManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 상대 전략 판단
		EnemyStrategyManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("EnemyStrategyManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("EnemyStrategyManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("EnemyStrategyManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("6");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "EnemyStrategyManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 일꾼 유닛에 대한 명령 (자원 채취, 이동 정도) 지시 및 정리
		ScvManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("ScvManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("ScvManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("ScvManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("7");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) {
		Logger::info(Config::Files::TimeoutFilename, true, "ScvManager Timeout (%dms)\n", time[i] - time[i - 1]);
		Logger::error("ScvManager Timeout (%dms)\n", time[i] - time[i - 1]);
	}

	try {
		// 게임 초기 정찰 유닛 지정 및 정찰 유닛 컨트롤을 실행한다
		ScoutManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("ScoutManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("ScoutManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("ScoutManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("8");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "ScoutManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 유닛 생산
		TrainManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("TrainManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("TrainManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("TrainManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("9");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "TrainManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 컴샛
		ComsatStationManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("ComsatStationManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("ComsatStationManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("ComsatStationManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("0");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "ComsatStationManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 엔베
		EngineeringBayManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("EngineeringBayManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("EngineeringBayManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("EngineeringBayManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("1");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "EngineeringBayManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 마린 상태
		MarineManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("MarineManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("MarineManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("MarineManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("2");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "MarineManager Timeout (%dms) %s.\n", time[i] - time[i - 1], SM.getMainStrategy().c_str());

	try {
		// 시즈 상태
		TankManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("TankManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("TankManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("TankManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("3");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "TankManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 벌쳐 상태
		VultureManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("VultureManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("VultureManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("VultureManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("4");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "VultureManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 골리앗 상태
		GoliathManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("GoliathManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("GoliathManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("GoliathManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("5");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "GoliathManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 레이쓰 상태
		WraithManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("WraithManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("WraithManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("WraithManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("6");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "WraithManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 사이언스 배슬
		VessleManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("VessleManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("VessleManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("VessleManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("7");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "VessleManager Timeout (%dms)\n", time[i] - time[i - 1]);

	try {
		// 드랍쉽
		DropshipManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
		Logger::error("DropshipManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (const exception &e) {
		Logger::error("DropshipManager Error. (Error : %s)\n", e.what());
#ifndef SERVERLOGDLL
		throw e;
#endif
	}
	catch (...) {
		Logger::error("DropshipManager Unknown Error.\n");
#ifndef SERVERLOGDLL
		throw;
#endif
	}

	Logger::debug("8");
	time[++i] = clock();

	if (time[i] - time[i - 1] > 55) Logger::info(Config::Files::TimeoutFilename, true, "DropshipManager Timeout (%dms)\n", time[i] - time[i - 1]);
}

// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
// 일꾼 탄생/파괴 등에 대한 업데이트 로직 버그 수정 : onUnitShow 가 아니라 onUnitComplete 에서 처리하도록 수정
void GameCommander::onUnitShow(Unit unit)
{
	if (unit->getPlayer() == E && !unit->isCompleted())
		InformationManager::Instance().onUnitShow(unit);
}

// BasicBot 1.1 Patch End //////////////////////////////////////////////////

void GameCommander::onUnitHide(Unit unit)
{
}

void GameCommander::onUnitCreate(Unit unit)
{
	if (unit->getPlayer() == S)
		InformationManager::Instance().onUnitCreate(unit);
}

// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
// 일꾼 탄생/파괴 등에 대한 업데이트 로직 버그 수정 : onUnitShow 가 아니라 onUnitComplete 에서 처리하도록 수정
void GameCommander::onUnitComplete(Unit unit)
{
	InformationManager::Instance().onUnitComplete(unit);

	// ResourceDepot 및 Worker 에 대한 처리
	if (unit->getPlayer() == S && unit != nullptr) {
		if (unit->getType() == Terran_SCV || unit->getType().isResourceDepot())
			ScvManager::Instance().onUnitCompleted(unit);
		else if (unit->getType() == Terran_Siege_Tank_Tank_Mode || unit->getType() == Terran_Siege_Tank_Siege_Mode)
			TankManager::Instance().onUnitCompleted(unit);
	}

}

// BasicBot 1.1 Patch End //////////////////////////////////////////////////

void GameCommander::onUnitDestroy(Unit unit)
{
	if (unit->getPlayer() == S) {

		// 무빙샷 벌쳐 셋 관리
		if (unit->getType() == Terran_Vulture)
			VultureManager::Instance().onUnitDestroyed(unit);
		// ResourceDepot 및 Worker 에 대한 처리
		else if (unit->getType() == Terran_SCV) {
			ScvManager::Instance().onUnitDestroyed(unit);
			ScoutManager::Instance().onUnitDestroyed(unit);
		}
		// ZelotDefenceUnit해제
		else if (unit->getType() == Terran_Marine)
			MarineManager::Instance().onUnitDestroyed(unit);
		else if (unit->getType() == Terran_Siege_Tank_Tank_Mode || unit->getType() == Terran_Siege_Tank_Siege_Mode)
			TankManager::Instance().onUnitDestroyed(unit);
		else if (unit->getType() == Terran_Goliath)
			GoliathManager::Instance().onUnitDestroyed(unit);
	}

	InformationManager::Instance().onUnitDestroy(unit);
}

void GameCommander::onUnitRenegade(Unit unit)
{

}

void GameCommander::onUnitMorph(Unit unit)
{
	if (unit->getType().isRefinery())
	{
		if (unit->getPlayer() == S)
			InformationManager::Instance().onUnitCreate(unit);
		else
			InformationManager::Instance().onUnitShow(unit);
	}
}

void GameCommander::onUnitDiscover(Unit unit)
{
}

void GameCommander::onUnitEvade(Unit unit)
{
}

void GameCommander::onUnitLifted(Unit unit)
{
	if (unit->getPlayer() == S)
		ScvManager::Instance().onUnitLifted(unit);
}

void GameCommander::onUnitLanded(Unit unit)
{
	if (unit->getPlayer() == S)
		ScvManager::Instance().onUnitLanded(unit);
}

// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
// onNukeDetect, onPlayerLeft, onSaveGame 이벤트를 처리할 수 있도록 메소드 추가

void GameCommander::onNukeDetect(Position target)
{
	InformationManager::Instance().nucLaunchedTime = TIME;
}

void GameCommander::onPlayerLeft(Player player)
{
}

void GameCommander::onSaveGame(string gameName)
{
}

// BasicBot 1.1 Patch End //////////////////////////////////////////////////

void GameCommander::onSendText(string text)
{
}

void GameCommander::onReceiveText(Player player, string text)
{
}

