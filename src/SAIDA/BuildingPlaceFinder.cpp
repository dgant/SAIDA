#include "BuildingPlaceFinder.h"
#include "../UnitManager/SoldierManager.h"

using namespace MyBot;

BuildingPlaceFinder::BuildingPlaceFinder()
{
	setTilesToAvoid();

	for (int dy = 0; dy <= 15; ++dy)
		for (int dx = dy; dx <= 15; ++dx)			// Only consider 1/8 of possible deltas. Other ones obtained by symmetry.
			if (dx || dy)
				DeltasByAscendingAltitude.emplace_back(TilePosition(dx, dy), altitude_t(0.5 + norm(dx, dy) * 32));

	sort(DeltasByAscendingAltitude.begin(), DeltasByAscendingAltitude.end(),
	[](const pair<TilePosition, altitude_t> &a, const pair<TilePosition, altitude_t> &b) {
		return a.second < b.second;
	});
}

BuildingPlaceFinder &BuildingPlaceFinder::Instance()
{
	static BuildingPlaceFinder instance;
	return instance;
}

TilePosition BuildingPlaceFinder::getSeedPositionFromSeedLocationStrategy(BuildOrderItem::SeedPositionStrategy seedPositionStrategy) const {
	TilePosition seedPosition = TilePositions::None;
	Position tempChokePoint;
	const BWEM::Base *tempBaseLocation;
	TilePosition tempTilePosition;

	switch (seedPositionStrategy) {

		case BuildOrderItem::SeedPositionStrategy::MainBaseLocation:
			seedPosition = INFO.getMainBaseLocation(S)->getTilePosition();
			break;

		case BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation:
			tempBaseLocation = INFO.getFirstExpansionLocation(S);

			if (tempBaseLocation) {
				seedPosition = tempBaseLocation->Location();
			}

			break;

		case BuildOrderItem::SeedPositionStrategy::FirstChokePoint:
			tempChokePoint = INFO.getFirstChokePosition(S);

			if (tempChokePoint) {
				seedPosition = (TilePosition)tempChokePoint;
			}

			break;

		case BuildOrderItem::SeedPositionStrategy::SecondChokePoint:
			tempChokePoint = INFO.getSecondChokePosition(S);

			if (tempChokePoint) {
				seedPosition = (TilePosition)tempChokePoint;
			}

			break;

		case BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation:
			tempBaseLocation = INFO.getSecondExpansionLocation(S) == nullptr ? INFO.enemyRace == Races::Terran ? INFO.getFirstMulti(S, true) : INFO.getFirstMulti(S) : INFO.getSecondExpansionLocation(S);

			if (tempBaseLocation) {
				seedPosition = tempBaseLocation->Location();
			}

			break;

		case BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation:
			tempBaseLocation = INFO.getThirdExpansionLocation(S) == nullptr ? INFO.enemyRace == Races::Terran ? INFO.getFirstMulti(S, true) : INFO.getFirstMulti(S) : INFO.getThirdExpansionLocation(S);

			if (tempBaseLocation) {
				seedPosition = tempBaseLocation->Location();
			}

			break;

		case BuildOrderItem::SeedPositionStrategy::IslandExpansionLocation:
			tempBaseLocation = INFO.getIslandExpansionLocation(S);

			if (tempBaseLocation) {
				seedPosition = tempBaseLocation->Location();
			}

			break;
	}

	return seedPosition;
}

