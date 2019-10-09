#include "SaidaUtil.h"
#include <stdio.h>
#include "./UnitManager/TankManager.h"

using namespace MyBot;

Position MyBot::getAvgPosition(uList units)
{
	Position avgPos = Positions::Origin;

	for (auto u : units)
		avgPos += u->pos();

	return (Position)(avgPos / units.size());
}

bool MyBot::isUseMapSettings()
{
	return Broodwar->getGameType() == GameTypes::Use_Map_Settings ? true : false;
}

bool MyBot::isSameArea(UnitInfo *u1, UnitInfo *u2) {
	return u1->pos().isValid() && u2->pos().isValid() && isSameArea(theMap.GetArea((WalkPosition)u1->pos()), theMap.GetArea((WalkPosition)u2->pos()));
}

bool MyBot::isSameArea(Position a1, Position a2) {
	return a1.isValid() && a2.isValid() && isSameArea(theMap.GetArea((WalkPosition)a1), theMap.GetArea((WalkPosition)a2));
}

bool MyBot::isSameArea(TilePosition a1, TilePosition a2) {
	return a1.isValid() && a2.isValid() && isSameArea(theMap.GetArea(a1), theMap.GetArea(a2));
}

bool MyBot::isSameArea(const Area *a1, const Area *a2) {
	if (a1 == nullptr || a2 == nullptr)
		return false;

	if (a1->Id() == a2->Id())
		return true;

	return INFO.isMainBasePairArea(a1, a2);
}

bool MyBot::isBlocked(Unit unit, int size) {
	// 시야에 없는 유닛에 대해서는 체크하지 않는다.
	if (!unit->exists())
		return false;

	return isBlocked(unit->getTop(), unit->getLeft(), unit->getBottom(), unit->getRight(), size);
}

bool MyBot::isBlocked(const UnitType unitType, Position centerPosition, int size) {
	return isBlocked(centerPosition.y - unitType.dimensionUp(), centerPosition.x - unitType.dimensionLeft(), centerPosition.y + unitType.dimensionDown() + 1, centerPosition.x + unitType.dimensionRight() + 1, size);
}

bool MyBot::isBlocked(const UnitType unitType, TilePosition topLeft, int size) {
	TilePosition bottomRight = topLeft + unitType.tileSize();
	return isBlocked(topLeft.y * 32, topLeft.x * 32, bottomRight.y * 32, bottomRight.x * 32, size);
}

bool MyBot::isBlocked(int top, int left, int bottom, int right, int size) {
	Position center = Position((left + right) / 2, (top + bottom) / 2);

	if (getAltitude(center + Position(0, size)) > size || getAltitude(center + Position(0, -size)) > size
		|| getAltitude(center + Position(size, 0)) > size || getAltitude(center + Position(-size, 0)) > size)
		return false;

	int minX = left / 8;
	int minY = top / 8;
	int maxY = bottom / 8;
	int maxX = right / 8;
	int x = left / 8;
	int y = top / 8;

	altitude_t beforeAltitude = getAltitude(WalkPosition(x, y));
	altitude_t firstAltitude = beforeAltitude;

	int blockedCnt = firstAltitude < size ? 1 : 0;
	int smallCnt = 0;

	for (x++; x < maxX; x++) {
		altitude_t altitude = getAltitude(WalkPosition(x, y));

		if (beforeAltitude >= size && altitude < size)
			blockedCnt++;

		if (size > altitude)
			smallCnt++;

		beforeAltitude = altitude;
	}

	for (x--; y < maxY; y++) {
		altitude_t altitude = getAltitude(WalkPosition(x, y));

		if (beforeAltitude >= size && altitude < size)
			blockedCnt++;

		if (size > altitude)
			smallCnt++;

		beforeAltitude = altitude;
	}

	for (y--; x >= minX; x--) {
		altitude_t altitude = getAltitude(WalkPosition(x, y));

		if (beforeAltitude >= size && altitude < size)
			blockedCnt++;

		if (size > altitude)
			smallCnt++;

		beforeAltitude = altitude;
	}

	for (x++; y > minY; y--) {
		altitude_t altitude = getAltitude(WalkPosition(x, y));

		if (beforeAltitude >= size && altitude < size)
			blockedCnt++;

		if (size > altitude)
			smallCnt++;

		beforeAltitude = altitude;
	}

	if (firstAltitude < size && beforeAltitude < size && blockedCnt > 1)
		blockedCnt--;

	bool narrow = false;

	if (blockedCnt == 1) {
		narrow = smallCnt > 2 * (maxX - minX + maxY - minY - 1) * 0.7;
		//cout << 2 * (maxX - minX + maxY - minY - 1) * 0.7 << endl;
	}

	// debug (임시)
	//for (int y = minY; y < maxY; y++) {
	//	for (int x = minX; x < maxX; x++) {
	//		altitude_t altitude = getAltitude(WalkPosition(x, y));

	//		if (altitude < 10)
	//			cout << " " << altitude << " ";
	//		else
	//			cout << altitude << " ";
	//	}

	//	cout << endl;
	//}

	return narrow || blockedCnt > 1;
}

bool MyBot::isInMyArea(UnitInfo *unitInfo)
{
	return unitInfo->type().isFlyer() ? isInMyAreaAir(unitInfo->pos()) : isInMyArea(unitInfo->pos());
}

//getMyAllbase =true를 주면 모든 베이스 지역으로 체크합니다..
bool MyBot::isInMyArea(Position p, bool getMyAllbase)
{
	if (getMyAllbase)
	{
		if (p == Positions::Unknown)
			return false;

		// 모든 Base를 체크
		if (isSameArea(MYBASE, p) ||
			(INFO.getFirstExpansionLocation(S) != nullptr &&
			INFO.getFirstExpansionLocation(S)->GetOccupiedInfo() == myBase &&
			isSameArea(INFO.getFirstExpansionLocation(S)->Center(), p)) ||
			(INFO.getSecondExpansionLocation(S) != nullptr &&
			INFO.getSecondExpansionLocation(S)->GetOccupiedInfo() == myBase &&
			isSameArea(INFO.getSecondExpansionLocation(S)->Center(), p)) ||
			(INFO.getThirdExpansionLocation(S) != nullptr &&
			INFO.getThirdExpansionLocation(S)->GetOccupiedInfo() == myBase &&
			isSameArea(INFO.getThirdExpansionLocation(S)->Center(), p))
			)
		{
			return true;
		}

		for (auto &b : INFO.getAdditionalExpansions())
		{
			if (isSameArea(b->Center(), p)) return true;
		}
	}
	else
	{
		if (p == Positions::Unknown)
			return false;

		// 모든 Base를 체크 하지 않고 본진, 앞마당만 본다.
		if (isSameArea(MYBASE, p) ||
			(INFO.getFirstExpansionLocation(S) != nullptr &&
			INFO.getFirstExpansionLocation(S)->GetOccupiedInfo() == myBase &&
			isSameArea(INFO.getFirstExpansionLocation(S)->Center(), p)))
		{
			return true;
		}
	}

	return false;
}

bool MyBot::isInMyAreaAir(Position p)
{
	if (p == Positions::Unknown)
		return false;

	// 모든 Base를 체크 하지 않고 본진, 앞마당만 본다.
	if (isSameArea(MYBASE, p) ||
		(INFO.getFirstExpansionLocation(S) != nullptr &&
		INFO.getFirstExpansionLocation(S)->GetOccupiedInfo() == myBase &&
		isSameArea(INFO.getFirstExpansionLocation(S)->Center(), p)))
	{
		return true;
	}

	int tileRad = S->getUpgradeLevel(UpgradeTypes::Charon_Boosters) ? 8 : 5;

	TilePosition nearestPos = findNearestTileInMyArea(TilePosition(MYBASE), TilePosition(p), tileRad);

	if (nearestPos != TilePositions::None)
		return true;
	else {
		if (INFO.getFirstExpansionLocation(S) != nullptr &&
			INFO.getFirstExpansionLocation(S)->GetOccupiedInfo() == myBase)
		{
			nearestPos = findNearestTileInMyArea(INFO.getFirstExpansionLocation(S)->getTilePosition(), TilePosition(p), tileRad);

			if (nearestPos != TilePositions::None)
				return true;
		}
	}

	return false;
}

// 공중 유닛 상대 포톤 등 회피 이동 코드
// unit : 이동 중인 공중 유닛
// pos : 최종 목표 지점
// 포톤 등을 발견하면 회피하여 pos 위치로 이동 한다.
//void MyBot::GoWithoutDamage(Unit unit, Position pos)
//{
//	//Unitset enemyUnit = Broodwar->getUnitsInRadius(unit->getPosition(), 350, Filter::IsEnemy && Filter::CanAttack);
//
//	//if (enemyUnit.size() == 0) {
//	//	unit->move(pos);
//	//	return;
//	//}
//
//	uList nearUnits = INFO.getAllInRadius(E, unit->getPosition(), 12 * TILE_SIZE, true, false);
//
//	if (nearUnits.empty())
//	{
//		CommandUtil::move(unit, pos);
//		return;
//	}
//
//	int closestDistance = INT_MAX;
//	UnitInfo *closestUnit = nullptr;
//
//	for (auto eu : nearUnits)
//	{
//		if (eu->isHide())
//			continue;
//
//		if (eu->type().isBuilding())
//		{
//			if (!(eu->type() == Terran_Missile_Turret || (eu->type() == Terran_Bunker && eu->getMarinesInBunker() > 0)))
//				continue;
//		}
//		else
//		{
//			if (eu->type().airWeapon().damageAmount() == 0)
//				continue;
//		}
//
//		int tempDistance = (int)eu->pos().getDistance(unit->getPosition());
//
//		if (tempDistance < closestDistance)
//		{
//			closestUnit = eu;
//			closestDistance = tempDistance;
//		}
//	}
//
//	if (closestUnit != nullptr)
//	{
//		//printf("가까운 유닛 : %s (%d, %d)\n", closestUnit->type().c_str(), closestUnit->pos().x, closestUnit->pos().y);
//		double m[2] = { 0, };
//		double *mp = m;
//		makeLine_dh(unit, closestUnit->unit(), mp, pos);
//	}
//
//	//for (auto eu : enemyUnit)
//	//{
//	//	//		Broodwar->drawCircle(CoordinateType::Map, eu->getPosition().x, eu->getPosition().y, eu->getType().groundWeapon().maxRange(), Colors::Red);
//	//	//		Broodwar->drawCircle(CoordinateType::Map, eu->getPosition().x, eu->getPosition().y, eu->getType().airWeapon().maxRange(), Colors::Purple);
//
//	//	double m[2] = { 0, };
//	//	double *mp = m;
//
//	//	makeLine_dh(unit, eu, mp, pos);
//
//	//	//		drawLine_dh(unit->getPosition(), m[0]);
//	//	//		drawLine_dh(unit->getPosition(), m[1]);
//	//}
//}

