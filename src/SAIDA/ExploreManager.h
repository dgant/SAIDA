#pragma once

#include "Common.h"
#include "InformationManager.h"
#include "UnitManager/ScvState.h"
#include "BuildingStrategy/BuildManager.h"

namespace MyBot
{
	namespace ScoutStatus
	{
		enum {
			NoScout = 0,						
			AssignScout,						
			LookAroundMyMainBase,				
			WaitAtMySecondChokePoint,			
			MovingToAnotherBaseLocation,		
			MoveAroundEnemyBaseLocation,		
			CheckEnemyFirstExpansion,		
			WaitAtEnemyFirstExpansion,			
			FinishFirstScout					
		};
	}

	
	class ExploreManager
	{
		ExploreManager();
		BWAPI::Unit						_workerScout;
		std::string                     _scoutStatus;
		std::string                     _gasStealStatus;
		bool							_scoutLocationOnly;
		bool			                _scoutUnderAttack;
		bool							_tryGasSteal;
		bool                            _didGasSteal;
		/*		
		bool USING_GAMECOMMANDER = true;	    // toggle GameCommander, effectively UAlbertaBot
		bool USING_ExploreManager = true;
		bool USING_COMBATCOMMANDER = true;
		bool USING_MACRO_SEARCH = true;	    // toggle use of Build Order Search, currently no backup
		bool USING_STRATEGY_IO = false;	// toggle the use of file io for strategy
		bool USING_UNIT_COMMAND_MGR = false;    // handles all unit commands
		*/
		bool                            _gasStealFinished;
		int                             _currentRegionVertexIndex;
		int                             _previousScoutHP;
		std::vector<BWAPI::Position>    _enemyRegionVertices;


		int								currentScoutStatus;
		int							currentScoutUnitTIME;
		Unit						currentScoutUnit;
		UnitInfo 					*currentScoutUnitInfo;
		Unit						dualScoutUnit;

		int								currentScoutTargetDistance;

		Position					hidingPosition = Positions::Unknown; 
		int								startHidingframe;

		/*
					Modules::USING_GAMECOMMANDER = true;
				Modules::USING_ExploreManager = true;
				Modules::USING_COMBATCOMMANDER = true;
				Modules::USING_MACRO_SEARCH = true;
				Modules::USING_STRATEGY_IO = false;
				Modules::USING_UNIT_COMMAND_MGR = false;

				Modules::USING_REPLAY_VISUALIZER = false;
				Modules::USING_MICRO_SEARCH = false;
				Modules::USING_ENHANCED_INTERFACE = false;
		*/
		bool							assignScoutIfNeeded();
		
		
		void waitTillFindFirstExpansion();

		
		void                            moveScoutUnit();
		void                            calculateEnemyAreaVertices();
		void                            drawScoutInformation(int x, int y);
		void                            moveScout();
		bool					isToFinishScvScout;
		int						scoutDeadCount;

		
		BWEM::Base 			*cardinalPoints[4];
		BWEM::Base 			*explorePoints[4];

		bool							checkCardinalPoints;
		BWEM::Base 			*closestBaseLocation;
		bool							isPassThroughChokePoint;



	public:
	
		static ExploreManager &Instance();

		
		void update();

		
		Unit getScoutUnit();

		int getScoutStatus();
		void onUnitDestroyed(Unit scouter);

	private:
		
		void		moveScoutInEnemyBaseArea(UnitInfo   *scouter);
		
		void		moveScoutToEnemBaseArea(UnitInfo   *scouter, Position targetPos);
		
		void		checkEnemyBuildingsFromMyPos(UnitInfo *scouter);
		
		void		checkEnemyRangedUnitsFromMyPos(UnitInfo *scouter);
		int getClosestVertexIndex(BWAPI::Unit unit);
		bool enemyWorkerInRadius();
		void followPerimeter();

		Position	scouterNextPosition = Positions::None;
		UnitInfo	*enemAttackBuilding = nullptr;
		bool	isExistDangerUnit = false;
		int		attackBuildingDist = INT_MAX;
		int		currentScoutArea = -1;
		
		int		changeScoutingAreaTIME = 0;
		int		goSpecialPosition = 0;
		int		immediateRangedDamage = 0;
		map<Position, int> enemBasePositions;
		int direction = 1;
	};
}