TilePosition	BuildingPlaceFinder::getBuildLocationWithSeedPositionAndStrategy(UnitType buildingType, TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy) const
{

	TilePosition desiredPosition = TilePositions::None;

	if (buildingType.isRefinery()) {
		return seedPosition;
	}

	if (INFO.selfRace == Races::Terran) {
		desiredPosition = SelfBuildingPlaceFinder::Instance().getBuildLocationWithSeedPositionAndStrategy(buildingType, seedPosition, seedPositionStrategy);
	}

	if (desiredPosition == TilePositions::None && !(buildingType == Terran_Armory && seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)) {
		desiredPosition = ReserveBuilding::Instance().getReservedPosition(buildingType);
	}

	if (desiredPosition != TilePositions::None) {
		return desiredPosition;
	}

	if (!seedPosition.isValid()) {
		seedPosition = getSeedPositionFromSeedLocationStrategy(seedPositionStrategy);
	}

	if (seedPosition.isValid()) {
		UnitInfo *closestScv = SoldierManager::Instance().chooseScvClosestTo((Position)seedPosition);

		if (closestScv != nullptr) {
			desiredPosition = getBuildLocationNear(buildingType, seedPosition, true, closestScv->unit());
		}
		else {
			desiredPosition = getBuildLocationNear(buildingType, seedPosition, true);
		}

		if (!desiredPosition.isValid() || !ReserveBuilding::Instance().reserveTiles(desiredPosition, { buildingType }, 0, 0, 0, 0, buildingType != Terran_Factory)) {
			cout << "[" << TIME << "] Reserve failed! " << buildingType << desiredPosition << endl;

			ReserveBuilding::Instance().debugCanReserveHere(desiredPosition, buildingType, buildingType.tileWidth(), buildingType.tileHeight(), 0, 0, 0, 0, buildingType.canBuildAddon());
		}
	}

	return desiredPosition;

}
/*
void BuildingPlaceFinder::computeResourceBox()
{
BWAPI::Position start(InformationManager::Instance().getMyMainBaseLocation()->getPosition());
BWAPI::Unitset unitsAroundNexus;

for (const auto unit : BWAPI::Broodwar->getAllUnits())
{
// if the units are less than 400 away add them if they are resources
if (unit->getDistance(start) < 300 && unit->getType().isMineralField())
{
unitsAroundNexus.insert(unit);
}
}

for (const auto unit : unitsAroundNexus)
{
int x = unit->getPosition().x;
int y = unit->getPosition().y;

int left = x - unit->getType().dimensionLeft();
int right = x + unit->getType().dimensionRight() + 1;
int top = y - unit->getType().dimensionUp();
int bottom = y + unit->getType().dimensionDown() + 1;

_boxTop     = top < _boxTop       ? top    : _boxTop;
_boxBottom  = bottom > _boxBottom ? bottom : _boxBottom;
_boxLeft    = left < _boxLeft     ? left   : _boxLeft;
_boxRight   = right > _boxRight   ? right  : _boxRight;
}

//BWAPI::Broodwar->printf("%d %d %d %d", boxTop, boxBottom, boxLeft, boxRight);
}

// makes final checks to see if a building can be built at a certain location
bool BuildingPlaceFinder::canBuildHere(BWAPI::TilePosition position,const Building & b) const
{

if (!BWAPI::Broodwar->canBuildHere(position, b.type, b.builderUnit))
{
	return false;
}

// check the reserve map
for (int x = position.x; x < position.x + b.type.tileWidth(); x++)
{
	for (int y = position.y; y < position.y + b.type.tileHeight(); y++)
	{
		if (_reserveMap[x][y])
		{
			return false;
		}
	}
}

// if it overlaps a base location return false
if (tileOverlapsBaseLocation(position, b.type))
{
	return false;
}

return true;
}

bool BuildingPlaceFinder::tileBlocksAddon(BWAPI::TilePosition position) const
{

	for (int i = 0; i <= 2; ++i)
	{
		for (auto unit : BWAPI::Broodwar->getUnitsOnTile(position.x - i, position.y))
		{
			if (unit->getType() == BWAPI::UnitTypes::Terran_Command_Center ||
				unit->getType() == BWAPI::UnitTypes::Terran_Factory ||
				unit->getType() == BWAPI::UnitTypes::Terran_Starport ||
				unit->getType() == BWAPI::UnitTypes::Terran_Science_Facility)
			{
				return true;
			}
		}
	}

	return false;
}
*/
TilePosition	BuildingPlaceFinder::getBuildLocationNear(UnitType buildingType, TilePosition desiredPosition, bool checkForReserve, Unit worker) const
{
	if (buildingType.isRefinery())
	{

		return desiredPosition;
	}

	if (Broodwar->self()->getRace() == Races::Protoss) {
		if (buildingType.requiresPsi() && Broodwar->self()->completedUnitCount(Protoss_Pylon) == 0)
		{
			return TilePositions::None;
		}
	}

	if (!desiredPosition.isValid())
	{
		desiredPosition = InformationManager::Instance().getMainBaseLocation(S)->getTilePosition();
	}


	TilePosition resultPosition = TilePositions::None;
	ConstructionTask b(buildingType, desiredPosition);
	b.constructionWorker = worker;

	int maxRange = 32; // maxRange = Broodwar->mapWidth()/4;

	int currentX = desiredPosition.x;
	int currentY = desiredPosition.y;
	int spiralMaxLength = 1;
	int numSteps = 0;
	bool isFirstStep = true;

	int spiralDirectionX = 0;
	int spiralDirectionY = 1;

	while (spiralMaxLength < maxRange)
	{
		TilePosition buildPos = TilePosition(currentX, currentY);

		if (buildPos.isValid() && isSameArea(desiredPosition, buildPos) && bw->isBuildable(buildPos, true)) {
			if (canBuildHere(buildPos, b.type, checkForReserve, b.constructionWorker)) {
				resultPosition = buildPos;
				break;
			}
		}

		currentX = currentX + spiralDirectionX;
		currentY = currentY + spiralDirectionY;
		numSteps++;

		if (numSteps == spiralMaxLength)
		{
			numSteps = 0;

			if (!isFirstStep)
				spiralMaxLength++;

			isFirstStep = !isFirstStep;

			if (spiralDirectionX == 0)
			{
				spiralDirectionX = spiralDirectionY;
				spiralDirectionY = 0;
			}
			else
			{
				spiralDirectionY = -spiralDirectionX;
				spiralDirectionX = 0;
			}
		}
	}

	return resultPosition;
}

