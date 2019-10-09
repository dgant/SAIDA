#pragma once

#include "Common.h"
#include "BuildingStrategy/SelfBuildingPlaceFinder.h"

namespace MyBot
{
	enum class ReserveTypes {
		UNIT_TYPE,
		SIZE,
		SPACE,	
		MINERALS, 
		GAS,	
		AVOID,  
		UNBUILDABLE	
	};
	class Building {
	private:
		int order;
		TilePosition pos;
		TilePosition tileSize;
		vector<UnitType> possibleList;
		ReserveTypes reserveType;
		Unit assignedWorker;
		int lastAssignFrame;
		vector< TilePosition > spaceList;

	public:
		Building() {};
		Building(int order, TilePosition pos, TilePosition tileSize, ReserveTypes reserveType);
		Building(int order, TilePosition pos, vector<UnitType> unitTypes);

		bool isReservedFor(TilePosition position, TilePosition tileSize) const;
		bool isReservedFor(TilePosition position, UnitType type) const;
		bool canAssignToType(UnitType type, bool checkAlreadySet = false) const;
		void assignWorker(Unit worker);
		void unassignWorker();
		int getOrder();
		void setOrder(int order);
		ReserveTypes getTypes() {
			return reserveType;
		}
		bool isBuildable() {
			return reserveType == ReserveTypes::UNIT_TYPE || reserveType == ReserveTypes::SIZE;
		}
		static bool isBuildable(ReserveTypes reserveType) {
			return reserveType == ReserveTypes::UNIT_TYPE || reserveType == ReserveTypes::SIZE;
		}

		const TilePosition getTilePosition() const {
			return pos;
		}
		TilePosition getTileSize() {
			return tileSize;
		}
		const vector<UnitType> getUnitTypes() const {
			return possibleList;
		}


		const TilePosition TopLeft() const {
			return pos;
		}


		const TilePosition BottomRight() const {
			if (reserveType == ReserveTypes::UNIT_TYPE) return pos + possibleList.front().tileSize();
			else return pos + tileSize;
		}

	
		const Position Center() const {
			return ((Position)TopLeft() + (Position)BottomRight()) / 2;
		}

		
		void addSpaceTile(int x, int y) {
			spaceList.emplace_back(TilePosition(x, y));
		}

		vector< TilePosition > getSpaceList() {
			return spaceList;
		}

		void addType(vector<UnitType> unitTypes) {
			if (reserveType != ReserveTypes::UNIT_TYPE) {
				reserveType = ReserveTypes::UNIT_TYPE;
			}

			for (auto unitType : unitTypes)
				possibleList.push_back(unitType);
		}

		bool operator == (const Building &building) const
		{
			return this->pos == building.pos;
		};

		
		string toString() {
			string text;

			if (reserveType == ReserveTypes::UNIT_TYPE) {
				for (UnitType type : possibleList) {
					string name = type.getName();
					text += (name.substr(name.find("_") + 1) + "\n");
				}
			}
			else
				text = "(" + to_string(tileSize.x) + ", " + to_string(tileSize.y) + ")";

			return to_string(order) + " " + text;
		}
	};

	class ReserveBuilding
	{
	private:
		ReserveBuilding();

		int reserveOrder;
		int firstReserveOrder;
		vector< vector<short> > _reserveMap;
		vector< vector<bool> > _avoidMap;
		vector< Building > _reserveList;
		vector< Building > _avoidList;

		bool					reserveTiles(Building &building, TilePosition position, int width, int height, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool canBuildAddon = false, bool forceReserve = false);
		void					freeTiles(int width, int height, TilePosition position);
		bool					canReserveHere(TilePosition position, UnitType type, int width, int height, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool canBuildAddon) const;

	public:
		static ReserveBuilding &Instance();

	
		bool					reserveTiles(TilePosition position, TilePosition size, int topSpace, int leftSpace, int rightSpace, int bottomSpace, ReserveTypes reserveType = ReserveTypes::SIZE);
		bool					reserveTiles(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool useTypeAddon = true);
		bool					reserveTilesFirst(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace);
	
		void					forceReserveTiles(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace);
		void					forceReserveTilesFirst(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace);

	
		void					freeTiles(TilePosition position, TilePosition size);
		void					freeTiles(TilePosition position, UnitType type, bool freeAddon = false);

	
		void					modifyReservePosition(UnitType unitType, TilePosition fromPosition, TilePosition toPosition);


		bool					canReserveHere(TilePosition position, int width, int height, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool canBuildAddon = false) const {
			return canReserveHere(position, UnitTypes::None, width, height, topSpace, leftSpace, rightSpace, bottomSpace, canBuildAddon);
		}
		bool					canReserveHere(TilePosition position, TilePosition size, int topSpace, int leftSpace, int rightSpace, int bottomSpace) const {
			return canReserveHere(position, UnitTypes::None, size.x, size.y, topSpace, leftSpace, rightSpace, bottomSpace, false);
		}
		bool					canReserveHere(TilePosition position, UnitType type, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool useTypeAddon = true, bool checkAddon = false) const {
		
			return canReserveHere(position, type, type.tileWidth(), type.tileHeight(), topSpace, leftSpace, rightSpace, bottomSpace, useTypeAddon ? type != Terran_Command_Center && type.canBuildAddon() : checkAddon);
		}

	
		bool					isReservedFor(TilePosition position, UnitType type) const;


		TilePosition			getReservedPosition(UnitType type);
		TilePosition			getReservedPosition(UnitType type, Unit builder, UnitType addon);
	
		TilePosition			getReservedPositionAtWorker(UnitType type, Unit builder);

		void					assignTiles(TilePosition position, TilePosition size, Unit worker = nullptr);
		void					assignTiles(TilePosition position, UnitType type, Unit worker = nullptr);
		void					unassignTiles(TilePosition position, UnitType type);

	
		void					reordering(TilePosition tiles[], int strIdx, int endIdx, UnitType unitType);


		void					addTypes(Building building, vector<UnitType> unitTypes);

		const vector< Building >	getReserveList();											
		const vector< Building >	getAvoidList();										
		const vector<vector<short>>	getReserveMap() {
			return _reserveMap;
		}

		void					debugCanReserveHere(TilePosition position, UnitType type, int width, int height, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool canBuildAddon) const;
	};
}