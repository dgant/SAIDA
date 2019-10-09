#pragma once

#include "../Common.h"
#include "../InformationManager.h"
#include "BuildingPlaceFinder.h"
#include "../ReserveBuilding.h"
#include "../HostileManager.h"

#define CHECK_TILE_SIZE 15

namespace MyBot
{
	enum class Direction {
		LEFT,
		UP,
		RIGHT,
		DOWN,
		NONE
	};

	class SelfBuildingPlaceFinder
	{
	private:
		SelfBuildingPlaceFinder();

		const TilePosition UP = TilePosition(0, -1);
		const TilePosition DOWN = TilePosition(0, 1);
		const TilePosition LEFT = TilePosition(-1, 0);
		const TilePosition RIGHT = TilePosition(1, 0);

		
		const Area *baseArea;

		
		Unit topMineral;
		Unit bottomMineral;
		Unit leftMineral;
		Unit rightMineral;
		
		bool isLeftOfBase;
		
		bool isTopOfBase;
		
		bool isBaseTop;
		
		bool isBaseLeft;
		
		bool isExistSpace;
		
		Direction chokePointDirection;
		
		TilePosition chokePoint;
	
		short baseAreaId;
		
		TilePosition baseTilePosition;
		
		TilePosition supplyStdPosition;
		
		TilePosition factoryStdPosition;

		
		void setBasicVariable(const Base *base);
		
		void calcFirstSupplyPosition();
		
		bool calcSecondSupplyPosition(TilePosition barracksPos);
		
		void calcFirstBarracksPosition();
		
		void calcStandardPositionToStuck();
		
		TilePosition calcNextBuildPosition(TilePosition stdPosition, TilePosition lastPos);
		
		void sortBuildOrder(int strIdx, int lstIdx);
		
		int calcFactoryPosition(TilePosition factoryStdPosition, TilePosition MOVE_DIRECTION1, TilePosition MOVE_DIRECTION2, int factoryCnt, bool recursive);

		
		TilePosition tilePos[25];
		
		int idx = 0, totCnt = 0;
		
		int TOT_FACTORY_COUNT = 11;

		
		TilePosition SUPPLY_MOVE_DIRECTION[2];
		TilePosition FACTORY_MOVE_DIRECTION[2];

		
		TilePosition getTurretPositionInSecondChokePoint();

		TilePosition supplyByBarracks;

	
		void setSecondChokePointReserveBuilding();
		bool checkCanBuildHere(TilePosition tilePosition, UnitType buildingType);
		map<Position, vector<TilePosition>> secondChokePointTilesMap;
		map<Position, pair<TilePosition, TilePosition>> closestTileMap;

		
		TilePosition barracksPositionInSecondChokePoint;
		TilePosition engineeringBayPositionInSecondChokePoint;
		vector<TilePosition> supplysPositionInSecondChokePoint;
	public:
		static SelfBuildingPlaceFinder &Instance();

		TilePosition getBuildLocationWithSeedPositionAndStrategy(UnitType buildingType, TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy);

		
		void freeSecondSupplyDepot();

		
		void drawMap();
		void drawSecondChokePointReserveBuilding();

		
		void update();

		TilePosition getBarracksPositionInSCP();
		TilePosition getEngineeringBayPositionInSCP();
		TilePosition getSupplysPositionInSCP();

		bool isFixedPositionOrder(UnitType unitType);
	};
}