TilePosition BuildingPlaceFinder::getBuildLocationBySpiralSearch(UnitType unitType, TilePosition possiblePosition, bool next, Position possibleArea, bool checkOnlyBuilding) {
	if (!possiblePosition.isValid())
		return TilePositions::None;

	if (next) {
		deltaIdx++;
	}
	else {
		altitudeIdx = 0;
		deltaIdx = 0;

		TilePosition w = possiblePosition - unitType.tileSize() / 2;

		if (w.isValid() && bw->isBuildable(w, true) && !isBasePosition(w, unitType)) {
			if (possibleArea == Positions::None || isSameArea(w, (TilePosition)possibleArea)) {
				if (checkOnlyBuilding) {
					bool canBuild = true;

					for (int x = 0; x < unitType.tileWidth() && canBuild; x++) {
						for (int y = 0; y < unitType.tileHeight(); y++) {
							if (!bw->isBuildable(w + TilePosition(x, y), true)) {
								canBuild = false;
								break;
							}
						}
					}

					if (canBuild)
						return w;
				}
				else {
					if (bw->canBuildHere(w, unitType)) {
						return w;
					}
				}
			}
		}
	}

	for (; altitudeIdx < DeltasByAscendingAltitude.size(); altitudeIdx++) {
		pair<TilePosition, altitude_t> &delta_altitude = DeltasByAscendingAltitude.at(altitudeIdx);
		const TilePosition d = delta_altitude.first;

		vector<TilePosition> deltas = { TilePosition(d.x, d.y), TilePosition(-d.x, d.y), TilePosition(d.x, -d.y), TilePosition(-d.x, -d.y),
										TilePosition(d.y, d.x), TilePosition(-d.y, d.x), TilePosition(d.y, -d.x), TilePosition(-d.y, -d.x)
									  };

		for (; deltaIdx < deltas.size(); deltaIdx++) {
			TilePosition w = possiblePosition + deltas.at(deltaIdx) - unitType.tileSize() / 2;

			if (w.isValid() && bw->isBuildable(w, true))
			{
				if (possibleArea != Positions::None) {
					if (!isSameArea(w, (TilePosition)possibleArea))
						continue;
				}

				if (bw->canBuildHere(w, unitType) && !isBasePosition(w, unitType)) {
					return w;
				}
			}
		}

		deltaIdx = 0;
	}

	return TilePositions::None;
}

