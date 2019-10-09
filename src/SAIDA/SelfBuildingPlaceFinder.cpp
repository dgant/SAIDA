#include "SelfBuildingPlaceFinder.h"
#include "../UnitManager/TankManager.h"

#define AREA_TOP baseArea->TopLeft().y
#define AREA_LEFT baseArea->TopLeft().x
#define AREA_BOTTOM baseArea->BottomRight().y + 1
#define AREA_RIGHT baseArea->BottomRight().x + 1

using namespace MyBot;


SelfBuildingPlaceFinder &SelfBuildingPlaceFinder::Instance()
{
	static SelfBuildingPlaceFinder instance;
	return instance;
}

SelfBuildingPlaceFinder::SelfBuildingPlaceFinder()
{
	setBasicVariable(INFO.getMainBaseLocation(S));

	
	calcFirstSupplyPosition();

	
	calcStandardPositionToStuck();

	
	TilePosition tmpPosition = supplyStdPosition;
	int strIdx = totCnt;
	int lstIdx;

	bool isNearEnough = false;
	vector<pair<TilePosition, int>> supplyReserveList;
	int range = isExistSpace ? 10 : 8;
	int STOP_CNT = 3;
	int skipCnt = 0;

	for (lstIdx = strIdx; lstIdx < 25 || STOP_CNT > 0; ) {
		tmpPosition = calcNextBuildPosition(supplyStdPosition, tmpPosition);

		if (tmpPosition == TilePositions::None) {
			break;
		}

		int dist = 0;

		
		if (!isNearEnough) {
			dist = tmpPosition.getApproxDistance(baseTilePosition);

			if (dist <= range) {
				isNearEnough = true;
			}
		}
		else {
			STOP_CNT--;
		}

		supplyReserveList.emplace_back(tmpPosition, dist);

		if (lstIdx >= 25) {
			skipCnt++;

			while (skipCnt > 0) {
				word maxIdx = 0;

				for (word i = 0; i < supplyReserveList.size(); i++) {
					if (supplyReserveList.at(i).second > supplyReserveList.at(maxIdx).second) {
						maxIdx = i;
					}
				}

				if (tmpPosition == supplyReserveList.at(maxIdx).first && STOP_CNT > 0)
					break;

				ReserveBuilding::Instance().freeTiles(supplyReserveList.at(maxIdx).first, Terran_Supply_Depot);
				supplyReserveList.erase(supplyReserveList.begin() + maxIdx);
				skipCnt--;
			}
		}
		else {
			lstIdx++;
		}
	}

	for (word i = 0; i < supplyReserveList.size(); i++) {
		tilePos[i + strIdx] = supplyReserveList.at(i).first;
	}


	sortBuildOrder(strIdx, lstIdx);

	
	if (INFO.enemyRace == Races::Protoss || INFO.enemyRace == Races::Unknown) {
		TOT_FACTORY_COUNT++;
	}

	calcFactoryPosition(factoryStdPosition, FACTORY_MOVE_DIRECTION[0], FACTORY_MOVE_DIRECTION[1], TOT_FACTORY_COUNT, false);

	
	calcFirstBarracksPosition();

	barracksPositionInSecondChokePoint = TilePositions::Unknown;
	engineeringBayPositionInSecondChokePoint = TilePositions::Unknown;
	setSecondChokePointReserveBuilding();
}

void SelfBuildingPlaceFinder::setBasicVariable(const Base *base) {
	baseTilePosition = base->getTilePosition();

	baseArea = base->GetArea();
	baseAreaId = baseArea->Id();

	bottomMineral = base->getBottomMineral();
	rightMineral = base->getRightMineral();
	leftMineral = base->getLeftMineral();
	topMineral = base->getTopMineral();

	isLeftOfBase = base->isMineralsLeftOfBase();
	isTopOfBase = base->isMineralsTopOfBase();

	isExistSpace = base->isExistBackYard();

	isBaseTop = AREA_BOTTOM - baseTilePosition.y > baseTilePosition.y + 3 - AREA_TOP;
	isBaseLeft = AREA_RIGHT - baseTilePosition.x > baseTilePosition.x + 4 - AREA_LEFT;

	const ChokePoint *cp = baseArea->ChokePoints().front();

	const Area *pairBase = INFO.getMainBasePairArea(baseArea);

	if (pairBase) {
		cp = &pairBase->ChokePoints(INFO.getFirstExpansionLocation(S)->GetArea()).front();
	}

	chokePoint = (TilePosition)cp->Pos(ChokePoint::middle);

	if (chokePoint.y - baseTilePosition.y >= 7) {
		chokePointDirection = Direction::DOWN;
	}
	else if (baseTilePosition.y - chokePoint.y >= 5) {
		chokePointDirection = Direction::UP;
	}
	else if (chokePoint.x < baseTilePosition.x) {
		chokePointDirection = Direction::LEFT;
	}
	else {
		chokePointDirection = Direction::RIGHT;
	}

	
	ReserveBuilding::Instance().reserveTiles(baseTilePosition - 1, TilePosition(1, 5), 0, 0, 0, 0, ReserveTypes::AVOID);
	ReserveBuilding::Instance().reserveTiles(baseTilePosition + TilePosition(6, 0), TilePosition(1, 4), 0, 0, 0, 0, ReserveTypes::AVOID);
}

void SelfBuildingPlaceFinder::calcFirstSupplyPosition() {
	if (INFO.enemyRace == Races::Terran) {
		bool next = false;

		Position p1 = INFO.getFirstChokePosition(S, ChokePoint::end1);
		Position p2 = INFO.getFirstChokePosition(S, ChokePoint::end2);

		int dx = p1.x - p2.x;
		int dy = p1.y - p2.y;
		int min = p1.y * dy + p1.x * dx;
		int max = p2.y * dy + p2.x * dx;
		bool positive = dx * dy >= 0;

		if ((dy > 0 && min > max) || (dy < 0 && min < max))
			swap(min, max);

		bool isNotBlocked = false;
		int topLeft = 0, bottomRIght = 0, bottomLeft = 0, topRight = 0;

		do {
			tilePos[0] = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Supply_Depot, (TilePosition)INFO.getFirstChokePosition(S), next, INFO.getMainBaseLocation(S)->Center());
			next = true;

			
			if (positive) {
				int topLeft = dy * tilePos[0].y * 32 + dx * tilePos[0].x * 32;
				int bottomRight = dy * (tilePos[0].y + Terran_Supply_Depot.tileHeight()) * 32 + dx * (tilePos[0].x + Terran_Supply_Depot.tileWidth()) * 32;

				isNotBlocked = dy > 0 ? bottomRight < min || topLeft > max : bottomRight > min || topLeft < max;
			}
			else {
				int bottomLeft = dy * (tilePos[0].y + Terran_Supply_Depot.tileHeight()) * 32 + dx * tilePos[0].x * 32;
				int topRight = dy * tilePos[0].y * 32 + dx * (tilePos[0].x + Terran_Supply_Depot.tileWidth()) * 32;

				isNotBlocked = dy > 0 ? bottomLeft < min || topRight > max : bottomLeft > min || topRight < max;
			}
		} while (!isNotBlocked && tilePos[0] != TilePositions::None);

		ReserveBuilding::Instance().forceReserveTiles(tilePos[0], { Terran_Supply_Depot }, 1, 1, 1, 1);

		
		TilePosition avoidTile = baseTilePosition + TilePosition(-1, Terran_Command_Center.tileHeight());

		for (int i = 0; i < Terran_Command_Center.tileWidth() + Terran_Comsat_Station.tileWidth() + 1; i++) {
			ReserveBuilding::Instance().reserveTiles(avoidTile + TilePosition(i, 0), TilePosition(1, 1), 0, 0, 0, 0, ReserveTypes::MINERALS);
		}
	}
	else {
		bool needZerg4DroneDefense = INFO.enemyRace == Races::Zerg && chokePointDirection == Direction::DOWN;

	
		if (isLeftOfBase) {
			tilePos[0] = baseTilePosition + TilePosition(1, Terran_Command_Center.tileHeight() + 1);
			ReserveBuilding::Instance().reserveTiles(tilePos[0], { Terran_Supply_Depot }, 1, 0, 1, 0);
		}
		else {
			tilePos[0] = baseTilePosition + TilePosition(0, Terran_Command_Center.tileHeight());
			ReserveBuilding::Instance().reserveTiles(tilePos[0], { Terran_Supply_Depot }, 0, 1, 1, !needZerg4DroneDefense);
		}

		
		if (needZerg4DroneDefense) {
			TilePosition t = tilePos[0] + TilePosition(0, Terran_Supply_Depot.tileHeight());
			ReserveBuilding::Instance().reserveTiles(t, Terran_Bunker.tileSize(), 0, 0, 0, 0, ReserveTypes::AVOID);
		}
	}

	totCnt++;
}

