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

#include "Stormbreaker.h"
#include "Common.h"

using namespace MyBot;


Stormbreaker::Stormbreaker() {

}

Stormbreaker::~Stormbreaker() {
}

void Stormbreaker::Broodwar_set()
{
	//Broodwar->setCommandOptimizationLevel(1);
	Broodwar << BWAPI::Broodwar->self()->getName() << '(' << Broodwar->self()->getRace() << ')' << " vs " << BWAPI::Broodwar->enemy()->getName() << '(' << Broodwar->enemy()->getRace() << ')' << endl;
	Broodwar << "Game begins！ Good Luck！" << endl;
	Broodwar->setLocalSpeed(0);
	Broodwar->setFrameSkip(0);
	Broodwar->enableFlag(1);
}

void Stormbreaker::onStart() {
	try{
		theMap.Initialize();
		theMap.EnableAutomaticPathAnalysis();
		bool startingLocationsOK = theMap.FindBasesForStartingLocations();
		assert(startingLocationsOK);
		Broodwar_set();
		try{
			ActorCriticForProductionStrategy::Instance().RLModelInitial();//读取read下的模型文件
		}
		catch (...)
		{
			Broodwar << "Loading success...................................................." << endl;
		}
		gameCommander.onStart();
	}
	catch (...)
	{
		Broodwar << "onStart............................." << endl;
	}

}

void Stormbreaker::onEnd(bool isWinner) {
	int frameElapse = BWAPI::Broodwar->getFrameCount();
	int currentSupply = BWAPI::Broodwar->self()->supplyUsed();

	bool win = false;

	if (frameElapse >= 80000 && BWAPI::Broodwar->self()->supplyUsed() >= 180 * 2)
	{
		win = true;
	}
	else
	{
		win = isWinner;
	}
	int reward = win == true ? 10 : -10;
	
	try{

	ActorCriticForProductionStrategy::Instance().update(reward);//利用本次对战的数据，进行模型更新

	}
	catch (...)
	{

	}
	
	gameCommander.onEnd();
	/*
	int gamePlayLength = frameElapse / (24 * 60);
	std::string enemyRace;
	if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Zerg)
	{
	enemyRace = "z";
	}
	else if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran)
	{
	enemyRace = "T";
	}
	else if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Protoss)
	{
	enemyRace = "p";
	}
	else
	enemyRace = "unknow";
	*/
	/*

	if (isWinner)
	cout << "[" << TIME << "] I won the game" << endl;
	else
	cout << "[" << TIME << "] I lost the game" << endl;
	*/

	/*

	if (INFO.enemyRace == Races::Protoss && mapName.find("CircuitBreaker") != string::npos) {

	if (!SharedMemoryManager::Instance().communication)
	Logger::info("Not_Comm_" + string(isWinner ? "Win" : "Lose") + CommonUtil::getYYYYMMDDHHMMSSOfNow(), false, " ");

	else
	Logger::info("Comm_" + string(isWinner ? "Win" : "Lose") + CommonUtil::getYYYYMMDDHHMMSSOfNow(), false, " ");
	}
	}
	*/
}
/*
void Stormbreaker::onFrame() {
if (TIME == 315)
cout << "122" << endl;
try {
gameCommander.onFrame();
}
catch (...) {
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

int cntOfSvcForGas = 0;//MyBot::WorkerManager::Instance().getNumGasWorkers();
int cntOfSvcForMineral = 0;// MyBot::WorkerManager::Instance().getNumMineralWorkers();
char line[80] = { 0, };
sprintf_s(line, sizeof(line), "%d, %d, %d, %d, %d, %d, %d\n", Broodwar->getFrameCount() / 24, currentMineral, currentGas, gatheredMineral, gatheredGas, cntOfSvcForMineral, cntOfSvcForGas);
string line_string = line;
Logger::appendTextToFile(mapFileName, line_string);

}

#endif

// BasicBot 1.2 Patch End //////////////////////////////////////////////////

try {
UXManager::Instance().update();
}
#endif
}
catch (const exception &e) {
Logger::error("UXManager Error. (Error : %s)\n", e.what());
//throw e;
#endif
}
catch (...) {
Logger::error("UXManager Unknown Error.\n");
//throw;
#endif
}
}

*/