bool BuildingPlaceFinder::canBuildHere(BWAPI::TilePosition position, const UnitType type, const bool checkForReserve, const Unit worker, int topSpace, int leftSpace, int rightSpace, int bottomSpace) const
{
	if (!Broodwar->canBuildHere(position, type, worker))
	{

		if (type != Terran_Missile_Turret && type != Terran_Bunker)
			return false;

		
		Unitset us = bw->getUnitsInRectangle(((Position)position) + Position(1, 1), (Position)(position + type.tileSize()) - Position(1, 1));

		if (us.empty())
			return false;

		for (auto u : us) {
			if (!u->getType().isWorker())
				return false;
		}
	}

	
	if (isBasePosition(position, type))
		return false;

	bool useTypeAddon = true;
	bool checkAddon = false;

	
	if (type == Terran_Factory && topSpace == 0 && leftSpace == 0 && rightSpace == 0 && bottomSpace == 0) {
		topSpace = 1;
		leftSpace = 1;
		rightSpace = 1;
		bottomSpace = 1;
		useTypeAddon = false;
		checkAddon = false;
	}
	else if (type == Terran_Armory) {
		topSpace = 1;
		leftSpace = 1;
		rightSpace = 1;
		bottomSpace = 1;
	}


	if (checkForReserve && !ReserveBuilding::Instance().canReserveHere(position, type, topSpace, leftSpace, rightSpace, bottomSpace, useTypeAddon, checkAddon)) {


		return false;
	}

	else if (!checkForReserve && !ReserveBuilding::Instance().isReservedFor(position, type)) {


		return false;
	}

	return true;
}

bool BuildingPlaceFinder::isBasePosition(TilePosition position, UnitType type) const {
	if (type.isResourceDepot())
		return false;

	vector<Base> bases = theMap.GetArea(position)->Bases();

	for (auto &base : bases) {
		int minX = base.getTilePosition().x;
		int minY = base.getTilePosition().y;
		int maxX = base.getTilePosition().x + Terran_Command_Center.tileSize().x;
		int maxY = base.getTilePosition().y + Terran_Command_Center.tileSize().y;

		int buildingMinX = position.x;
		int buildingMinY = position.y;
		int buildingMaxX = position.x + type.tileSize().x;
		int buildingMaxY = position.y + type.tileSize().y;

	
		if (minX < buildingMaxX && buildingMinX < maxX && minY < buildingMaxY && buildingMinY < maxY)
			return true;

		if (INFO.selfRace == Races::Terran) {
		
			if (type.canBuildAddon()) {
				int addonMinX = position.x + 4;
				int addonMinY = position.y;
				int addonMaxX = addonMinX + 2;
				int addonMaxY = addonMinY + 3;

			
				if (minX < addonMaxX && addonMinX < maxX && minY < addonMaxY && addonMinY < maxY)
					return true;
			}

			
			minX += Terran_Command_Center.tileSize().x;
			maxX += Terran_Comsat_Station.tileSize().x;

			if (type == Terran_Missile_Turret)
				minY++;

			if (minX < buildingMaxX && buildingMinX < maxX && minY < buildingMaxY && buildingMinY < maxY)
				return true;
		}
	}

	return false;
}

TilePosition BuildingPlaceFinder::getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy seedPositionStrategy) const
{
	const Base *base = nullptr;

	if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::MainBaseLocation) {
		base = INFO.getMainBaseLocation(S);
	}
	else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation) {
		base = INFO.getFirstExpansionLocation(S);
	}
	else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation) {
		base = INFO.getSecondExpansionLocation(S);
	}
	else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation) {
		base = INFO.getThirdExpansionLocation(S);
	}
	else if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::IslandExpansionLocation) {
		base = INFO.getIslandExpansionLocation(S);
	}

	if (base)
		for (auto &geyser : base->Geysers())
			for (auto unit : bw->getUnitsOnTile(geyser->Unit()->getInitialTilePosition()))
				if (unit->getType() == Resource_Vespene_Geyser)
					return unit->getTilePosition();

	return TilePositions::None;
}

TilePosition BuildingPlaceFinder::getRefineryPositionNear(Base *base) const
{
	if (base != nullptr)
		for (auto &geyser : base->Geysers())
			for (auto unit : bw->getUnitsOnTile(geyser->Unit()->getTilePosition()))
				if (unit->getType() == Resource_Vespene_Geyser)
					return unit->getTilePosition();

	return TilePositions::None;
}

