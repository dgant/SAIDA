#pragma once

#include "Common.h"
#include "InformationManager.h"

namespace MyBot
{
	vector<UnitInfo *>getEnemyScountInMyArea();
	Position getDefencePos(Position mid, Position end, Position mybase);
	uList getEnemyInMyYard(int pixel, UnitType uType = UnitTypes::AllUnits, bool hide = true);
	uList getMyAttackableUnitsForScan(Unit targetUnit, int radius = 12 * TILE_SIZE);
	int needCountForBreakMulti(UnitType uType);

	UnitInfo *getClosestUnit(Position myPosition, uList units, bool gDist = true);
	uList getEnemyOutOfRange(Position pos, bool gDist = true, bool hide = false);

	bool isMyCommandAtFirstExpansion();

	TilePosition findNearestTileInMyArea(TilePosition origin, TilePosition target, int Range);
	vector<Position> getWidePositions(Position source, Position target, bool forward = true, int gap = TILE_SIZE, int angle = 30);
	vector<Position> getRoundPositions(Position source, int gap = TILE_SIZE, int angle = 30);
	vector<Position> getRootToMineral(Position start);
	Position getDirectionDistancePosition(Position source, Position direction, int distance = TILE_SIZE);

	// Back Position ���� API
	Position getBackPostion(UnitInfo *unit, Position target, int length, bool avoidUnit = false);
	void moveBackPostion(UnitInfo *unit, Position ePos, int length);
	void moveBackPostionbyMe(UnitInfo *unit, Position ePos);
	void moveBackPostionMarine(UnitInfo *unit, Position ePos, int length);
	bool isValidPath(Position st, Position en);
	int getPathValue(Position st, Position en);
	int getPathValueForMarine(Position pos, Position target);
	int getPathValueForAir(Position en);
	int getGroundDistance(Position st, Position en);
	int getAltitude(Position pos);
	int getAltitude(TilePosition pos);
	int getAltitude(WalkPosition pos);

	// ���� ���� ��� ���� �� ȸ�� �̵� �ڵ�
	//void GoWithoutDamage(Unit unit, Position pos);
	//void makeLine_dh(Unit unit, Unit target, double *m, Position pos);
	//void drawLine_dh(Position unit, double m);

	bool isUseMapSettings();

	bool isSameArea(UnitInfo *u1, UnitInfo *u2);
	bool isSameArea(const Area *a1, const Area *a2);
	bool isSameArea(TilePosition a1, TilePosition a2);
	bool isSameArea(Position a1, Position a2);

	bool isInMyArea(UnitInfo *unitInfo);// �� Command Center�� �ִ� Area�� �ִ��� �Ǵ�.
	bool isInMyArea(Position pos, bool getMyAllbase = false);
	bool isInMyAreaAir(Position pos);

	// �� ������ ���� �����ִ��� üũ�Ѵ�.
	bool isBlocked(Unit unit, int size = 32);
	bool isBlocked(const UnitType unitType, Position centerPosition, int size = 32);
	bool isBlocked(const UnitType unitType, TilePosition topLeft, int size = 32);
	bool isBlocked(int top, int left, int bottom, int right, int size = 32);

	// UnitList�� ��� Postion ��
	Position getAvgPosition(uList units);
	// �ش� Position�� +1, -1 Tile ������ Random Position
	Position getRandPosition(Position p);
	Position findRandomeSpot(Position p);

	// Unit�� target���� Tower�� ���ذ��� Function : direction�� ��/�Ʒ� ��.
	// ���� ���� ���� ���� ���̸� Positions::None�� return��.
	// direction �� 1�� �ð����, -1�� �ݽð����
	bool goWithoutDamage(Unit u, Position target, int direction, int dangerGap = 3 * TILE_SIZE);
	bool goWithoutDamageForSCV(Unit u, Position target, int direction);
	void kiting(UnitInfo *attacker, UnitInfo *target, int dangerPoint, int threshold);
	void attackFirstkiting(UnitInfo *attacker, UnitInfo *target, int dangerPoint, int threshold);
	void pControl(UnitInfo *attacker, UnitInfo *target);
	UnitInfo *getGroundWeakTargetInRange(UnitInfo *attacker, bool worker = false);

	// �� �� ���� ���� ������ ������ �´� �߰� ���� ���մϴ�.(r1�� p1���κ����� �����Դϴ�)
	Position getMiddlePositionByRate(Position p1, Position p2, double r1);

	// �� �� ���� ���� ������ �Ÿ��� �´� �߰� ���� ���մϴ�.(dist�� p1���κ����� �Ÿ��Դϴ�)
	Position getMiddlePositionByDist(Position p1, Position p2, int dist);

