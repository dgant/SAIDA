#pragma once

#include "Common.h"
#include "ConstructionTask.h"
#include "MetaType.h"
#include "BuildOrderQueue.h"
#include "InformationManager.h"
#include "ReserveBuilding.h"

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

	/// 건설위치 탐색을 위한 class
	class ConstructionPlaceFinder
	{
	private:
		ConstructionPlaceFinder();

		/// BaseLocation 과 Mineral / Geyser 사이의 타일들을 찾아 _tilesToAvoid 에 저장합니다<br>
		/// BaseLocation 과 Geyser 사이, ResourceDepot 건물과 Mineral 사이 공간으로 건물 건설 장소를 정하면 <br>
		/// 일꾼 유닛들이 장애물이 되어서 건설 시작되기까지 시간이 오래걸리고, 지어진 건물이 장애물이 되어서 자원 채취 속도도 느려지기 때문에, 이 공간은 건물을 짓지 않는 공간으로 두기 위함입니다
		void					setTilesToAvoid();

		// myPosition 과 targetPosition 의 GroundHeight의 차이를 return
		//  < 0 : 내 위치가 낮음. > 내 위치가 높음. == 0 : 높이 동일
		int  compareHeight(const TilePosition &myPosition, const TilePosition &targetPosition) const;

		Line getLine(const Position &p1, const Position &p2);
		void getBox(Box &box, const TilePosition &p1, const TilePosition &p2);

	public:

		/// static singleton 객체를 리턴합니다
		static ConstructionPlaceFinder &Instance();

		TilePosition		getSeedPositionFromSeedLocationStrategy(BuildOrderItem::SeedPositionStrategy seedPositionStrategy) const;
		/// seedPosition 및 seedPositionStrategy 파라메터를 활용해서 건물 건설 가능 위치를 탐색해서 리턴합니다<br>
		/// seedPosition 주위에서 가능한 곳을 선정하거나, seedPositionStrategy 에 따라 지형 분석결과 해당 지점 주위에서 가능한 곳을 선정합니다<br>
		/// seedPosition, seedPositionStrategy 을 입력하지 않으면, MainBaseLocation 주위에서 가능한 곳을 리턴합니다
		TilePosition		getBuildLocationWithSeedPositionAndStrategy(UnitType buildingType, TilePosition seedPosition = TilePositions::None, BuildOrderItem::SeedPositionStrategy seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseLocation) const;

		/// desiredPosition 근처에서 건물 건설 가능 위치를 탐색해서 리턴합니다<br>
		/// desiredPosition 주위에서 가능한 곳을 찾아 반환합니다<br>
		/// desiredPosition 이 valid 한 곳이 아니라면, desiredPosition 를 MainBaseLocation 로 해서 주위를 찾는다<br>
		/// Returns a suitable TilePosition to build a given building type near specified TilePosition aroundTile.<br>
		/// Returns TilePositions::None, if suitable TilePosition is not exists (다른 유닛들이 자리에 있어서, Pylon, Creep, 건물지을 타일 공간이 전혀 없는 경우 등)
		TilePosition		getBuildLocationNear(UnitType buildingType, TilePosition desiredPosition, bool checkForReserve, Unit worker = nullptr) const;

		// possiblePosition 위치에서 unitType 을 지을수 있는 곳을 나선형으로 검색한다. next 가 true 인 경우 이어서 검사.
		// 전역변수
		word altitudeIdx = 0;
		word deltaIdx = 0;
		vector<pair<TilePosition, altitude_t>> DeltasByAscendingAltitude;
		TilePosition		getBuildLocationBySpiralSearch(UnitType unitType, TilePosition possiblePosition, bool next = false, Position possibleArea = Positions::None, bool checkOnlyBuilding = false);

		/// seedPosition area 에 존재하는 가스 광산들 (Resource_Vespene_Geyser) 중 예약되어있지 않은 곳(isReservedTile), 이미 Refinery 가 지어져있지않은 곳 중 가장 가까운 곳을 리턴합니다.
		/// 없는 경우 TilePositions::None 리턴
		TilePosition		getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy seedPositionStrategy) const;
		TilePosition		getRefineryPositionNear(Base *base) const;

		/// 해당 위치에 건물 건설이 가능한지 여부를 리턴합니다<br>
		/// Broodwar 의 canBuildHere 와 ReserveBuilding 을 체크합니다
		bool					canBuildHere(TilePosition position, const UnitType type, const bool checkForReserve, const Unit worker = nullptr, int topSpace = 0, int leftSpace = 0, int rightSpace = 0, int bottomSpace = 0) const;
		// base 위치를 (테란인 경우는 addon 포함) 침범하는지 체크한다.
		bool					isBasePosition(TilePosition position, UnitType type) const;

		// origin 의 area 에서 target 위치와 가장 가까우면서 unitType 을 지을 수 있는 위치를 반환한다.
		TilePosition			findNearestBuildableTile(TilePosition origin, TilePosition target, UnitType unitType);
	};
}