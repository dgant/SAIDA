#pragma once

#include "../Common.h"
#include "BuildOrderManager.h"
#include "ConstructionManager.h"

#define BM BuildManager::Instance()

namespace MyBot
{
	/// @see ConstructionManager
	class BuildManager
	{
		BuildManager();
		Unit			getProducer(MetaType t, Position closestTo = Positions::None, int producerID = -1);

		Unit         getClosestUnitToPosition(const Unitset &units, Position closestTo);

		bool                canMakeNow(Unit producer, MetaType t);

		TilePosition getDesiredPosition(UnitType unitType, TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy);

		void				checkBuildOrderQueueDeadlockAndAndFixIt();

	public:
		static BuildManager 	&Instance();
		void				update();
		BuildOrderQueue     buildQueue;

		BuildOrderQueue 	*getBuildQueue();

		bool				isProducerWillExist(UnitType producerType);

		bool                hasEnoughResources(MetaType type);

		int                 getAvailableMinerals();
		int                 getAvailableGas();

		int                 getFirstItemMineral();
		int                 getFirstItemGas();
	};


}