//void MyBot::makeLine_dh(Unit unit, Unit target, double *m, Position pos)
//{
//	int r = unit->isFlying() ? target->getType().airWeapon().maxRange() : target->getType().groundWeapon().maxRange();
//
//	if (target->getType() == Terran_Bunker)
//		r = 7 * TILE_SIZE;
//
//	if (r == 0) return;
//
//	//printf("Enemy Building is Detected at (%d, %d) with radius %d!\n", target->getTilePosition().x, target->getTilePosition().y, r);
//
//	// 원 안에 있으면 동작 안함.
//	if (unit->getPosition().getDistance(target->getPosition()) < r)
//	{
//		CommandUtil::move(unit, getBackPostion(unit->getPosition(), target->getPosition(), r / TILE_SIZE + 3));
//		return;
//	}
//
//	int a = target->getPosition().x - unit->getPosition().x; // enemy.x
//	int b = target->getPosition().y - unit->getPosition().y; // enemy.y
//	int c = pos.x - unit->getPosition().x;
//	int d = pos.y - unit->getPosition().y;
//
//	int SAFE = 200;
//
//	m[0] = (a * b + (r * sqrt((a * a) + (b * b) - (r * r)))) / ((a * a) - (r * r));
//	m[1] = (a * b - (r * sqrt((a * a) + (b * b) - (r * r)))) / ((a * a) - (r * r));
//
//
//	double nm[2] = { 0, };
//
//	Position p[2];
//	Position pt[2];
//
//	for (int i = 0; i < 2; i++) {
//		p[i].x = (int)((a + m[i] * b) / (m[i] * m[i] + 1));
//		p[i].y = (int)((a + m[i] * b) * m[i] / (m[i] * m[i] + 1));
//
//		pt[i].x = (int)(((r + SAFE) * p[i].x - SAFE * a) / r); // +unit->getPosition().x;
//		pt[i].y = (int)(((r + SAFE) * p[i].y - SAFE * b) / r); // +unit->getPosition().y;
//
//		if (pt[i].x == 0) nm[i] = 0;
//		else nm[i] = (double)pt[i].y / (double)pt[i].x;
//
//		pt[i].x = pt[i].x + unit->getPosition().x;
//		pt[i].y = pt[i].y + unit->getPosition().y;
//	}
//
//	if (nm[0] < nm[1]) {
//		double tmp = nm[0];
//		nm[0] = nm[1];
//		nm[1] = tmp;
//	}
//
//	//	drawLine_dh(unit->getPosition(), nm[0]);
//	//	drawLine_dh(unit->getPosition(), nm[1]);
//
//	bool escape = false;
//
//	if ((a * nm[0] > b && a * nm[1] < b && c * nm[0] > d && c * nm[1] < d) || (a * nm[0] > b && a * nm[1] > b && c * nm[0] > d && c * nm[1] > d) ||
//			(a * nm[0] < b && a * nm[1] < b && c * nm[0] < d && c * nm[1] < d) || (a * nm[0] < b && a * nm[1] > b && c * nm[0] < d && c * nm[1] > d))
//		escape = true;
//
//	//	Broodwar->drawCircle(CoordinateType::Map, pt[0].x, pt[0].y, 10, Colors::Blue);
//	//	Broodwar->drawCircle(CoordinateType::Map, pt[1].x, pt[1].y, 10, Colors::Blue);
//
//	if (escape == false)
//		unit->move(pos);
//	else {
//		if (pos.getDistance(pt[0]) > pos.getDistance(pt[1]))
//			unit->move(pt[1]);
//		else
//			unit->move(pt[0]);
//	}
//}
//
//void MyBot::drawLine_dh(Position unit, double m)
//{
//	Position newPos[2];
//	int tmpX[2] = { 1, -1 };
//	int tmpY[2] = { 1, -1 };
//
//	for (int i = 0; i < 2; i++)
//	{
//		newPos[i].x = unit.x + (300 * tmpX[i]);
//		newPos[i].y = unit.y + (int)(300 * m * tmpY[i]);
//
//		if (newPos[i].x < 0) newPos[i].x = 0;
//
//		if (newPos[i].y < 0) newPos[i].y = 0;
//
//		if (newPos[i].x > 4095) newPos[i].x = 4095;
//
//		if (newPos[i].y > 4095) newPos[i].y = 4095;
//
//		Broodwar->drawLine(CoordinateType::Map, unit.x, unit.y, newPos[i].x, newPos[i].y, Colors::Red);
//	}
//}


bool MyBot::isValidPath(Position s, Position e)
{
	TilePosition TS = TilePosition(s);
	TilePosition TE = TilePosition(e);
	WalkPosition WS = WalkPosition(s);
	WalkPosition WE = WalkPosition(e);

	// not walkable
	if (!s.isValid() || !e.isValid() || getAltitude(WE) <= 0) {
		return false;
	}

	// 건물이나 미네랄 있으면 False;
	Unitset tmp = Broodwar->getUnitsInRadius(e, 40, Filter::IsBuilding || Filter::IsNeutral);
	// 상대 지상 유닛이 있으면 False;
	uList eUnits = INFO.getUnitsInRadius(E, e, 30, true, false, false);

	if (tmp.size() || eUnits.size())
		return false;

	return true;
}

int MyBot::getPathValue(Position st, Position en)
{
	TilePosition TS = TilePosition(st);
	TilePosition TE = TilePosition(en);
	WalkPosition WS = WalkPosition(st);
	WalkPosition WE = WalkPosition(en);

	int point = 0;

	// Not walkable 또는 장애물이 있는 경우
	if (theMap.Valid(en) == false || getAltitude(WE) <= 0 ||
		Broodwar->getUnitsInRadius(en, 16, Filter::IsBuilding || Filter::IsNeutral).size())
	{
		return -1;
	}

	bool nearChoke = false;

	if (theMap.GetArea(TS) == nullptr || theMap.GetArea(TE) == nullptr)
	{
		nearChoke = true;
	}
	else
	{
		if (theMap.GetArea(TS) != theMap.GetArea(TE) && Broodwar->getGroundHeight(TS) != Broodwar->getGroundHeight(TE))
		{
			int dist = 0;
			theMap.GetPath(st, en, &dist);

			if (dist == -1 || dist > st.getApproxDistance(en) * 2)
				return -1;
			else // 영역 다른데 높이도 다른데 거리는 가까워... ChokePoint 근처다.
				nearChoke = true;
		}
	}

	int dangerPoint = 0;
	getDangerUnitNPoint(en, &dangerPoint, false);
	point = 2 * dangerPoint;

	if (nearChoke == false)
	{
		point = point + getAltitude(WE);
	}
	else {
		int chokeAlt = getAltitude(WE) < 100 ? 100 : getAltitude(WE);
		point = point + chokeAlt;
	}

	/*
	// 본진으로의 가중치
	int gDistS = 0, gDistE = 0;
	int gDistA = 0, gDistFromE = 0;

	theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), st, &gDistS);
	theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), en, &gDistE);

	theMap.GetPath(INFO.getMainBaseLocation(S)->Center(), INFO.getMainBaseLocation(E)->Center(), &gDistA);
	theMap.GetPath(INFO.getMainBaseLocation(E)->Center(), st, &gDistFromE);

	if (gDistS > 0 && gDistE > 0)
	{
	point = point + ((gDistS - gDistE) * (gDistFromE / gDistA));
	}
	*/
	return point;
}

int MyBot::getGroundDistance(Position st, Position en)
{
	int dist = 0;
	theMap.GetPath(st, en, &dist);
	return dist;
}

// invalid 할 경우 -1 return;
int MyBot::getAltitude(Position pos)
{
	return getAltitude((WalkPosition)pos);
}

int MyBot::getAltitude(TilePosition pos)
{
	return getAltitude((WalkPosition)pos);
}

int MyBot::getAltitude(WalkPosition pos)
{
	return pos.isValid() ? theMap.GetMiniTile(pos).Altitude() : -1;
}

int MyBot::getPathValueForMarine(Position st, Position en)
{
	TilePosition TS = TilePosition(st);
	TilePosition TE = TilePosition(en);
	WalkPosition WS = WalkPosition(st);
	WalkPosition WE = WalkPosition(en);

	int point = 0;

	// Not walkable 또는 장애물이 있는 경우
	if (theMap.Valid(en) == false || getAltitude(WE) <= 0)
		return -1;

	int gap = 16;
	int gap_s = 8;
	Position pos = en;
	uList buildings = INFO.getBuildingsInRadius(S, pos, 4 * TILE_SIZE, true, false);
	uList Scv = INFO.getTypeUnitsInRadius(Terran_SCV, S, pos, 3 * TILE_SIZE);
	Unitset minerals = Broodwar->getUnitsInRadius(en, TILE_SIZE, Filter::IsNeutral);

	for (auto m : minerals)
	{
		if (m->getTop() - gap_s <= pos.y && m->getBottom() + gap_s >= pos.y && m->getLeft() - gap_s <= pos.x && m->getRight() + gap_s >= pos.x)
			return -1;
	}

	for (auto b : buildings)
	{
		if (b->unit()->getTop() - (gap * 2) <= pos.y && b->unit()->getBottom() + gap >= pos.y && b->unit()->getLeft() - gap <= pos.x && b->unit()->getRight() + gap >= pos.x)
			return -1;
	}

	for (auto m : Scv)
	{
		if (m->unit()->getTop() - gap_s <= pos.y && m->unit()->getBottom() + gap_s >= pos.y && m->unit()->getLeft() - gap_s <= pos.x && m->unit()->getRight() + gap_s >= pos.x)
			return -1;
	}

	int dangerPoint = 0;
	getDangerUnitNPoint(en, &dangerPoint, false);
	point = 2 * dangerPoint;

	point = point + (2 * getAltitude(WE));

	uList centers = INFO.getBuildings(Terran_Command_Center, S);

	if (centers.size() == 0)
	{
		return -1;
	}

	UnitInfo *center = centers[0];
	Unit mineral = bw->getClosestUnit(center->pos(), Filter::IsMineralField);
	Position target = (center->pos() + mineral->getPosition()) / 2;
	point = point - en.getApproxDistance(target);

	return point;
}

int MyBot::getPathValueForAir(Position en)
{	// Not walkable 또는 장애물이 있는 경우
	if (theMap.Valid(en) == false)
		return -1;

	int dangerPoint = 0;
	getDangerUnitNPoint(en, &dangerPoint, true);

	return dangerPoint;
}

UnitInfo *MyBot::getDangerUnitNPoint(Position pos, int *point, bool isFlyer)
{
	uList enemyUnits = INFO.getUnitsInRadius(E, pos, 18 * TILE_SIZE, true, true, false, true);
	uList enemyDefence = INFO.getDefenceBuildingsInRadius(E, pos, 16 * TILE_SIZE, false, true);

	if (isFlyer)
	{
		if (INFO.enemyRace == Races::Terran)
		{
			for (auto b : INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, pos, 16 * TILE_SIZE, false, true))
				enemyDefence.push_back(b);
		}
		if (INFO.enemyRace == Races::Protoss)
		{
			for (auto b : INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, pos, 16 * TILE_SIZE, false, true))
				enemyDefence.push_back(b);
		}
		if (INFO.enemyRace == Races::Zerg)
		{
			for (auto b : INFO.getTypeBuildingsInRadius(Zerg_Spore_Colony, E, pos, 16 * TILE_SIZE, false, true))
				enemyDefence.push_back(b);
		}
	}

	int min_gap = 1000;
	UnitInfo *dangerUnit = nullptr;

	for (auto eu : enemyUnits)
	{
		int weaponRange = isFlyer ? E->weaponMaxRange(eu->type().airWeapon()) : E->weaponMaxRange(eu->type().groundWeapon());

		if (eu->type() == Protoss_Carrier || eu->type() == Zerg_Scourge)
			weaponRange = 8 * TILE_SIZE;

		if (weaponRange == 0 || eu->type() == Protoss_Arbiter)
			continue;

		/// 임시로 일단 조치하고 해보자
		// Wraith는 지상유닛에서는 빼본다.... 별거 아닌 놈이라서
		if (!isFlyer && (eu->type() == Terran_Wraith || eu->type() == Protoss_Scout || eu->type() == Protoss_Carrier || eu->type() == Protoss_Interceptor))
			continue;

		if (eu->type() == Zerg_Lurker)
		{
			if (!eu->isBurrowed())
				continue;
		}
		else
		{
			if (eu->isBurrowed())
				continue;
		}

		int gap = pos.getApproxDistance(eu->pos()) - weaponRange;

		if (min_gap > gap)
		{
			min_gap = gap;
			dangerUnit = eu;
		}
	}

	for (auto eb : enemyDefence)
	{
		int weaponRange = isFlyer ? eb->type().airWeapon().maxRange() : eb->type().groundWeapon().maxRange();

		if (eb->type() == Terran_Bunker)
		{
			if (eb->getMarinesInBunker() > 0)
				weaponRange = 6 * TILE_SIZE;
			else
				continue;
		}

		int gap = pos.getApproxDistance(eb->pos()) - weaponRange;

		if (min_gap > gap)
		{
			min_gap = gap;
			dangerUnit = eb;
		}
	}

	//	if (dangerUnit == nullptr)
	if (!isFlyer)
	{
		uList myActiveMines = INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, pos, 8 * TILE_SIZE);

		for (auto m : myActiveMines)
		{
			if (m->isDangerMine())
			{
				int gap = pos.getApproxDistance(m->pos()) - 4 * TILE_SIZE; // 활성화된 마인은 주변 4 TILE 까지 위험하다

				if (min_gap > gap)
				{
					min_gap = gap;
					dangerUnit = m;
				}
			}
		}

		uList enemyWorkers = INFO.getTypeUnitsInRadius(INFO.getWorkerType(INFO.enemyRace), E, pos, 10 * TILE_SIZE);

		for (auto ew : enemyWorkers)
		{
			int gap = pos.getApproxDistance(ew->pos()) - ew->type().groundWeapon().maxRange();

			if (min_gap > gap)
			{
				min_gap = gap;
			}
		}
	}

	*point = min_gap;

	return dangerUnit;
}

