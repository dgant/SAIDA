#pragma once
#include "../Common.h"
#include "Timer.hpp"

namespace Stormbreaker_Time
{

class TimerManager
{
	std::vector<BOSS::Timer> _timers;
	std::vector<std::string> _timerNames;

	int _barWidth;

public:

	//enum Type { All, InformationManager, SelfBuildingPlaceFinder, BuildManager, ConstructionManager, StrategyManager, MicroManager, HostileManager, CombatCommander, SoldierManager, ExploreManager, TrainManager, ComsatStationManager, EngineeringBayManager, MarineManager, TankManager, VultureManager, GoliathManager, WraithManager, VessleManager, DropshipManager, NumTypes };
	enum Type { All, InformationManager, SelfBuildingPlaceFinder, BuildManager, ConstructionManager, StrategyManager, HostileManager, CombatCommander, SoldierManager, ExploreManager, TrainManager, ComsatStationManager, EngineeringBayManager, MarineManager, TankManager, VultureManager, GoliathManager, WraithManager, VessleManager, DropshipManager, NumTypes };

	TimerManager();

	void startTimer(const TimerManager::Type t);

	void stopTimer(const TimerManager::Type t);

	double getTotalElapsed();

	void displayTimers(int x, int y);
};

}