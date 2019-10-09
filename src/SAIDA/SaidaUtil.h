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

	// Back Position 관련 API
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

	// 공중 유닛 상대 포톤 등 회피 이동 코드
	//void GoWithoutDamage(Unit unit, Position pos);
	//void makeLine_dh(Unit unit, Unit target, double *m, Position pos);
	//void drawLine_dh(Position unit, double m);

	bool isUseMapSettings();

	bool isSameArea(UnitInfo *u1, UnitInfo *u2);
	bool isSameArea(const Area *a1, const Area *a2);
	bool isSameArea(TilePosition a1, TilePosition a2);
	bool isSameArea(Position a1, Position a2);

	bool isInMyArea(UnitInfo *unitInfo);// 내 Command Center가 있는 Area에 있는지 판단.
	bool isInMyArea(Position pos, bool getMyAllbase = false);
	bool isInMyAreaAir(Position pos);

	// 내 유닛이 길을 막고있는지 체크한다.
	bool isBlocked(Unit unit, int size = 32);
	bool isBlocked(const UnitType unitType, Position centerPosition, int size = 32);
	bool isBlocked(const UnitType unitType, TilePosition topLeft, int size = 32);
	bool isBlocked(int top, int left, int bottom, int right, int size = 32);

	// UnitList의 평균 Postion 값
	Position getAvgPosition(uList units);
	// 해당 Position의 +1, -1 Tile 사이의 Random Position
	Position getRandPosition(Position p);
	Position findRandomeSpot(Position p);

	// Unit이 target으로 Tower를 피해가는 Function : direction은 위/아래 임.
	// 가는 곳이 갈수 없는 곳이면 Positions::None을 return함.
	// direction 은 1은 시계방향, -1은 반시계방향
	bool goWithoutDamage(Unit u, Position target, int direction, int dangerGap = 3 * TILE_SIZE);
	bool goWithoutDamageForSCV(Unit u, Position target, int direction);
	void kiting(UnitInfo *attacker, UnitInfo *target, int dangerPoint, int threshold);
	void attackFirstkiting(UnitInfo *attacker, UnitInfo *target, int dangerPoint, int threshold);
	void pControl(UnitInfo *attacker, UnitInfo *target);
	UnitInfo *getGroundWeakTargetInRange(UnitInfo *attacker, bool worker = false);

	// 두 점 사이 직선 위에서 비율에 맞는 중간 점을 구합니다.(r1은 p1으로부터의 비율입니다)
	Position getMiddlePositionByRate(Position p1, Position p2, double r1);

	// 두 점 사이 직선 위에서 거리에 맞는 중간 점을 구합니다.(dist는 p1으로부터의 거리입니다)
	Position getMiddlePositionByDist(Position p1, Position p2, int dist);

	// 반올림 함수(c++ math에는 반올림이 없네요)
	double round(double value, int pos = 0);

	// 중심과 각도(도 단위)와 원위의 한 점이 주어지면 원위의 한점에서 각도만큼 떨어진 원위의 또 다른 한 점의 좌표를 구합니다.
	Position getCirclePosFromPosByDegree(Position center, Position fromPos, double degree);
	// 중심과 각도(라디안 단위)와 원위의 한 점이 주어지면 원위의 한점에서 각도만큼 떨어진 원위의 또 다른 한 점의 좌표를 구합니다.
	Position getCirclePosFromPosByRadian(Position center, Position fromPos, double radian);
	// 한점과 거리와 각도(도 단위)가 주어지면 그 점에서 그 각도로 거리만큼 떨어진 점의 좌표를 구합니다.
	Position getPosByPosDistDegree(Position pos, int dist, double degree);
	// 한점과 거리와 각도(라디안 단위)가 주어지면 그 점에서 그 각도로 거리만큼 떨어진 점의 좌표를 구합니다.
	Position getPosByPosDistRadian(Position pos, int dist, double degree);

	Base *getMinePosition(Base *preTarget);
	bool isBeingRepaired(Unit u);

	int getDamage(Unit attacker, Unit target);
	int getDamage(UnitType attackerType, UnitType targetType, Player attackerPlayer, Player targetPlayer);
	UnitInfo *getDangerUnitNPoint(Position p, int *point, bool isFlyer);
	UnitInfo *getDangerUnitNPointForSCV(Unit u, int *point, bool isFlyer);

	// AttackRange 계산 기준으로 거리를 가져온다. weaponRange 와 비교 가능
	int getAttackDistance(int aLeft, int aTop, int aRight, int aBottom, int tLeft, int tTop, int tRight, int tBottom);
	int getAttackDistance(Unit attacker, Unit target);
	int getAttackDistance(Unit attacker, UnitType targetType, Position targetPosition);
	int getAttackDistance(UnitType attackerType, Position attackerPosition, Unit target);
	int getAttackDistance(UnitType attackerType, Position attackerPosition, UnitType targetType, Position targetPosition);

	// 특정 유닛이 특정지점으로 갔을 때 적군으로부터 받는 총 데미지를 리턴한다.
	int getDamageAtPosition(Position p, Unit unit, uList enemyList, bool onlyFromBuilding = true);
	int getDamageAtPosition(Position p, UnitType unitType, uList enemyList, bool onlyFromBuilding = true);
	bool checkZeroAltitueAroundMe(Position myPosition, int width);
	bool needWaiting(UnitInfo *uInfo);

	// Dropship Route 관련
	vector<Position> makeDropshipRoute(Position start, Position end, bool reverse = false);
	Position getCloestEndPosition(Position p);

	// Need Kiting Unit Type
	bool isNeedKitingUnitType(UnitType uType);
	bool isNeedKitingUnitTypeforMarine(UnitType uType);
	bool isNeedKitingUnitTypeinAir(UnitType uType);
	// 적이 섬 지역이거나 FirstExpansionLocation 에 멀티(리소스디팟)를 지었으면 true, 멀티가 없으면 false
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
		// UnitType 을 FilterType 으로 변환
		map <UnitType, unsigned int> filterTypeMap;
		vector <UnitType> enemyTowerTypes;
		map<UnitType, vector<vector<unsigned int>>> damageMap;
		// 방어타워의 공격범위를 저장한 맵 생성
		DefenseBuilding();

		bool isValid(Position p, UnitType towerType, UnitType unitType);
	public:
		static DefenseBuilding &Instance();

		// unitType 이 toGoPosition 로 이동할 때 towerPosition 에 있는 방어타워의 공격 범위 이내인지 체크한다.
		bool isInAttackRange(Position towerPosition, UnitType towerType, Position toGoPosition, UnitType unitType);
		bool isInAttackRange(Unit towerUnit, Position toGoPosition, Unit unit);
	};
}