void SelfBuildingPlaceFinder::calcStandardPositionToStuck() {
	
	if (chokePointDirection == Direction::DOWN) {
		
		supplyStdPosition.x = isBaseLeft ? AREA_LEFT : AREA_RIGHT - Terran_Supply_Depot.tileWidth();
		supplyStdPosition.y = AREA_TOP;

		SUPPLY_MOVE_DIRECTION[0] = isBaseLeft ? RIGHT : LEFT;
		SUPPLY_MOVE_DIRECTION[1] = DOWN;

		
		factoryStdPosition.x = isBaseLeft ? AREA_LEFT : AREA_RIGHT - Terran_Factory.tileWidth() - Terran_Machine_Shop.tileWidth() - 1;
		factoryStdPosition.y = bottomMineral->getBottom() / TILEPOSITION_SCALE + 1;

		
		FACTORY_MOVE_DIRECTION[0] = DOWN;
		FACTORY_MOVE_DIRECTION[1] = isBaseLeft ? RIGHT : LEFT;
	}
	
	else if (chokePointDirection == Direction::UP) {
		
		supplyStdPosition.x = isBaseLeft ? AREA_LEFT : AREA_RIGHT - Terran_Supply_Depot.tileWidth();
		supplyStdPosition.y = AREA_BOTTOM - Terran_Supply_Depot.tileHeight();

		
		SUPPLY_MOVE_DIRECTION[0] = isBaseLeft ? RIGHT : LEFT;
		SUPPLY_MOVE_DIRECTION[1] = UP;

		
		factoryStdPosition.x = isBaseLeft ? AREA_LEFT : AREA_RIGHT - Terran_Factory.tileWidth() - Terran_Machine_Shop.tileWidth() - 1;
		factoryStdPosition.y = topMineral->getTop() / TILEPOSITION_SCALE - Terran_Factory.tileHeight() - 1;

		
		FACTORY_MOVE_DIRECTION[0] = UP;
		FACTORY_MOVE_DIRECTION[1] = isBaseLeft ? RIGHT : LEFT;
	}
	
	else if (chokePointDirection == Direction::LEFT) {
		
		factoryStdPosition.x = AREA_LEFT;
		factoryStdPosition.y = AREA_TOP;

		
		FACTORY_MOVE_DIRECTION[0] = DOWN;
		FACTORY_MOVE_DIRECTION[1] = RIGHT;

		
		if (isExistSpace) {
			supplyStdPosition.x = isLeftOfBase ? AREA_LEFT : AREA_RIGHT - Terran_Supply_Depot.tileWidth();
			supplyStdPosition.y = isBaseTop ? AREA_TOP : AREA_BOTTOM - Terran_Supply_Depot.tileHeight();

			
			SUPPLY_MOVE_DIRECTION[0] = isBaseTop ? DOWN : UP;
			SUPPLY_MOVE_DIRECTION[1] = isLeftOfBase ? RIGHT : LEFT;
		}
		
		else {
			supplyStdPosition.x = AREA_RIGHT - Terran_Supply_Depot.tileWidth();
			supplyStdPosition.y = isBaseTop ? AREA_TOP : AREA_BOTTOM - Terran_Supply_Depot.tileHeight();

			
			SUPPLY_MOVE_DIRECTION[0] = isBaseTop ? DOWN : UP;
			SUPPLY_MOVE_DIRECTION[1] = LEFT;
		}
	}
	else if (chokePointDirection == Direction::RIGHT) {
		
		factoryStdPosition.x = AREA_RIGHT - Terran_Factory.tileWidth() - Terran_Machine_Shop.tileWidth() - 1;
		factoryStdPosition.y = AREA_TOP;

		
		FACTORY_MOVE_DIRECTION[0] = DOWN;
		FACTORY_MOVE_DIRECTION[1] = LEFT;

		
		if (isExistSpace) {
			supplyStdPosition.x = isLeftOfBase ? AREA_LEFT : AREA_RIGHT - Terran_Supply_Depot.tileWidth();
			supplyStdPosition.y = isBaseTop ? AREA_TOP : AREA_BOTTOM - Terran_Supply_Depot.tileHeight();

			
			SUPPLY_MOVE_DIRECTION[0] = isBaseTop ? DOWN : UP;
			SUPPLY_MOVE_DIRECTION[1] = isLeftOfBase ? RIGHT : LEFT;
		}
		
		else {
			supplyStdPosition.x = AREA_LEFT;
			supplyStdPosition.y = isBaseTop ? AREA_TOP : AREA_BOTTOM - Terran_Supply_Depot.tileHeight();

			
			SUPPLY_MOVE_DIRECTION[0] = isBaseTop ? DOWN : UP;
			SUPPLY_MOVE_DIRECTION[1] = RIGHT;
		}
	}
}


void SelfBuildingPlaceFinder::sortBuildOrder(int strIdx, int lstIdx) {
	
	sort(tilePos + strIdx, tilePos + lstIdx, [](TilePosition a, TilePosition b) {
		return a.getDistance(INFO.getMainBaseLocation(S)->getTilePosition()) < b.getDistance(INFO.getMainBaseLocation(S)->getTilePosition());
	});

	
	sort(tilePos + strIdx + 3, tilePos + lstIdx, [](TilePosition a, TilePosition b) {
		
		bool distA = abs(INFO.getMainBaseLocation(S)->getTilePosition().x - a.x) <= 12 && abs(INFO.getMainBaseLocation(S)->getTilePosition().y - a.y) <= 12;
		bool distB = abs(INFO.getMainBaseLocation(S)->getTilePosition().x - b.x) <= 12 && abs(INFO.getMainBaseLocation(S)->getTilePosition().y - b.y) <= 12;

		if (distA && !distB)
			return true;
		else if (!distA && distB)
			return false;

		if (theMap.GetTile(a).MinAltitude() == theMap.GetTile(b).MinAltitude()) {
			return a.getDistance(INFO.getMainBaseLocation(S)->getTilePosition()) > b.getDistance(INFO.getMainBaseLocation(S)->getTilePosition());
		}

		return theMap.GetTile(a).MinAltitude() < theMap.GetTile(b).MinAltitude();
	});

	ReserveBuilding::Instance().reordering(tilePos, strIdx, lstIdx, Terran_Supply_Depot);
}

