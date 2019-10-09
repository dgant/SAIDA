#pragma once

#include "../Common.h"
#include "../ExploreManager.h"
#include "BuildingPlaceFinder.h"
#include "../InformationManager.h"
#include "../StrategyManager.h"
#include "../UnitManager\SoldierManager.h"

#define CM ConstructionManager::Instance()

namespace MyBot
{

	class ConstructionManager
	{
		ConstructionManager();

		vector<ConstructionTask> constructionQueue;

		int             reservedMinerals;				///< minerals reserved for planned buildings
		int             reservedGas;					///< gas reserved for planned buildings

		bool            isBuildingPositionExplored(const ConstructionTask &b) const;


		void            removeCompletedConstructionTasks(const vector<ConstructionTask> &toRemove);
		
		void			cancelNotUnderConstructionTasks(const vector<ConstructionTask> &toRemove);

		
		void            validateWorkersAndBuildings();
	
		void            assignWorkersToUnassignedBuildings();
		
		void            constructAssignedBuildings();
		
		void            checkForStartedConstruction();

		void            checkForDeadTerranBuilders();
	
		void            checkForCompletedBuildings();

	
		void            checkForDeadlockConstruction();
		clock_t time[10];
	public:

		static ConstructionManager 	&Instance();

	
		void                update();


		const vector<ConstructionTask> *getConstructionQueue();

		void                addConstructionTask(UnitType type, TilePosition desiredPosition, Unit builder = nullptr);
	
		void				cancelConstructionTask(ConstructionTask ct);

		int                 getConstructionQueueItemCount(UnitType queryType, TilePosition queryTilePosition = TilePositions::None, int maxRange = 16);
	
		bool                existConstructionQueueItem(UnitType queryType, TilePosition queryTilePosition = TilePositions::None, int maxRange = 16);

		int                 getReservedMinerals();
		
		int                 getReservedGas();
	
		void				clearNotUnderConstruction();
	};
}