void BuildingPlaceFinder::setTilesToAvoid()
{
	for (const Area &area : theMap.Areas()) {
		
		if (area.ChokePoints().empty()) continue;

		for (const Base &base : area.Bases()) {
		
			int bx0 = base.Location().x;
			int by0 = base.Location().y;
			int bx4 = base.Location().x + 4;
			int by3 = base.Location().y + 3;

		
			int cx0 = bx4;
			int cy0 = by0 + 1;
			int cx2 = cx0 + 2;
			int cy2 = by3;

			vector <Line> lines;

			Box box;

			box.top = by0;
			box.left = bx0;
			box.right = cx2;
			box.bottom = by3;

			for (Mineral *mineral : base.Minerals()) {
				
				lines.push_back(getLine(base.Center(), mineral->Pos()));

			
				getBox(box, mineral->TopLeft(), mineral->TopLeft() + mineral->Size());
			}

			for (Geyser *geyser : base.Geysers()) {
				
				TilePosition pos(geyser->BottomRight().x + 1, geyser->TopLeft().y);

				for (int y = 0; y <= geyser->Type().tileHeight(); y++) {
					TilePosition rightOfGeyser = pos + TilePosition(0, y);

					if (ReserveBuilding::Instance().canReserveHere(rightOfGeyser, TilePosition(1, 1), 0, 0, 0, 0)) {
						ReserveBuilding::Instance().reserveTiles(rightOfGeyser, TilePosition(1, 1), 0, 0, 0, 0, ReserveTypes::GAS);
					}
				}

				pos = TilePosition(geyser->TopLeft().x, geyser->BottomRight().y + 1);

				for (int y = 0; y < Terran_Bunker.tileHeight(); y++) {
					for (int x = 0; x < Terran_Bunker.tileWidth(); x++) {
						TilePosition rightOfGeyser = pos + TilePosition(x, y);

						if (ReserveBuilding::Instance().canReserveHere(rightOfGeyser, TilePosition(1, 1), 0, 0, 0, 0)) {
							ReserveBuilding::Instance().reserveTiles(rightOfGeyser, TilePosition(1, 1), 0, 0, 0, 0, ReserveTypes::GAS);
						}
					}
				}

				Position rightOfGas = geyser->Pos() + Position(48, 0);

				
				if (abs(base.Center().x - rightOfGas.x) / TILE_SIZE < 2) {
					lines.push_back(getLine(Position(rightOfGas.x, base.Center().y), rightOfGas));
				}
				else {
				
					lines.push_back(getLine(base.Center(), geyser->Pos()));

					
					lines.push_back(getLine(base.Center() + Position(48, 0), rightOfGas));
				}


				
				getBox(box, geyser->TopLeft(), geyser->TopLeft() + geyser->Size());
			}

			for (int x = box.left; x < box.right; x++) {
				for (int y = box.top; y < box.bottom; y++) {
					TilePosition t1(x, y);

					
					if (!(t1.x < bx0 || bx4 <= t1.x || t1.y < by0 || by3 <= t1.y) || !(t1.x < cx0 || cx2 <= t1.x || t1.y < cy0 || cy2 <= t1.y)) {
						continue;
					}

					if (!ReserveBuilding::Instance().canReserveHere(t1, TilePosition(1, 1), 0, 0, 0, 0)) {
						continue;
					}

					Position p1(x * 32, y * 32);
					Position p2((x + 1) * 32, y * 32);
					Position p3(x * 32, (y + 1) * 32);
					Position p4((x + 1) * 32, (y + 1) * 32);

					bool isAvoidTile = false;

					for (Line &line : lines) {
		
						if (line.p1.x == line.p2.x) {
							if (line.p1.y > p1.y || line.p2.y < p1.y) {
								continue;
							}
						}
						else if (line.p1.y == line.p2.y) {
							if (line.p1.x > p1.x || line.p2.x < p1.x) {
								continue;
							}
						}
						else {
							if (line.p1.x > p1.x || line.p2.x < p1.x || line.p1.y > p1.y || line.p2.y < p1.y) {
								continue;
							}
						}

						
						int tmpY1 = line.dx * p1.y;
						int tmpX1 = line.dy * p1.x + line.cdx;

						int tmpY2 = line.dx * p2.y;
						int tmpX2 = line.dy * p2.x + line.cdx;

						int tmpY3 = line.dx * p3.y;
						int tmpX3 = line.dy * p3.x + line.cdx;

						int tmpY4 = line.dx * p4.y;
						int tmpX4 = line.dy * p4.x + line.cdx;

				
						if (line.dx == 0 && ((line.p1.x == p1.x && line.p1.x == p3.x) || (line.p1.x == p2.x && line.p1.x == p4.x))) {
							isAvoidTile = true;
							break;
						}
						
						else if (line.dy == 0 && ((line.p1.y == p1.y && line.p1.y == p2.y) || (line.p1.y == p3.y && line.p1.y == p4.y))) {
							isAvoidTile = true;
							break;
						}
					
						else if (tmpY1 == tmpX1 || tmpY2 == tmpX2 || tmpY3 == tmpX3 || tmpY4 == tmpX4) {
							isAvoidTile = true;
							break;
						}
						
						else if ((tmpY1 > tmpX1 && tmpY2 < tmpX2) || (tmpY1 < tmpX1 && tmpY2 > tmpX2)) {
							isAvoidTile = true;
							break;
						}
						
						else if ((tmpY1 > tmpX1 && tmpY3 < tmpX3) || (tmpY1 < tmpX1 && tmpY3 > tmpX3)) {
							isAvoidTile = true;
							break;
						}
				
						else if ((tmpY4 > tmpX4 && tmpY2 < tmpX2) || (tmpY4 < tmpX4 && tmpY2 > tmpX2)) {
							isAvoidTile = true;
							break;
						}
					}

					if (isAvoidTile) {
						bool isExistUnit = false;

						for (Unit u : Broodwar->getUnitsOnTile(t1)) {
							TilePosition topLeft = u->getTilePosition();
							TilePosition bottomRight = u->getTilePosition() + u->getType().tileSize();

							if (topLeft.y <= t1.y && topLeft.x <= t1.x && bottomRight.x > t1.x && bottomRight.y > t1.y) {
								isExistUnit = true;
								break;
							}
						}

						if (!isExistUnit) {
							ReserveBuilding::Instance().reserveTiles(t1, TilePosition(1, 1), 0, 0, 0, 0, ReserveTypes::MINERALS);
						}
					}
				}
			}
		}
	}
}

