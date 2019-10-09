#include "MetaStrategy.h"

using namespace MyBot;
//initializes consts

/*
MetaStrategy::MetaStrategy() : rng(std::time(0)) {
name = "none";

//initalizes behaviors
//	vector<string> &portfolioNames = Configuration::getInstance()->portfolioNames;
vector<string>::iterator it;
/*
for (it = portfolioNames.begin(); it != portfolioNames.end(); it++) {
Logging::getInstance()->log("Adding '%s' to the portfolio.", it->c_str());
portfolio.insert(make_pair(*it, loadAIModule(*it)));
}
*/

/*
map<string, BWAPI::AIModule*>::iterator behv;
for (behv = portfolio.begin(); behv != portfolio.end(); behv++){
strategyNames.insert(make_pair(behv->second, behv->first));
//Logging::getInstance()->log("Added %s to reverse map", (*behv).first.c_str() );
}


}
*/
//initializes reverse map
/*
BWAPI::AIModule* MetaStrategy::getCurrentStrategy(){
return currentStrategy;
}
*/
/*
BWAPI::AIModule* MetaStrategy::getCurrentStrategy(){
return currentStrategy;
}

BWAPI::AIModule* MetaStrategy::loadAIModule(string moduleName) {
//the signature of the function that creates an AI module
typedef BWAPI::AIModule* (*PFNCreateAI)(BWAPI::Game*);

BWAPI::AIModule* newAI;

HINSTANCE hDLL;               // Handle to DLL]

Logging* logger = Logging::getInstance();
string path = Configuration::getInstance()->INPUT_DIR + moduleName + ".dll";

// attempts to load the library and call newAIModule
// on error, loads dummy AI
hDLL = LoadLibrary(path.c_str());
if (hDLL != NULL) {
logger->log("Successfully loaded %s's DLL.", moduleName.c_str());
// Obtain the AI module function
PFNCreateAI newAIModule = (PFNCreateAI)GetProcAddress(hDLL, "newAIModule");
// The two lines below might be needed in BWAPI 4.1.2
//PFNGameInit gameInitFunction = (PFNGameInit)GetProcAddress(hDLL, "gameInit");
//gameInitFunction(Broodwar);

if (newAIModule) {
// Call the AI module function and assign the client variable
newAI = newAIModule(Broodwar);
logger->log("Successfully created '%s' module.", moduleName.c_str());
}
else {
logger->log("Failed to use newAIModule for '%s'. Loading dummy", moduleName.c_str());
newAI = new AIModule();
}
}
else { //hDLL is null
logger->log("Failed to load '%s'!. Loading dummy", path.c_str());
newAI = new AIModule();
}

return newAI;
}

std::string MetaStrategy::getCurrentStrategyName(){
return strategyNames[currentStrategy];
}

void MetaStrategy::onStart() {
map<string, BWAPI::AIModule*>::iterator behv;

for(behv = portfolio.begin(); behv != portfolio.end(); behv++){
Logging::getInstance()->log("%s: onStart()", behv->first.c_str());
behv->second->onStart();
}
}

string MetaStrategy::getName(){
return name;
}

*/


/*
void MetaStrategy::printInfo() {
    Broodwar->drawTextScreen(180, 5, "\x0F%s", currentStrategyId.c_str());
}
*/

void MetaStrategy::clearBuildConstructQueue() {
	CM.clearNotUnderConstruction();
	BM.buildQueue.clearAll();
	BasicBuildStrategy::Instance().executeBasicBuildStrategy();
}

MetaStrategy::MetaStrategy()
{
	enemyInitialBuild = UnknownBuild;
	enemyMainBuild = UnknownMainBuild;
	enemyGasRushRefinery = nullptr;

	int distance = getGroundDistance(theMap.Center(), MYBASE);


	centerToBaseOfSCV = distance / Terran_SCV.topSpeed();
	timeToBuildBarrack = (int)centerToBaseOfSCV + Terran_Barracks.buildTime();

	centerToBaseOfProbe = distance / Protoss_Probe.topSpeed();
	timeToBuildGateWay = (int)centerToBaseOfProbe + Protoss_Gateway.buildTime() + Protoss_Pylon.buildTime();


	centerToBaseOfMarine = distance / Terran_Marine.topSpeed();
	centerToBaseOfZ = distance / Protoss_Zealot.topSpeed();
}

void MetaStrategy::checkEnemyInitialBuild()
{

	if (INFO.enemyRace == Races::Zerg && enemyInitialBuild != UnknownBuild) {

		if (enemyInitialBuild == Zerg_9_Drone || enemyInitialBuild == Zerg_12_Ap || enemyInitialBuild == Zerg_12_Hat)
		//	useFirstAttackUnit();

		return;
	}

	if (!INFO.getUnitInfo(enemyGasRushRefinery, E)) {

		UnitType refineryType = INFO.enemyRace == Races::Protoss ? Protoss_Assimilator : INFO.enemyRace == Races::Terran ? Terran_Refinery : Zerg_Extractor;

		for (auto refinery : INFO.getBuildings(refineryType, E)) {
			if (isSameArea(INFO.getMainBaseLocation(S)->Center(), refinery->pos())) {
				enemyGasRushRefinery = refinery->unit();
				break;
			}
		}
	}
	else {
		enemyGasRushRefinery = nullptr;
	}

//	useFirstExpansion();
}
/*
void MetaStrategy::checkEnemyMainBuild()
{
checkEnemyBuilding();
checkEnemyUnits();
checkEnemyTechNUpgrade();
}

*/