UnitInfo *MyBot::getDangerUnitNPointForSCV(Unit u, int *point, bool isFlyer)
{
	Position pos = u->getPosition();

	uList enemyUnits = INFO.getUnitsInRadius(E, pos, 20 * TILE_SIZE, true, true, false, true);
	uList enemyDefence = INFO.getDefenceBuildingsInRadius(E, pos, 20 * TILE_SIZE, false, true);
	uList enemyBuilding = INFO.getBuildingsInRadius(E, pos, 20 * TILE_SIZE, true, false, true);

	if (isFlyer)
	{
		if (INFO.enemyRace == Races::Terran)
		{
			for (auto b : INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, pos, 20 * TILE_SIZE, false, true))
				enemyDefence.push_back(b);
		}

		if (INFO.enemyRace == Races::Zerg)
		{
			for (auto b : INFO.getTypeBuildingsInRadius(Zerg_Spore_Colony, E, pos, 20 * TILE_SIZE, false, true))
				enemyDefence.push_back(b);
		}
	}

	int min_gap = 1000;
	UnitInfo *dangerUnit = nullptr;

	for (auto eu : enemyUnits)
	{
		int weaponRange = isFlyer ? E->weaponMaxRange(eu->type().airWeapon()) : E->weaponMaxRange(eu->type().groundWeapon());

		if (weaponRange == 0 || eu->type() == Protoss_Arbiter)
			continue;

		if (eu->type() == Zerg_Lurker)
		{
			if (!eu->isBurrowed())
				weaponRange = 0;
		}
		else
		{
			if (eu->isBurrowed())
				weaponRange = 0;
		}

		int gap = pos.getApproxDistance(eu->pos()) - weaponRange;

		if (min_gap > gap)
		{
			min_gap = gap;
			dangerUnit = eu;
		}
	}

	for (auto eb : enemyDefence)
	{
		int weaponRange = isFlyer ? eb->type().airWeapon().maxRange() : eb->type().groundWeapon().maxRange();

		if (eb->type() == Terran_Bunker)
		{
			if (eb->getMarinesInBunker() > 0)
				weaponRange = 6 * TILE_SIZE;
			else
				continue;
		}

		int gap = getAttackDistance(eb->type(), eb->pos(), u) - weaponRange;

		if (min_gap > gap)
		{
			min_gap = gap;
			dangerUnit = eb;
		}
	}

	// 사거리 이상인 경우만 자원 체크
	if (min_gap > 0) {
		for (auto eb : enemyBuilding) {
			int gap = pos.getApproxDistance(eb->pos()) - 3 * TILE_SIZE;

			if (min_gap > gap)
			{
				min_gap = gap;
				dangerUnit = eb;
			}
		}

		Ressource *neutralUnit = nullptr;

		for (auto mineral : INFO.getMainBaseLocation(E)->Minerals()) {
			int gap = pos.getApproxDistance(mineral->Pos()) - 2 * TILE_SIZE;

			if (min_gap > gap)
			{
				min_gap = gap;
				neutralUnit = mineral;
			}
		}


		for (auto geyser : INFO.getMainBaseLocation(E)->Geysers()) {
			int gap = pos.getApproxDistance(geyser->Pos()) - 2 * TILE_SIZE;

			if (min_gap > gap)
			{
				min_gap = gap;
				neutralUnit = geyser;
			}
		}

		if (neutralUnit)
			dangerUnit = new UnitInfo(neutralUnit->Unit(), neutralUnit->Pos());
	}

	*point = min_gap;

	return dangerUnit;
}

vector<Position> MyBot::getWidePositions(Position source, Position target, bool forward, int gap, int angle)
{
	vector<Position> l;

	Position defencePos = source;

	Position forward_pos = { 0, 0 };
	// Back Move 하는 거리를 backDistaceNeed 로 통일 시키기 위한 Alpha값 찾기
	double forwardDistaceNeed = gap;
	double distance = defencePos.getDistance(target);
	double alpha = forwardDistaceNeed / distance;

	if (forward)
	{
		forward_pos.x = defencePos.x - (int)((defencePos.x - target.x) * alpha);
		forward_pos.y = defencePos.y - (int)((defencePos.y - target.y) * alpha);
	}
	else // Backend
	{
		forward_pos.x = defencePos.x + (int)((defencePos.x - target.x) * alpha);
		forward_pos.y = defencePos.y + (int)((defencePos.y - target.y) * alpha);
	}

	l.push_back(forward_pos);
	//	Broodwar->drawCircleMap(forward_pos, 2, Colors::Cyan, true);
	double degrees[4] = { angle, -angle, angle * 2, -angle * 2 };

	for (int pos_idx = 0; pos_idx <= 3; pos_idx++)
	{
		Position tmp_pos = getCirclePosFromPosByDegree(defencePos, forward_pos, degrees[pos_idx]);
		//		Broodwar->drawCircleMap(tmp_pos, 2, Colors::Cyan, true);
		l.push_back(tmp_pos);
	}

	return l;
}

vector<Position> MyBot::getRoundPositions(Position source, int gap, int angle)
{
	vector<Position> l;
	Position pos = source + Position(gap, 0);

	int total_angle = 0;

	for (int i = 0; i < 360; i++)
	{
		if (total_angle >= 360)
			break;

		pos = getCirclePosFromPosByDegree(source, pos, angle);
		total_angle += angle;

		if (theMap.Valid(pos) == false || getAltitude(pos) <= 0 ||
			bw->getUnitsInRadius(pos, TILE_SIZE, (Filter::IsMineralField || Filter::IsRefinery || (Filter::IsBuilding && !Filter::IsFlyingBuilding))).size())
			continue;

		l.push_back(pos);
	}

	return l;
}

vector<Position> MyBot::getRootToMineral(Position start)
{
	Position cur = start;
	vector<Position> rightPoses;
	vector<Position> leftPoses;

	bool finish = false;
	int direction = 1;
	int angle = 15;

	while (direction != 0)
	{
		if (direction == 1)
			rightPoses.push_back(cur);
		else
			leftPoses.push_back(cur);

		UnitInfo *cTower = INFO.getClosestTypeUnit(E, cur, INFO.getAdvancedDefenseBuildingType(INFO.enemyRace), 12 * TILE_SIZE, true);

		cur = getCirclePosFromPosByDegree(cTower->pos(), cur, angle * direction);

		Unitset destination = bw->getUnitsInRadius(cur, 4 * TILE_SIZE, (Filter::IsWorker));

		if (destination.size())
		{
			if (direction == 1)
			{
				cur = start;
				direction = -1;
			}
			else if (direction == -1)
				direction = 0;
		}

		if (cur.isValid() == false || bw->isWalkable((WalkPosition)cur) == false)
		{
			if (direction == 1)
			{
				cur = start;
				rightPoses.clear();
				direction = -1;
			}
			else if (direction == -1)
			{
				leftPoses.clear();
				direction = 0;
			}
		}
	}

	if (rightPoses.size() == 0)
		return leftPoses;
	else if (leftPoses.size() == 0)
		return rightPoses;
	else
		return rightPoses.size() < leftPoses.size() ? rightPoses : leftPoses;
}

Position MyBot::getDirectionDistancePosition(Position source, Position direction, int distance)
{
	Position forward_pos = { 0, 0 };
	// Back Move 하는 거리를 backDistaceNeed 로 통일 시키기 위한 Alpha값 찾기
	double forwardDistanceNeed = distance;
	double dist = source.getDistance(direction);
	double alpha = forwardDistanceNeed / dist;

	forward_pos.x = source.x - (int)((source.x - direction.x) * alpha);
	forward_pos.y = source.y - (int)((source.y - direction.y) * alpha);

	return forward_pos;
}

Position MyBot::getBackPostion(UnitInfo *uInfo, Position ePos, int length, bool avoidUnit)
{
	Position back_pos = { 0, 0 };
	Position myPos = uInfo->pos();

	// Back Move 하는 거리를 backDistaceNeed 로 통일 시키기 위한 Alpha값 찾기
	double distance = myPos.getDistance(ePos);
	double alpha = (double)length / distance;

	back_pos.x = myPos.x + (int)((myPos.x - ePos.x) * alpha);
	back_pos.y = myPos.y + (int)((myPos.y - ePos.y) * alpha);

	Position standardPos = back_pos;

	int total_angle = 0;

	int value = uInfo->type().isFlyer() ? getPathValueForAir(myPos) : getPathValue(myPos, myPos);
	Position bestPos = myPos;

	while (total_angle < 360)
	{
		back_pos = getCirclePosFromPosByDegree(myPos, standardPos, total_angle);
		total_angle += 30;

		if (avoidUnit)
		{
			Position behind = getDirectionDistancePosition(myPos, back_pos, 2 * TILE_SIZE);

			if (INFO.getUnitsInRadius(S, behind, (int)(1.5 * TILE_SIZE), true, false, false).size())
				continue;
		}

		int tmp_val = uInfo->type().isFlyer() ? getPathValueForAir(back_pos) : getPathValue(myPos, back_pos);

		if (value < tmp_val)
		{
			value = tmp_val;
			bestPos = back_pos;
		}
	}

	return bestPos;
}

// Unit, My Unit Position, Enemy Unit Position, move Distance
void MyBot::moveBackPostion(UnitInfo *uInfo, Position ePos, int length)
{
	if (uInfo->frame() >= TIME)
		return;

	Position back_pos = { 0, 0 };
	Position myPos = uInfo->pos();

	// Back Move 하는 거리를 backDistaceNeed 로 통일 시키기 위한 Alpha값 찾기
	double distance = myPos.getDistance(ePos);
	double alpha = (double)length / distance;

	back_pos.x = myPos.x + (int)((myPos.x - ePos.x) * alpha);
	back_pos.y = myPos.y + (int)((myPos.y - ePos.y) * alpha);

	int angle = 30;
	int total_angle = 0;

	int value = uInfo->type().isFlyer() ? getPathValueForAir(back_pos) : getPathValue(myPos, back_pos);
	Position bPos = back_pos;

	while (total_angle <= 360)
	{
		back_pos = getCirclePosFromPosByDegree(myPos, back_pos, angle);
		total_angle += angle;

		int tmp_val = uInfo->type().isFlyer() ? getPathValueForAir(back_pos) : getPathValue(myPos, back_pos);

		if (value < tmp_val)
		{
			value = tmp_val;
			bPos = back_pos;
		}
	}

	uInfo->unit()->move(bPos);

	bw->drawCircleMap(bPos, 2, Colors::Blue, true);

	uInfo->setFrame();

	if (!uInfo->type().isFlyer())
	{
		Position behind = getDirectionDistancePosition(myPos, bPos, 2 * TILE_SIZE);

		uList tmpUnits = INFO.getUnitsInRadius(S, behind, (int)(1.5 * TILE_SIZE), true, false, false);

		for (auto u : tmpUnits)
		{
			if (!isStuckOrUnderSpell(u))
				moveBackPostionbyMe(u, myPos);
		}
	}
}


// Unit, My Unit Position, Enemy Unit Position, move Distance
void MyBot::moveBackPostionMarine(UnitInfo *uInfo, Position ePos, int length)
{
	if (uInfo->frame() >= TIME)
		return;

	Position back_pos = { 0, 0 };
	Position myPos = uInfo->pos();

	// Back Move 하는 거리를 backDistaceNeed 로 통일 시키기 위한 Alpha값 찾기
	double distance = myPos.getDistance(ePos);
	double alpha = (double)length / distance;

	back_pos.x = myPos.x + (int)((myPos.x - ePos.x) * alpha);
	back_pos.y = myPos.y + (int)((myPos.y - ePos.y) * alpha);

	int angle = 15;
	int total_angle = 0;

	int value = getPathValueForMarine(myPos, back_pos);
	Position bPos = back_pos;

	while (total_angle <= 360)
	{
		back_pos = getCirclePosFromPosByDegree(myPos, back_pos, angle);
		total_angle += angle;

		int tmp_val = getPathValueForMarine(myPos, back_pos);

		if (value < tmp_val)
		{
			value = tmp_val;
			bPos = back_pos;
		}
	}

	uInfo->unit()->move(bPos);
}

