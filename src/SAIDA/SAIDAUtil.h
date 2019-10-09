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

	/*
	attacker == BWAPI::UnitTypes::Terran_Bunker ||
	attacker == BWAPI::UnitTypes::Protoss_Carrier ||
	attacker == BWAPI::UnitTypes::Protoss_Reaver;
	*/
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
	/*
	
	// Handle carriers and reavers correctly in the case of floating buildings.
// We have to check unit->isFlying() because unitType->isFlyer() is not correct
// for a lifted terran building.
// There is no way to handle bunkers here. You have to know what's in it.
BWAPI::WeaponType UnitUtil::GetWeapon(BWAPI::UnitType attacker, BWAPI::Unit target)
{
	if (attacker == BWAPI::UnitTypes::Protoss_Carrier)
	{
		return GetWeapon(BWAPI::UnitTypes::Protoss_Interceptor, target);
	}
	if (attacker == BWAPI::UnitTypes::Protoss_Reaver)
	{
		return GetWeapon(BWAPI::UnitTypes::Protoss_Scarab, target);
	}
	return target->isFlying() ? attacker.airWeapon() : attacker.groundWeapon();
}
	*/
	bool IsMorphedBuildingType(BWAPI::UnitType type);
	bool IsCombatSimUnit(BWAPI::UnitType type);
	bool IsCombatUnit(BWAPI::UnitType type);
	bool IsCombatUnit(BWAPI::Unit unit);



	bool isUseMapSettings();

	bool isSameArea(UnitInfo *u1, UnitInfo *u2);
	bool isSameArea(const Area *a1, const Area *a2);
	bool isSameArea(TilePosition a1, TilePosition a2);
	bool isSameArea(Position a1, Position a2);

	bool isInMyArea(UnitInfo *unitInfo);
	bool isInMyArea(Position pos, bool getMyAllbase = false);
	bool isInMyAreaAir(Position pos);

	bool isBlocked(Unit unit, int size = 32);
	bool isBlocked(const UnitType unitType, Position centerPosition, int size = 32);
	bool isBlocked(const UnitType unitType, TilePosition topLeft, int size = 32);
	bool isBlocked(int top, int left, int bottom, int right, int size = 32);


	Position getAvgPosition(uList units);

	Position getRandPosition(Position p);
	Position findRandomeSpot(Position p);


	bool goWithoutDamage(Unit u, Position target, int direction, int dangerGap = 3 * TILE_SIZE);
	bool goWithoutDamageForSCV(Unit u, Position target, int direction);
	void kiting(UnitInfo *attacker, UnitInfo *target, int dangerPoint, int threshold);
	void attackFirstkiting(UnitInfo *attacker, UnitInfo *target, int dangerPoint, int threshold);
	void pControl(UnitInfo *attacker, UnitInfo *target);
	UnitInfo *getGroundWeakTargetInRange(UnitInfo *attacker, bool worker = false);


	Position getMiddlePositionByRate(Position p1, Position p2, double r1);


	Position getMiddlePositionByDist(Position p1, Position p2, int dist);

	double round(double value, int pos = 0);


	Position getCirclePosFromPosByDegree(Position center, Position fromPos, double degree);

	Position getCirclePosFromPosByRadian(Position center, Position fromPos, double radian);

	Position getPosByPosDistDegree(Position pos, int dist, double degree);

	Position getPosByPosDistRadian(Position pos, int dist, double degree);

	Base *getMinePosition(Base *preTarget);
	bool isBeingRepaired(Unit u);

	int getDamage(Unit attacker, Unit target);
	int getDamage(UnitType attackerType, UnitType targetType, Player attackerPlayer, Player targetPlayer);
	UnitInfo *getDangerUnitNPoint(Position p, int *point, bool isFlyer);
	UnitInfo *getDangerUnitNPointForSCV(Unit u, int *point, bool isFlyer);


	int getAttackDistance(int aLeft, int aTop, int aRight, int aBottom, int tLeft, int tTop, int tRight, int tBottom);
	int getAttackDistance(Unit attacker, Unit target);
	int getAttackDistance(Unit attacker, UnitType targetType, Position targetPosition);
	int getAttackDistance(UnitType attackerType, Position attackerPosition, Unit target);
	int getAttackDistance(UnitType attackerType, Position attackerPosition, UnitType targetType, Position targetPosition);

	int getDamageAtPosition(Position p, Unit unit, uList enemyList, bool onlyFromBuilding = true);
	int getDamageAtPosition(Position p, UnitType unitType, uList enemyList, bool onlyFromBuilding = true);
	bool checkZeroAltitueAroundMe(Position myPosition, int width);
	bool needWaiting(UnitInfo *uInfo);


	vector<Position> makeDropshipRoute(Position start, Position end, bool reverse = false);
	Position getCloestEndPosition(Position p);

	bool isNeedKitingUnitType(UnitType uType);

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

		map <UnitType, unsigned int> filterTypeMap;
		vector <UnitType> enemyTowerTypes;
		map<UnitType, vector<vector<unsigned int>>> damageMap;
		
		DefenseBuilding();

		bool isValid(Position p, UnitType towerType, UnitType unitType);
	public:
		static DefenseBuilding &Instance();
		bool isInAttackRange(Position towerPosition, UnitType towerType, Position toGoPosition, UnitType unitType);
		bool isInAttackRange(Unit towerUnit, Position toGoPosition, Unit unit);
	};
}