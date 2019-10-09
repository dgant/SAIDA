#include "ConstructionPlaceFinder.h"
#include "UnitManager/ScvManager.h"

using namespace MyBot;

ConstructionPlaceFinder::ConstructionPlaceFinder()
{
	setTilesToAvoid();

	// DeltasByAscendingAltitude 변수 초기화
	for (int dy = 0; dy <= 15; ++dy)
		for (int dx = dy; dx <= 15; ++dx)			// Only consider 1/8 of possible deltas. Other ones obtained by symmetry.
			if (dx || dy)
				DeltasByAscendingAltitude.emplace_back(TilePosition(dx, dy), altitude_t(0.5 + norm(dx, dy) * 32));

	sort(DeltasByAscendingAltitude.begin(), DeltasByAscendingAltitude.end(),
	[](const pair<TilePosition, altitude_t> &a, const pair<TilePosition, altitude_t> &b) {
		return a.second < b.second;
	});
}

ConstructionPlaceFinder &ConstructionPlaceFinder::Instance()
{
	static ConstructionPlaceFinder instance;
	return instance;
}

TilePosition ConstructionPlaceFinder::getSeedPositionFromSeedLocationStrategy(BuildOrderItem::SeedPositionStrategy seedPositionStrategy) const {
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

TilePosition	ConstructionPlaceFinder::getBuildLocationWithSeedPositionAndStrategy(UnitType buildingType, TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy) const
{
	// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
	// 빌드 실행 유닛 (일꾼/건물) 결정 로직이 seedLocation 이나 seedLocationStrategy 를 잘 반영하도록 수정

	TilePosition desiredPosition = TilePositions::None;

	if (buildingType.isRefinery()) {
		return seedPosition;
	}

	if (INFO.selfRace == Races::Terran) {
		desiredPosition = TerranConstructionPlaceFinder::Instance().getBuildLocationWithSeedPositionAndStrategy(buildingType, seedPosition, seedPositionStrategy);
	}

	// 예약된 타일에서 Position이 있는지 확인한다. 아머리는 MainBase 가 아닌 경우 예약된 곳 말고 다른곳으로 짓도록 한다.
	if (desiredPosition == TilePositions::None && !(buildingType == Terran_Armory && seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation)) {
		desiredPosition = ReserveBuilding::Instance().getReservedPosition(buildingType);
	}

	if (desiredPosition != TilePositions::None) {
		return desiredPosition;
	}

	// seedPosition 을 입력하지 않은 경우 seedPosition 을 찾는다.
	if (!seedPosition.isValid()) {
		seedPosition = getSeedPositionFromSeedLocationStrategy(seedPositionStrategy);
	}

	if (seedPosition.isValid()) {
		//cout << "getBuildLocationNear 예약위해 탐색 " << seedPosition.x << ", " << seedPosition.y << endl;
		UnitInfo *closestScv = ScvManager::Instance().chooseScvClosestTo((Position)seedPosition);

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

	// BasicBot 1.1 Patch End //////////////////////////////////////////////////
}

TilePosition	ConstructionPlaceFinder::getBuildLocationNear(UnitType buildingType, TilePosition desiredPosition, bool checkForReserve, Unit worker) const
{
	if (buildingType.isRefinery())
	{
		//cout << "getRefineryPositionNear "<< endl;

		return desiredPosition;
	}

	if (Broodwar->self()->getRace() == Races::Protoss) {
		// special easy case of having no pylons
		if (buildingType.requiresPsi() && Broodwar->self()->completedUnitCount(Protoss_Pylon) == 0)
		{
			return TilePositions::None;
		}
	}

	if (!desiredPosition.isValid())
	{
		desiredPosition = InformationManager::Instance().getMainBaseLocation(S)->getTilePosition();
	}

	//cout << endl << "getBuildLocationNear " << buildingType.getName().c_str() << " " << desiredPosition.x << "," << desiredPosition.y << " checkForReserve : " << checkForReserve
	//	 << " worker " << (worker == nullptr ? -1 : worker->getID()) << endl;

	//returns a valid build location near the desired tile position (x,y).
	TilePosition resultPosition = TilePositions::None;
	ConstructionTask b(buildingType, desiredPosition);
	b.constructionWorker = worker;

	// maxRange 를 설정하지 않거나, maxRange 를 128으로 설정하면 지도 전체를 다 탐색하는데, 매우 느려질뿐만 아니라, 대부분의 경우 불필요한 탐색이 된다
	// maxRange 는 16 ~ 64가 적당하다
	int maxRange = 32; // maxRange = Broodwar->mapWidth()/4;

	// desiredPosition 으로부터 시작해서 spiral 하게 탐색하는 방법
	// 처음에는 아래 방향 (0,1) -> 오른쪽으로(1,0) -> 위로(0,-1) -> 왼쪽으로(-1,0) -> 아래로(0,1) -> ..
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

		// 다른 방향으로 전환한다
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

TilePosition ConstructionPlaceFinder::getBuildLocationBySpiralSearch(UnitType unitType, TilePosition possiblePosition, bool next, Position possibleArea, bool checkOnlyBuilding) {
	if (!possiblePosition.isValid())
		return TilePositions::None;

	if (next) {
		deltaIdx++;
	}
	else {
		altitudeIdx = 0;
		deltaIdx = 0;

		// 인풋 위치에 지을수 있는지 체크
		TilePosition w = possiblePosition - unitType.tileSize() / 2;

		// tile 유효성 체크. 다른 건물이 있거나 base 위치는 못짓는다.
		if (w.isValid() && bw->isBuildable(w, true) && !isBasePosition(w, unitType)) {
			// build 할 area 체크
			if (possibleArea == Positions::None || isSameArea(w, (TilePosition)possibleArea)) {
				// 건물이 있는지만 체크.
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
				// 유닛이 있는지도 체크됨.
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

bool ConstructionPlaceFinder::canBuildHere(BWAPI::TilePosition position, const UnitType type, const bool checkForReserve, const Unit worker, int topSpace, int leftSpace, int rightSpace, int bottomSpace) const
{
	// This function checks for creep, power, and resource distance requirements in addition to the tiles' buildability and possible units obstructing the build location.
	if (!Broodwar->canBuildHere(position, type, worker))
	{
		//if (type == Terran_Supply_Depot || type.isResourceDepot()) {
		//	if (worker == nullptr)
		//		cout << "Broodwar->canBuildHere return false at " << position << endl;
		//	else
		//		cout << "Broodwar->canBuildHere return false at " << position << ", worker id : " << worker->getID() << endl;
		//}

		if (type != Terran_Missile_Turret && type != Terran_Bunker)
			return false;

		// 벙커와 터렛은 일꾼이 많은 지역에 짓기 때문에 canBuildHere 에서 false 로 나올 확률이 매우 높아 예외처리 해준다.
		Unitset us = bw->getUnitsInRectangle(((Position)position) + Position(1, 1), (Position)(position + type.tileSize()) - Position(1, 1));

		if (us.empty())
			return false;

		for (auto u : us) {
			if (!u->getType().isWorker())
				return false;
		}
	}

	// base 위치에는 다른 건물은 짓지 않는다.
	if (isBasePosition(position, type))
		return false;

	bool useTypeAddon = true;
	bool checkAddon = false;

	// 후반에 팩토리 지을 공간이 없는 경우 나선형 검색을 하면서 사방 공간을 예약한다.
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

	// 예약을 위해 체크 하는 경우, 이미 예약된 곳이면 예약 불가
	if (checkForReserve && !ReserveBuilding::Instance().canReserveHere(position, type, topSpace, leftSpace, rightSpace, bottomSpace, useTypeAddon, checkAddon)) {
		//if (type == Terran_Supply_Depot || type.isResourceDepot()) {
		//	cout << "isReservedTile return false at " << position << endl;
		//}

		return false;
	}
	// 건설을 위해 체크하는 경우, 해당 UnitType 에 대해 예약된 곳이 아니면 건설 불가
	else if (!checkForReserve && !ReserveBuilding::Instance().isReservedFor(position, type)) {
		//if (type == Terran_Supply_Depot || type.isResourceDepot()) {
		//	cout << "isReservedFor return false at " << position << endl;
		//}

		return false;
	}

	return true;
}

bool ConstructionPlaceFinder::isBasePosition(TilePosition position, UnitType type) const {
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

		// 베이스 위치 체크
		if (minX < buildingMaxX && buildingMinX < maxX && minY < buildingMaxY && buildingMinY < maxY)
			return true;

		if (INFO.selfRace == Races::Terran) {
			// 지을 건물의 addon 위치 체크 (avoid tile 공간도 포함)
			if (type.canBuildAddon()) {
				int addonMinX = position.x + 4;
				int addonMinY = position.y;
				int addonMaxX = addonMinX + 2;
				int addonMaxY = addonMinY + 3;

				// 베이스 위치 체크
				if (minX < addonMaxX && addonMinX < maxX && minY < addonMaxY && addonMinY < maxY)
					return true;
			}

			// 컴셋 위치 체크
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

TilePosition ConstructionPlaceFinder::getRefineryPositionNear(BuildOrderItem::SeedPositionStrategy seedPositionStrategy) const
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

TilePosition ConstructionPlaceFinder::getRefineryPositionNear(Base *base) const
{
	if (base != nullptr)
		for (auto &geyser : base->Geysers())
			for (auto unit : bw->getUnitsOnTile(geyser->Unit()->getTilePosition()))
				if (unit->getType() == Resource_Vespene_Geyser)
					return unit->getTilePosition();

	return TilePositions::None;
}

void ConstructionPlaceFinder::setTilesToAvoid()
{
	for (const Area &area : theMap.Areas()) {
		// 섬인 경우 건물 지을 공간이 절대적으로 좁기 때문에 건물 안짓는 공간을 두지 않는다
		if (area.ChokePoints().empty()) continue;

		for (const Base &base : area.Bases()) {
			// dimensions of the base location
			int bx0 = base.Location().x;
			int by0 = base.Location().y;
			int bx4 = base.Location().x + 4;
			int by3 = base.Location().y + 3;

			// dimensions of the comset location
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
				// 직선의 방정식 구하기 (미네랄 갯수만큼)
				lines.push_back(getLine(base.Center(), mineral->Pos()));

				// 이동경로 체크할 사각형 구하기
				getBox(box, mineral->TopLeft(), mineral->TopLeft() + mineral->Size());
			}

			for (Geyser *geyser : base.Geysers()) {
				// 가스의 우측, 아래쪽은 avoid tile 로 예약한다.
				TilePosition pos(geyser->BottomRight().x + 1, geyser->TopLeft().y);

				// 가스 우측 예약
				for (int y = 0; y <= geyser->Type().tileHeight(); y++) {
					TilePosition rightOfGeyser = pos + TilePosition(0, y);

					if (ReserveBuilding::Instance().canReserveHere(rightOfGeyser, TilePosition(1, 1), 0, 0, 0, 0)) {
						ReserveBuilding::Instance().reserveTiles(rightOfGeyser, TilePosition(1, 1), 0, 0, 0, 0, ReserveTypes::GAS);
					}
				}

				pos = TilePosition(geyser->TopLeft().x, geyser->BottomRight().y + 1);

				// 가스 아래쪽 예약 (벙커 지어질 만큼)
				for (int y = 0; y < Terran_Bunker.tileHeight(); y++) {
					for (int x = 0; x < Terran_Bunker.tileWidth(); x++) {
						TilePosition rightOfGeyser = pos + TilePosition(x, y);

						if (ReserveBuilding::Instance().canReserveHere(rightOfGeyser, TilePosition(1, 1), 0, 0, 0, 0)) {
							ReserveBuilding::Instance().reserveTiles(rightOfGeyser, TilePosition(1, 1), 0, 0, 0, 0, ReserveTypes::GAS);
						}
					}
				}

				Position rightOfGas = geyser->Pos() + Position(48, 0);

				// 가스의 오른쪽 위치가 기지의 범위 내인 경우
				if (abs(base.Center().x - rightOfGas.x) / TILE_SIZE < 2) {
					lines.push_back(getLine(Position(rightOfGas.x, base.Center().y), rightOfGas));
				}
				else {
					// 직선의 방정식 구하기 (가스 갯수만큼)
					lines.push_back(getLine(base.Center(), geyser->Pos()));

					// 직선의 방정식 구하기 (가스 오른쪽 - 베이스 오른쪽)
					lines.push_back(getLine(base.Center() + Position(48, 0), rightOfGas));
				}


				// 이동경로 체크할 사각형 구하기
				getBox(box, geyser->TopLeft(), geyser->TopLeft() + geyser->Size());
			}

			for (int x = box.left; x < box.right; x++) {
				for (int y = box.top; y < box.bottom; y++) {
					TilePosition t1(x, y);

					// 본진, 컴셋, 가스, 미네랄 위치는 제외
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
						//if (t1 == TilePosition(119, 6)) {
						//	cout << "라인 : " << line.p1 << line.p2 << endl;
						//}
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

						// 직선거리에 포함되는 타일들은 모두 avoid 에 등록
						int tmpY1 = line.dx * p1.y;
						int tmpX1 = line.dy * p1.x + line.cdx;

						int tmpY2 = line.dx * p2.y;
						int tmpX2 = line.dy * p2.x + line.cdx;

						int tmpY3 = line.dx * p3.y;
						int tmpX3 = line.dy * p3.x + line.cdx;

						int tmpY4 = line.dx * p4.y;
						int tmpX4 = line.dy * p4.x + line.cdx;

						//if (t1 == TilePosition(119, 6)) {
						//	cout << "temp : " << tmpY1 << ", " << tmpX1 << "  " << tmpY2 << ", " << tmpX2 << "  " << tmpY3 << ", " << tmpX3 << "  " << tmpY4 << ", " << tmpX4 << endl;
						//}

						// 직선이 y 축과 평행하고 타일 좌 우의 양 꼭지점을 지나는 경우
						if (line.dx == 0 && ((line.p1.x == p1.x && line.p1.x == p3.x) || (line.p1.x == p2.x && line.p1.x == p4.x))) {
							isAvoidTile = true;
							break;
						}
						// 직선이 x 축과 평행하고 타일 상 하의 양 꼭지점을 지나는 경우
						else if (line.dy == 0 && ((line.p1.y == p1.y && line.p1.y == p2.y) || (line.p1.y == p3.y && line.p1.y == p4.y))) {
							isAvoidTile = true;
							break;
						}
						// 직선이 타일 꼭지점 중 하나를 지나는 경우
						else if (tmpY1 == tmpX1 || tmpY2 == tmpX2 || tmpY3 == tmpX3 || tmpY4 == tmpX4) {
							isAvoidTile = true;
							break;
						}
						// 직선이 타일 상단의 양 꼭지점 사이를 지나는 경우
						else if ((tmpY1 > tmpX1 && tmpY2 < tmpX2) || (tmpY1 < tmpX1 && tmpY2 > tmpX2)) {
							isAvoidTile = true;
							break;
						}
						// 직선이 타일 좌단의 양 꼭지점 사이를 지나는 경우
						else if ((tmpY1 > tmpX1 && tmpY3 < tmpX3) || (tmpY1 < tmpX1 && tmpY3 > tmpX3)) {
							isAvoidTile = true;
							break;
						}
						// 직선이 타일 우단의 양 꼭지점 사이를 지나는 경우
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

Line ConstructionPlaceFinder::getLine(const Position &p1, const Position &p2) {
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

void ConstructionPlaceFinder::getBox(Box &box, const TilePosition &topLeft, const TilePosition &bottomRight) {
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

TilePosition ConstructionPlaceFinder::findNearestBuildableTile(TilePosition origin, TilePosition target, UnitType unitType) {
	int originAreaId = theMap.GetArea(origin)->Id();
	TilePosition topLeft = theMap.GetArea(origin)->TopLeft();
	TilePosition bottomRight = theMap.GetArea(origin)->BottomRight();

	TilePosition max = target;
	TilePosition min = origin;

	while (min != max) {
		TilePosition center = (min + max) / 2;

		//cout << "min : " << min << " max : " << max << " center : " << center << endl;
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

	//cout << "nearest Tile : " << min << endl;
	return min;
}

int MyBot::ConstructionPlaceFinder::compareHeight(const TilePosition &myPosition, const TilePosition &tarCenter) const
{
	if (&myPosition == nullptr || &tarCenter == nullptr) {
		return -1;
	}

	const BWEM::Tile &myTile = theMap.GetTile(myPosition, BWEM::utils::check_t::no_check);
	const BWEM::Tile &targetTile = theMap.GetTile(tarCenter, BWEM::utils::check_t::no_check);

	return myTile.GroundHeight() - targetTile.GroundHeight();
}