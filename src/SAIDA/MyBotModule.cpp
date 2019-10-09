/*
+----------------------------------------------------------------------+
| UAlbertaBot                                                          |
+----------------------------------------------------------------------+
| University of Alberta - AIIDE StarCraft Competition                  |
+----------------------------------------------------------------------+
|                                                                      |
+----------------------------------------------------------------------+
| Author: David Churchill <dave.churchill@gmail.com>                   |
+----------------------------------------------------------------------+
*/

#include "MyBotModule.h"
#include "Common.h"

using namespace MyBot;

#ifdef SERVERLOG
void trans_func(unsigned int errCode, EXCEPTION_POINTERS *pException) {
	throw SAIDA_Exception(errCode, pException);
}
#endif

MyBotModule::MyBotModule() {
#ifdef SERVERLOG
	original = _set_se_translator(trans_func);
#endif

	FileUtil::MakeDirectory(Config::Files::WriteDirectory.c_str());
	Config::Files::LogFilename = CommonUtil::getYYYYMMDDHHMMSSOfNow() + Broodwar->mapFileName() + Broodwar->enemy()->getName() + "_LastGameLog.dat";
	Config::Files::TimeoutFilename = "Timeout_" + CommonUtil::getYYYYMMDDHHMMSSOfNow() + Broodwar->mapFileName() + Broodwar->enemy()->getName() + ".dat";
	Config::Files::ErrorLogFilename = "Error_" + CommonUtil::getYYYYMMDDHHMMSSOfNow() + Broodwar->mapFileName() + Broodwar->enemy()->getName() + ".dat";
}

MyBotModule::~MyBotModule() {
#ifdef SERVERLOG
	_set_se_translator(original);
#endif
}

void MyBotModule::onStart() {
	Logger::debug("game start!!\n");

#ifdef _LOGGING
	cout << "Map File Name :: " << Broodwar->mapFileName() << endl;
	mapFileName = Broodwar->mapFileName() + "_" + CommonUtil::getYYYYMMDDHHMMSSOfNow() + ".csv";
	string header = "SECONDS, REMAINING_MINERAL, REMAINING_GAS, GATHERED_MINERAL, GATHERED_GAS, NUMBER_OF_SCV_OF_FOR_MINERAL, NUMBER_OF_SCV_FOR_GAS\n";
	Logger::appendTextToFile(mapFileName, header);
#endif // _LOGGING

	if (Config::BWAPIOptions::EnableCompleteMapInformation)
	{
		Broodwar->enableFlag(Flag::CompleteMapInformation);
	}

	if (Config::BWAPIOptions::EnableUserInput)
	{
		Broodwar->enableFlag(Flag::UserInput);
	}

	Broodwar->setCommandOptimizationLevel(1);

	try
	{
		// Retrieve you and your enemy's races. enemy() will just return the first enemy.
		// If you wish to deal with multiple enemies then you must use enemies().
		if (Broodwar->enemy()) // First make sure there is an enemy
			Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << endl;

		Broodwar << "Map initialization..." << endl;

		theMap.Initialize();
		theMap.EnableAutomaticPathAnalysis();
		bool startingLocationsOK = theMap.FindBasesForStartingLocations();
		assert(startingLocationsOK);

		//BWEM::utils::MapPrinter::Initialize(&theMap);
		//BWEM::utils::printMap(theMap);      // will print the map into the file <StarCraftFolder>bwapi-data/map.bmp
		//BWEM::utils::pathExample(theMap);   // add to the printed map a path between two starting locations

		Broodwar << "gg" << endl;
	}
	catch (const exception &e)
	{
		Broodwar << "EXCEPTION: " << e.what() << endl;
	}

	Broodwar->setLocalSpeed(Config::BWAPIOptions::SetLocalSpeed);
	Broodwar->setFrameSkip(Config::BWAPIOptions::SetFrameSkip);

	gameCommander.onStart();
}

void MyBotModule::onEnd(bool isWinner) {
	if (isWinner)
		cout << "[" << TIME << "] I won the game" << endl;
	else
		cout << "[" << TIME << "] I lost the game" << endl;

	gameCommander.onEnd(isWinner);

	Broodwar->sendText("Game End");
	Logger::debug("game end!\n");
}