// Unit, My Unit Position, Enemy Unit Position, move Distance
void MyBot::moveBackPostionbyMe(UnitInfo *uInfo, Position ePos)
{
	if (uInfo->frame() >= TIME || uInfo->type() == Terran_Siege_Tank_Siege_Mode)
		return;

	if (INFO.getCompletedCount(Terran_Factory, S) > 1 && isInMyArea(uInfo))
		return;

	Position back_pos = { 0, 0 };
	Position myPos = uInfo->pos();

	// Back Move 하는 거리를 backDistaceNeed 로 통일 시키기 위한 Alpha값 찾기
	double backDistaceNeed = 2 * TILE_SIZE;
	double distance = myPos.getDistance(ePos);
	double alpha = backDistaceNeed / distance;

	back_pos.x = myPos.x + (int)((myPos.x - ePos.x) * alpha);
	back_pos.y = myPos.y + (int)((myPos.y - ePos.y) * alpha);

	double degrees[11] = { 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330 };

	Position newPos = back_pos;

	if (!isValidPath(myPos, back_pos))
	{
		for (int i = 0; i <= 10; i++)
		{
			newPos = getCirclePosFromPosByDegree(myPos, back_pos, degrees[i]);

			if (isValidPath(myPos, newPos))
				break;
		}
	}

	uList tmpUnits = INFO.getUnitsInRadius(S, newPos, TILE_SIZE, true, false, false);
	Broodwar->drawCircleMap(newPos, TILE_SIZE, Colors::Red);

	uInfo->setFrame();
	uInfo->unit()->move(newPos);

	for (auto u : tmpUnits)
	{
		moveBackPostionbyMe(u, myPos);
	}
}

// hide 된 유닛 중 Unknown 이 아닌 유닛까지 포함해서 가져오도록 수정.(2018-05-18 윤일주)
uList MyBot::getEnemyInMyYard(int pixel, UnitType uType, bool hide)
{
	uList eList;
	int distance = 0;
	uList enemyList;

	if (uType == UnitTypes::AllUnits) {
		enemyList = INFO.getAllInRadius(E, Positions::Origin, 0, true, true, hide);
	}
	else if (uType == UnitTypes::Men) {
		enemyList = INFO.getUnitsInRadius(E, Positions::Origin, 0, true, true, true, hide);
	}
	else {
		if (uType == UnitTypes::Buildings)
			enemyList = INFO.getBuildingsInRadius(E, Positions::Origin, 0, true, true, hide);
		else if (uType.isBuilding())
			enemyList = INFO.getTypeBuildingsInRadius(uType, E);
		else
			enemyList = INFO.getTypeUnitsInRadius(uType, E, Positions::Origin, 0, hide);
	}

	if (SM.getMainStrategy() == AttackAll || SM.getMainStrategy() == DrawLine || SM.getMainStrategy() == WaitLine)
	{
		for (auto e : enemyList) {
			if (isInMyArea(e)) {
				eList.push_back(e);
			}
		}

		return eList;
	}

	if (!enemyList.empty()) {
		uList myUnits;
		uList nearMyUnits;

		for (auto e : enemyList) {
			if (e->pos() == Positions::Unknown)
				continue;

			theMap.GetPath(INFO.getMainBaseLocation(S)->getPosition(), e->pos(), &distance);

			if (isInMyArea(e) || (distance >= 0 && distance < pixel)) {
				eList.push_back(e);
			}
			else if (e->type().groundWeapon().targetsGround()) {
				if (myUnits.empty()) {
					myUnits = INFO.getAllInRadius(S);

					for (auto su : myUnits)
					if (isInMyArea(su))
						nearMyUnits.push_back(su);
				}

				int maxRange = e->type().groundWeapon().maxRange();

				for (auto nmu : nearMyUnits) {
					if (e->pos().getApproxDistance(nmu->pos()) <= maxRange) {
						eList.push_back(e);
						break;
					}
				}
			}
		}
	}

	return eList;
}

uList MyBot::getMyAttackableUnitsForScan(Unit targetUnit, int radius)
{
	if (targetUnit->getType() == Terran_Wraith)
		radius = 6 * TILE_SIZE;

	uList myAttackableUnits;
	uList myUnits = INFO.getUnitsInRadius(S, targetUnit->getPosition(), radius, true, true, false);

	for (auto mu : myUnits)
	{
		if (!mu->isComplete())
			continue;

		if (mu->type().isWorker())
			continue;

		if (mu->unit()->isStasised())
			continue;

		if (mu->type() == Terran_Marine)
			continue;

		if (mu->unit()->isInWeaponRange(targetUnit))
			myAttackableUnits.push_back(mu);
	}

	return myAttackableUnits;
}

TilePosition MyBot::findNearestTileInMyArea(TilePosition origin, TilePosition target, int Range) {

	if (theMap.GetArea(origin) == nullptr)
		return TilePositions::None;

	if (theMap.GetArea(origin) != nullptr && theMap.GetArea(target) != nullptr &&
		theMap.GetArea(origin) == theMap.GetArea(target))
		return target;

	int originAreaId = theMap.GetArea(origin)->Id();

	TilePosition max = target;
	TilePosition min = origin;

	while (min != max) {
		TilePosition center = (min + max) / 2;

		if (min == center || max == center) {
			break;
		}
		else if (theMap.GetArea(center) != nullptr && theMap.GetArea(center)->Id() == originAreaId) {
			min = center;
		}
		else {
			max = center;
		}
	}

	if (target.getApproxDistance(min) <= Range)
		return min;
	else
		return TilePositions::None;
}

/*
작성자 : 최현진
작성일 : 2018-01-31
기능 : 상대 정찰일꾼이 내 본진에 있는지 확인
return : [true]-적 정찰일꾼이 현재 본진 지역에 보임, [false]-적 정찰일꾼이 현재 본진 지역에 보이지 않음
->  없으면 null, 있으면 적 정찰일꾼 자체를 넘기도록 수정하였습니다. (손근)
주의 : 적 정찰일꾼이 현재 본진 지역에 있더라도, 내 시야가 정찰일꾼이 보이지 않으면 없는 것으로 판단함
*/
vector<UnitInfo *>MyBot::getEnemyScountInMyArea()
{
	vector<UnitInfo *> scouts;

	uList enemyWorkers = INFO.getUnits(INFO.getWorkerType(INFO.enemyRace), E);

	for (auto u : enemyWorkers)
	{
		//상대 worker 정찰병 찾기
		if (u->pos() != Positions::Unknown)
		{
			if (isSameArea(u->pos(), INFO.getMainBaseLocation(S)->getPosition())
				|| isSameArea(u->pos(), INFO.getFirstExpansionLocation(S)->getPosition()))
				scouts.push_back(u);
		}
	}

	return scouts;
}

Position MyBot::getRandPosition(Position pos)
{
	Position rPos(rand() % 65 - 32, rand() % 65 - 32);

	return (pos + rPos).isValid() ? (pos + rPos) : pos;
}


Position MyBot::findRandomeSpot(Position p) {
	int cnt = 0;

	while (cnt < 10) {
		Position rPos(2 * rand() % 65 - 32, 2 * rand() % 65 - 32);
		Position newP = p + rPos;

		if (getAltitude(newP) > 0)
			return newP;

		cnt++;
	}

	return p;
}


Position MyBot::getMiddlePositionByRate(Position p1, Position p2, double r1)
{
	int totx = p2.x - p1.x;
	int toty = p2.y - p1.y;

	int x = p1.x + (int)round((double)totx * r1, 1);
	int y = p1.y + (int)round((double)toty * r1, 1);

	return Position(x, y);
}

Position MyBot::getMiddlePositionByDist(Position p1, Position p2, int dist)
{
	double totDist = p1.getDistance(p2);

	if (totDist == 0.)
		return p1;

	double rate = (double)dist / totDist;

	return getMiddlePositionByRate(p1, p2, rate);
}

double MyBot::round(double value, int pos)
{
	double temp;
	temp = value * pow(10, pos);  // 원하는 소수점 자리수만큼 10의 누승을 함
	temp = floor(temp + 0.5);          // 0.5를 더한후 버림하면 반올림이 됨
	temp *= pow(10, -pos);           // 다시 원래 소수점 자리수로

	return temp;
}

Position MyBot::getCirclePosFromPosByDegree(Position center, Position fromPos, double degree)
{
	return getCirclePosFromPosByRadian(center, fromPos, (degree * M_PI / 180));
}

Position MyBot::getCirclePosFromPosByRadian(Position center, Position fromPos, double radian)
{
	int x = (int)((double)(fromPos.x - center.x) * cos(radian) - (double)(fromPos.y - center.y) * sin(radian) + center.x);
	int y = (int)((double)(fromPos.x - center.x) * sin(radian) + (double)(fromPos.y - center.y) * cos(radian) + center.y);

	return Position(x, y);
}

Position MyBot::getPosByPosDistDegree(Position pos, int dist, double degree)
{
	return getPosByPosDistRadian(pos, dist, (degree * M_PI / 180));
}

Position MyBot::getPosByPosDistRadian(Position pos, int dist, double radian)
{
	int x = pos.x + (int)(dist * cos(radian));
	int y = pos.y + (int)(dist * sin(radian));
	return Position(x, y);
}
int MyBot::getDamage(Unit attacker, Unit target)
{
	return Broodwar->getDamageFrom(attacker->getType(), target->getType(), attacker->getPlayer(), target->getPlayer());
}

int MyBot::getDamage(UnitType attackerType, UnitType targetType, Player attackerPlayer, Player targetPlayer)
{
	return Broodwar->getDamageFrom(attackerType, targetType, attackerPlayer, targetPlayer);
}

Base *MyBot::getMinePosition(Base *preTarget)
{
	int dist = INT_MAX;
	Base *bestBase = nullptr;
	Position prePosition = Positions::Origin;

	if (preTarget == nullptr)
		prePosition = INFO.getSecondChokePosition(S);
	else
		prePosition = preTarget->Center();

	//2. Base
	for (auto base : INFO.getBaseLocations())
	{
		if (base->GetOccupiedInfo() == myBase ||
			(base->GetOccupiedInfo() == enemyBase && base->GetEnemyGroundDefenseBuildingCount() == 0))
			continue;

		// 상대방 앞마당보다 가까운 멀티는 스카웃 하지 않는다.(안드로메다 등)
		if (INFO.getFirstExpansionLocation(E) &&
			getGroundDistance(INFO.getMainBaseLocation(E)->Center(), base->Center()) <=
			getGroundDistance(INFO.getMainBaseLocation(E)->Center(), INFO.getFirstExpansionLocation(E)->Center()))
			continue;

		if (base->isIsland()) continue;

		// 맵 중앙은 스카웃 하지 않는다. 어차피 봐
		if (base->Center().getApproxDistance(theMap.Center()) < 10 * TILE_SIZE) continue;

		// 본지 40초가 안된곳은 돌지 않는다.
		if (base->GetLastVisitedTime() + (24 * 60) > TIME) continue;

		int curDist = getGroundDistance(base->Center(), prePosition);

		if (curDist != -1 && dist > curDist)
		{
			bestBase = base;
			dist = curDist;
		}
	}

	return bestBase;
}

int MyBot::getAttackDistance(int aLeft, int aTop, int aRight, int aBottom, int tLeft, int tTop, int tRight, int tBottom) {
	// compute x distance
	int xDist = aLeft - tRight;

	if (xDist < 0)
	{
		xDist = tLeft - aRight;

		if (xDist < 0)
			xDist = 0;
	}

	// compute y distance
	int yDist = aTop - tBottom;

	if (yDist < 0)
	{
		yDist = tTop - aBottom;

		if (yDist < 0)
			yDist = 0;
	}

	// compute actual distance
	return Positions::Origin.getApproxDistance(Position(xDist, yDist));
}

int MyBot::getAttackDistance(Unit attacker, Unit target) {
	return getAttackDistance(attacker->getLeft(), attacker->getTop(), attacker->getRight(), attacker->getBottom(), target->getLeft() - 1, target->getTop() - 1, target->getRight() + 1, target->getBottom() + 1);
}

int MyBot::getAttackDistance(Unit attacker, UnitType targetType, Position targetPosition) {
	return getAttackDistance(attacker->getLeft(), attacker->getTop(), attacker->getRight(), attacker->getBottom(), targetPosition.x - targetType.dimensionLeft() - 1, targetPosition.y - targetType.dimensionUp() - 1, targetPosition.x + targetType.dimensionRight() + 1, targetPosition.y + targetType.dimensionDown() + 1);
}
int MyBot::getAttackDistance(UnitType attackerType, Position attackerPosition, Unit target) {
	return getAttackDistance(attackerPosition.x - attackerType.dimensionLeft(), attackerPosition.y - attackerType.dimensionUp(), attackerPosition.x + attackerType.dimensionRight(), attackerPosition.y + attackerType.dimensionDown(), target->getLeft() - 1, target->getTop() - 1, target->getRight() + 1, target->getBottom() + 1);
}