int SelfBuildingPlaceFinder::calcFactoryPosition(TilePosition factoryStdPosition, TilePosition MOVE_DIRECTION1, TilePosition MOVE_DIRECTION2, int factoryCnt, bool recursive) {
	if (factoryCnt == 0)
		return 0;

	bool debug = false;

	int iSize = AREA_RIGHT - AREA_LEFT;
	int upDownSize = AREA_BOTTOM - AREA_TOP;
	int upDown = 0;
	TilePosition firstPosition = TilePositions::None;
	TilePosition lastPosition = TilePositions::None;
	bool reserveTile = false;

	TilePosition gasPosition = TilePositions::None;

	for (auto geysers : baseArea->Geysers()) {
		gasPosition = geysers->TopLeft();
	}

	
	for (int i = 0; i < iSize; ) {
		firstPosition = TilePositions::None;

		TilePosition leftRightPosition = factoryStdPosition + MOVE_DIRECTION2 * i;

		
		if (!leftRightPosition.isValid() || leftRightPosition.x < AREA_LEFT || leftRightPosition.x > AREA_RIGHT)
			break;

		int cnt = 0;

		while (true) {
			TilePosition upDownPosition = leftRightPosition + MOVE_DIRECTION1 * upDown;

			if (!upDownPosition.isValid() ||
					((theMap.GetArea(upDownPosition) == nullptr || !isSameArea(baseArea, theMap.GetArea(upDownPosition))) &&
					 (upDownPosition.y < AREA_TOP || upDownPosition.y > AREA_BOTTOM || upDown >= upDownSize))) {
				if (debug) {
					cout << "AREA_TOP : " << AREA_TOP;
					cout << ", AREA_BOTTOM : " << AREA_BOTTOM;
					cout << ", upDownSize : " << upDownSize;
					cout << ", upDownPosition : " << upDownPosition << endl;
				}

				i += reserveTile ? Terran_Factory.tileWidth() + Terran_Machine_Shop.tileWidth() + 1 : 1;
				upDown = 0;
				reserveTile = false;
				break;
			}

			
			if (theMap.GetArea(upDownPosition) == nullptr || !isSameArea(baseArea, theMap.GetArea(upDownPosition))) {
				if (debug)
					cout << "area : " << upDownPosition << endl;

				upDown++;
				continue;
			}

			
			if (upDownPosition.y == gasPosition.y - 3 && upDownPosition.x >= gasPosition.x - 6 && upDownPosition.x <= gasPosition.x + 4) {
				if (debug)
					cout << "gas : " << upDownPosition << endl;

				upDown++;
				continue;
			}

			
			if (upDownPosition.y + 3 <= topMineral->getTilePosition().y && topMineral->getTilePosition().y <= upDownPosition.y + 4 && upDownPosition.x + 6 >= topMineral->getTilePosition().x && upDownPosition.x <= topMineral->getTilePosition().x) {
				if (debug)
					cout << "mineral : " << upDownPosition << endl;

				upDown++;
				continue;
			}

			if (BuildingPlaceFinder::Instance().canBuildHere(upDownPosition, Terran_Factory, true, nullptr, MOVE_DIRECTION1 == UP, chokePointDirection == Direction::LEFT, chokePointDirection != Direction::LEFT, MOVE_DIRECTION1 == DOWN)) {
				if (firstPosition == TilePositions::None) {
					firstPosition = upDownPosition;
				}

				upDown += Terran_Factory.tileHeight();

				cnt++;

				if (cnt >= 2 || (factoryCnt < TOT_FACTORY_COUNT && cnt > 0)) {
					reserveTile = true;
				}
			}
			else if (cnt) {
				if (debug)
					cout << "cannot build (cnt > 0) : " << upDownPosition << endl;

				break;
			}
			else {
				if (debug)
					cout << "cannot build : " << upDownPosition << endl;

				upDown++;
			}
		}

		
		if (cnt >= 2 || (factoryCnt < TOT_FACTORY_COUNT && cnt > 0)) {
			
			lastPosition = firstPosition;

			int interval = 0;

			for (int j = 1; j <= cnt; j++) {
				TilePosition reservePosition = firstPosition + MOVE_DIRECTION1 * (j - 1) * Terran_Factory.tileHeight() + MOVE_DIRECTION1 * interval;
				bool reserved;
				char *kindFordebug = "기본";

				
				if (interval == 1) {
					reserved = ReserveBuilding::Instance().reserveTiles(reservePosition, { Terran_Factory, Terran_Starport, Terran_Science_Facility }, 0, chokePointDirection == Direction::LEFT, chokePointDirection != Direction::LEFT, 0);
					kindFordebug = "간격";
				}
				
				else if (reservePosition.y <= chokePoint.y && chokePoint.y < reservePosition.y + Terran_Factory.tileHeight()) {
					if (j == cnt)
						break;

					reservePosition = reservePosition + DOWN * (MOVE_DIRECTION1 == DOWN ? 1 : 0);
					reserved = ReserveBuilding::Instance().reserveTiles(reservePosition, { Terran_Factory, Terran_Starport, Terran_Science_Facility }, 1, chokePointDirection == Direction::LEFT, chokePointDirection != Direction::LEFT, 0);
					interval = 1;
					kindFordebug = "초크";
				}
				
				else if (j == 1 && MOVE_DIRECTION1 == UP && reservePosition.y <= chokePoint.y && chokePoint.y < reservePosition.y + 2 * Terran_Factory.tileHeight()) {
					reservePosition = reservePosition + UP;
					reserved = ReserveBuilding::Instance().reserveTiles(reservePosition, { Terran_Factory, Terran_Starport, Terran_Science_Facility }, 0, chokePointDirection == Direction::LEFT, chokePointDirection != Direction::LEFT, 1);
					interval = 1;
					kindFordebug = "1초크";
				}
				
				else if (j == 1 && MOVE_DIRECTION1 == DOWN && reservePosition.y - Terran_Factory.tileHeight() <= chokePoint.y && chokePoint.y < reservePosition.y) {
					reservePosition = reservePosition + DOWN;
					reserved = ReserveBuilding::Instance().reserveTiles(reservePosition, { Terran_Factory, Terran_Starport, Terran_Science_Facility }, 1, chokePointDirection == Direction::LEFT, chokePointDirection != Direction::LEFT, 0);
					interval = 1;
					kindFordebug = "2초크";
				}
				else {
					reserved = ReserveBuilding::Instance().reserveTiles(reservePosition, { Terran_Factory, Terran_Starport, Terran_Science_Facility }, MOVE_DIRECTION1 == UP && j == cnt, chokePointDirection == Direction::LEFT, chokePointDirection != Direction::LEFT, MOVE_DIRECTION1 == DOWN && j == cnt);
				}

				if (reserved) {
					if (debug)
						cout << "[" << kindFordebug << "] @" << reservePosition << " CP(L) : " << (chokePointDirection == Direction::LEFT) << " 상 : " << (MOVE_DIRECTION1 == UP) << " CP @" << chokePoint << " 순서 : " << j << "/" << cnt << " cnt " << factoryCnt << "/" << TOT_FACTORY_COUNT << endl;

					if (!--factoryCnt)
						return 0;
				}
			}
		}
	}

	
	if (!recursive) {
		if (debug)
			cout << "상하 반대" << lastPosition << endl;

		
		factoryCnt = calcFactoryPosition(lastPosition, MOVE_DIRECTION1 * -1, MOVE_DIRECTION2, factoryCnt, true);

		if (debug)
			cout << "상하, 좌우 반대" << lastPosition << endl;

		
		factoryCnt = calcFactoryPosition(lastPosition, MOVE_DIRECTION1 * -1, MOVE_DIRECTION2 * -1, factoryCnt, true);

		if (debug)
			cout << "최초 기준 반대방향" << lastPosition << endl;


		factoryCnt = calcFactoryPosition(factoryStdPosition, MOVE_DIRECTION1 * -1, MOVE_DIRECTION2, factoryCnt, true);
	}

	return factoryCnt;
}

TilePosition SelfBuildingPlaceFinder::calcNextBuildPosition(TilePosition supplyStdPosition, TilePosition lastPos) {
	
	for (int i = 0; i < 20; i++) {
		
		for (int move0 = 0; move0 <= 20; move0++) {
			TilePosition tmp = lastPos + SUPPLY_MOVE_DIRECTION[0] * move0;

			
			if (!tmp.isValid() || theMap.GetArea(tmp) == nullptr || baseAreaId != theMap.GetArea(tmp)->Id()) {
				continue;
			}

			if (BuildingPlaceFinder::Instance().canBuildHere(tmp, Terran_Supply_Depot, true)) {
				ReserveBuilding::Instance().reserveTiles(tmp, { Terran_Supply_Depot, Terran_Academy, Terran_Armory }, 0, 0, 0, 0);
				totCnt++;
				return tmp;
			}
		}

		int move1;

		if (SUPPLY_MOVE_DIRECTION[1] == LEFT || SUPPLY_MOVE_DIRECTION[1] == RIGHT) {
			if (lastPos.y != supplyStdPosition.y) {
				move1 = Terran_Supply_Depot.tileWidth();
			}
			else {
				move1 = 1;
			}

			lastPos.x = (lastPos + SUPPLY_MOVE_DIRECTION[1] * move1).x;
			lastPos.y = supplyStdPosition.y;
		}
		else {
			if (lastPos.x != supplyStdPosition.x) {
				move1 = Terran_Supply_Depot.tileHeight();
			}
			else {
				move1 = 1;
			}

			lastPos.x = supplyStdPosition.x;
			lastPos.y = (lastPos + SUPPLY_MOVE_DIRECTION[1] * move1).y;
		}
	}

	return TilePositions::None;
}

void SelfBuildingPlaceFinder::calcFirstBarracksPosition() {
	TilePosition tilePosition, desirePosition = TilePositions::None;

	
	if (INFO.enemyRace == Races::Terran) {
		TilePosition targetPosition = TilePositions::Origin;
		double dist = 0;

		switch (INFO.getMapPlayerLimit()) {
			case 2:

				
				for (TilePosition startingLocation : theMap.StartingLocations()) {
					if (startingLocation == baseTilePosition) {
						continue;
					}

					targetPosition = startingLocation;
				}

				break;

			case 3:

				
				for (TilePosition startingLocation : theMap.StartingLocations()) {
					if (startingLocation == baseTilePosition) {
						continue;
					}

					targetPosition += startingLocation;
				}

				targetPosition /= 2;
				break;

			case 4:

				for (TilePosition startingLocation : theMap.StartingLocations()) {
					double tmpDist = pow(baseTilePosition.x - startingLocation.x, 2) + pow(baseTilePosition.y - startingLocation.y, 2);

					if (tmpDist > dist) {
						dist = tmpDist;
						targetPosition = startingLocation;
					}
				}

				break;

			default:
				break;
		}

		desirePosition = BuildingPlaceFinder::Instance().findNearestBuildableTile(baseTilePosition, targetPosition, Terran_Barracks);
	}
	
	else if (INFO.enemyRace == Races::Zerg) {
		TilePosition geyserPosition = TilePositions::None;

		
		for (auto geyser : INFO.getMainBaseLocation(S)->Geysers()) {
			if (INFO.getMainBaseLocation(S)->getTilePosition().y > geyser->TopLeft().y) {
				geyserPosition = geyser->TopLeft();
				break;
			}
			else {
				
				geyserPosition = INFO.getMainBaseLocation(S)->getTilePosition() - TilePosition(0, Terran_Bunker.tileHeight());
			}

		}

		
		for (auto move : {
					TilePosition(5, 0), TilePosition(-1, -3), TilePosition(-5, 0)
				}) {
			if (bw->canBuildHere(geyserPosition + move, Terran_Barracks)) {
				desirePosition = geyserPosition + move;
				break;
			}
		}
	}
	
	else if (INFO.enemyRace == Races::Protoss || INFO.enemyRace == Races::Unknown) {
		tilePosition = baseTilePosition;

		
		if (chokePointDirection == Direction::UP) {
			tilePosition += TilePosition(0, -8);
		}
		
		else if (chokePointDirection == Direction::DOWN) {
			tilePosition += TilePosition(0, 5);
		}
		
		else if (chokePointDirection == Direction::LEFT) {
			tilePosition += TilePosition(-5, 0);
		}
	
		else if (chokePointDirection == Direction::RIGHT) {
			tilePosition += TilePosition(7, 0);
		}

		int minDist[2] = { 128, 128 };
		TilePosition nearPosition[2];

		for (auto &b : ReserveBuilding::Instance().getReserveList()) {
			for (auto t : b.getUnitTypes()) {
				if (t == Terran_Factory) {
					int dist = b.TopLeft().getApproxDistance(tilePosition);

					if (dist < minDist[0]) {
						minDist[1] = minDist[0];
						nearPosition[1] = nearPosition[0];
						minDist[0] = dist;
						nearPosition[0] = b.getTilePosition();
					}
					else if (dist < minDist[1]) {
						minDist[1] = dist;
						nearPosition[1] = b.getTilePosition();
					}
				}
			}
		}

		for (auto t : nearPosition) {
			desirePosition = t - TilePosition(1, 0);

			if (bw->canBuildHere(desirePosition, Terran_Barracks)) {
				supplyByBarracks = desirePosition + TilePosition(Terran_Barracks.tileWidth(), 1);

				ReserveBuilding::Instance().forceReserveTilesFirst(supplyByBarracks, { Terran_Supply_Depot }, 0, 0, 0, 0);
				TilePosition sortList[] = { tilePos[0], supplyByBarracks };
				ReserveBuilding::Instance().reordering(sortList, 0, 2, Terran_Supply_Depot);

				totCnt++;

				break;
			}
		}
	}


	if (desirePosition == TilePositions::None) {
		cout << "cannot find barrack position" << endl;

		return;
	}

	ReserveBuilding::Instance().forceReserveTiles(desirePosition, { Terran_Barracks }, 0, 0, 0, 0);
}

