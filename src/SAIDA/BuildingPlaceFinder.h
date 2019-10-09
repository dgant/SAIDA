#pragma once

#include "../Common.h"
#include "ConstructionTask.h"
#include "../MetaType.h"
#include "BuildOrderManager.h"
#include "../InformationManager.h"
#include "../ReserveBuilding.h"

namespace MyBot
{
	struct Line {
		Position p1;
		Position p2;

		int dx;
		int dy;
		int cdx;
	};

	struct Box {
		int top;
		int left;
		int right;
		int bottom;
	};

	class BuildingPlaceFinder
	{
	private:
		BuildingPlaceFinder();

		void					setTilesToAvoid();

		int  compareHeight(const TilePosition &myPosition, const TilePosition &targetPosition) const;

		Line getLine(const Position &p1, const Position &p2);
		void getBox(Box &box, const TilePosition &p1, const TilePosition &p2);

	public:

		static BuildingPlaceFinder &Instance();

		TilePosition		getSeedPositionFromSeedLocationStrategy(BuildOrderItem::SeedPositionStrategy seedPositionStrategy) const;

		TilePosition		getBuildLocationWithSeedPositionAndStrategy(UnitType buildingType, TilePosition seedPosition = TilePositions::None, BuildOrderItem::SeedPositionStrategy seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseLocation) const;

		TilePosition		getBuildLocationNear(UnitType buildingType, TilePosition desiredPosition, bool checkForReserve, Unit worker = nullptr) const;


		word altitudeIdx = 0;
		word deltaIdx = 0;
		vector<pair<TilePosition, altitude_t>> DeltasByAscendingAltitude;
		TilePosition		getBuildLocationBySpiralSearch(UnitType unitType, TilePosition possiblePosition, bool next = false, Position possibleArea = Positions::None, bool checkOnlyBuilding = false);

	
		TilePosition		getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy seedPositionStrategy) const;
		TilePosition		getRefineryPositionNear(Base *base) const;

	
	
		bool					canBuildHere(TilePosition position, const UnitType type, const bool checkForReserve, const Unit worker = nullptr, int topSpace = 0, int leftSpace = 0, int rightSpace = 0, int bottomSpace = 0) const;
		
		bool					isBasePosition(TilePosition position, UnitType type) const;

		
		TilePosition			findNearestBuildableTile(TilePosition origin, TilePosition target, UnitType unitType);
	};
}