int MyBot::getAttackDistance(UnitType attackerType, Position attackerPosition, UnitType targetType, Position targetPosition) {
	return getAttackDistance(attackerPosition.x - attackerType.dimensionLeft(), attackerPosition.y - attackerType.dimensionUp(), attackerPosition.x + attackerType.dimensionRight(), attackerPosition.y + attackerType.dimensionDown(), targetPosition.x - targetType.dimensionLeft() - 1, targetPosition.y - targetType.dimensionUp() - 1, targetPosition.x + targetType.dimensionRight() + 1, targetPosition.y + targetType.dimensionDown() + 1);
}

int MyBot::getDamageAtPosition(Position p, Unit unit, uList enemyList, bool onlyFromBuilding) {
	int damage = 0;

	for (auto enemy : enemyList) {
		if (!enemy->isHide() && UnitUtil::CanAttack(enemy->unit(), unit)) {
			if (enemy->type().isBuilding()) {
				if (DefenseBuilding::Instance().isInAttackRange(enemy->unit(), p, unit))
					damage += getDamage(enemy->unit(), unit);
			}
			else if (!onlyFromBuilding) {
				if (getAttackDistance(enemy->unit(), unit->getType(), p) < UnitUtil::GetAttackRange(enemy->unit(), unit))
					damage += getDamage(enemy->unit(), unit);
			}
		}
		// hide 유닛인 경우도 시즈와 같이 사거리가 긴 경우가 있기 때문에 계산해준다.
		else if (enemy->pos() != Positions::Unknown && UnitUtil::GetWeapon(enemy->type(), unit->isFlying()) != WeaponTypes::None) {
			// 건물 중에는 사거리 긴 유닛이 없음.
			if (!onlyFromBuilding && !enemy->type().isBuilding()) {
				if (getAttackDistance(enemy->type(), enemy->pos(), unit->getType(), p) < UnitUtil::GetAttackRange(enemy->type(), E, unit->isFlying()))
					damage += getDamage(enemy->type(), unit->getType(), E, S);
			}
		}
	}

	return damage;
}

int MyBot::getDamageAtPosition(Position p, UnitType unitType, uList enemyList, bool onlyFromBuilding) {
	int damage = 0;

	for (auto enemy : enemyList) {
		if (!enemy->isHide() && UnitUtil::GetWeapon(enemy->type(), unitType.isFlyer()) != WeaponTypes::None) {
			if (enemy->type().isBuilding()) {
				if (DefenseBuilding::Instance().isInAttackRange(enemy->pos(), enemy->type(), p, unitType))
					damage += getDamage(enemy->type(), unitType, E, S);
			}
			else if (!onlyFromBuilding) {
				if (getAttackDistance(enemy->unit(), unitType, p) < UnitUtil::GetAttackRange(enemy->type(), E, unitType.isFlyer()))
					damage += getDamage(enemy->type(), unitType, E, S);
			}
		}
		// hide 유닛인 경우도 시즈와 같이 사거리가 긴 경우가 있기 때문에 계산해준다.
		else if (enemy->pos() != Positions::Unknown && UnitUtil::GetWeapon(enemy->type(), unitType.isFlyer()) != WeaponTypes::None) {
			// 건물 중에는 사거리 긴 유닛이 없음.
			if (!onlyFromBuilding && !enemy->type().isBuilding()) {
				if (getAttackDistance(enemy->type(), enemy->pos(), unitType, p) < UnitUtil::GetAttackRange(enemy->type(), E, unitType.isFlyer()))
					damage += getDamage(enemy->type(), unitType, E, S);
			}
		}
	}

	return damage;
}

DefenseBuilding &DefenseBuilding::Instance() {
	static DefenseBuilding defenseBuilding;
	return defenseBuilding;
}

DefenseBuilding::DefenseBuilding() {
	if (INFO.selfRace == Races::Terran) {
		myUnitTypes = { Terran_SCV, Terran_Marine, Terran_Vulture, Terran_Goliath, Terran_Siege_Tank_Tank_Mode };

		unsigned int type = 1;

		if (myUnitTypes.size() <= 32) {
			for (auto myUnitType : myUnitTypes) {
				filterTypeMap[myUnitType] = type;
				type <<= 1;
			}
		}
		else {
			cout << "myUnitTypes's size cannot exceed 32." << endl;
		}
	}

	if (INFO.enemyRace == Races::Zerg) {
		enemyTowerTypes = { Zerg_Sunken_Colony, Zerg_Spore_Colony };

		for (auto enemyTowerType : enemyTowerTypes) {
			damageMap[enemyTowerType] = vector< vector<unsigned int> >(MAX_SIZE, vector<unsigned int>(MAX_SIZE, 0));
		}
	}

	for (auto enemyTowerType : enemyTowerTypes) {
		for (auto myUnitType : myUnitTypes) {
			if ((enemyTowerType.groundWeapon().targetsGround() && !myUnitType.isFlyer()) || (enemyTowerType.airWeapon().targetsAir() && myUnitType.isFlyer())) {
				// 방워타워는 사거리 업그레이드가 없기때문에 그대로 사용.
				int weaponRange = myUnitType.isFlyer() ? enemyTowerType.airWeapon().maxRange() : enemyTowerType.groundWeapon().maxRange();

				for (int x = 0; x < MAX_SIZE; x++) {
					for (int y = 0; y < MAX_SIZE; y++) {
						if (getAttackDistance(enemyTowerType, CENTER_UNIT, myUnitType, Position(x, y)) <= weaponRange) {
							damageMap[enemyTowerType][x][y] |= filterTypeMap[myUnitType];
						}
					}
				}
			}
		}
	}
}

bool DefenseBuilding::isValid(Position p, UnitType towerType, UnitType unitType) {
	if (filterTypeMap.find(unitType) == filterTypeMap.end()) {
		cout << unitType << " unit type is not set." << endl;
		return false;
	}

	if (damageMap.find(towerType) == damageMap.end()) {
		cout << towerType << " tower type is not set." << towerType.groundWeapon() << endl;
		return false;
	}

	if (p.x < 0 || p.y < 0 || p.x >= MAX_SIZE || p.y >= MAX_SIZE) {
		return false;
	}

	return true;
}

bool DefenseBuilding::isInAttackRange(Position towerPosition, UnitType towerType, Position toGoPosition, UnitType unitType) {
	Position move = CENTER_UNIT - towerPosition;

	Position relativePosition = move + toGoPosition;

	if (isValid(relativePosition, towerType, unitType)) {
		return (damageMap[towerType][relativePosition.x][relativePosition.y] & filterTypeMap[unitType]) != 0;
	}

	return false;
}

bool DefenseBuilding::isInAttackRange(Unit towerUnit, Position toGoPosition, Unit unit) {
	return isInAttackRange(towerUnit->getPosition(), towerUnit->getType(), toGoPosition, unit->getType());
}

bool MyBot::isBeingRepaired(Unit u)
{
	for (auto scv : INFO.getTypeUnitsInRadius(Terran_SCV, S, u->getPosition(), 2 * TILE_SIZE))
	{
		if ((scv->getState() == "Repair") && scv->getTarget() == u)
		{
			return true;
		}

		if (scv->getState() == "Nightingale" && u->getType() == Terran_Science_Vessel && u->getHitPoints() < u->getType().maxHitPoints())
		{
			return true;
		}
	}

	return false;
}

bool MyBot::checkZeroAltitueAroundMe(Position myPosition, int width) {

	if (myPosition == Positions::Unknown) {
		return false;
	}

	if (getAltitude(myPosition) > 100)
		return false;

	if (INFO.getUnitsInRadius(S, myPosition, 2 * TILE_SIZE, true, false).size() > 5)
		return true;

	// 12방향 체크
	double dx[] = { 1, -1, 0, 0, 1, -1, 1, -1, 0.5, -0.5, 0.5, -0.5 };
	double dy[] = { 0, 0, -1, 1, 1, -1, -1, 1, 1, -1, -1, 1 };

	bool flag = false;

	for (int i = 0; i <= 10; i += 2) {

		for (int j = 1; j <= width; j++) {

			Position newPosition((int)((double)myPosition.x + (double)j * 8 * dx[i]), (int)((double)myPosition.y + (double)j * 8 * dy[i]));

			if (getAltitude(newPosition) <= 0)
				flag = true;
		}

		if (flag) {

			i += 1;

			for (int j = 1; j <= width; j++) {

				Position newPosition((int)((double)myPosition.x + (double)j * 8 * dx[i]), (int)((double)myPosition.y + (double)j * 8 * dy[i]));

				if (getAltitude(newPosition) <= 0)
					return true;
			}
		}

		flag = false;

	}

	return false;
}


/*
작성자 : 최현진

작성일 : 2018-01-30
기능 : blockFirstChokePoint함수에서 막는 위치를 Chokepoint 본진쪽으로 조금(15) 이동시키기 위해 사용
Parameter : 첫번째 mid position, 두번째 end 중 한개 position, 세번째 나의 BaseLocation position
*/
Position MyBot::getDefencePos(Position mid, Position end, Position mybase)
{
	int x = end.x - mid.x;
	int y = end.y - mid.y;

	double m = (double)y / (double)x;
	double nm = -1 * (1 / m); // y = nm*x

	int defence_gap = 2 * TILE_SIZE;

	Position pos1(mid.x - defence_gap, mid.y - (int)(nm * defence_gap));
	Position pos2(mid.x + defence_gap, mid.y + (int)(nm * defence_gap));

	if (theMap.GetArea((WalkPosition)pos1) == theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition()))
		return pos1;
	else
		return pos2;
}

bool MyBot::needWaiting(UnitInfo *uInfo)
{
	if (uInfo->type() == Terran_Bunker && uInfo->getMarinesInBunker() == 0)
		return false;

	if (uInfo->type().isBuilding() || uInfo->type().isFlyer())
		return true;

	if (E->weaponMaxRange(uInfo->type().groundWeapon()) > 5 * TILE_SIZE)
		return true;

	return false;
}

bool MyBot::HasEnemyFirstExpansion() {

	BWEM::Base *enemyFirstExpansionLocation = INFO.getFirstExpansionLocation(E);

	if (enemyFirstExpansionLocation == nullptr)
		return false;

	uList enemyResourceDepot;

	if (INFO.enemyRace != Races::Zerg) {
		enemyResourceDepot = INFO.getBuildings(INFO.getBasicResourceDepotBuildingType(INFO.enemyRace), E);

		if (enemyResourceDepot.size() > 1)
			return true;
	}
	else {
		enemyResourceDepot = INFO.getBuildings(Zerg_Hatchery, E);
		uList lairs = INFO.getBuildings(Zerg_Lair, E);
		enemyResourceDepot.insert(enemyResourceDepot.end(), lairs.begin(), lairs.end());
		uList hives = INFO.getBuildings(Zerg_Hive, E);
		enemyResourceDepot.insert(enemyResourceDepot.end(), hives.begin(), hives.end());
	}

	for (auto e : enemyResourceDepot)
	if (isSameArea(enemyFirstExpansionLocation->Center(), e->pos()))
		return true;

	return false;
}

bool MyBot::goWithoutDamage(Unit u, Position target, int direction, int dangerGap)
{
	// 목적지 도달
	if (target.getApproxDistance(u->getPosition()) < 2 * TILE_SIZE)
		return true;

	bool isFlyer = u->isFlying() ? true : false;
	// configuration 가능 , 각도, Gap
	int angle = 20;

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(u->getPosition(), &dangerPoint, isFlyer);

	if (dangerPoint > dangerGap || dangerUnit == nullptr)
	{
		CommandUtil::move(u, target);
		bw->drawCircleMap(getDirectionDistancePosition(u->getPosition(), target, 2 * TILE_SIZE), 2, Colors::Red, true);
		return true;
	}

	// 각도 체크
	Position vector1 = dangerUnit->pos() - u->getPosition();
	Position vector2 = target - u->getPosition();
	int inner = (vector1.x * vector2.x) + (vector1.y * vector2.y);

	if (inner < 0)
	{
		CommandUtil::move(u, target);
		bw->drawCircleMap(getDirectionDistancePosition(u->getPosition(), target, 2 * TILE_SIZE), 2, Colors::Red, true);
		return true;
	}

	int weaponRange = weaponRange = isFlyer ? E->weaponMaxRange(dangerUnit->type().airWeapon()) : E->weaponMaxRange(dangerUnit->type().groundWeapon());

	Position back = getDirectionDistancePosition(dangerUnit->pos(), u->getPosition(), weaponRange + dangerGap);
	Position movePos = getCirclePosFromPosByDegree(dangerUnit->pos(), back, angle * direction);

	if (movePos.isValid() == false)
		return false;

	if (isFlyer == false && bw->isWalkable((WalkPosition)movePos) == false)
		return false;

	u->move(movePos);
	bw->drawCircleMap(movePos, 2, Colors::Red, true);
	return true;
}

