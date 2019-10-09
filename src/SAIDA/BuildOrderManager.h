#pragma once

#include "../Common.h"
#include "../MetaType.h"

namespace MyBot
{
	
	struct BuildOrderItem
	{
		MetaType			metaType;		
		int					priority;		
		bool				blocking;		
		TilePosition seedLocation;	
		int					producerID;		

		
		enum SeedPositionStrategy {
			MainBaseLocation,			
			FirstChokePoint,			
			FirstExpansionLocation,		
			SecondChokePoint,			
			SeedPositionSpecified,		
			SecondExpansionLocation,	
			ThirdExpansionLocation,		
			IslandExpansionLocation     
		};
		SeedPositionStrategy		seedLocationStrategy;	

		
		BuildOrderItem(MetaType _metaType, int _priority = 0, bool _blocking = true, int _producerID = -1)
			: metaType(_metaType)
			, priority(_priority)
			, blocking(_blocking)
			, producerID(_producerID)
			, seedLocation(TilePositions::None)
			, seedLocationStrategy(SeedPositionStrategy::MainBaseLocation)
		{
		}

		
		BuildOrderItem(MetaType _metaType, TilePosition _seedLocation, int _priority = 0, bool _blocking = true, int _producerID = -1)
			: metaType(_metaType)
			, priority(_priority)
			, blocking(_blocking)
			, producerID(_producerID)
			, seedLocation(_seedLocation)
			, seedLocationStrategy(SeedPositionStrategy::SeedPositionSpecified)
		{
		}

		
		BuildOrderItem(MetaType _metaType, SeedPositionStrategy _SeedPositionStrategy, int _priority = 0, bool _blocking = true, int _producerID = -1)
			: metaType(_metaType)
			, priority(_priority)
			, blocking(_blocking)
			, producerID(_producerID)
			, seedLocation(TilePositions::None)
			, seedLocationStrategy(_SeedPositionStrategy)
		{
		}



		bool operator<(const BuildOrderItem &x) const
		{
			return priority < x.priority;
		}
	};

	class BuildOrderQueue
	{
		
		deque< BuildOrderItem >			queue;

		int lowestPriority;
		int highestPriority;
		int defaultPrioritySpacing;

		
		int numSkippedItems;

	public:

		BuildOrderQueue();

		
		void queueItem(BuildOrderItem b);

		void queueAsHighestPriority(MetaType				_metaType, bool blocking = true, int _producerID = -1);
		void queueAsHighestPriority(MetaType                _metaType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseLocation, bool blocking = true);
		void queueAsHighestPriority(MetaType                _metaType, TilePosition _seedPosition, bool blocking = true);
		void queueAsHighestPriority(UnitType         _unitType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseLocation, bool blocking = true);
		void queueAsHighestPriority(UnitType         _unitType, TilePosition _seedPosition, bool blocking = true);
		void queueAsHighestPriority(TechType         _techType, bool blocking = true, int _producerID = -1);
		void queueAsHighestPriority(UpgradeType      _upgradeType, bool blocking = true, int _producerID = -1);

		
		void queueAsLowestPriority(MetaType				   _metaType, bool blocking = true, int _producerID = -1);
		void queueAsLowestPriority(MetaType                _metaType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseLocation, bool blocking = true);
		void queueAsLowestPriority(MetaType                _metaType, TilePosition _seedPosition, bool blocking = true);
		void queueAsLowestPriority(UnitType         _unitType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseLocation, bool blocking = true);
		void queueAsLowestPriority(UnitType         _unitType, TilePosition _seedPosition, bool blocking = true);
		void queueAsLowestPriority(TechType         _techType, bool blocking = true, int _producerID = -1);
		void queueAsLowestPriority(UpgradeType      _upgradeType, bool blocking = true, int _producerID = -1);

		bool removeQueue(UnitType _unitType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy, bool isSame);
		void clearAll();											

		size_t size();												
		bool isEmpty();

		BuildOrderItem getHighestPriorityItem();					
		void removeHighestPriorityItem();							

		bool canSkipCurrentItem();
		void skipCurrentItem();										
		void removeCurrentItem();									
		BuildOrderItem getNextItem();								

		
		int getItemCount(MetaType                _metaType, TilePosition queryTilePosition = TilePositions::None);
		
		int getItemCount(UnitType         _unitType, TilePosition queryTilePosition = TilePositions::None);
		int getItemCount(TechType         _techType);
		int getItemCount(UpgradeType      _upgradeType);
		bool existItem(MetaType                _metaType, TilePosition queryTilePosition = TilePositions::None);
		
		bool existItem(UnitType         _unitType, TilePosition queryTilePosition = TilePositions::None);
		bool existItem(TechType         _techType);
		bool existItem(UpgradeType      _upgradeType);

		BuildOrderItem operator [] (int i);							

		const deque< BuildOrderItem > 			*getQueue();
	};
}