bool SelfBuildingPlaceFinder::calcSecondSupplyPosition(TilePosition barracksPos) {
	TilePosition pos = barracksPos + TilePosition(Terran_Barracks.tileWidth(), 1);

	if (BuildingPlaceFinder::Instance().canBuildHere(pos, Terran_Supply_Depot, true)) {
		supplyByBarracks = pos;
		return ReserveBuilding::Instance().reserveTiles(pos, { Terran_Supply_Depot }, 0, 0, 0, 0);
	}

	return false;
}

void SelfBuildingPlaceFinder::freeSecondSupplyDepot() {
	ReserveBuilding::Instance().freeTiles(supplyByBarracks, Terran_Supply_Depot);
}

TilePosition SelfBuildingPlaceFinder::getBuildLocationWithSeedPositionAndStrategy(UnitType buildingType, TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy) {
	TilePosition desiredPosition = TilePositions::None;

	if (buildingType == Terran_Bunker) {
		switch (seedPositionStrategy) {

			case BuildOrderItem::SeedPositionStrategy::MainBaseLocation:

				
				if (INFO.enemyRace == Races::Zerg && chokePointDirection == Direction::DOWN) {
					desiredPosition = tilePos[0] + TilePosition(0, Terran_Supply_Depot.tileHeight());
				}
				
				else {
					
					for (auto geyser : INFO.getMainBaseLocation(S)->Geysers()) {
						if (INFO.getMainBaseLocation(S)->getTilePosition().y > geyser->TopLeft().y) {
							desiredPosition = geyser->TopLeft() + TilePosition(0, 2);
							break;
						}
						else {
							desiredPosition = INFO.getMainBaseLocation(S)->getTilePosition() - TilePosition(0, Terran_Bunker.tileHeight());
						}
					}
				}

				break;

			case BuildOrderItem::SeedPositionStrategy::FirstChokePoint:
				

				break;

			case BuildOrderItem::SeedPositionStrategy::SecondChokePoint:
				
				desiredPosition = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Bunker, INFO.getSecondChokePointBunkerPosition(), false, INFO.getFirstExpansionLocation(S)->getPosition());

				
				if (INFO.getFirstExpansionLocation(S)->Center().getApproxDistance((Position)desiredPosition + ((Position)Terran_Bunker.tileSize() / 2)) < 5 * TILE_SIZE) {
					if (INFO.getFirstExpansionLocation(S)->getTilePosition().x == (desiredPosition + Terran_Bunker.tileSize()).x) {

						desiredPosition -= TilePosition(1, 0);
					}

					if (INFO.getFirstExpansionLocation(S)->getTilePosition().y == (desiredPosition + Terran_Bunker.tileSize()).y) {
						desiredPosition -= TilePosition(0, 1);
					}

					if ((INFO.getFirstExpansionLocation(S)->getTilePosition() + Terran_Command_Center.tileSize()).x == desiredPosition.x) {
						desiredPosition += TilePosition(1, 0);
					}

					if ((INFO.getFirstExpansionLocation(S)->getTilePosition() + Terran_Command_Center.tileSize()).y == desiredPosition.y) {
						desiredPosition += TilePosition(0, 1);
					}
				}

				break;
		}

		if (desiredPosition.isValid() && !ReserveBuilding::Instance().reserveTiles(desiredPosition, { buildingType }, 0, 0, 0, 0)) {
			cout << "Terran_Bunker Reserve failed! " << desiredPosition << endl;
			desiredPosition = TilePositions::None;
		}
	}
	else if (buildingType == Terran_Factory) {
		
		if (ESM.getEnemyInitialBuild() <= Zerg_9_Drone || ESM.getEnemyInitialBuild() == Toss_2g_zealot || ESM.getEnemyInitialBuild() == Toss_1g_forward || ESM.getEnemyInitialBuild() == Toss_2g_forward
				|| ESM.getEnemyInitialBuild() == Terran_bunker_rush || ESM.getEnemyInitialBuild() == Toss_cannon_rush || ESM.getEnemyInitialBuild() == Zerg_sunken_rush) {
			uList bunkers = INFO.getTypeBuildingsInRadius(Terran_Bunker, S, (Position)baseTilePosition, 10 * TILE_SIZE);

			TilePosition pos = baseTilePosition;

			if (!bunkers.empty()) {
				pos = bunkers.front()->unit()->getTilePosition();
			}

			
			if (INFO.getAllCount(Terran_Factory, S) || ESM.getEnemyInitialBuild() == Toss_cannon_rush || ESM.getEnemyInitialBuild() == Zerg_sunken_rush) {
				int dist = 10;

				for (auto &building : ReserveBuilding::Instance().getReserveList()) {
					if (building.canAssignToType(buildingType, true) && bw->canBuildHere(building.TopLeft(), Terran_Factory)) {
						int tempDist = pos.getApproxDistance(building.TopLeft());

						if (tempDist < dist) {
							dist = tempDist;
							desiredPosition = building.TopLeft();
						}
					}
				}

				
			}
			
			else {
				
				int bunkerMinX = 0;
				int bunkerMinY = 0;
				int bunkerMaxX = 0;
				int bunkerMaxY = 0;

				for (auto b : ReserveBuilding::Instance().getReserveList()) {
					for (auto t : b.getUnitTypes()) {
						if (t == Terran_Bunker) {
							bunkerMinX = b.TopLeft().x;
							bunkerMinY = b.TopLeft().y;
							bunkerMaxX = b.TopLeft().x + Terran_Bunker.tileWidth();
							bunkerMaxY = b.TopLeft().y + Terran_Bunker.tileHeight();
							break;
						}
					}

					if (bunkerMaxX > 0)
						break;
				}

				int geyserMinX = 0;
				int geyserMinY = 0;
				int geyserMaxX = 0;
				int geyserMaxY = 0;

				
				for (auto geyser : INFO.getMainBaseLocation(S)->Geysers()) {
					if (INFO.getMainBaseLocation(S)->getTilePosition().y > geyser->TopLeft().y) {
						geyserMinX = geyser->TopLeft().x;
						geyserMinY = geyser->TopLeft().y + Terran_Refinery.tileHeight();
						geyserMaxX = geyser->TopLeft().x + Terran_Bunker.tileWidth();
						geyserMaxY = geyser->TopLeft().y + Terran_Refinery.tileHeight() + Terran_Bunker.tileHeight();
						break;
					}
				}

				int dist = 8;

				bool isFirst = false;
				bool searchContinue = false;
				desiredPosition = pos;

				do {
					
					desiredPosition = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Factory, desiredPosition, isFirst, Positions::None, true);

					if (desiredPosition == TilePositions::None)
						break;

					searchContinue = false;

				
					if (bunkerMinX < desiredPosition.x + Terran_Factory.tileWidth() && desiredPosition.x < bunkerMaxX
							&& bunkerMinY < desiredPosition.y + Terran_Factory.tileHeight() && desiredPosition.y < bunkerMaxY) {
						searchContinue = true;
					}
					
					else if (geyserMinX < desiredPosition.x + Terran_Factory.tileWidth() && desiredPosition.x < geyserMaxX
							 && geyserMinY < desiredPosition.y + Terran_Factory.tileHeight() && desiredPosition.y < geyserMaxY) {
						searchContinue = true;
					}
					else if (!INFO.getTypeUnitsInRectangle(Terran_SCV, S, (Position)desiredPosition, (Position)(desiredPosition + Terran_Factory.tileSize())).empty()) {
						searchContinue = true;
					}

					isFirst = true;
				} while (searchContinue);

				ReserveBuilding::Instance().forceReserveTilesFirst(desiredPosition, { Terran_Factory }, 0, 0, 0, 0);

				
			}
		}
		
		else if (INFO.getFirstChokePosition(S) != Positions::None) {
			
			if (INFO.getAllCount(Terran_Factory, S) == 0) {
				int dist = INT_MAX;

				for (auto &building : ReserveBuilding::Instance().getReserveList()) {
					if (building.canAssignToType(buildingType, true) && bw->canBuildHere(building.TopLeft(), Terran_Factory)) {
						
						bool canBuild = true;

						for (int x = building.BottomRight().x; x < building.BottomRight().x + 2 && canBuild; x++) {
							for (int y = building.TopLeft().y + 1; y < building.BottomRight().y; y++) {
								if (!bw->isBuildable(x, y, true)) {
									canBuild = false;
									break;
								}
							}
						}

						if (!canBuild || INFO.getFirstChokePosition(S).getApproxDistance(building.Center()) < 6 * TILE_SIZE)
							continue;

						int tempDist = INFO.getMainBaseLocation(S)->Center().getApproxDistance((Position)building.TopLeft() + ((Position)Terran_Factory.tileSize() / 2));

					
						if (tempDist < dist) {
							dist = tempDist;
							desiredPosition = building.TopLeft();
						}
					}
				}
			}
			
			else if (INFO.getAllCount(Terran_Factory, S) < 4) {
				int dist = INT_MAX;

				for (auto &building : ReserveBuilding::Instance().getReserveList()) {
					if (building.canAssignToType(buildingType, true) && bw->canBuildHere(building.TopLeft(), Terran_Factory)) {
						int tempDist = INFO.getMainBaseLocation(S)->Center().getApproxDistance((Position)building.TopLeft() + ((Position)Terran_Factory.tileSize() / 2));

						if (tempDist < dist) {
							dist = tempDist;
							desiredPosition = building.TopLeft();
						}
					}
				}
			}
			else {
				int dist = INT_MAX;

				for (auto &building : ReserveBuilding::Instance().getReserveList()) {
					if (building.canAssignToType(buildingType, true) && bw->canBuildHere(building.TopLeft(), Terran_Factory)) {
						int tempDist = INFO.getFirstChokePosition(S).getApproxDistance((Position)building.TopLeft() + ((Position)Terran_Factory.tileSize() / 2));

						if (tempDist < dist) {
							dist = tempDist;
							desiredPosition = building.TopLeft();
						}
					}
				}
			}
		}
	}
	else if (buildingType == Terran_Starport || buildingType == Terran_Science_Facility) {
		
		if (INFO.getFirstChokePosition(S) != Positions::None) {
			int dist = INT_MIN;

			for (auto &building : ReserveBuilding::Instance().getReserveList()) {
				if (building.canAssignToType(buildingType, true) && bw->canBuildHere(building.TopLeft(), buildingType)) {
					int tempDist = getGroundDistance(INFO.getFirstChokePosition(S), (Position)building.TopLeft() + ((Position)buildingType.tileSize() / 2));

					if (tempDist > dist) {
						dist = tempDist;
						desiredPosition = building.TopLeft();
					}
				}
			}
		}
	}
	else if (buildingType == Terran_Missile_Turret) {
		const Base *base = nullptr;

		if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::MainBaseLocation)
			base = INFO.getMainBaseLocation(S);
		else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)
			base = INFO.getFirstExpansionLocation(S);
		else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation)
			base = INFO.getSecondExpansionLocation(S);
		else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation)
			base = INFO.getThirdExpansionLocation(S);
		else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::SeedPositionSpecified) {
			for (auto b : INFO.getBaseLocations()) {
				if (b->getTilePosition() == seedPosition) {
					base = b;
					break;
				}
			}
		}

		if (base) {
			vector<TilePosition> buildPos;
			
			Direction geyserDirection = Direction::NONE;
			int geyserX = 0;

			for (auto g : base->Geysers()) {
				int diffX = base->Center().x - g->Pos().x;
				int diffY = base->Center().y - g->Pos().y;
				geyserX = g->TopLeft().x;

			
				if (abs(diffX) <= abs(diffY))
					geyserDirection = diffY > 0 ? Direction::UP : Direction::DOWN;
				
				else
					geyserDirection = diffX > 0 ? Direction::LEFT : Direction::RIGHT;
			}

			
			if (base->isMineralsPlacedVertical()) {
			
				if (base->isMineralsLeftOfBase()) {
					
					if (geyserDirection == Direction::DOWN) {
						
						buildPos.push_back(base->getTilePosition() + TilePosition(-2, 1));

						
						if (base->getTilePosition().x == geyserX)
							buildPos.push_back(base->getTilePosition() + TilePosition(1, 3));
						else
							buildPos.push_back(base->getTilePosition() + TilePosition(0, 3));

						
						buildPos.push_back(base->getTilePosition() + TilePosition(0, -2));
					}
					else {
						
						buildPos.push_back(base->getTilePosition() + TilePosition(-2, 1));
						
						buildPos.push_back(base->getTilePosition() + TilePosition(0, -2));
						
						buildPos.push_back(base->getTilePosition() + TilePosition(0, 3));
					}
				}
				
				else if (base->isMineralsRightOfBase()) {
					
					buildPos.push_back(base->getTilePosition() + TilePosition(4, -1));

					
					vector<vector<short>> rMap = ReserveBuilding::Instance().getReserveMap();

					vector<TilePosition> tPosVec;

					
					tPosVec.push_back(base->getTilePosition() + TilePosition(2, 3));
					
					tPosVec.push_back(base->getTilePosition() + TilePosition(1, 5));

					for (auto tPos : tPosVec) {
						if (rMap[tPos.x - 1][tPos.y + 2] + rMap[tPos.x][tPos.y + 2]
								+ rMap[tPos.x + 1][tPos.y + 2] + rMap[tPos.x + 2][tPos.y + 2] == 0
								&& bw->isBuildable(tPos.x - 1, tPos.y + 2, true) && bw->isBuildable(tPos.x, tPos.y + 2, true)
								&& bw->isBuildable(tPos.x + 1, tPos.y + 2, true) && bw->isBuildable(tPos.x + 2, tPos.y + 2, true)) {
							buildPos.push_back(tPos);
						}
					}

					
					buildPos.push_back(base->getTilePosition() + TilePosition(1, -2));
				}
			}
			
			else {
			
				if (base->isMineralsTopOfBase()) {
					
					if (geyserDirection == Direction::RIGHT) {
						
						buildPos.push_back(base->getTilePosition() + TilePosition(0, -2));
						
						buildPos.push_back(base->getTilePosition() + TilePosition(4, -1));
					}
					else if (geyserDirection == Direction::LEFT) {
						
						buildPos.push_back(base->getTilePosition() + TilePosition(0, -2));
						
						buildPos.push_back(base->getTilePosition() + TilePosition(-2, 1));
					}
				}
				
				else if (base->isMineralsBottomOfBase()) {
					
					buildPos.push_back(base->getTilePosition() + TilePosition(0, 3));
					buildPos.push_back(base->getTilePosition() + TilePosition(3, 3));
				}
			}

			for (auto t : buildPos) {
				if (ReserveBuilding::Instance().canReserveHere(t, Terran_Missile_Turret, 0, 0, 0, 0)) {
					desiredPosition = t;
					break;
				}
			}
		}
		else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::FirstChokePoint) {
			bool next = false;

			Position p1 = INFO.getFirstChokePosition(S, ChokePoint::end1);
			Position p2 = INFO.getFirstChokePosition(S, ChokePoint::end2);

			int dx = p1.x - p2.x;
			int dy = p1.y - p2.y;
			int min = p1.y * dy + p1.x * dx;
			int max = p2.y * dy + p2.x * dx;
			bool positive = dx * dy >= 0;

			if ((dy > 0 && min > max) || (dy < 0 && min < max))
				swap(min, max);

			bool isNotBlocked = false;
			int topLeft = 0, bottomRIght = 0, bottomLeft = 0, topRight = 0;

			do {
				desiredPosition = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Missile_Turret, (TilePosition)INFO.getFirstChokePosition(S), next, INFO.getMainBaseLocation(S)->Center());
				next = true;

			
				if (positive) {
					int topLeft = dy * desiredPosition.y * 32 + dx * desiredPosition.x * 32;
					int bottomRight = dy * (desiredPosition.y + Terran_Missile_Turret.tileHeight()) * 32 + dx * (desiredPosition.x + Terran_Missile_Turret.tileWidth()) * 32;

					isNotBlocked = dy > 0 ? bottomRight < min || topLeft > max : bottomRight > min || topLeft < max;
				}
				else {
					int bottomLeft = dy * (desiredPosition.y + Terran_Missile_Turret.tileHeight()) * 32 + dx * desiredPosition.x * 32;
					int topRight = dy * desiredPosition.y * 32 + dx * (desiredPosition.x + Terran_Missile_Turret.tileWidth()) * 32;

					isNotBlocked = dy > 0 ? bottomLeft < min || topRight > max : bottomLeft > min || topRight < max;
				}
			} while (!isNotBlocked && desiredPosition != TilePositions::None);
		}
		else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::SecondChokePoint) {
			desiredPosition = getTurretPositionInSecondChokePoint();
		}

		if (desiredPosition.isValid() && !ReserveBuilding::Instance().reserveTiles(desiredPosition, { buildingType }, 0, 0, 0, 0)) {
			cout << "Terran_Missile_Turret Reserve failed! " << desiredPosition << endl;
		}
	}
	else if (buildingType == Terran_Command_Center) {
		if (seedPosition != TilePositions::None) {
			ReserveBuilding::Instance().forceReserveTilesFirst(seedPosition, { Terran_Command_Center }, 0, 0, 0, 0);
			desiredPosition = seedPosition;
		}
	}
	else if (buildingType == Terran_Supply_Depot || buildingType == Terran_Academy || buildingType == Terran_Armory) {
		
		if (buildingType == Terran_Armory && seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)
			return TilePositions::None;
		
		else if (isFixedPositionOrder(buildingType))
			desiredPosition = supplyByBarracks;
		
		else if (INFO.getAllCount(Terran_Supply_Depot, S) + INFO.getAllCount(Terran_Academy, S) + INFO.getAllCount(Terran_Armory, S) >= 6) {
			int unbuildableTileCnt = 0;

			for (auto rb : ReserveBuilding::Instance().getReserveList()) {
				if (rb.canAssignToType(buildingType, true)) {
					TilePosition str = rb.TopLeft() - 1;
					TilePosition end = rb.BottomRight();
					int tmpCnt = 0;

					for (int dx = 0, size = buildingType.tileSize().x + 1; dx <= size; dx++) {
						TilePosition up = str + TilePosition(dx, 0);

						if (!up.isValid() || !bw->isBuildable(up, true))
							tmpCnt++;

						TilePosition down = end - TilePosition(dx, 0);

						if (!down.isValid() || !bw->isBuildable(down, true))
							tmpCnt++;
					}

					for (int dy = 1, size = buildingType.tileSize().y; dy <= size; dy++) {
						TilePosition left = str + TilePosition(0, dy);

						if (!left.isValid() || !bw->isBuildable(left, true))
							tmpCnt++;

						TilePosition right = end - TilePosition(0, dy);

						if (!right.isValid() || !bw->isBuildable(right, true))
							tmpCnt++;
					}

					
					if (unbuildableTileCnt < tmpCnt && tmpCnt < 14) {
						unbuildableTileCnt = tmpCnt;
						desiredPosition = rb.getTilePosition();
					}
					
					else if (unbuildableTileCnt == tmpCnt && tmpCnt > 0) {
						if (MYBASE.getApproxDistance((Position)rb.getTilePosition()) < MYBASE.getApproxDistance((Position)desiredPosition))
							desiredPosition = rb.getTilePosition();
					}
				}
			}

			
		}

		
	}

	return desiredPosition;
}