Line BuildingPlaceFinder::getLine(const Position &p1, const Position &p2) {
	Line line;

	line.dx = p1.x - p2.x;
	line.dy = p1.y - p2.y;
	line.cdx = p1.y * line.dx - line.dy * p1.x;

	if (p1.x < p2.x) {
		line.p1.x = p1.x;
		line.p2.x = p2.x;
	}
	else {
		line.p1.x = p2.x;
		line.p2.x = p1.x;
	}

	if (p1.y < p2.y) {
		line.p1.y = p1.y;
		line.p2.y = p2.y;
	}
	else {
		line.p1.y = p2.y;
		line.p2.y = p1.y;
	}

	return line;
}

void BuildingPlaceFinder::getBox(Box &box, const TilePosition &topLeft, const TilePosition &bottomRight) {
	if (box.left > topLeft.x) {
		box.left = topLeft.x;
	}

	if (box.top > topLeft.y) {
		box.top = topLeft.y;
	}

	if (box.right < bottomRight.x) {
		box.right = bottomRight.x;
	}

	if (box.bottom < bottomRight.y) {
		box.bottom = bottomRight.y;
	}
}

TilePosition BuildingPlaceFinder::findNearestBuildableTile(TilePosition origin, TilePosition target, UnitType unitType) {
	int originAreaId = theMap.GetArea(origin)->Id();
	TilePosition topLeft = theMap.GetArea(origin)->TopLeft();
	TilePosition bottomRight = theMap.GetArea(origin)->BottomRight();

	TilePosition max = target;
	TilePosition min = origin;

	while (min != max) {
		TilePosition center = (min + max) / 2;

		if (min == center || max == center) {
			break;
		}
		else if (topLeft.x < center.x && topLeft.y < center.y && center.x < bottomRight.x && center.y < bottomRight.y && theMap.GetArea(center) != nullptr && theMap.GetArea(center)->Id() == originAreaId && Broodwar->canBuildHere(center, unitType)) {
			min = center;
		}
		else {
			max = center;
		}
	}

	if (min == origin)
		return TilePositions::None;


	return min;
}

int MyBot::BuildingPlaceFinder::compareHeight(const TilePosition &myPosition, const TilePosition &tarCenter) const
{
	if (&myPosition == nullptr || &tarCenter == nullptr) {
		return -1;
	}

	const BWEM::Tile &myTile = theMap.GetTile(myPosition, BWEM::utils::check_t::no_check);
	const BWEM::Tile &targetTile = theMap.GetTile(tarCenter, BWEM::utils::check_t::no_check);

	return myTile.GroundHeight() - targetTile.GroundHeight();
}