bool MyBot::goWithoutDamageForSCV(Unit u, Position target, int direction)
{
	// 목적지 도달
	if (target.getApproxDistance(u->getPosition()) < 2 * TILE_SIZE)
		return true;

	bool isFlyer = u->getType().isFlyer();
	// configuration 가능 , 각도, Gap
	int angle = 20;
	int dangerGap = 3 * TILE_SIZE;
	//
	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPointForSCV(u, &dangerPoint, isFlyer);
	Position dangerUnitPos;
	UnitType dangerUnitType;

	if (dangerUnit != nullptr) {
		dangerUnitPos = dangerUnit->pos();
		dangerUnitType = dangerUnit->type();

		if (dangerUnitType.isNeutral()) {
			delete dangerUnit;
		}
	}

	if (dangerPoint > dangerGap || dangerUnit == nullptr)
	{
		CommandUtil::move(u, target);
		bw->drawCircleMap(getDirectionDistancePosition(u->getPosition(), target, 2 * TILE_SIZE), 2, Colors::Red, true);
		return true;
	}

	// 각도 체크
	Position vector1 = dangerUnitPos - u->getPosition();
	Position vector2 = target - u->getPosition();
	int inner = (vector1.x * vector2.x) + (vector1.y * vector2.y);

	if (inner < 0)
	{
		CommandUtil::move(u, target);
		bw->drawCircleMap(getDirectionDistancePosition(u->getPosition(), target, 2 * TILE_SIZE), 2, Colors::Red, true);
		return true;
	}

	int weaponRange;
	bool isInWeaponRange = false;

	if (dangerUnitType.isNeutral()) {
		weaponRange = 2 * TILE_SIZE;
	}
	else if (isFlyer && dangerUnitType.airWeapon().targetsAir()) {
		weaponRange = E->weaponMaxRange(dangerUnitType.airWeapon()) + dangerGap;
		isInWeaponRange = dangerUnit->unit()->isInWeaponRange(u);
	}
	else if (!isFlyer && dangerUnitType.groundWeapon().targetsGround()) {
		weaponRange = E->weaponMaxRange(dangerUnitType.groundWeapon()) + dangerGap;
		isInWeaponRange = dangerUnit->unit()->isInWeaponRange(u);
	}
	else {
		weaponRange = 4 * TILE_SIZE;
	}

	Position movePos = getDirectionDistancePosition(dangerUnitPos, u->getPosition(), weaponRange);

	if (!isInWeaponRange)
		movePos = getCirclePosFromPosByDegree(dangerUnitPos, movePos, angle * direction);

	if (movePos.isValid() == false)
		return false;

	if (isFlyer == false && bw->isWalkable((WalkPosition)movePos) == false)
		return false;

	CommandUtil::move(u, movePos);
	bw->drawCircleMap(movePos, 2, Colors::Blue, true);
	return true;
}

void MyBot::kiting(UnitInfo *attacker, UnitInfo *target, int distance, int threshold)
{
	/*int backDistance = 3;
	int weapon_range = attacker->type().groundWeapon().maxRange();
	int distToTarget = attacker->pos().getApproxDistance(target->pos());

	if (target->type().isWorker())
	backDistance = 2;

	if (distance > threshold)
	{
	if (attacker->unit()->getGroundWeaponCooldown() == 0)
	CommandUtil::attackUnit(attacker->unit(), target->unit());

	if (attacker->posChange(target) == PosChange::Farther)
	{
	attacker->unit()->move((attacker->pos() + target->vPos()) / 2);
	//CommandUtil::attackUnit(attacker->unit(), target->unit());
	}
	else if (!target->type().isWorker() && distToTarget < weapon_range)
	moveBackPostion(attacker, target->pos(), backDistance * TILE_SIZE);
	// CommandUtil::attackUnit(attacker->unit(), target->unit());
	}
	else
	{
	moveBackPostion(attacker, target->pos(), backDistance * TILE_SIZE);
	CommandUtil::attackUnit(attacker->unit(), target->unit());
	}
	}*/
	int backDistance = 2;

	if (attacker->unit()->getGroundWeaponCooldown() == 0)
		CommandUtil::attackUnit(attacker->unit(), target->unit());
	else if (distance < threshold)
		moveBackPostion(attacker, target->pos(), backDistance * TILE_SIZE);
}

void MyBot::attackFirstkiting(UnitInfo *attacker, UnitInfo *target, int distance, int threshold)
{
	int backDistance = 2;

	if (attacker->unit()->getGroundWeaponCooldown() == 0)
		CommandUtil::attackUnit(attacker->unit(), target->unit());
	else if (distance < threshold)
		moveBackPostion(attacker, target->pos(), backDistance * TILE_SIZE);
}

void MyBot::pControl(UnitInfo *attacker, UnitInfo *target)
{
	if (attacker->unit()->getGroundWeaponCooldown() == 0)
	{
		if (attacker->pos().getApproxDistance(target->pos()) > 6 * TILE_SIZE)
			attacker->unit()->attack(target->unit());
		else
		{
			Position patrolPos = getDirectionDistancePosition(attacker->pos(), target->pos(), attacker->pos().getApproxDistance(target->pos()) + 2 * TILE_SIZE);

			if ((TIME - attacker->unit()->getLastCommandFrame() < 24) && attacker->unit()->getLastCommand().getType() == UnitCommandTypes::Patrol)
				return;

			attacker->unit()->patrol(patrolPos);
		}
	}
	else
	{
		if (attacker->posChange(target) == PosChange::Farther && attacker->pos().getApproxDistance(target->vPos()) > 4 * TILE_SIZE)
			attacker->unit()->move((attacker->pos() + target->vPos()) / 2);
		else {
			if (!target->type().isWorker() || attacker->pos().getApproxDistance(target->vPos()) < 2 * TILE_SIZE)
				moveBackPostion(attacker, target->pos(), 3 * TILE_SIZE);
		}
	}
}

UnitInfo *MyBot::getGroundWeakTargetInRange(UnitInfo *attacker, bool worker)
{
	uList enemy;

	if (worker)
		enemy = INFO.getUnitsInRadius(E, attacker->pos(), (int)(1.5 * S->weaponMaxRange(attacker->type().groundWeapon())), true, false, true);
	else
		enemy = INFO.getUnitsInRadius(E, attacker->pos(), (int)(1.5 * S->weaponMaxRange(attacker->type().groundWeapon())), true, false, false);

	int hp = INT_MAX;
	UnitInfo *u = nullptr;

	for (auto eu : enemy)
	{
		if (eu->type() == UnitTypes::Zerg_Egg || eu->type() == UnitTypes::Zerg_Larva || eu->type() == UnitTypes::Protoss_Interceptor ||
			eu->type() == UnitTypes::Protoss_Scarab || eu->type() == UnitTypes::Zerg_Broodling)
			continue;

		if (hp > eu->hp())
		{
			hp = eu->hp();
			u = eu;
		}
	}

	return u;
}

int MyBot::needCountForBreakMulti(UnitType uType)
{
	if (SM.getSecondAttackPosition() == Positions::Unknown)
		return 0;

	const Base *base = INFO.getNearestBaseLocation(SM.getSecondAttackPosition());

	if (uType == Terran_Vulture)
	{
		return base->GetEnemyGroundDefenseUnitCount() + 3;
	}
	else if (uType == Terran_Goliath)
	{
		return base->GetEnemyGroundDefenseBuildingCount() * 2 + base->GetEnemyGroundDefenseUnitCount() + 1;
	}
	else if (uType == Terran_Siege_Tank_Tank_Mode)
	{
		if (INFO.enemyRace == Races::Terran)
			return base->GetEnemyGroundDefenseUnitCount() + 1;
		else {
			int tankneedcnt = base->GetEnemyGroundDefenseBuildingCount() + 1;

			if (TM.getUsableTankCnt() > 8) {
				tankneedcnt = max(tankneedcnt, TM.getUsableTankCnt() - 8);
			}

			return tankneedcnt;
		}
	}

	return 0;
}