bool SelfBuildingPlaceFinder::isFixedPositionOrder(UnitType unitType) {
	if (unitType == Terran_Supply_Depot)
		return INFO.enemyRace == Races::Protoss && INFO.getAllCount(Terran_Supply_Depot, S) == 1 && BM.getBuildQueue()->getItemCount(Terran_Supply_Depot) + CM.getConstructionQueueItemCount(Terran_Supply_Depot) == 1;

	return false;
}

void SelfBuildingPlaceFinder::drawMap() {
	for (int i = 0; i < 25; i++) {
		Broodwar->drawBoxMap((Position)tilePos[i], (Position)(tilePos[i] + Terran_Supply_Depot.tileSize()), Colors::Blue);
		Broodwar->drawTextMap((Position)(tilePos[i] + (tilePos[i] + Terran_Supply_Depot.tileSize())) / 2, "%d", i + 1);
	}
}

TilePosition SelfBuildingPlaceFinder::getTurretPositionInSecondChokePoint()
{
	TilePosition mySecondChokePointBunker = BasicBuildStrategy::Instance().getSecondChokePointTurretPosition();

	if (!mySecondChokePointBunker.isValid())
		return TilePositions::None;


	if (INFO.getTypeBuildingsInRadius(Terran_Bunker, S, (Position)mySecondChokePointBunker, 5 * TILE_SIZE).size() == 0) {
		const vector<ConstructionTask> *constructionQueue = CM.getConstructionQueue();

		for (auto &c : *constructionQueue) {
			
			if (c.type == Terran_Bunker) {
				TilePosition desiredPosition = c.status == ConstructionStatus::Assigned ? c.finalPosition : c.desiredPosition;

				mySecondChokePointBunker = desiredPosition;
				break;
			}
		}
	}

	bool secondTurret = false;

	if (INFO.getAllCount(Terran_Missile_Turret, S) > 0 || CM.existConstructionQueueItem(Terran_Missile_Turret))
	{
		secondTurret = true;
	}

	TilePosition diff = mySecondChokePointBunker - INFO.getFirstExpansionLocation(S)->getTilePosition();
	TilePosition unit;
	TilePosition move = TilePosition(0, 0);
	int sign;

	if (INFO.enemyRace == Races::Protoss) {
		unit = abs(diff.x) > abs(diff.y) ? TilePosition(1, 0) : TilePosition(0, 1);

		if (abs(diff.x) > abs(diff.y))
			
			sign = diff.x < 0 ? 1 : -1;
		else
			
			sign = diff.y < 0 ? 1 : -1;
	}
	else if (INFO.enemyRace == Races::Zerg) {
		unit = abs(diff.x) > abs(diff.y) ? TilePosition(0, 1) : TilePosition(1, 0);
		sign = secondTurret ? -1 : 1;

		
		if (abs(diff.x) > abs(diff.y)) {
			
			if (INFO.getSecondChokePosition(S).isValid() && mySecondChokePointBunker.x * 32 > INFO.getSecondChokePosition(S).x)
				move = TilePosition(1, 0);
		}
	}
	else {
		unit = abs(diff.x) > abs(diff.y) ? TilePosition(0, 1) : TilePosition(1, 0);
		sign = secondTurret ? -1 : 1;
	}

	for (int i = 1; i < 10; i++)
	{
		TilePosition p = mySecondChokePointBunker + move + unit * i * sign;

		if (p.isValid() && bw->canBuildHere(p, Terran_Missile_Turret) && ReserveBuilding::Instance().canReserveHere(p, Terran_Missile_Turret, 0, 0, 0, 0))
			return p;
	}

	return BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Missile_Turret, mySecondChokePointBunker, false, INFO.getFirstExpansionLocation(S)->getPosition());
}