	// �ݿø� �Լ�(c++ math���� �ݿø��� ���׿�)
	double round(double value, int pos = 0);

	// �߽ɰ� ����(�� ����)�� ������ �� ���� �־����� ������ �������� ������ŭ ������ ������ �� �ٸ� �� ���� ��ǥ�� ���մϴ�.
	Position getCirclePosFromPosByDegree(Position center, Position fromPos, double degree);
	// �߽ɰ� ����(���� ����)�� ������ �� ���� �־����� ������ �������� ������ŭ ������ ������ �� �ٸ� �� ���� ��ǥ�� ���մϴ�.
	Position getCirclePosFromPosByRadian(Position center, Position fromPos, double radian);
	// ������ �Ÿ��� ����(�� ����)�� �־����� �� ������ �� ������ �Ÿ���ŭ ������ ���� ��ǥ�� ���մϴ�.
	Position getPosByPosDistDegree(Position pos, int dist, double degree);
	// ������ �Ÿ��� ����(���� ����)�� �־����� �� ������ �� ������ �Ÿ���ŭ ������ ���� ��ǥ�� ���մϴ�.
	Position getPosByPosDistRadian(Position pos, int dist, double degree);

	Base *getMinePosition(Base *preTarget);
	bool isBeingRepaired(Unit u);

	int getDamage(Unit attacker, Unit target);
	int getDamage(UnitType attackerType, UnitType targetType, Player attackerPlayer, Player targetPlayer);
	UnitInfo *getDangerUnitNPoint(Position p, int *point, bool isFlyer);
	UnitInfo *getDangerUnitNPointForSCV(Unit u, int *point, bool isFlyer);

	// AttackRange ��� �������� �Ÿ��� �����´�. weaponRange �� �� ����
	int getAttackDistance(int aLeft, int aTop, int aRight, int aBottom, int tLeft, int tTop, int tRight, int tBottom);
	int getAttackDistance(Unit attacker, Unit target);
	int getAttackDistance(Unit attacker, UnitType targetType, Position targetPosition);
	int getAttackDistance(UnitType attackerType, Position attackerPosition, Unit target);
	int getAttackDistance(UnitType attackerType, Position attackerPosition, UnitType targetType, Position targetPosition);

	// Ư�� ������ Ư���������� ���� �� �������κ��� �޴� �� �������� �����Ѵ�.
	int getDamageAtPosition(Position p, Unit unit, uList enemyList, bool onlyFromBuilding = true);
	int getDamageAtPosition(Position p, UnitType unitType, uList enemyList, bool onlyFromBuilding = true);
	bool checkZeroAltitueAroundMe(Position myPosition, int width);
	bool needWaiting(UnitInfo *uInfo);

	// Dropship Route ����
	vector<Position> makeDropshipRoute(Position start, Position end, bool reverse = false);
	Position getCloestEndPosition(Position p);

	// Need Kiting Unit Type
	bool isNeedKitingUnitType(UnitType uType);
	bool isNeedKitingUnitTypeforMarine(UnitType uType);
	bool isNeedKitingUnitTypeinAir(UnitType uType);
	// ���� �� �����̰ų� FirstExpansionLocation �� ��Ƽ(���ҽ�����)�� �������� true, ��Ƽ�� ������ false
	bool HasEnemyFirstExpansion();

	bool isStuckOrUnderSpell(UnitInfo *uInfo);

	bool escapeFromSpell(UnitInfo *uInfo);

	int getBuildStartFrame(Unit building);
	int getRemainingBuildFrame(Unit building);

	class DefenseBuilding {
	private:
		const int MAX_SIZE = 600;
		const Position CENTER_UNIT = Position(MAX_SIZE / 2, MAX_SIZE / 2);
		vector <UnitType> myUnitTypes;
		// UnitType �� FilterType ���� ��ȯ
		map <UnitType, unsigned int> filterTypeMap;
		vector <UnitType> enemyTowerTypes;
		map<UnitType, vector<vector<unsigned int>>> damageMap;
		// ���Ÿ���� ���ݹ����� ������ �� ����
		DefenseBuilding();

		bool isValid(Position p, UnitType towerType, UnitType unitType);
	public:
		static DefenseBuilding &Instance();

		// unitType �� toGoPosition �� �̵��� �� towerPosition �� �ִ� ���Ÿ���� ���� ���� �̳����� üũ�Ѵ�.
		bool isInAttackRange(Position towerPosition, UnitType towerType, Position toGoPosition, UnitType unitType);
		bool isInAttackRange(Unit towerUnit, Position toGoPosition, Unit unit);
	};
}