vector<Position> MyBot::makeDropshipRoute(Position startPosition, Position endPosition, bool reverse)
{
	int rightMax = theMap.Size().x * 32 - 1;
	int bottomMax = theMap.Size().y * 32 - 1;

	Position leftUp = Position(0, 0);
	Position rightUp = Position(rightMax, 0);
	Position leftDown = Position(0, bottomMax);
	Position rightDown = Position(rightMax, bottomMax);

	Position st = getCloestEndPosition(startPosition);
	Position en = getCloestEndPosition(endPosition);

	vector<Position> dropshipRoute;

	dropshipRoute.push_back(st);

	if (st.x == 0) { // 내가 왼쪽 벽에 붙어 있다.
		if (en.x == 0) { // 적이 왼쪽
			if (reverse) {
				if (st.y > en.y) {// 내가 아래에 있다.
					//왼쪽 아래 , 오른쪽 아래, 오른쪽 위, 왼쪽 위 모서리 추가
					dropshipRoute.push_back(leftDown);
					dropshipRoute.push_back(rightDown);
					dropshipRoute.push_back(rightUp);
					dropshipRoute.push_back(leftUp);
				}
				else {
					//왼쪽 위 , 오른쪽 위, 오른쪽 아래, 왼쪽 아래 모서리 추가
					dropshipRoute.push_back(leftUp);
					dropshipRoute.push_back(rightUp);
					dropshipRoute.push_back(rightDown);
					dropshipRoute.push_back(leftDown);
				}
			}
		}
		else if (en.x == rightMax) {// 적이 오른쪽
			if (st.y + en.y < (bottomMax - st.y) + (bottomMax - en.y)) // 위쪽이 더 가까워
			{
				if (reverse) {
					dropshipRoute.push_back(leftDown);
					dropshipRoute.push_back(rightDown);
				}
				else {
					//왼쪽 위 , 오른쪽 위 모서리 추가
					dropshipRoute.push_back(leftUp);
					dropshipRoute.push_back(rightUp);
				}
			}
			else
			{
				if (reverse) {
					//왼쪽 위 , 오른쪽 위 모서리 추가
					dropshipRoute.push_back(leftUp);
					dropshipRoute.push_back(rightUp);
				}
				else	{
					//왼쪽 아래 , 오른쪽 아래 모서리 추가
					dropshipRoute.push_back(leftDown);
					dropshipRoute.push_back(rightDown);
				}
			}
		}
		else if (en.y == 0) { // 적이 위에
			if (reverse) {
				dropshipRoute.push_back(leftDown);
				dropshipRoute.push_back(rightDown);
				dropshipRoute.push_back(rightUp);
			}
			else	{
				// 왼쪽 위 모서리 하나 추가
				dropshipRoute.push_back(leftUp);
			}
		}
		else { // 적이 아래에
			if (reverse) {
				dropshipRoute.push_back(leftUp);
				dropshipRoute.push_back(rightUp);
				dropshipRoute.push_back(rightDown);
			}
			else	{
				// 왼쪽 아래 모서리 하나 추가
				dropshipRoute.push_back(leftDown);
			}
		}
	}
	else if (st.x == rightMax) { // 내가 오른쪽에
		if (en.x == 0) { // 적이 왼쪽
			if (st.y + en.y < (bottomMax - st.y) + (bottomMax - en.y)) // 위쪽이 더 가까워
			{
				if (reverse)	{
					dropshipRoute.push_back(rightDown);
					dropshipRoute.push_back(leftDown);
				}
				else	{
					//오른쪽 위, 왼쪽 위 모서리 추가
					dropshipRoute.push_back(rightUp);
					dropshipRoute.push_back(leftUp);
				}
			}
			else	{
				//오른쪽 아래, 왼쪽 아래 모서리 추가
				dropshipRoute.push_back(rightDown);
				dropshipRoute.push_back(leftDown);
			}
		}
		else if (en.x == rightMax) { // 적이 오른쪽
			if (reverse) {
				if (st.y > en.y) {// 내가 아래에 있다.
					//오른쪽 아래 , 왼쪽 아래, 왼쪽 위, 오른쪽 위 모서리 추가
					dropshipRoute.push_back(rightDown);
					dropshipRoute.push_back(leftDown);
					dropshipRoute.push_back(leftUp);
					dropshipRoute.push_back(rightUp);
				}
				else {
					//오른쪽 위 , 왼쪽 위, 왼쪽 아래, 오른쪽 아래 모서리 추가
					dropshipRoute.push_back(rightUp);
					dropshipRoute.push_back(leftUp);
					dropshipRoute.push_back(leftDown);
					dropshipRoute.push_back(rightDown);
				}
			}
		}
		else if (en.y == 0) { // 적이 위에
			if (reverse) {
				dropshipRoute.push_back(rightDown);
				dropshipRoute.push_back(leftDown);
				dropshipRoute.push_back(leftUp);
			}
			else	{
				// 오른쪽 위 모서리 하나 추가
				dropshipRoute.push_back(rightUp);
			}
		}
		else {// 적이 아래에
			if (reverse) {
				dropshipRoute.push_back(rightUp);
				dropshipRoute.push_back(leftUp);
				dropshipRoute.push_back(leftDown);
			}
			else {
				// 오른쪽 아래 모서리 하나 추가
				dropshipRoute.push_back(rightDown);
			}
		}
	}
	else if (st.y == 0) {// 내가 위에
		if (en.x == 0) { // 적이 왼쪽
			if (reverse)	{
				dropshipRoute.push_back(rightUp);
				dropshipRoute.push_back(rightDown);
				dropshipRoute.push_back(leftDown);
			}
			else	{
				// 왼쪽 위 모서리 하나 추가
				dropshipRoute.push_back(leftUp);
			}
		}
		else if (en.x == rightMax) { // 적이 오른쪽
			if (reverse) {
				dropshipRoute.push_back(leftUp);
				dropshipRoute.push_back(leftDown);
				dropshipRoute.push_back(rightDown);
			}
			else {
				// 오른쪽 위 모서리 하나 추가
				dropshipRoute.push_back(rightUp);
			}
		}
		else if (en.y == 0) { // 적이 위에
			if (reverse) {
				if (st.x > en.x) { // 내가 오른쪽에 있음.
					dropshipRoute.push_back(rightUp);
					dropshipRoute.push_back(rightDown);
					dropshipRoute.push_back(leftDown);
					dropshipRoute.push_back(leftUp);
				}
				else {
					dropshipRoute.push_back(leftUp);
					dropshipRoute.push_back(leftDown);
					dropshipRoute.push_back(rightDown);
					dropshipRoute.push_back(rightUp);
				}
			}
		}
		else {// 적이 아래에
			if (st.x + en.x < (rightMax - st.x) + (rightMax - en.x)) // 왼쪽이 더 가까워
			{
				if (reverse) {
					dropshipRoute.push_back(rightUp);
					dropshipRoute.push_back(rightDown);
				}
				else	{
					//왼쪽 위, 왼쪽 아래 모서리 추가
					dropshipRoute.push_back(leftUp);
					dropshipRoute.push_back(leftDown);
				}
			}
			else
			{
				if (reverse) {
					//왼쪽 위, 왼쪽 아래 모서리 추가
					dropshipRoute.push_back(leftUp);
					dropshipRoute.push_back(leftDown);
				}
				else {
					//오른쪽 위, 오른쪽 아래 모서리 추가
					dropshipRoute.push_back(rightUp);
					dropshipRoute.push_back(rightDown);
				}
			}
		}
	}
	else { // 내가 아래에
		if (en.x == 0) { // 적이 왼쪽
			if (reverse) {
				dropshipRoute.push_back(rightDown);
				dropshipRoute.push_back(rightUp);
				dropshipRoute.push_back(leftUp);
			}
			else {
				// 왼쪽 아래 모서리 하나 추가
				dropshipRoute.push_back(leftDown);
			}
		}
		else if (en.x == rightMax) { // 적이 오른쪽
			if (reverse)	{
				dropshipRoute.push_back(leftDown);
				dropshipRoute.push_back(leftUp);
				dropshipRoute.push_back(rightUp);
			}
			else {
				// 오른쪽 아래 모서리 하나 추가
				dropshipRoute.push_back(rightDown);
			}
		}
		else if (en.y == 0) { // 적이 위에
			if (st.x + en.x < (rightMax - st.x) + (rightMax - en.x)) // 왼쪽이 더 가까워
			{
				if (reverse) {
					dropshipRoute.push_back(rightDown);
					dropshipRoute.push_back(rightUp);
				}
				else {
					//왼쪽 아래, 왼쪽 위 모서리 추가
					dropshipRoute.push_back(leftDown);
					dropshipRoute.push_back(leftUp);
				}
			}
			else
			{
				if (reverse) {
					//왼쪽 아래, 왼쪽 위 모서리 추가
					dropshipRoute.push_back(leftDown);
					dropshipRoute.push_back(leftUp);
				}
				else {
					//오른쪽 아래, 오른쪽 위 모서리 추가
					dropshipRoute.push_back(rightDown);
					dropshipRoute.push_back(rightUp);
				}
			}
		}
		else {// 적이 아래에
			if (reverse) {
				if (st.x > en.x) { // 내가 오른쪽에 있음.
					dropshipRoute.push_back(rightDown);
					dropshipRoute.push_back(rightUp);
					dropshipRoute.push_back(leftUp);
					dropshipRoute.push_back(leftDown);
				}
				else {
					dropshipRoute.push_back(leftDown);
					dropshipRoute.push_back(leftUp);
					dropshipRoute.push_back(rightUp);
					dropshipRoute.push_back(rightDown);
				}
			}
		}
	}

	dropshipRoute.push_back(en);
	dropshipRoute.push_back(endPosition);

	return dropshipRoute;
}

Position MyBot::getCloestEndPosition(Position p)
{
	// 4 가지 Position

	int rightMax = theMap.Size().x * 32 - 1;
	int bottomMax = theMap.Size().y * 32 - 1;

	Position l(0, p.y);
	Position r(rightMax, p.y);
	Position t(p.x, 0);
	Position b(p.x, bottomMax);

	Position minPos = l;
	int minDist = p.x;

	if (minDist > rightMax - p.x)
	{
		minPos = r;
		minDist = rightMax - p.x;
	}

	if (minDist > p.y)
	{
		minPos = t;
		minDist = p.y;
	}

	if (minDist > bottomMax - p.y)
	{
		minPos = b;
		minDist = bottomMax - p.y;
	}

	return minPos;
}

UnitInfo *MyBot::getClosestUnit(Position myPosition, uList units, bool gDist)
{
	UnitInfo *closest = nullptr;
	int closestDist = INT_MAX;

	for (auto u : units)
	{
		if (u->pos() == Positions::Unknown)
			continue;

		if (gDist)
		{
			int dist = getGroundDistance(myPosition, u->pos());

			if (dist >= 0 && dist < closestDist)
			{
				closest = u;
				closestDist = dist;
			}
		}
		else
		{
			Position newPos = myPosition - u->pos();

			if ((newPos.x * newPos.x + newPos.y * newPos.y) < closestDist)
			{
				closest = u;
				closestDist = (newPos.x * newPos.x + newPos.y * newPos.y);
			}
		}
	}

	return closest;
}

uList MyBot::getEnemyOutOfRange(Position pos, bool gDist, bool hide)
{
	uList list;
	uList list_worker;

	if (INFO.getMainBaseLocation(E) == nullptr)
		return list;

	Position EnBase = INFO.getMainBaseLocation(E)->Center();

	if (getGroundDistance(EnBase, pos) < 0)
		return list;

	int Threshold = gDist ? getGroundDistance(EnBase, pos) : pos.getApproxDistance(EnBase);

	for (auto &u : INFO.getUnits(E))
	{
		if (hide == false && u.second->isHide())
			continue;

		if (u.second->type().isFlyer())
			continue;

		if (u.second->type() == Terran_Vulture_Spider_Mine)
			continue;

		if (u.second->pos() == Positions::Unknown)
			continue;

		if (gDist)
		{
			int dist = getGroundDistance(u.second->pos(), EnBase);

			if (dist > Threshold)
			{
				if (u.second->type().isWorker())
					list_worker.push_back(u.second);
				else
					list.push_back(u.second);
			}
		}
		else
		{
			int dist = u.second->pos().getApproxDistance(EnBase);

			if (dist > Threshold)
			{
				if (u.second->type().isWorker())
					list_worker.push_back(u.second);
				else
					list.push_back(u.second);
			}
		}
	}

	return list.size() ? list : list_worker;
}

bool MyBot::isNeedKitingUnitTypeforMarine(UnitType uType)
{
	return  uType == Terran_Firebat  || Terran_Medic ||  uType == Protoss_Dark_Templar || uType == Protoss_Dark_Archon || Terran_Dropship
		|| uType == Zerg_Overlord || uType == Zerg_Defiler || uType == Zerg_Devourer || uType == Protoss_Corsair
		|| uType == Protoss_Carrier || uType == Zerg_Scourge || uType == Terran_Science_Vessel || uType == Terran_Valkyrie;
}
bool MyBot::isNeedKitingUnitType(UnitType uType)
{
	return uType == Protoss_Zealot || uType == Protoss_Dark_Templar || uType == Protoss_Dark_Archon || uType == Terran_Marine
		|| uType == Terran_Firebat || uType == Zerg_Zergling || uType == Zerg_Ultralisk || uType.isWorker();
}
bool MyBot::isNeedKitingUnitTypeinAir(UnitType uType)
{
	return uType == Protoss_Zealot || uType == Protoss_Dark_Templar || uType == Protoss_Dark_Archon || uType == Protoss_Dragoon
		|| uType == Terran_Firebat || uType == Terran_Marine || uType == Zerg_Zergling || uType == Zerg_Ultralisk || uType == Zerg_Hydralisk || uType.isWorker()
		|| uType == Zerg_Mutalisk || uType == Zerg_Guardian || uType == Zerg_Queen || uType == Zerg_Devourer || uType == Zerg_Scourge || uType == Zerg_Zergling
		|| uType == Zerg_Lurker || uType == Zerg_Defiler || uType == Terran_Science_Vessel || uType == Terran_Ghost
		|| uType == Terran_Battlecruiser || uType == Terran_Vulture || uType == Terran_Siege_Tank_Tank_Mode || uType == Terran_Siege_Tank_Siege_Mode || uType == Terran_Siege_Tank_Tank_Mode
		|| uType == Terran_Wraith || uType == Terran_Medic || uType == Protoss_High_Templar||uType == Protoss_Shuttle
		|| uType == Protoss_Reaver || uType == Protoss_Observer || uType == Protoss_Scout || uType == Protoss_Corsair
		|| uType == Protoss_Carrier || uType == Terran_Valkyrie || uType == Terran_Dropship ||  uType == Protoss_Archon;
}
bool MyBot::isMyCommandAtFirstExpansion() {
	return INFO.getFirstExpansionLocation(S)
		&& (INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, INFO.getFirstExpansionLocation(S)->Center(), 5 * TILE_SIZE, true).size() > 0);
}

bool MyBot::isStuckOrUnderSpell(UnitInfo *uInfo)
{
	if (uInfo->frame() >= TIME)
		return true;

	if (escapeFromSpell(uInfo))
		return true;

	if (uInfo->type() != Terran_Siege_Tank_Siege_Mode && uInfo->unit()->isStuck())
	{
		if (uInfo->unit()->getLastCommand().getType() != UnitCommandTypes::Stop)
			uInfo->unit()->stop();

		bw->drawCircleMap(uInfo->pos(), 12, Colors::Red, true);
		uInfo->setFrame();
		return true;
	}

	if (INFO.needMoveInside() // 안으로 가야 되고
		&& uInfo->pos().getApproxDistance(INFO.getFirstChokePosition(E)) < 6 * TILE_SIZE // 6 TILE 안에 있는 유닛 중에
		&& (uInfo->pos().getApproxDistance(INFO.getFirstChokePosition(E)) < 3 * TILE_SIZE // 초크 근처거나
		|| isSameArea(uInfo->pos(), INFO.getMainBaseLocation(E)->Center())))					// 이미 상대 본진에 들어간 유닛에 대하여
	{
		bw->drawCircleMap(uInfo->pos(), 12, Colors::Cyan, true);

		if (uInfo->type() == Terran_Siege_Tank_Siege_Mode)
			uInfo->unit()->unsiege();
		else
		{
			if (uInfo->unit()->getGroundWeaponCooldown() == 0 || uInfo->unit()->getAirWeaponCooldown() == 0)
				CommandUtil::attackMove(uInfo->unit(), INFO.getMainBaseLocation(E)->Center());
			else
				CommandUtil::move(uInfo->unit(), INFO.getMainBaseLocation(E)->Center());
		}

		uInfo->setFrame();
		return true;
	}
	else
		return false;
}