void SelfBuildingPlaceFinder::drawSecondChokePointReserveBuilding()
{
	if (INFO.getSecondChokePosition(S) == Positions::None)
		return;

	TilePosition closestTile1 = closestTileMap[INFO.getSecondChokePosition(S)].first;
	TilePosition closestTile2 = closestTileMap[INFO.getSecondChokePosition(S)].second;

	if (closestTile1.isValid())
		Broodwar->drawBoxMap((Position)closestTile1, (Position)(closestTile1 + 1), Colors::Green);

	if (closestTile2.isValid())
		Broodwar->drawBoxMap((Position)closestTile2, (Position)(closestTile2 + 1), Colors::Green);

	if (barracksPositionInSecondChokePoint.isValid())
	{
		Broodwar->drawBoxMap((Position)barracksPositionInSecondChokePoint, (Position)(barracksPositionInSecondChokePoint + TilePosition(Terran_Barracks.tileWidth(), Terran_Barracks.tileHeight())), Colors::White);
		Broodwar->drawLineMap((Position)barracksPositionInSecondChokePoint, (Position)(barracksPositionInSecondChokePoint + TilePosition(Terran_Barracks.tileWidth(), Terran_Barracks.tileHeight())), Colors::White);
		bw->drawTextMap(((Position)barracksPositionInSecondChokePoint).x + 10, ((Position)barracksPositionInSecondChokePoint).y + 10, "%cBarracks", Text::White);
	}

	if (engineeringBayPositionInSecondChokePoint.isValid())
	{
		Broodwar->drawBoxMap((Position)engineeringBayPositionInSecondChokePoint, (Position)(engineeringBayPositionInSecondChokePoint + TilePosition(Terran_Engineering_Bay.tileWidth(), Terran_Engineering_Bay.tileHeight())), Colors::White);
		Broodwar->drawLineMap((Position)engineeringBayPositionInSecondChokePoint, (Position)(engineeringBayPositionInSecondChokePoint + TilePosition(Terran_Engineering_Bay.tileWidth(), Terran_Engineering_Bay.tileHeight())), Colors::White);
		bw->drawTextMap(((Position)(engineeringBayPositionInSecondChokePoint)).x + 10, ((Position)engineeringBayPositionInSecondChokePoint).y + 10, "%cEngineering", Text::White);
	}

	for (auto supply : supplysPositionInSecondChokePoint)
	{
		if (supply.isValid())
		{
			Broodwar->drawBoxMap((Position)supply, (Position)(supply + TilePosition(Terran_Supply_Depot.tileWidth(), Terran_Supply_Depot.tileHeight())), Colors::White);
			Broodwar->drawLineMap((Position)supply, (Position)(supply + TilePosition(Terran_Supply_Depot.tileWidth(), Terran_Supply_Depot.tileHeight())), Colors::White);
			bw->drawTextMap(((Position)supply).x + 10, ((Position)supply).y + 10, "%cSupply", Text::White);
		}
	}

}

