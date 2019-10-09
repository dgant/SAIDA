#include "GameCommander.h"
void timeout(long frametime)
{
	if (frametime > 42)
	{
		Broodwar << "Time out and cost:" << frametime << endl;
		return;
	}
}
using namespace MyBot;

GameCommander::GameCommander() {
	isToFindError = false;
	memset(time, 0, sizeof(time));
	timeout_10s = 0;
	timeout_1s = 0;
	timeout_55ms = 0;
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
	}//可以删除,应该可以找到开始的地方吧

	StrategyManager::Instance().onStart();//删除策略 on Start函数为空
}


void GameCommander::onFrame()
{
	_timerManager.startTimer(Stormbreaker_Time::TimerManager::All);
	/*
	long start;
	long finished;
	long duration;
	int duration_s;
	*/
	if (Broodwar->isPaused()
			|| Broodwar->self() == nullptr || Broodwar->self()->isDefeated() || Broodwar->self()->leftGame()
			|| Broodwar->enemy() == nullptr || Broodwar->enemy()->isDefeated() || Broodwar->enemy()->leftGame()) {
		return;
	}//敌我双方被打败或者离开游戏自动结束
	//start = clock();
	_timerManager.startTimer(Stormbreaker_Time::TimerManager::InformationManager);
	try {

		InformationManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::InformationManager);
	_timerManager.startTimer(Stormbreaker_Time::TimerManager::SelfBuildingPlaceFinder);
	try {

		SelfBuildingPlaceFinder::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::SelfBuildingPlaceFinder);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::BuildManager);
	try {

		BuildManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::BuildManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::ConstructionManager);
	try {

		ConstructionManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::ConstructionManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::StrategyManager);
	try {

		StrategyManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::StrategyManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::HostileManager);
	try {

		HostileManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::HostileManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::SoldierManager);
	try {

		SoldierManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::SoldierManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::ExploreManager);
	try {

		ExploreManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::ExploreManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::TrainManager);
	try {

		TrainManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::TrainManager);
	/*
	_timerManager.startTimer(Stormbreaker_Time::TimerManager::ComsatStationManager);
	try {

	ComsatStationManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::ComsatStationManager);
	_timerManager.startTimer(Stormbreaker_Time::TimerManager::EngineeringBayManager);
	try {

	EngineeringBayManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::EngineeringBayManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::MarineManager);
	try {

	MarineManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::MarineManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::TankManager);
	try {

	TankManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::TankManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::VultureManager);
	try {

	VultureManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::VultureManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::GoliathManager);
	try {

	GoliathManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::GoliathManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::WraithManager);
	try {

	WraithManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::WraithManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::VessleManager);
	try {

	VessleManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::VessleManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::DropshipManager);
	try {

	DropshipManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::DropshipManager);
	*/
	UnitManager();
	_timerManager.displayTimers(490, 225);
}
void GameCommander::UnitManager()
{
	_timerManager.startTimer(Stormbreaker_Time::TimerManager::ComsatStationManager);
	try {

		ComsatStationManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::ComsatStationManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::CombatCommander);
	try {

	Combat::CombatCommander::Instance().update();
	}
	catch (...) {
	}

	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::CombatCommander);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::EngineeringBayManager);
	try {

		EngineeringBayManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::EngineeringBayManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::MarineManager);
	try {

		MarineManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::MarineManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::TankManager);
	try {

		TankManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::TankManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::VultureManager);
	try {

		VultureManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::VultureManager);
	/*
	_timerManager.startTimer(Stormbreaker_Time::TimerManager::MicroManager);
	try {

	Micro::MicroManager.update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::MicroManager);

	*/

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::GoliathManager);
	try {

		GoliathManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::GoliathManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::WraithManager);
	try {

		WraithManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::WraithManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::VessleManager);
	try {

		VessleManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::VessleManager);

	_timerManager.startTimer(Stormbreaker_Time::TimerManager::DropshipManager);
	try {

		DropshipManager::Instance().update();
	}
	catch (...) {
	}
	_timerManager.stopTimer(Stormbreaker_Time::TimerManager::DropshipManager);
}
void GameCommander::onEnd()
{

}

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


void GameCommander::onUnitComplete(Unit unit)
{
	InformationManager::Instance().onUnitComplete(unit);


	if (unit->getPlayer() == S && unit != nullptr) {
		if (unit->getType() == Terran_SCV || unit->getType().isResourceDepot())
			SoldierManager::Instance().onUnitCompleted(unit);
		else if (unit->getType() == Terran_Siege_Tank_Tank_Mode || unit->getType() == Terran_Siege_Tank_Siege_Mode)
			TankManager::Instance().onUnitCompleted(unit);
	}

}



void GameCommander::onUnitDestroy(Unit unit)
{
	if (unit->getPlayer() == S) {


		if (unit->getType() == Terran_Vulture)
			VultureManager::Instance().onUnitDestroyed(unit);
		
		else if (unit->getType() == Terran_SCV) {
			SoldierManager::Instance().onUnitDestroyed(unit);
			ExploreManager::Instance().onUnitDestroyed(unit);
		}
		
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
	if (unit->getType().isRefinery())//单位类型为炼油厂
	{
		if (unit->getPlayer() == S)
			InformationManager::Instance().onUnitCreate(unit);
		else
			InformationManager::Instance().onUnitShow(unit);
	}
	/*
	if (unit == NULL)
	{
	return;
	}

	if (myArmy.find(unit->getType()) == myArmy.end())
	{
	BWAPI::UnitType u = unit->getType();
	return;
	}

	unRallyArmy.push_back(unit);
	*/

}

void GameCommander::onUnitDiscover(Unit unit)
{
}

void GameCommander::onUnitEvade(Unit unit)
{
}

void GameCommander::onUnitLifted(Unit unit)
{
	if (unit->getPlayer() == S)//获取属于这个单位的队员
		SoldierManager::Instance().onUnitLifted(unit);
}

void GameCommander::onUnitLanded(Unit unit)
{
	if (unit->getPlayer() == S)
		SoldierManager::Instance().onUnitLanded(unit);
}



void GameCommander::onNukeDetect(Position target)
{
	InformationManager::Instance().nucLaunchedTime = TIME;
}
void GameCommander::Attack()
{
	/*
	// do init first


	isNeedDefend = false;
	isNeedTacticDefend = false;
	zerglingHarassFlag = true;

	//get the natural choke
	BWAPI::Position natrualLocation = BWAPI::Position(InformationManager::Instance().getFirstWaitLinePosition());

	baseChokePosition = BWTA::getNearestChokepoint(BWAPI::Broodwar->self()->getStartLocation())->getCenter();
	double2 direc = baseChokePosition - natrualLocation;
	double2 direcNormal = direc / direc.len();

	int targetx = natrualLocation.x + int(direcNormal.x * 32 * 3);
	int targety = natrualLocation.y + int(direcNormal.y * 32 * 3);

	rallyPosition = BWAPI::Position(targetx, targety);

	triggerZerglingBuilding = true;


	lastAttackPosition = BWAPI::Positions::None;
	defendAddSupply = 0;
	hasWorkerScouter = false;
	isFirstMutaAttack = true;
	isFirstHydraAttack = true;

	BWTA::Region* nextBase = BWTA::getRegion(InformationManager::Instance().getFirstWaitLinePosition());
	double maxDist = 0;
	BWTA::Chokepoint* maxChoke = nullptr;
	*/
	

}