bool MyBot::escapeFromSpell(UnitInfo *uInfo) {

	// 내 마인이 곧 터질꺼 같으면 ... 일단 빼자
	for (auto mm : INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, uInfo->pos(), 3 * TILE_SIZE))
	{
		if (mm->isDangerMine())
		{
			moveBackPostion(uInfo, mm->pos(), 3 * TILE_SIZE);
			return true;
		}
	}

	// 다크스웜
	if (INFO.enemyRace == Races::Zerg) {

		bool needToEscape = false;
		Unitset spells = bw->getUnitsInRadius(uInfo->pos(), 20 * TILE_SIZE, Filter::IsSpell);

		if (spells.empty())
			return false;

		uList eList = INFO.getUnitsInRadius(E, uInfo->pos(), 20 * TILE_SIZE, true, false, false);

		// 내가 다크스웜 안에 있는 적의 사거리에서 벗어난다.
		for (auto e : eList) {
			int range = e->unit()->getType() == Zerg_Lurker ? 10 * TILE_SIZE : e->unit()->getType().groundWeapon().maxRange() + 2 * TILE_SIZE;

			if (e->unit()->isUnderDarkSwarm()) {
				if (e->pos().getApproxDistance(uInfo->pos()) <= range) {
					needToEscape = true;
					break;
				}
			}
		}

		if (uInfo->unit()->isUnderDarkSwarm())
			needToEscape = true;

		if (!needToEscape)
			return needToEscape;

		if (uInfo->type() == Terran_Siege_Tank_Siege_Mode) {

			if (uInfo->unit()->getGroundWeaponCooldown() == 0) {

				// 스웜 밖의 디파일러는 쏘자
				uList defilers = INFO.getTypeUnitsInRadius(Zerg_Defiler, E, uInfo->pos(), 12 * TILE_SIZE);

				for (auto d : defilers) {
					if (!uInfo->unit()->isUnderDarkSwarm() && uInfo->unit()->isInWeaponRange(d->unit())) {
						CommandUtil::attackUnit(uInfo->unit(), d->unit());
						return true;
					}
				}
			}

			TM.unsiege(uInfo->unit());
			return true;

		}
		else {

			UnitInfo *closeUnit = INFO.getClosestUnit(E, uInfo->pos(), GroundUnitKind, uInfo->type().groundWeapon().maxRange(), true);

			if (closeUnit && !closeUnit->unit()->isUnderDarkSwarm()) {
				if (uInfo->unit()->getGroundWeaponCooldown() == 0) {
					CommandUtil::attackUnit(uInfo->unit(), closeUnit->unit());
					return true;
				}
			}

			CommandUtil::move(uInfo->unit(), MYBASE);
			return true;
		}
	}
	// 스캐럽, 스톰, Sweep
	else if (INFO.enemyRace == Races::Protoss) {

		if (uInfo->unit()->isUnderDisruptionWeb()) {

			if (uInfo->type() == Terran_Siege_Tank_Siege_Mode) {
				TM.unsiege(uInfo->unit());
			}
			else if (uInfo->type() == Terran_Siege_Tank_Tank_Mode)
				CommandUtil::move(uInfo->unit(), MYBASE);
			else if (uInfo->type() == Terran_Goliath)
				CommandUtil::move(uInfo->unit(), MYBASE);
			else if (uInfo->type() == Terran_Vulture)
				CommandUtil::move(uInfo->unit(), MYBASE);
			else
				return false;

			return true;
		}


		// 스캐럽 피하기
		Unitset units = bw->getUnitsInRadius(uInfo->pos(), 8 * TILE_SIZE);
		Unit scarab = nullptr;

		for (auto u : units) {
			if (u->getType() == Protoss_Scarab) {
				scarab = u;
				break;
			}
		}

		if (scarab) {

			if (uInfo->type() == Terran_Siege_Tank_Tank_Mode)
				moveBackPostion(uInfo, scarab->getPosition(), 3 * TILE_SIZE);
			else if (uInfo->type() == Terran_Goliath)
				moveBackPostion(uInfo, scarab->getPosition(), 3 * TILE_SIZE);
			else if (uInfo->type() == Terran_Vulture)
				moveBackPostion(uInfo, scarab->getPosition(), 3 * TILE_SIZE);
			else
				// 시즈모드 탱크
				return false;

			return true;
		}

		// 스톰 피하기
		if (uInfo->unit()->isUnderStorm()) {
			if (uInfo->type() == Terran_Siege_Tank_Tank_Mode)
				CommandUtil::move(uInfo->unit(), MYBASE);
			else if (uInfo->type() == Terran_Goliath)
				CommandUtil::move(uInfo->unit(), MYBASE);
			else if (uInfo->type() == Terran_Vulture)
				CommandUtil::move(uInfo->unit(), MYBASE);

			else
				// 시즈모드 탱크
				return false;

			return true;

		}

	}
	else {

		if (INFO.nucLaunchedTime != 0) {

			// [Note] 핵에 대한 대처 2가지
			// 1. 핵 쏘는 고스트를 잡는다.
			// 2. 도망간다.
			// 공중에서 떨어지고 있는 핵을 발견하면 무조건 도망가자.
			uList nList = INFO.getTypeUnitsInRadius(Terran_Nuclear_Missile, E, uInfo->pos(), 10 * TILE_SIZE);
			Position nucHitPoint = Positions::Origin;

			if (!nList.empty()) {
				// 혹시나 핵이 여러개라면 평균에서 멀어지자.
				for (auto n : nList)
					nucHitPoint += n->pos();

				nucHitPoint /= nList.size();

				// 공중에서 핵이 떨어지고 있으면 도망간다. 시즈는 못도망감
				if (uInfo->type() != Terran_Siege_Tank_Siege_Mode)
					moveBackPostion(uInfo, nucHitPoint, 15 * TILE_SIZE);

				return true;
			}

			// 떨어지고 있는 핵이 없고, NukeDot을 발견했을 때. (ComsatManager 에서 스캔뿌린다.)
			// 핵쏘는 고스트를 죽이거나, 도망가거나
			if (!bw->getNukeDots().empty()) {
				// 나랑 가까운 핵
				for (auto p : bw->getNukeDots()) {
					// 고스트 최대 사거리 9 + 핵 사거리 8 + 여유 1
					if (p.getApproxDistance(uInfo->pos()) <= 18 * TILE_SIZE) {
						nucHitPoint = p;
						break;
					}
				}

				if (nucHitPoint != Positions::Origin) {
					// 고스트를 죽여보자
					// 고스트 사거리 9 타일
					UnitInfo *nucLaucherGhost = INFO.getClosestTypeUnit(E, nucHitPoint, Terran_Ghost, 10 * TILE_SIZE, true, false, false);
					// 핵이 발사되고 떨어지는 시간 13초(여유있게, 실제로는 15~17초)
					int remainingTime = (24 * 13) - (TIME - INFO.nucLaunchedTime);
					int distanceFromGhost = 0;
					bool amIUnderNucRange = uInfo->pos().getApproxDistance(nucHitPoint) <= 11 * TILE_SIZE;

					if (amIUnderNucRange)
						bw->drawCircleMap(uInfo->pos(), 5, Colors::Red, true);

					if (nucLaucherGhost) {
						// 고스트가 발견됨
						// (고스트와 나와 거리 - 내 공격 사거리) / 내 속도 = 고스트까지 가는 시간
						distanceFromGhost = uInfo->pos().getApproxDistance(nucLaucherGhost->pos()) - uInfo->type().groundWeapon().maxRange();
						int timeToGetGhost = distanceFromGhost / (int)(uInfo->type().topSpeed() == 0 ? 2 : uInfo->type().topSpeed());

						// 핵이 떨어지기 전에 고스트로 닿을 수 있다면
						if (remainingTime > timeToGetGhost) {
							int siegeCnt = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, nucLaucherGhost->pos(), 12 * TILE_SIZE - uInfo->type().groundWeapon().maxRange(), true).size();

							// Detected 이면, 주변에 적이 많지 않고 거리가 가까우면 죽임
							// 고스트 주변에 시즈가 많으면 도망간다.
							// 내 본진에 떨구는 핵은 그냥 죽이려고 시도해보자
							if (nucLaucherGhost->unit()->isDetected() || ComsatStationManager::Instance().getAvailableScanCount() > 0) {
								if (siegeCnt < 4 || isInMyArea(nucLaucherGhost)) {
									if (uInfo->type() == Terran_Siege_Tank_Siege_Mode) {
										if (uInfo->unit()->isInWeaponRange(nucLaucherGhost->unit()))
											CommandUtil::attackUnit(uInfo->unit(), nucLaucherGhost->unit());
										else
											uInfo->unit()->unsiege();
									}
									else
										CommandUtil::attackUnit(uInfo->unit(), nucLaucherGhost->unit());

									if (uInfo->unit()->isSelected())
										cout << "고스트 죽이자" << endl;

									return true;

								}
							}
						}
					}

					// 고스트를 못주이면 도망가자
					if (amIUnderNucRange) {
						if (uInfo->type() == Terran_Siege_Tank_Siege_Mode)
							uInfo->unit()->unsiege();
						else
							moveBackPostion(uInfo, nucHitPoint, 15 * TILE_SIZE);

						if (uInfo->unit()->isSelected())
							cout << "도망가자" << endl;

						return true;
					}

				}
			}
		}

		// 테란전
		// 마인 피하기
		uList mList = INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, E, uInfo->pos(), 6 * TILE_SIZE);

		if (mList.empty())
			return false;

		int distance = INT_MAX;
		UnitInfo *mine = nullptr;

		// 가장 가까운 튀어나온 마인을 찾는다.
		for (auto m : mList) {
			int temp = m->pos().getApproxDistance(uInfo->pos());

			if (!m->unit()->isBurrowed() && temp < distance) {
				mine = m;
				distance = temp;
			}
		}

		// 시즈 모드 탱크는 아무것도 안함
		if (!mine || uInfo->type() == Terran_Siege_Tank_Siege_Mode)
			return false;

		if (uInfo->unit()->getGroundWeaponCooldown() == 0)
			CommandUtil::attackUnit(uInfo->unit(), mine->unit());
		else
			moveBackPostion(uInfo, mine->pos(), 3 * TILE_SIZE);

		return true;

	}

	return false;
}

// Zerg 변태중인것은 알수 없음. 완성된 건물은 현재 완성되었다고 판단하고 계산.
int MyBot::getBuildStartFrame(Unit building) {
	if (!building->exists())
		return -1;

	int buildTime = building->getType().buildTime();
	UnitInfo *buildingInfo = INFO.getUnitInfo(building, S);

	if (building->isCompleted() && buildingInfo != nullptr)
		return buildingInfo->getCompleteTime() - buildTime;

	int maxHP = building->getType().maxHitPoints();
	int hp = building->getHitPoints();

	// build 시작시점의 hp
	int initHP = int(maxHP * 0.1);
	// 프레임당 hp 증가량
	double hpPerFrame = double(maxHP - initHP) / buildTime;

	int elapsedFrame = int((hp - initHP) / hpPerFrame);

	return TIME - elapsedFrame;
}

int MyBot::getRemainingBuildFrame(Unit building) {
	if (building->getPlayer() == S)
		return building->getRemainingBuildTime();

	if (!building->getType().isBuilding())
		return 0;

	int buildTime = building->getType().buildTime();
	int maxHP = building->getType().maxHitPoints();
	int hp = building->getHitPoints();

	// build 시작시점의 hp
	int initHP = int(maxHP * 0.1);
	// 프레임당 hp 증가량
	double hpPerFrame = double(maxHP - initHP) / buildTime;

	int remainingFrame = int((maxHP - hp) / hpPerFrame);

	return remainingFrame;
}