void SelfBuildingPlaceFinder::update()
{
	
	{
		bool needUpdate = false;

		if (!checkCanBuildHere(barracksPositionInSecondChokePoint, Terran_Barracks))
		{
			cout << "배럭때문에 업데이트" << endl;
			needUpdate = true;
		}
		else if (!checkCanBuildHere(engineeringBayPositionInSecondChokePoint, Terran_Engineering_Bay))
		{
			cout << TIME << "엔베때문에 업데이트" << endl;
			needUpdate = true;
		}

		

		if (needUpdate)
			setSecondChokePointReserveBuilding();
	}
}

TilePosition SelfBuildingPlaceFinder::getBarracksPositionInSCP()
{
	return barracksPositionInSecondChokePoint;
}

TilePosition SelfBuildingPlaceFinder::getEngineeringBayPositionInSCP()
{
	return engineeringBayPositionInSecondChokePoint;
}

TilePosition SelfBuildingPlaceFinder::getSupplysPositionInSCP()
{
	TilePosition supplyPosition = TilePositions::Unknown;

	for (auto s : supplysPositionInSecondChokePoint)
	{
		bool isExist = false;

		
		for (auto existSupply : INFO.getBuildings(Terran_Supply_Depot, S))
		{
			if (existSupply->unit()->getTilePosition() == s)
			{
				isExist = true;
				break;
			}
		}

		if (isExist)
		{
			cout << "이미 지어진 위치라 스킵" << endl;
			continue;
		}

		if (CM.existConstructionQueueItem(Terran_Supply_Depot))
		{
			const vector<ConstructionTask> *constructionQueue = CM.getConstructionQueue();

			
			isExist = false;

			for (const auto &b : *constructionQueue)
			{
				
				if (b.type == Terran_Supply_Depot && b.desiredPosition == s)
				{
					isExist = true;
					break;
				}
			}

			if (isExist)
			{
				cout << "CM 스킵" << endl;
				continue;
			}
		}

		
		if (bw->canBuildHere(s, Terran_Supply_Depot))
		{
			cout << "건설 가능한 위치 찾았다 - ";
			cout << s << endl;
			supplyPosition = s;
			break;
		}
	}

	return supplyPosition;
}