void Stormbreaker::onFrame() {
	try {
		gameCommander.onFrame();
	}
	catch (...) {
	}
	try{
		drawgameinfo();
		drawUnitStatus();
		//	Broodwar->drawTextScreen(5, 5, "%s(%s)vs%s(%s)", BWAPI::Broodwar->self()->getName(), Broodwar->self()->getRace(), BWAPI::Broodwar->enemy()->getName(), Broodwar->enemy()->getRace());
		//drawUnitStatus();
		drawBullets();
		drawVisibilityData();

	}
	catch (...)
	{ }

	// BasicBot 1.2 Patch End //////////////////////////////////////////////////
}
void Stormbreaker::drawgameinfo()
{
	int x = 5;
	int y = 5;
	Broodwar->drawTextScreen(x, y, "\x04Players:");
	Broodwar->drawTextScreen(x + 50, y, "%c%s(%s) \x04vs. %c%s(%s)",
		S->getTextColor(), S->getName().c_str(), INFO.selfRace.c_str(),
		E->getTextColor(), E->getName().c_str(), INFO.enemyRace.c_str());
	y += 12;

	Broodwar->drawTextScreen(x, y, "\x04Map:");
	Broodwar->drawTextScreen(x + 50, y, "\x03%s", Broodwar->mapFileName().c_str(), Broodwar->mapWidth(), Broodwar->mapHeight());
	y += 12;
	Broodwar->drawTextScreen(x, y, "\x04Map size:");
	Broodwar->drawTextScreen(x + 50, y, "(%d x %d size)", Broodwar->mapWidth(), Broodwar->mapHeight());
	Broodwar->setTextSize();
	y += 12;

	Broodwar->drawTextScreen(x, y, "\x04Frame:");
	Broodwar->drawTextScreen(x + 50, y, "\x04%d", Broodwar->getFrameCount());
	y += 12;
	Broodwar->drawTextScreen(x, y, "\x04Time:", Broodwar->getFrameCount());
	Broodwar->drawTextScreen(x + 50, y, "\x04%4d:%3d", (int)(Broodwar->getFrameCount() / (23.8 * 60)), (int)((int)(Broodwar->getFrameCount() / 23.8) % 60));
}
void Stormbreaker::drawUnitStatus()
{
	BWAPI::Unitset selfunits = BWAPI::Broodwar->self()->getUnits();
	std::map<UnitType, int> unitTypeCounts;
	//for (std::set<Unit*>::iterator i = myUnits.begin(); i != myUnits.end(); i++)
	for (auto unit : selfunits)
	{
		if (!unit->getType().isBuilding() && unit->getType().canAttack())
		{
			int unitWidth = unit->getType().tileWidth() * 32;
			int unitHeight = unit->getType().tileHeight() * 32 / 8;
			BWAPI::Position unitPosition = unit->getPosition();
			BWAPI::Broodwar->drawBoxMap(unitPosition.x - unitWidth / 4, unitPosition.y + unitHeight / 4, unitPosition.x + unitWidth / 4, unitPosition.y - unitHeight / 4, BWAPI::Colors::Red, true);
			double healthPercent = double(unit->getHitPoints()) / unit->getType().maxHitPoints();
			BWAPI::Broodwar->drawBoxMap(unitPosition.x - unitWidth / 4, unitPosition.y + unitHeight / 4, unitPosition.x - unitWidth / 4 + int(unitWidth * healthPercent), unitPosition.y - unitHeight / 4, BWAPI::Colors::Green, true);
		}

		if (unitTypeCounts.find(unit->getType()) == unitTypeCounts.end())
		{
			unitTypeCounts.insert(std::make_pair(unit->getType(), 0));
		}
		unitTypeCounts.find(unit->getType())->second++;
	}
	int line_build = 1;
	int line_build_no = 1;
	int sum_building = 0;
	int sum_building_no = 0;
	for (std::map<UnitType, int>::iterator i = unitTypeCounts.begin(); i != unitTypeCounts.end(); i++)
	{
		if ((*i).first.isBuilding())
		{
			Broodwar->drawTextScreen(250, 16 * line_build + 20, "%s:%d", (*i).first.getName().c_str(), (*i).second, BWAPI::Colors::Black);
			line_build++;
			sum_building += (*i).second;
		}
		else
		{
			Broodwar->drawTextScreen(400, 16 * line_build_no + 20, "%s:%d", (*i).first.getName().c_str(), (*i).second, BWAPI::Colors::Black);
			line_build_no++;
			sum_building_no += (*i).second;
		}
	}
	Broodwar->drawTextScreen(250, 20, "Total UnitsNum:%d", sum_building_no, BWAPI::Colors::Black);
	Broodwar->drawTextScreen(400, 20, "Total BuildingNum:%d", sum_building, BWAPI::Colors::Black);
	Broodwar->drawTextScreen(BWAPI::Broodwar->getMousePosition().x + 20, BWAPI::Broodwar->getMousePosition().y + 20, "%d %d", (BWAPI::Broodwar->getScreenPosition().x + BWAPI::Broodwar->getMousePosition().x) / TILE_SIZE, (BWAPI::Broodwar->getScreenPosition().y + BWAPI::Broodwar->getMousePosition().y) / TILE_SIZE);
}
void Stormbreaker::drawBullets()
{
	BWAPI::Bulletset bullets = Broodwar->getBullets();
	//for (std::set<Bullet*>::iterator i = bullets.begin(); i != bullets.end(); i++)
	for (auto bullet : bullets)
	{
		Position p = bullet->getPosition();
		double velocityX = bullet->getVelocityX();
		double velocityY = bullet->getVelocityY();
		if (bullet->getPlayer() == Broodwar->self())
		{
			Broodwar->drawLineMap(p.x, p.y, p.x + (int)velocityX, p.y + (int)velocityY, Colors::Green);
			Broodwar->drawTextMap(p.x, p.y, "\x07%s", bullet->getType().getName().c_str());
		}
		else
		{
			Broodwar->drawLineMap(p.x, p.y, p.x + (int)velocityX, p.y + (int)velocityY, Colors::Red);
			Broodwar->drawTextMap(p.x, p.y, "\x06%s", bullet->getType().getName().c_str());
		}
	}
}


