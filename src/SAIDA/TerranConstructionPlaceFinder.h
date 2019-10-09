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

		// ���̽� Area
		const Area *baseArea;

		// �̳׶�
		Unit topMineral;
		Unit bottomMineral;
		Unit leftMineral;
		Unit rightMineral;
		// �̳׶��� ���̽��� ���ʿ� ��ġ�Ǿ��ִ°�?
		bool isLeftOfBase;
		// �̳׶��� ���̽��� ���ʿ� ��ġ�Ǿ��ִ°�?
		bool isTopOfBase;
		// ���̽��� area�� ���ʿ� �ִ°�?
		bool isBaseTop;
		// ���̽��� area�� ���ʿ� �ִ°�?
		bool isBaseLeft;
		// �̳׶� ���ʿ� ���ö��̸� ���� ������ �����ϴ°�?
		bool isExistSpace;
		// ��ũ����Ʈ�� ����
		Direction chokePointDirection;
		// base �� ù��° ��ũ����Ʈ
		TilePosition chokePoint;
		// base �� �����ϴ� ���� areaId
		short baseAreaId;
		// ���̽� ��ġ
		TilePosition baseTilePosition;
		// ���ö��̸� ������� ���� �⺻ ��ġ
		TilePosition supplyStdPosition;
		// ���丮�� ������� ���� �⺻ ��ġ
		TilePosition factoryStdPosition;

		// �⺻ ���� ���� �����Ѵ�.
		void setBasicVariable(const Base *base);
		// ���� ���ö��� ��ġ ��� �� ����
		void calcFirstSupplyPosition();
		// �ι�° ���ö��� ��ġ ��� �� ���� (��밡 �����佺�� ��쿡��)
		bool calcSecondSupplyPosition(TilePosition barracksPos);
		// ���� �跰 ��ġ ��� �� ����
		void calcFirstBarracksPosition();
		// ������� ���� ������ġ ���
		void calcStandardPositionToStuck();
		// ���� ���ö��̸� ���� ���� ����Ѵ�.
		TilePosition calcNextBuildPosition(TilePosition stdPosition, TilePosition lastPos);
		// ���ö��� ���� ���� ����.
		void sortBuildOrder(int strIdx, int lstIdx);
		// ���丮 ��ġ ����
		int calcFactoryPosition(TilePosition factoryStdPosition, TilePosition MOVE_DIRECTION1, TilePosition MOVE_DIRECTION2, int factoryCnt, bool recursive);

		// ���ö��� ���� �Ǽ� ��ġ
		TilePosition tilePos[25];
		// ���ö��� ���� �ε���
		int idx = 0, totCnt = 0;
		// ���丮 ���� ������ ����
		int TOT_FACTORY_COUNT = 11;

		// �Ǽ���ġ�� ã�� ���� �̵� ���� ����
		TilePosition SUPPLY_MOVE_DIRECTION[2];
		TilePosition FACTORY_MOVE_DIRECTION[2];

		// ��������ũ����Ʈ �ͷ� ��ġ ã��
		TilePosition getTurretPositionInSecondChokePoint();

		// ��밡 �����佺�� ��� �跰 �� ���ö��� ��ġ (���� -> �����佺 �̿��� ������ ��� ���� ������ ���� ���)
		TilePosition supplyByBarracks;

		/// SecondChokePoint �ǹ� ��ġ ����
		void setSecondChokePointReserveBuilding();
		bool checkCanBuildHere(TilePosition tilePosition, UnitType buildingType);
		map<Position, vector<TilePosition>> secondChokePointTilesMap;
		map<Position, pair<TilePosition, TilePosition>> closestTileMap;

		/// SecondChokePoint �ǹ� ��ġ
		TilePosition barracksPositionInSecondChokePoint;
		TilePosition engineeringBayPositionInSecondChokePoint;
		vector<TilePosition> supplysPositionInSecondChokePoint;
	public:
		static TerranConstructionPlaceFinder &Instance();

		TilePosition getBuildLocationWithSeedPositionAndStrategy(UnitType buildingType, TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy);

		// ��밡 �����̾��ٰ� �����佺�� �ƴ� ��� ���ö��� ���� ����
		void freeSecondSupplyDepot();

		// �ӽ�. ��� ����� ȭ�鿡 ����Ѵ�.
		void drawMap();
		void drawSecondChokePointReserveBuilding();

		// SecondChokePoint �ǹ� �Ǽ� ���� üũ
		void update();

		TilePosition getBarracksPositionInSCP();
		TilePosition getEngineeringBayPositionInSCP();
		TilePosition getSupplysPositionInSCP();

		bool isFixedPositionOrder(UnitType unitType);
	};
}