void SelfBuildingPlaceFinder::setSecondChokePointReserveBuilding()
{
	if (INFO.getSecondChokePosition(S) == Positions::None)
		return;

	vector<TilePosition> secondChokePointTiles;

	
	TilePosition closestTile1 = TilePositions::Unknown;
	TilePosition closestTile2 = TilePositions::Unknown;

	if (secondChokePointTilesMap.find(INFO.getSecondChokePosition(S)) == secondChokePointTilesMap.end()) {
		
		for (auto p : INFO.getSecondChokePoint(S)->Geometry())
		{
			if (!bw->isBuildable(TilePosition(p), true))
				continue;

			auto existTilePosition = find_if(secondChokePointTiles.begin(), secondChokePointTiles.end(), [p](TilePosition t) {
				return t == TilePosition(p);
			});

			if (existTilePosition == secondChokePointTiles.end())
			{
				secondChokePointTiles.push_back(TilePosition(p));
			}
		}

		secondChokePointTilesMap[INFO.getSecondChokePosition(S)] = secondChokePointTiles;

		if (!secondChokePointTiles.empty())
		{
			if (INFO.getFirstChokePoint(S)->Center().getApproxDistance(WalkPosition(*secondChokePointTiles.begin()))
					< INFO.getFirstChokePoint(S)->Center().getApproxDistance(WalkPosition(*secondChokePointTiles.rbegin())))
			{
				closestTile1 = TilePosition(*secondChokePointTiles.begin());
				closestTile2 = TilePosition(*secondChokePointTiles.rbegin());
			}
			else
			{
				closestTile1 = TilePosition(*secondChokePointTiles.rbegin());
				closestTile2 = TilePosition(*secondChokePointTiles.begin());
			}

			closestTileMap[INFO.getSecondChokePosition(S)] = make_pair(closestTile1, closestTile2);
		}
		else
		{
			
			return;
		}
	}
	else {
		if (closestTileMap.find(INFO.getSecondChokePosition(S)) == closestTileMap.end())
			return;

		secondChokePointTiles = secondChokePointTilesMap[INFO.getSecondChokePosition(S)];
		closestTile1 = closestTileMap[INFO.getSecondChokePosition(S)].first;
		closestTile2 = closestTileMap[INFO.getSecondChokePosition(S)].second;
	}

	bool isBarracksPlaced = false;
	bool isEngineeringBayPlaced = false;
	

	for (auto b : INFO.getBuildings(Terran_Barracks, S))
	{
		if (b->unit()->getTilePosition() == barracksPositionInSecondChokePoint && !b->getLift())
		{
			
			isBarracksPlaced = true;
			break;
		}
	}

	if (!isBarracksPlaced)
		barracksPositionInSecondChokePoint = TilePositions::Unknown;

	for (auto e : INFO.getBuildings(Terran_Engineering_Bay, S))
	{
		if (e->unit()->getTilePosition() == engineeringBayPositionInSecondChokePoint && !e->getLift())
		{
			isEngineeringBayPlaced = true;
			break;
		}
	}

	if (!isEngineeringBayPlaced)
		engineeringBayPositionInSecondChokePoint = TilePositions::Unknown;

	

	int x = closestTile1.x - closestTile2.x;
	int y = closestTile1.y - closestTile2.y;
	int magin = 0;

	
	if (abs(x) <= abs(y))
	{
	

		magin = abs(y) + 1;

		
		bool isCommandCenterLeftOfSCP = INFO.getFirstExpansionLocation(S)->Center().x < INFO.getSecondChokePosition(S).x;

		
		bool right = false;
	
		bool down = closestTile1.y < closestTile2.y;

		
		if (isCommandCenterLeftOfSCP)
			right = closestTile1.x < closestTile2.x;
		else
			right = closestTile1.x <= closestTile2.x;

		vector<TilePosition> checkPosition;

		
		checkPosition.push_back(closestTile1);

		while (true)
		{
			
			if (magin < 2 || checkPosition.empty())
			{
				break;
			}

			
			TilePosition topLeft = TilePositions::Unknown;
			UnitType buildingType = UnitTypes::Unknown;

			if (!isBarracksPlaced || !isEngineeringBayPlaced)
			{
				for (auto basePosition : checkPosition)
				{
					if (right)
					{
						

						for (int i = Terran_Barracks.tileWidth() - 1; i >= 0; i--)
						{
							TilePosition tempPosition = basePosition - TilePosition(i, down ? 0 : (Terran_Barracks.tileHeight() - 1));

							if (!tempPosition.isValid())
							{
								continue;
							}

							if (!bw->canBuildHere(tempPosition, Terran_Barracks))
							{
								continue;
							}

							topLeft = tempPosition;
							buildingType = !isBarracksPlaced ? Terran_Barracks : (!isEngineeringBayPlaced ? Terran_Engineering_Bay : UnitTypes::Unknown);

						
							if (tempPosition.x == closestTile2.x)
								break;
							else if (tempPosition.x > closestTile2.x)
								break;
							else
								continue;
						}
					}
					else
					{
						

						for (int i = 0; i <= Terran_Barracks.tileWidth() - 1; i++)
						{
							TilePosition tempPosition = basePosition - TilePosition(i, down ? 0 : (Terran_Barracks.tileHeight() - 1));

							if (!tempPosition.isValid())
							{
								continue;
							}

							if (!bw->canBuildHere(tempPosition, Terran_Barracks))
							{
								continue;
							}

							topLeft = tempPosition;
							buildingType = !isBarracksPlaced ? Terran_Barracks : (!isEngineeringBayPlaced ? Terran_Engineering_Bay : UnitTypes::Unknown);

							
							if (tempPosition.x == closestTile2.x)
								break;
							else if (tempPosition.x < closestTile2.x)
								break;
							else
								continue;
						}
					}
				}
			}

			if (topLeft != TilePositions::Unknown && buildingType != UnitTypes::Unknown)
			{
		
				magin -= buildingType.tileHeight();
			}
			else
			{
				
				for (auto basePosition : checkPosition)
				{
					if (right)
					{
					
						for (int i = Terran_Supply_Depot.tileWidth() - 1; i >= 0; i--)
						{
							TilePosition tempPosition = basePosition - TilePosition(i, down ? 0 : (Terran_Supply_Depot.tileHeight() - 1));

							if (!tempPosition.isValid())
							{
								continue;
							}

							if (!bw->canBuildHere(tempPosition, Terran_Supply_Depot))
							{
								continue;
							}

							topLeft = tempPosition;
							buildingType = Terran_Supply_Depot;

						
							if (tempPosition.x == closestTile2.x)
								break;
							else if (tempPosition.x > closestTile2.x)
								break;
							else
								continue;
						}
					}
					else
					{
						
						for (int i = 0; i <= Terran_Supply_Depot.tileWidth() - 1; i++)
						{
							TilePosition tempPosition = basePosition - TilePosition(i, down ? 0 : (Terran_Supply_Depot.tileHeight() - 1));

							if (!tempPosition.isValid())
							{
								continue;
							}

							if (!bw->canBuildHere(tempPosition, Terran_Supply_Depot))
							{
								continue;
							}

							topLeft = tempPosition;
							buildingType = Terran_Supply_Depot;

							
							if (tempPosition.x == closestTile2.x)
								break;
							else if (tempPosition.x < closestTile2.x)
								break;
							else
								continue;
						}
					}
				}

				if (topLeft != TilePositions::Unknown && buildingType == Terran_Supply_Depot)
				{
					
					magin -= buildingType.tileHeight();
				}
			}

			if (!checkPosition.empty())
				checkPosition.clear();

			if (topLeft == TilePositions::Unknown)
			{
				

				topLeft = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Barracks, closestTile1, false);

				if (topLeft == TilePositions::None)
				{
					topLeft = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Supply_Depot, closestTile1, false);

					if (topLeft != TilePositions::None)
					{
						buildingType = Terran_Supply_Depot;
					}
					else
					{
						
						return;
					}
				}
				else
				{
					buildingType = !isBarracksPlaced ? Terran_Barracks : (!isEngineeringBayPlaced ? Terran_Engineering_Bay : UnitTypes::Unknown);
				}
			}

			if (topLeft != TilePositions::Unknown)
			{
				

				if (buildingType == Terran_Barracks)
				{
					isBarracksPlaced = true;
					barracksPositionInSecondChokePoint = topLeft;
				}
				else if (buildingType == Terran_Engineering_Bay)
				{
					isEngineeringBayPlaced = true;
					engineeringBayPositionInSecondChokePoint = topLeft;
				}
				else
				{
					if (supplysPositionInSecondChokePoint.size() < 2)
						supplysPositionInSecondChokePoint.push_back(topLeft);
				}

				int x = right ? topLeft.x : topLeft.x + buildingType.tileWidth() - 1;
				int y = down ? topLeft.y + buildingType.tileHeight() : topLeft.y - 1;

				for (int i = 0; i < buildingType.tileWidth(); i++)
				{
					checkPosition.push_back(TilePosition(x + (right ? 1 : -1) * i, y));
				}

				
			}
		}
	}
	else
	{
		
		magin = abs(x) + 1;

		
		bool upC = false;

		
		if (y < 0)
		{
			
			upC = false;
		}
		else
		{
			
			upC = true;
		}

		
		bool right = false;
		bool down = false;

	
		if (upC)
		{
			if (closestTile1.y >= closestTile2.y)
			{
				down = false;
			}
			else
			{
				down = true;
			}
		}
		else
		{
			if (closestTile1.y <= closestTile2.y)
			{
				down = true;
			}
			else
			{
				down = false;
			}
		}

		
		if (closestTile1.x < closestTile2.x)
		{
			right = true;
		}

		vector<TilePosition> checkPosition;

		
		checkPosition.push_back(closestTile1);

		while (true)
		{
			
			if (magin < 3 || checkPosition.empty())
			{
				break;
			}

			TilePosition topLeft = TilePositions::Unknown;
			UnitType buildingType = UnitTypes::Unknown;

			if (!isBarracksPlaced || !isEngineeringBayPlaced)
			{
				for (auto basePosition : checkPosition)
				{
					if (down)
					{
						

						for (int i = Terran_Barracks.tileHeight() - 1; i >= 0; i--)
						{
							TilePosition tempPosition = basePosition - TilePosition(right ? 0 : (Terran_Barracks.tileWidth() - 1), i);

							if (!tempPosition.isValid())
							{
								continue;
							}

							if (!bw->canBuildHere(tempPosition, Terran_Barracks))
							{
								continue;
							}

							topLeft = tempPosition;
							buildingType = !isBarracksPlaced ? Terran_Barracks : (!isEngineeringBayPlaced ? Terran_Engineering_Bay : UnitTypes::Unknown);

						
							if (tempPosition.y == closestTile2.y)
								break;
							else if (tempPosition.y > closestTile2.y)
								break;
							else
								continue;
						}
					}
					else
					{
						

						for (int i = 0; i <= Terran_Barracks.tileHeight() - 1; i++)
						{
							TilePosition tempPosition = basePosition - TilePosition(right ? 0 : (Terran_Barracks.tileWidth() - 1), i);

							if (!tempPosition.isValid())
							{
								continue;
							}

							if (!bw->canBuildHere(tempPosition, Terran_Barracks))
							{
								continue;
							}

							topLeft = tempPosition;
							buildingType = !isBarracksPlaced ? Terran_Barracks : (!isEngineeringBayPlaced ? Terran_Engineering_Bay : UnitTypes::Unknown);

						
							if (tempPosition.y == closestTile2.y)
								break;
							else if (tempPosition.y < closestTile2.y)
								break;
							else
								continue;
						}
					}
				}
			}

			if (topLeft != TilePositions::Unknown && buildingType != UnitTypes::Unknown)
			{
			
				magin -= buildingType.tileWidth();
			}
			else
			{
				
				for (auto basePosition : checkPosition)
				{
					if (down)
					{
						
						for (int i = Terran_Supply_Depot.tileHeight() - 1; i >= 0; i--)
						{
							TilePosition tempPosition = basePosition - TilePosition(right ? 0 : (Terran_Barracks.tileWidth() - 1), i);

							if (!tempPosition.isValid())
							{
								continue;
							}

							if (!bw->canBuildHere(tempPosition, Terran_Supply_Depot))
							{
								continue;
							}

							topLeft = tempPosition;
							buildingType = Terran_Supply_Depot;

							
							if (tempPosition.y == closestTile2.y)
								break;
							else if (tempPosition.y > closestTile2.y)
								break;
							else
								continue;
						}
					}
					else
					{
						
						for (int i = 0; i <= Terran_Supply_Depot.tileHeight() - 1; i++)
						{
							TilePosition tempPosition = basePosition - TilePosition(right ? 0 : (Terran_Barracks.tileWidth() - 1), i);

							if (!tempPosition.isValid())
							{
								continue;
							}

							if (!bw->canBuildHere(tempPosition, Terran_Supply_Depot))
							{
								continue;
							}

							topLeft = tempPosition;
							buildingType = Terran_Supply_Depot;

						
							if (tempPosition.y == closestTile2.y)
								break;
							else if (tempPosition.y < closestTile2.y)
								break;
							else
								continue;
						}
					}
				}

				if (topLeft != TilePositions::Unknown && buildingType == Terran_Supply_Depot)
				{
					
					magin -= buildingType.tileWidth();
				}
			}

			if (!checkPosition.empty())
				checkPosition.clear();

			if (topLeft == TilePositions::Unknown)
			{
				

				topLeft = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Barracks, closestTile1, false);

				if (topLeft == TilePositions::None)
				{
					topLeft = BuildingPlaceFinder::Instance().getBuildLocationBySpiralSearch(Terran_Supply_Depot, closestTile1, false);

					if (topLeft != TilePositions::None)
					{
						buildingType = Terran_Supply_Depot;
					}
					else
					{
						
						return;
					}
				}
				else
				{
					buildingType = !isBarracksPlaced ? Terran_Barracks : (!isEngineeringBayPlaced ? Terran_Engineering_Bay : UnitTypes::Unknown);
				}
			}

			if (topLeft != TilePositions::Unknown)
			{
				

				if (buildingType == Terran_Barracks)
				{
					isBarracksPlaced = true;
					barracksPositionInSecondChokePoint = topLeft;
				}
				else if (buildingType == Terran_Engineering_Bay)
				{
					isEngineeringBayPlaced = true;
					engineeringBayPositionInSecondChokePoint = topLeft;
				}
				else
				{
					if (supplysPositionInSecondChokePoint.size() < 2)
						supplysPositionInSecondChokePoint.push_back(topLeft);
				}

				
				int x = right ? topLeft.x + buildingType.tileWidth() : topLeft.x - 1;
				int y = down ? topLeft.y : topLeft.y + (buildingType.tileHeight() - 1);

				for (int i = 0; i < buildingType.tileHeight(); i++)
				{
					checkPosition.push_back(TilePosition(x, y + (down ? 1 : -1) * i));
				}

			}
		}
	}

	TM.getClosestPosition();
}

bool SelfBuildingPlaceFinder::checkCanBuildHere(TilePosition tilePosition, UnitType buildingType)
{
	
	if (!tilePosition.isValid())
		return true;

	uList buildings = INFO.getBuildings(buildingType, S);

	for (auto b : buildings) {
		
		if ((b->unit()->canLift() || !b->unit()->isFlying()) && b->unit()->getTilePosition() == tilePosition)
			return true;
	
		else if (b->unit()->canLand() && b->unit()->getLastCommand().getType() == UnitCommandTypes::Land && b->unit()->getLastCommand().getTargetTilePosition() == tilePosition)
			if (b->unit()->getLastCommandFrame() + 72 > TIME || b->unit()->land(tilePosition))
				return true;
	}

	
	TilePosition bottomRight = tilePosition + buildingType.tileSize();

	for (int x = tilePosition.x; x < bottomRight.x; x++)
		for (int y = tilePosition.y; y < bottomRight.y; y++)
			if (!bw->isBuildable(x, y, true))
				return false;

	return true;
}
