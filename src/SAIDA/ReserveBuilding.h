#pragma once

#include "Common.h"
#include "TerranConstructionPlaceFinder.h"

namespace MyBot
{
	enum class ReserveTypes {
		UNIT_TYPE,
		SIZE,
		SPACE,	// 여유 공간을 위한 예약 (건설불가)
		MINERALS, // 미네랄과 base 사이 (터렛 등 일부 건물만 건설 가능)
		GAS,	// 가스와 base 사이 (터렛 등 일부 건물만 건설 가능)
		AVOID,  // 가스와 base 사이 (터렛 등 일부 건물만 건설 가능)
		UNBUILDABLE	// unbuildable tile
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

		// 상단 좌측 좌표
		const TilePosition TopLeft() const {
			return pos;
		}

		// 하단 우측 좌표
		const TilePosition BottomRight() const {
			if (reserveType == ReserveTypes::UNIT_TYPE) return pos + possibleList.front().tileSize();
			else return pos + tileSize;
		}

		// Center 좌표
		const Position Center() const {
			return ((Position)TopLeft() + (Position)BottomRight()) / 2;
		}

		// Space 추가
		void addSpaceTile(int x, int y) {
			spaceList.emplace_back(TilePosition(x, y));
		}

		vector< TilePosition > getSpaceList() {
			return spaceList;
		}

		// Type 추가
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

		// 디버그를 위해 사용
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

		// 기능 : 타일을 건설을 위해 주변에 공간을 두고 예약한다. 주변의 공간에는 건설 불가. getReservedPosition(UnitType) 호출 시 예약한 순서대로 리턴해준다.
		// 유효성 체크 : position 이 유효한지 체크. 이미 예약된 공간인지 체크.
		// return : 예약 성공 여부
		bool					reserveTiles(TilePosition position, TilePosition size, int topSpace, int leftSpace, int rightSpace, int bottomSpace, ReserveTypes reserveType = ReserveTypes::SIZE);
		bool					reserveTiles(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool useTypeAddon = true);
		bool					reserveTilesFirst(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace);
		// 타일을 중복으로 예약해야 할 일이 생기는 경우 사용. 무조건 타일을 예약한다.
		void					forceReserveTiles(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace);
		void					forceReserveTilesFirst(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace);

		// 건물 건설 예정 타일로 예약했던 것을 해제합니다. (건설 완료 후 호출. 랜딩완료 후 호출. 지을수 없는 상태가 되면 호출.)
		// 주의 : 별도의 유효성 체크는 하지 않는다.
		void					freeTiles(TilePosition position, TilePosition size);
		void					freeTiles(TilePosition position, UnitType type, bool freeAddon = false);

		// 기능 : unitType, fromPosition 으로 예약된 타일의 위치를 toPosition으로 변경한다.
		void					modifyReservePosition(UnitType unitType, TilePosition fromPosition, TilePosition toPosition);

		// 기능 : 타일이 예약 가능한지 체크.
		// 유효성 체크 : position 이 유효한지 체크. 이미 예약된 공간인지 체크.
		// return : 예약된 타일인 경우 true, 아닌경우 false
		bool					canReserveHere(TilePosition position, int width, int height, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool canBuildAddon = false) const {
			return canReserveHere(position, UnitTypes::None, width, height, topSpace, leftSpace, rightSpace, bottomSpace, canBuildAddon);
		}
		bool					canReserveHere(TilePosition position, TilePosition size, int topSpace, int leftSpace, int rightSpace, int bottomSpace) const {
			return canReserveHere(position, UnitTypes::None, size.x, size.y, topSpace, leftSpace, rightSpace, bottomSpace, false);
		}
		bool					canReserveHere(TilePosition position, UnitType type, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool useTypeAddon = true, bool checkAddon = false) const {
			// Command Center 이 지어지는 곳은 addon 이 가능한 미네랄 근처이거나 addon 을 하지 않고 띄우기 위한 위치이므로 addon 체크를 하지 않는다.
			return canReserveHere(position, type, type.tileWidth(), type.tileHeight(), topSpace, leftSpace, rightSpace, bottomSpace, useTypeAddon ? type != Terran_Command_Center && type.canBuildAddon() : checkAddon);
		}

		// 기능 : 타일이 특정 UnitType 혹은 Size 로 예약이 되었는지 체크
		// return : 특정 UnitType 혹은 Size 로 예약된 타일인 경우 true, 아닌경우 false
		bool					isReservedFor(TilePosition position, UnitType type) const;

		// 기능 : 예약된 리스트 중 건설할 위치를 리턴한다.
		// return : type 을 지을 수 있는 TilePosition. 없는 경우 TilePosition.None 리턴.
		TilePosition			getReservedPosition(UnitType type);
		TilePosition			getReservedPosition(UnitType type, Unit builder, UnitType addon);
		// worker 가 위치한 곳에서 지을 수 있는 TilePosition 을 리턴한다.
		TilePosition			getReservedPositionAtWorker(UnitType type, Unit builder);

		// 기능 : 예약된 리스트에 건설 일꾼을 할당한다. 일꾼이 인풋으로 들어오지 않은 경우, 특정 시간동안 예약을 못하도록 체크한다.
		void					assignTiles(TilePosition position, TilePosition size, Unit worker = nullptr);
		void					assignTiles(TilePosition position, UnitType type, Unit worker = nullptr);
		void					unassignTiles(TilePosition position, UnitType type);

		// 기능 : 예약된 리스트 중에 건물 건설 순서를 재 조정한다.
		void					reordering(TilePosition tiles[], int strIdx, int endIdx, UnitType unitType);

		// 기능 : 이미 예약된 리스트에 UnitType 을 추가한다.
		void					addTypes(Building building, vector<UnitType> unitTypes);

		const vector< Building >	getReserveList();											///< reserveList을 리턴합니다
		const vector< Building >	getAvoidList();											///< avoidList을 리턴합니다
		const vector<vector<short>>	getReserveMap() {
			return _reserveMap;
		}

		// debug 를 위해 사용.
		void					debugCanReserveHere(TilePosition position, UnitType type, int width, int height, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool canBuildAddon) const;
	};
}