#pragma once

#include "Common.h"
#include "InformationManager.h"
#include "BuildingStrategy/BuildManager.h"
#include "BuildingStrategy/ConstructionManager.h"
#include "ExploreManager.h"
#include "StrategyManager.h"
#include "HostileManager.h"
#include "TrainManager.h"
#include "UnitManager\ComsatStationManager.h"
#include "UnitManager\EngineeringBayManager.h"
#include "UnitManager\MarineManager.h"
#include "UnitManager\SoldierManager.h"
#include "UnitManager\TankManager.h"
#include "UnitManager\VultureManager.h"
#include "UnitManager\GoliathManager.h"
#include "UnitManager\WraithManager.h"
#include "UnitManager\VessleManager.h"
#include "UnitManager\CombatCommander.h"
#include "UnitManager\MicroManager.h"
#include "UnitManager\DropshipManager.h"
#include "ActorCriticForProductionStrategy.h"
#include "Timeout\TimerManager.h"

namespace MyBot
{
	class UnitToAssign
	{
	public:

		BWAPI::Unit unit;
		bool isAssigned;

		UnitToAssign(BWAPI::Unit u)
		{
			unit = u;
			isAssigned = false;
		}
	};

	class GameCommander
	{
		
		bool isToFindError;
		clock_t time[20];
		int timeout_10s = 0;
		bool hasWorkerScouter = false;
		bool isFirstMutaAttack = true;
		bool isFirstHydraAttack = true;
		BWAPI::Position			baseChokePosition, lastAttackPosition;
		BWAPI::Position rallyPosition;
		bool triggerZerglingBuilding = true;
		std::set<BWAPI::Unit> enemyInvadeSet;
		BWAPI::Unitset          _validUnits;
		BWAPI::Unitset          _combatUnits;
		BWAPI::Unitset          _scoutUnits;
		int timeout_1s = 0;
		int timeout_55ms = 0;
		Stormbreaker_Time::TimerManager		    _timerManager;
		bool isNeedDefend = false;
		bool isNeedTacticDefend = false;
		bool zerglingHarassFlag = true;
		int defendAddSupply = 0;
		/*
		int						_surrenderTime;    // for giving up early

		bool                    _initialScoutSet;
		bool					_scoutAlways;
		bool					_scoutIfNeeded;

		void                    assignUnit(BWAPI::Unit unit, BWAPI::Unitset & set);
		bool                    isAssigned(BWAPI::Unit unit) const;
		*/


	public: 

		static GameCommander &Instance();

		GameCommander();
		~GameCommander();

	
		void onStart();
		
	
		void onFrame();
		void onEnd();
		void Attack();
		void UnitManager();
		void onUnitCreate(Unit unit);
	
		void onUnitDestroy(Unit unit);
		void onUnitMorph(Unit unit);
		void onUnitRenegade(Unit unit);
	
		void onUnitComplete(Unit unit);
	
		void onUnitDiscover(Unit unit);
		void onUnitEvade(Unit unit);
		void onUnitShow(Unit unit);
		void onUnitHide(Unit unit);
		void onUnitLanded(Unit unit);
		void onUnitLifted(Unit unit);
		/*
		void onUnitHide(BWAPI::Unit unit);
		void onUnitCreate(BWAPI::Unit unit);
		void onUnitComplete(BWAPI::Unit unit);
		void onUnitRenegade(BWAPI::Unit unit);
		void onUnitDestroy(BWAPI::Unit unit);
		void onUnitMorph(BWAPI::Unit unit);
		*/
		void onNukeDetect(Position target);

	};

}