void Stormbreaker::drawVisibilityData()
{
	for (int x = 0; x<Broodwar->mapWidth(); x++)
	{
		for (int y = 0; y<Broodwar->mapHeight(); y++)
		{
			if (Broodwar->isExplored(x, y))
			{
				if (Broodwar->isVisible(x, y))
					Broodwar->drawDotMap(x * 32 + 16, y * 32 + 16, Colors::Green);
				else
					Broodwar->drawDotMap(x * 32 + 16, y * 32 + 16, Colors::Blue);
			}
			else
				Broodwar->drawDotMap(x * 32 + 16, y * 32 + 16, Colors::Red);
		}
	}
}


void Stormbreaker::showPlayers()
{
	BWAPI::Playerset players = Broodwar->getPlayers();
	//for (std::set<Player*>::iterator i = players.begin(); i != players.end(); i++)
	for (auto i : players)
	{
		Broodwar->printf("Player [%d]: %s is in force: %s", i->getID(), i->getName().c_str(), i->getForce()->getName().c_str());
	}
}

void Stormbreaker::showForces()
{
	BWAPI::Forceset forces = Broodwar->getForces();
	for (auto i : forces)
	{
		BWAPI::Playerset players = i->getPlayers();
		Broodwar->printf("Force %s has the following players:", i->getName().c_str());
		for (auto j : players)
		{
			Broodwar->printf("  - Player [%d]: %s", j->getID(), j->getName().c_str());
		}
	}
}

void Stormbreaker::onUnitCreate(Unit unit) {
	gameCommander.onUnitCreate(unit);
}

void Stormbreaker::onUnitDestroy(Unit unit) {
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

void Stormbreaker::onUnitMorph(Unit unit) {
	gameCommander.onUnitMorph(unit);
}

/*
//initializes activeMetaStrategy if it has not been initialized before
if (activeMetaStrategy == NULL){

//retrieve what config says about strategy
string metaStrategyName = Configuration::getInstance()->metaStrategyID;

Logging::getInstance()->log("Meta strategy: %s", metaStrategyName.c_str());

if (metaStrategyName == "epsilon-greedy") {
activeMetaStrategy = new EpsilonGreedy();
}
else if (metaStrategyName == "probabilistic") {
activeMetaStrategy = new Probabilistic();
}
else if (metaStrategyName == "random-switch") {
activeMetaStrategy = new RandomSwitch();
}

else {	//otherwise, go to Single Choice using 'metaStrategyName' as the chosen behavior
Logging::getInstance()->log("Using SingleChoice with strategy: %s", metaStrategyName.c_str());
activeMetaStrategy = new SingleChoice(metaStrategyName);
}
//return new EpsilonGreedy();	//failsafe default...
}

return activeMetaStrategy;
}
*/

void Stormbreaker::onUnitRenegade(Unit unit) {
	gameCommander.onUnitRenegade(unit);
}

void Stormbreaker::onUnitComplete(Unit unit) {
	gameCommander.onUnitComplete(unit);
}

void Stormbreaker::onUnitDiscover(Unit unit) {
	gameCommander.onUnitDiscover(unit);
}

void Stormbreaker::onUnitEvade(Unit unit) {
	gameCommander.onUnitEvade(unit);
}

void Stormbreaker::onUnitShow(Unit unit) {
	gameCommander.onUnitShow(unit);
}

void Stormbreaker::onUnitHide(Unit unit) {
	gameCommander.onUnitHide(unit);
}

void Stormbreaker::onNukeDetect(Position target) {
	gameCommander.onNukeDetect(target);
}

