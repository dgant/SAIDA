#include "TimerManager.h"

using namespace Stormbreaker_Time;

TimerManager::TimerManager() 
    : _timers(std::vector<BOSS::Timer>(NumTypes))
    , _barWidth(40)
{
	_timerNames.push_back("All");
	_timerNames.push_back("InformationManager");
	_timerNames.push_back("SelfBuildingPlaceFinder");
	_timerNames.push_back("BuildManager");
	_timerNames.push_back("ConstructionManager");
	_timerNames.push_back("StrategyManager");
	_timerNames.push_back("HostileManager");
	_timerNames.push_back("SoldierManager");
	_timerNames.push_back("ExploreManager");
	_timerNames.push_back("TrainManager");
	_timerNames.push_back("ComsatStationManager");
	_timerNames.push_back("CombatCommander");
	_timerNames.push_back("EngineeringBayManager");
	_timerNames.push_back("MarineManager");
	_timerNames.push_back("TankManager");
	_timerNames.push_back("VultureManager");
	//_timerNames.push_back("MicroManager");
	_timerNames.push_back("GoliathManager");
	_timerNames.push_back("WraithManager");
	_timerNames.push_back("VessleManager");
	_timerNames.push_back("DropshipManager");

//	enum Type { All, InformationManager, SelfBuildingPlaceFinder, BuildManager, ConstructionManager, StrategyManager, HostileManager, CombatCommander, SoldierManager, ExploreManager, TrainManager, ComsatStationManager, EngineeringBayManager, MarineManager, TankManager, VultureManager, MicroManager, GoliathManager, WraithManager, VessleManager, DropshipManager, NumTypes };
	enum Type { All, InformationManager, SelfBuildingPlaceFinder, BuildManager, ConstructionManager, StrategyManager, HostileManager, CombatCommander, SoldierManager, ExploreManager, TrainManager, ComsatStationManager, EngineeringBayManager, MarineManager, TankManager, VultureManager, GoliathManager, WraithManager, VessleManager, DropshipManager, NumTypes };

}

void TimerManager::startTimer(const TimerManager::Type t)
{
	_timers[t].start();
}

void TimerManager::stopTimer(const TimerManager::Type t)
{
	_timers[t].stop();
}

double TimerManager::getTotalElapsed()
{
	return _timers[0].getElapsedTimeInMilliSec();
}

void TimerManager::displayTimers(int x, int y)
{

	BWAPI::Broodwar->drawBoxScreen(x-5, y-5, x+110+_barWidth, y+5+(10*_timers.size()), BWAPI::Colors::Black, true);

	int yskip = 0;
	double total = _timers[0].getElapsedTimeInMilliSec();
	for (size_t i(0); i<_timers.size(); ++i)
	{
		double elapsed = _timers[i].getElapsedTimeInMilliSec();
        if (elapsed > 55)
        {
            BWAPI::Broodwar->printf("Timer Debug: %s %lf", _timerNames[i].c_str(), elapsed);
        }

		int width = (total == 0) ? 0 : int(_barWidth * (elapsed / total));
		BWAPI::Broodwar->drawTextScreen(x, y + yskip - 3, "\x04 %s", _timerNames[i].c_str());
		BWAPI::Broodwar->drawBoxScreen(x + 60, y + yskip, x + 60 + width + 1, y + yskip + 8, BWAPI::Colors::White);
		BWAPI::Broodwar->drawTextScreen(x + 70 + _barWidth, y + yskip - 3, "%.4lf", elapsed);
		yskip += 10;

	}
}