void MyBotModule::onFrame() {
	try {
		gameCommander.onFrame();
	}
	catch (...) {
#ifndef SERVERLOGDLL
		Logger::error("Detail Logs\nMainStrategy : %s\nEnemyRace : %s\nMy Base : (%d, %d)\nEnemy Base : (%d, %d)\nMainTarget : (%d, %d)"
					  , SM.getMainStrategy().c_str()
					  , INFO.enemyRace.c_str()
					  , INFO.getMainBaseLocation(S)->getTilePosition().x, INFO.getMainBaseLocation(S)->getTilePosition().y
					  , INFO.getMainBaseLocation(E)->getTilePosition().x, INFO.getMainBaseLocation(E)->getTilePosition().y
					  , SM.getMainAttackPosition().x / 32, SM.getMainAttackPosition().y / 32);
		//throw;
#endif
	}

#ifdef _LOGGING

	if (Broodwar->getFrameCount() % (24 * Config::Propeties::duration) == 0)
	{
		int prevGas = 0;
		int prevMineral = 0;
		int gatheredGas = Broodwar->self()->gatheredGas();
		int gatheredMineral = Broodwar->self()->gatheredMinerals();
		int currentMineral = Broodwar->self()->minerals();
		int currentGas = Broodwar->self()->gas();

		//queue<int> mQueue = MyBot::InformationManager::Instance().mineralAccumulatedQueue;
		//queue<int> gQueue = MyBot::InformationManager::Instance().gasAccumulatedQueue;

		//mQueue.push(gatheredMineral);
		//gQueue.push(gatheredGas);

		//if (mQueue.size() > Config::Propeties::duration) {
		//prevMineral = mQueue.front();
		//mQueue.pop();
		//}
		//if (gQueue.size() > Config::Propeties::duration) {
		//prevGas = gQueue.front();
		//gQueue.pop();
		//}

		// ScvManager로 대체 필요
		int cntOfSvcForGas = 0;//MyBot::WorkerManager::Instance().getNumGasWorkers();  ///일꾼 가스
		int cntOfSvcForMineral = 0;// MyBot::WorkerManager::Instance().getNumMineralWorkers();  //일꾼 미네랄
		//int currentFrame = InformationManager::Instance().durationIdx;  // 프레임 (듀레이션) 인덱스

		//어떤 맵, 프레임 인덱스, 일꾼 미네랄, 일꾼가스
		char line[80] = { 0, };
		sprintf_s(line, sizeof(line), "%d, %d, %d, %d, %d, %d, %d\n", Broodwar->getFrameCount() / 24, currentMineral, currentGas, gatheredMineral, gatheredGas, cntOfSvcForMineral, cntOfSvcForGas);
		string line_string = line;
		Logger::appendTextToFile(mapFileName, line_string);

	}

#endif

	// BasicBot 1.2 Patch End //////////////////////////////////////////////////

	// 화면 출력 및 사용자 입력 처리
	// 빌드서버에서는 Dependency가 없는 빌드서버 전용 UXManager 를 실행시킵니다
	try {
		UXManager::Instance().update();
	}
	catch (SAIDA_Exception e) {
#ifndef SERVERLOGDLL
		Logger::error("UXManager Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		//throw e;
#endif
	}
	catch (const exception &e) {
#ifndef SERVERLOGDLL
		Logger::error("UXManager Error. (Error : %s)\n", e.what());
		//throw e;
#endif
	}
	catch (...) {
#ifndef SERVERLOGDLL
		Logger::error("UXManager Unknown Error.\n");
		//throw;
#endif
	}

	// BasicBot 1.1 Patch End //////////////////////////////////////////////////
}

// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
// 타임아웃 패배, 자동 패배 체크 추가

void MyBotModule::onUnitCreate(Unit unit) {
	gameCommander.onUnitCreate(unit);
}

void MyBotModule::onUnitDestroy(Unit unit) {
	gameCommander.onUnitDestroy(unit);

	try
	{
		if (unit->getType().isMineralField())    theMap.OnMineralDestroyed(unit);
		else if (unit->getType().isSpecialBuilding()) theMap.OnStaticBuildingDestroyed(unit);
	}
	catch (const exception &e)
	{
		Broodwar << "EXCEPTION: " << e.what() << endl;
	}
}

void MyBotModule::onUnitMorph(Unit unit) {
	gameCommander.onUnitMorph(unit);
}

void MyBotModule::onUnitRenegade(Unit unit) {
	gameCommander.onUnitRenegade(unit);
}

void MyBotModule::onUnitComplete(Unit unit) {
	gameCommander.onUnitComplete(unit);
}

void MyBotModule::onUnitDiscover(Unit unit) {
	gameCommander.onUnitDiscover(unit);
}

void MyBotModule::onUnitEvade(Unit unit) {
	gameCommander.onUnitEvade(unit);
}

void MyBotModule::onUnitShow(Unit unit) {
	gameCommander.onUnitShow(unit);
}

void MyBotModule::onUnitHide(Unit unit) {
	gameCommander.onUnitHide(unit);
}

void MyBotModule::onNukeDetect(Position target) {
	gameCommander.onNukeDetect(target);
}

void MyBotModule::onPlayerLeft(Player player) {
	gameCommander.onPlayerLeft(player);
}

void MyBotModule::onSaveGame(string gameName) {
	gameCommander.onSaveGame(gameName);
}

void MyBotModule::ParseTextCommand(const string &commandString)
{
	// Make sure to use %s and pass the text as a parameter,
	// otherwise you may run into problems when you use the %(percent) character!

	Player self = Broodwar->self();
	bool speedChange = false;

	if (commandString == "afap" || commandString == "vf") {
		Config::BWAPIOptions::SetLocalSpeed = 0;
		speedChange = true;
	}
	else if (commandString == "fast" || commandString == "f") {
		Config::BWAPIOptions::SetLocalSpeed = 24;
		speedChange = true;
	}
	else if (commandString == "slow" || commandString == "s") {
		Config::BWAPIOptions::SetLocalSpeed = 42;
		speedChange = true;
	}
	else if (commandString == "asap" || commandString == "vs") {
		Config::BWAPIOptions::SetLocalSpeed = 300;
		speedChange = true;
	}
	else if (commandString == "+") {
		Config::BWAPIOptions::SetLocalSpeed /= 2;
		speedChange = true;
	}
	else if (commandString == "-") {
		Config::BWAPIOptions::SetLocalSpeed *= 2;
		speedChange = true;
	}
	else if (commandString == "fc") {
		Config::Debug::DrawLastCommandInfo = !Config::Debug::DrawLastCommandInfo;
	}
	else if (commandString == "st")
	{
		Config::Debug::DrawUnitStatus = !Config::Debug::DrawUnitStatus;
	}
	else if (commandString == "mu")
	{
		Config::Debug::DrawMyUnit = !Config::Debug::DrawMyUnit;

		if (Config::Debug::DrawMyUnit)
			Config::Debug::DrawEnemyUnit = false;
	}
	else if (commandString == "eu")
	{
		Config::Debug::DrawEnemyUnit = !Config::Debug::DrawEnemyUnit;

		if (Config::Debug::DrawEnemyUnit)
			Config::Debug::DrawMyUnit = false;
	}

	if (speedChange) {
		Broodwar->setLocalSpeed(Config::BWAPIOptions::SetLocalSpeed);
		Broodwar->setFrameSkip(0);
	}

	if (commandString == "endthegame") {
		//bwapi->setFrameSkip(16);   // Not needed if using setGUI(false).
		Broodwar->setGUI(false);
	}

	else if (commandString == "b") {
		Broodwar->sendText("black sheep wall");
	}
	else if (commandString == "show") {
		Broodwar->sendText("show me the money");
	}
}

void MyBotModule::onSendText(string text)
{
	ParseTextCommand(text);

	gameCommander.onSendText(text);

	BWEM::utils::MapDrawer::ProcessCommand(text);

	Broodwar->sendText("%s", text.c_str());
}

void MyBotModule::onReceiveText(Player player, string text) {
	gameCommander.onReceiveText(player, text);
}
