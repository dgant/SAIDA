#pragma once

#include "Common.h"
#include "InformationManager.h"
#include "ConstructionPlaceFinder.h"
#include "ReserveBuilding.h"
#include "EnemyStrategyManager.h"

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

	class TerranConstructionPlaceFinder
	{
	private:
		TerranConstructionPlaceFinder();

		const TilePosition UP = TilePosition(0, -1);
		const TilePosition DOWN = TilePosition(0, 1);
		const TilePosition LEFT = TilePosition(-1, 0);
		const TilePosition RIGHT = TilePosition(1, 0);

		// 베이스 Area
		const Area *baseArea;

		// 미네랄
		Unit topMineral;
		Unit bottomMineral;
		Unit leftMineral;
		Unit rightMineral;
		// 미네랄이 베이스의 왼쪽에 배치되어있는가?
		bool isLeftOfBase;
		// 미네랄이 베이스의 위쪽에 배치되어있는가?
		bool isTopOfBase;
		// 베이스가 area의 위쪽에 있는가?
		bool isBaseTop;
		// 베이스가 area의 왼쪽에 있는가?
		bool isBaseLeft;
		// 미네랄 뒷쪽에 서플라이를 지을 공간이 존재하는가?
		bool isExistSpace;
		// 초크포인트의 방향
		Direction chokePointDirection;
		// base 의 첫번째 초크포인트
		TilePosition chokePoint;
		// base 가 존재하는 곳의 areaId
		short baseAreaId;
		// 베이스 위치
		TilePosition baseTilePosition;
		// 서플라이를 모아짓기 위한 기본 위치
		TilePosition supplyStdPosition;
		// 팩토리를 모아짓기 위한 기본 위치
		TilePosition factoryStdPosition;

		// 기본 변수 값을 세팅한다.
		void setBasicVariable(const Base *base);
		// 최초 서플라이 위치 계산 및 예약
		void calcFirstSupplyPosition();
		// 두번째 서플라이 위치 계산 및 예약 (상대가 프로토스인 경우에만)
		bool calcSecondSupplyPosition(TilePosition barracksPos);
		// 최초 배럭 위치 계산 및 예약
		void calcFirstBarracksPosition();
		// 모아짓기 위한 기준위치 계산
		void calcStandardPositionToStuck();
		// 다음 서플라이를 지을 곳을 계산한다.
		TilePosition calcNextBuildPosition(TilePosition stdPosition, TilePosition lastPos);
		// 서플라이 짓는 순서 결정.
		void sortBuildOrder(int strIdx, int lstIdx);
		// 팩토리 위치 결정
		int calcFactoryPosition(TilePosition factoryStdPosition, TilePosition MOVE_DIRECTION1, TilePosition MOVE_DIRECTION2, int factoryCnt, bool recursive);

		// 서플라이 디팟 건설 위치
		TilePosition tilePos[25];
		// 서플라이 디팟 인덱스
		int idx = 0, totCnt = 0;
		// 팩토리 공간 예약할 갯수
		int TOT_FACTORY_COUNT = 11;

		// 건설위치를 찾기 위한 이동 방향 순서
		TilePosition SUPPLY_MOVE_DIRECTION[2];
		TilePosition FACTORY_MOVE_DIRECTION[2];

		// 세컨드초크포인트 터렛 위치 찾기
		TilePosition getTurretPositionInSecondChokePoint();

		// 상대가 프로토스인 경우 배럭 옆 서플라이 위치 (랜덤 -> 프로토스 이외의 종족인 경우 예약 해제를 위해 사용)
		TilePosition supplyByBarracks;

		/// SecondChokePoint 건물 위치 관련
		void setSecondChokePointReserveBuilding();
		bool checkCanBuildHere(TilePosition tilePosition, UnitType buildingType);
		map<Position, vector<TilePosition>> secondChokePointTilesMap;
		map<Position, pair<TilePosition, TilePosition>> closestTileMap;

		/// SecondChokePoint 건물 위치
		TilePosition barracksPositionInSecondChokePoint;
		TilePosition engineeringBayPositionInSecondChokePoint;
		vector<TilePosition> supplysPositionInSecondChokePoint;
	public:
		static TerranConstructionPlaceFinder &Instance();

		TilePosition getBuildLocationWithSeedPositionAndStrategy(UnitType buildingType, TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy);

		// 상대가 랜덤이었다가 프로토스가 아닌 경우 서플라이 예약 해제
		void freeSecondSupplyDepot();

		// 임시. 계산 결과를 화면에 출력한다.
		void drawMap();
		void drawSecondChokePointReserveBuilding();

		// SecondChokePoint 건물 건설 여부 체크
		void update();

		TilePosition getBarracksPositionInSCP();
		TilePosition getEngineeringBayPositionInSCP();
		TilePosition getSupplysPositionInSCP();

		bool isFixedPositionOrder(UnitType unitType);
	};
}