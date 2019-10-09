#include "ReserveBuilding.h"

using namespace MyBot;




Building::Building(int order, TilePosition pos, TilePosition tileSize, ReserveTypes reserveType) {
	this->order = order;
	this->pos = pos;
	this->tileSize = tileSize;
	this->reserveType = reserveType;
	this->assignedWorker = nullptr;
	this->lastAssignFrame = -1;
}

Building::Building(int order, TilePosition pos, vector<UnitType> unitTypes) {
	this->order = order;
	this->pos = pos;
	this->tileSize = unitTypes.at(0).tileSize();
	this->possibleList = unitTypes;
	this->reserveType = ReserveTypes::UNIT_TYPE;
	this->assignedWorker = nullptr;
	this->lastAssignFrame = -1;
}

bool Building::isReservedFor(TilePosition position, TilePosition tileSize) const {
	return this->pos == position && this->tileSize == tileSize;
}

bool Building::isReservedFor(TilePosition position, UnitType type) const {
	return this->pos == position && canAssignToType(type);
}

bool Building::canAssignToType(UnitType type, bool checkAlreadySet) const {
	if (checkAlreadySet) {
		if (this->lastAssignFrame + 24 * 20 > TIME)
			return false;
	}

	if (reserveType == ReserveTypes::SIZE) {
		return this->tileSize == type.tileSize();
	}
	else if (reserveType == ReserveTypes::UNIT_TYPE) {
		for (UnitType ut : possibleList) {
			if (ut == type) {
			
				bool canBuild = true;

				for (int x = pos.x; x < pos.x + ut.tileWidth() && canBuild; x++) {
					for (int y = pos.y; y < pos.y + ut.tileHeight(); y++) {
						if (!bw->isBuildable(x, y, true)) {
							canBuild = false;
							break;
						}
					}
				}

				if (canBuild)
					return true;
			}
		}
	}
	else if (reserveType == ReserveTypes::SPACE) {
		return false;
	}
	else if (reserveType == ReserveTypes::MINERALS || reserveType == ReserveTypes::GAS) {
		return false;
	}
	else {
	}

	return false;
}

void Building::assignWorker(Unit worker) {
	this->assignedWorker = worker;
	this->lastAssignFrame = TIME;
}

void Building::unassignWorker() {
	if (this->lastAssignFrame < TIME) {
		this->assignedWorker = nullptr;
		this->lastAssignFrame = -1;
	}
}

int Building::getOrder() {
	return this->order;
}

void Building::setOrder(int order) {
	this->order = order;
}

ReserveBuilding::ReserveBuilding() {
	firstReserveOrder = 0;
	reserveOrder = 0;
	_reserveMap = vector< vector<short> >(bw->mapWidth(), vector<short>(bw->mapHeight(), 0));
	_avoidMap = vector< vector<bool> >(bw->mapWidth(), vector<bool>(bw->mapHeight(), 0));


	for (int i = 0; i < bw->mapWidth(); i++) {
		for (int j = 0; j < bw->mapHeight(); j++) {
			if (!bw->isBuildable(i, j)) {
				_avoidMap[i][j] = true;
		
			}
		}
	}

	if (INFO.selfRace == Races::Terran) {

		TilePosition base = INFO.getMainBaseLocation(S)->getTilePosition();

		reserveTiles(base + TilePosition(Terran_Command_Center.tileWidth(), 1), { Terran_Comsat_Station }, 0, 0, 0, 0);

	
		SelfBuildingPlaceFinder::Instance();
	}
}

ReserveBuilding &ReserveBuilding::Instance() {
	static ReserveBuilding instance;
	return instance;
}


bool ReserveBuilding::reserveTiles(Building &building, TilePosition position, int width, int height, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool canBuildAddon, bool forceReserve) {

	if (!forceReserve && !canReserveHere(position, building.getUnitTypes().empty() ? UnitTypes::None : building.getUnitTypes().front(), width, height, topSpace, leftSpace, rightSpace, bottomSpace, canBuildAddon))
		return false;

	int addonSize = canBuildAddon ? 2 : 0;
	int rwidth = position.x + width + rightSpace + addonSize;
	int rheight = position.y + height + bottomSpace;

	for (int x = position.x - leftSpace; x < rwidth; x++) {
		for (int y = position.y - topSpace; y < rheight; y++) {
			TilePosition p = TilePosition(x, y);

			if (p.isValid()) {
				if (building.isBuildable()) {
					_reserveMap[x][y]++;
				}
				else {
					_avoidMap[x][y] = true;
				}

				if (x < position.x || (position.x + width <= x && position.y == y) || (position.x + width + addonSize <= x && position.y <= y) || (y < position.y && x <= position.x + width) || position.y + height <= y) {
					building.addSpaceTile(x, y);
				}
			}
		}
	}

	return true;
}

bool ReserveBuilding::reserveTiles(TilePosition position, TilePosition size, int topSpace, int leftSpace, int rightSpace, int bottomSpace, ReserveTypes reserveType) {
	Building b(reserveOrder, position, size, reserveType);
	bool success = reserveTiles(b, position, size.x, size.y, topSpace, leftSpace, rightSpace, bottomSpace);

	if (success) {
		if (Building::isBuildable(reserveType)) {
			reserveOrder++;

			_reserveList.push_back(b);
		}
		else
			_avoidList.emplace_back(0, position, size, reserveType);
	}

	return success;
}

bool ReserveBuilding::reserveTiles(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool useTypeAddon) {
	Building b(reserveOrder, position, unitTypes);
	UnitType type = unitTypes.empty() ? UnitTypes::None : unitTypes.at(0);

	bool canBuildAddon = false;

	vector<UnitType> addonUnitTypes;

	if (useTypeAddon) {
		for (UnitType unitType : unitTypes) {
			if (unitType.canBuildAddon()) {
				for (UnitType t : unitType.buildsWhat()) {
					if (t.isAddon()) {
						addonUnitTypes.push_back(t);
					}
				}

				canBuildAddon = true;
			}
		}
	}

	bool success = reserveTiles(b, position, type.tileWidth(), type.tileHeight(), topSpace, leftSpace, rightSpace, bottomSpace, canBuildAddon);


	if (success) {
		reserveOrder++;

		_reserveList.push_back(b);

		if (canBuildAddon)
			_reserveList.emplace_back(reserveOrder++, position + TilePosition(type.tileWidth(), 1), addonUnitTypes);
	}

	return success;
}

void ReserveBuilding::forceReserveTiles(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace) {
	Building b(reserveOrder++, position, unitTypes);

	reserveTiles(b, position, unitTypes.front().tileWidth(), unitTypes.front().tileHeight(), topSpace, leftSpace, rightSpace, bottomSpace, false, true);

	_reserveList.push_back(b);
}

bool ReserveBuilding::reserveTilesFirst(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace) {
	Building b(--firstReserveOrder, position, unitTypes);
	UnitType type;
	bool canBuildAddon = false;
	vector<UnitType> addonUnitTypes;

	for (UnitType unitType : unitTypes) {
		type = unitType;

		if (unitType.canBuildAddon()) {
			for (UnitType t : unitType.buildsWhat()) {
				if (t.isAddon()) {
					addonUnitTypes.push_back(t);
				}
			}

			canBuildAddon = true;
		}
	}

	bool success = reserveTiles(b, position, type.tileWidth(), type.tileHeight(), topSpace, leftSpace, rightSpace, bottomSpace, canBuildAddon);


	if (success) {
		_reserveList.insert(_reserveList.begin(), b);

		if (canBuildAddon)
			_reserveList.emplace_back(reserveOrder++, position + TilePosition(type.tileWidth(), 1), addonUnitTypes);
	}

	return success;
}

void ReserveBuilding::forceReserveTilesFirst(TilePosition position, vector<UnitType> unitTypes, int topSpace, int leftSpace, int rightSpace, int bottomSpace) {
	if (!position.isValid())
		return;

	Building b(--firstReserveOrder, position, unitTypes);

	reserveTiles(b, position, unitTypes.front().tileWidth(), unitTypes.front().tileHeight(), topSpace, leftSpace, rightSpace, bottomSpace, false, true);

	_reserveList.insert(_reserveList.begin(), b);
}


void ReserveBuilding::freeTiles(int width, int height, TilePosition position) {
	int rwidth = position.x + width;
	int rheight = position.y + height;

	for (int x = position.x; x < rwidth; x++) {
		for (int y = position.y; y < rheight; y++) {
			if (_reserveMap[x][y] > 0)
				_reserveMap[x][y]--;
		}
	}
}

void ReserveBuilding::freeTiles(TilePosition position, TilePosition size) {
	for (word i = 0; i < _reserveList.size(); i++) {
		Building b = _reserveList.at(i);

		if (b.isReservedFor(position, size)) {
			_reserveList.erase(_reserveList.begin() + i);
			freeTiles(size.x, size.y, position);

			for (auto space : b.getSpaceList())
				if (_reserveMap[space.x][space.y] > 0)
					_reserveMap[space.x][space.y]--;

			break;
		}
	}
}

void ReserveBuilding::freeTiles(TilePosition position, UnitType type, bool freeAddon) {
	for (word i = 0; i < _reserveList.size(); i++) {
		Building b = _reserveList.at(i);

		if (b.isReservedFor(position, type.tileSize())) {
			_reserveList.erase(_reserveList.begin() + i);
			freeTiles(type.tileWidth(), type.tileHeight(), position);

			for (auto space : b.getSpaceList())
				if (_reserveMap[space.x][space.y] > 0)
					_reserveMap[space.x][space.y]--;

			if (freeAddon) {
				if (type.canBuildAddon()) {
					for (UnitType addonUnitType : type.buildsWhat()) {
						if (addonUnitType.isAddon()) {
							freeTiles(position + TilePosition(4, 1), addonUnitType);
							break;
						}
					}
				}
			}

			break;
		}
	}
}

void ReserveBuilding::modifyReservePosition(UnitType unitType, TilePosition fromPosition, TilePosition toPosition) {
	freeTiles(fromPosition, unitType, true);
	reserveTiles(toPosition, { unitType }, 0, 0, 0, 0);
}

bool ReserveBuilding::canReserveHere(TilePosition position, UnitType unitType, int width, int height, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool canBuildAddon) const {
	TilePosition strPosition = position - TilePosition(leftSpace, topSpace);
	int rwidth = position.x + width + rightSpace + (canBuildAddon ? 2 : 0);
	int rheight = position.y + height + bottomSpace;


	if (!strPosition.isValid() || rwidth > (int)_reserveMap.size() || rheight > (int)_reserveMap[0].size()) {
		return false;
	}


	if (canBuildAddon) {
		bool needToCheck = true;

	
		for (auto u : bw->getUnitsOnTile(position + TilePosition(width, 1))) {
			if (u->getTilePosition() == position + TilePosition(width, 1) && u->getType().isAddon() && u->getType().whatBuilds().first == unitType) {
				needToCheck = false;
				canBuildAddon = false;
				rightSpace = 0;
				rwidth -= 2;
				break;
			}
		}

		if (needToCheck) {
			if (!canReserveHere(position + TilePosition(width, 1), 2, 2, 0, 0, 0, 0)) {
				return false;
			}

			if (!bw->canBuildHere(position + TilePosition(width, 1), Terran_Comsat_Station)) {
				return false;
			}
		}
	}

	int buildingWidth = position.x + width + (canBuildAddon ? 2 : 0);
	int buildingHeight = position.y + height;

	for (int x = strPosition.x; x < rwidth; x++) {
		for (int y = strPosition.y; y < rheight; y++) {
		
			if (position.x > x || (position.y > y && x <= position.x + width) || (position.x + width <= x && position.y == y) || (buildingWidth <= x && position.y <= y) || buildingHeight <= y) {
			
				if (unitType == Terran_Command_Center) {
					for (auto unit : bw->getUnitsInRectangle(x * 32 + 1, y * 32 + 1, x * 32 + 31, y * 32 + 31)) {
						if (unit->getType() != Terran_Missile_Turret && unit->getType() != Terran_Bunker) {
							return false;
						}
					}
				}
				else if (_reserveMap[x][y] || !bw->isBuildable(x, y, true)) {
					return false;
				}
				else {
					for (auto unit : bw->getUnitsInRectangle(x * 32 + 1, y * 32 + 1, x * 32 + 31, y * 32 + 31)) {
						if (unit->getType().isSpecialBuilding()) {
							return false;
						}
					}
				}
			}

			else {
				if (_reserveMap[x][y] || !bw->isBuildable(x, y, true) || (_avoidMap[x][y] && unitType != Terran_Bunker && unitType != Terran_Missile_Turret)) {
					
					if (canBuildAddon && y < position.y && x >= position.x + width)
						continue;

					return false;
				}
			}
		}
	}

	return true;
}

void ReserveBuilding::debugCanReserveHere(TilePosition position, UnitType unitType, int width, int height, int topSpace, int leftSpace, int rightSpace, int bottomSpace, bool canBuildAddon) const {
	TilePosition strPosition = position - TilePosition(leftSpace, topSpace);
	int rwidth = position.x + width + rightSpace + (canBuildAddon ? 2 : 0);
	int rheight = position.y + height + bottomSpace;


	if (!strPosition.isValid() || rwidth > (int)_reserveMap.size() || rheight > (int)_reserveMap[0].size()) {
		cout << "validation fail." << strPosition << " " << rwidth << ", " << rheight << endl;
		return;
	}


	if (canBuildAddon) {
		bool needToCheck = true;

		
		for (auto u : bw->getUnitsOnTile(position + TilePosition(width, 1))) {
			if (u->getTilePosition() == position + TilePosition(width, 1) && u->getType().isAddon() && u->getType().whatBuilds().first == unitType) {
				needToCheck = false;
				canBuildAddon = false;
				rightSpace = 0;
				rwidth -= 2;
				break;
			}
		}

		if (needToCheck) {
			if (!canReserveHere(position + TilePosition(width, 1), 2, 2, 0, 0, 0, 0)) {
				cout << "addon canReserveHere fail." << position + TilePosition(width, 1) << endl;
				return;
			}

			if (!bw->canBuildHere(position + TilePosition(width, 1), Terran_Comsat_Station)) {
				cout << "bw->canBuildHere fail." << position + TilePosition(width, 1) << endl;
				return;
			}
		}
	}

	int buildingWidth = position.x + width + (canBuildAddon ? 2 : 0);
	int buildingHeight = position.y + height;

	for (int x = strPosition.x; x < rwidth; x++) {
		for (int y = strPosition.y; y < rheight; y++) {
		
			if (position.x > x || (position.y > y && x <= position.x + width) || (position.x + width <= x && position.y == y) || (buildingWidth <= x && position.y <= y) || buildingHeight <= y) {
				
				if (unitType == Terran_Command_Center) {
					for (auto unit : bw->getUnitsInRectangle(x * 32 + 1, y * 32 + 1, x * 32 + 31, y * 32 + 31)) {
						if (unit->getType() != Terran_Missile_Turret && unit->getType() != Terran_Bunker) {
							cout << "another building exists near command center" << endl;
							return;
						}
					}
				}
				else if (_reserveMap[x][y] || !bw->isBuildable(x, y, true)) {
					cout << "space tile check fail.(" << x << ", " << y << ") _reserveMap : " << _reserveMap[x][y] << " bw->isBuildable : " << bw->isBuildable(x, y, true) << endl;

					cout << strPosition << rwidth << ", " << rheight << endl;

					for (int b = strPosition.y; b < rheight; b++) {
						for (int a = strPosition.x; a < rwidth; a++) {
							printf("%2d", _reserveMap[a][b]);
						}

						cout << endl;
					}

					return;
				}
				else {
					for (auto unit : bw->getUnitsInRectangle(x * 32 + 1, y * 32 + 1, x * 32 + 31, y * 32 + 31)) {
						if (unit->getType().isSpecialBuilding()) {
							cout << "isSpecialBuilding exist fail.(" << x << ", " << y << ")" << endl;
							return;
						}
					}
				}
			}
		
			else {
				if (_reserveMap[x][y] || !bw->isBuildable(x, y, true) || (_avoidMap[x][y] && unitType != Terran_Bunker && unitType != Terran_Missile_Turret)) {
					
					if (canBuildAddon && y < position.y && x >= position.x + width)
						continue;

					cout << "already reserved fail.(" << x << ", " << y << ") _reserveMap : " << _reserveMap[x][y] << " _avoidMap : " << _avoidMap[x][y] << unitType << endl;
					return;
				}
			}
		}
	}
}

bool ReserveBuilding::isReservedFor(TilePosition position, UnitType type) const {
	for (const Building &b : _reserveList) {
		if (b.isReservedFor(position, type)) {
			return true;
		}
	}

	return false;
}

TilePosition ReserveBuilding::getReservedPosition(UnitType type) {
	for (Building &b : _reserveList) {
		if (b.canAssignToType(type, true)) {
			return b.TopLeft();
		}
	}

	return TilePositions::None;
}

TilePosition ReserveBuilding::getReservedPosition(UnitType type, Unit builder, UnitType addon) {
	for (Building &b : _reserveList) {
		if (b.canAssignToType(type, true) && bw->canBuildHere(b.TopLeft(), addon, builder)) {
			return b.TopLeft();
		}
	}

	return TilePositions::None;
}

TilePosition ReserveBuilding::getReservedPositionAtWorker(UnitType type, Unit builder) {
	for (Building &b : _reserveList) {
		TilePosition tl = b.TopLeft();
		TilePosition worker = (TilePosition)builder->getPosition();
		TilePosition br = b.BottomRight();

		if (b.canAssignToType(type, false) && tl.x <= worker.x && tl.y == worker.y && worker.x < br.x && worker.y < br.y)
			return b.TopLeft();
	}

	return TilePositions::None;
}


void ReserveBuilding::assignTiles(TilePosition position, TilePosition size, Unit worker) {
	for (word i = 0; i < _reserveList.size(); i++) {
		Building &b = _reserveList.at(i);

		if (b.isReservedFor(position, size)) {
			b.assignWorker(worker);
			break;
		}
	}
}

void ReserveBuilding::assignTiles(TilePosition position, UnitType type, Unit worker) {
	for (word i = 0; i < _reserveList.size(); i++) {
		Building &b = _reserveList.at(i);

		if (b.isReservedFor(position, type)) {
			b.assignWorker(worker);
			break;
		}
	}
}

void ReserveBuilding::unassignTiles(TilePosition position, UnitType type) {
	for (word i = 0; i < _reserveList.size(); i++) {
		Building &b = _reserveList.at(i);

		if (b.isReservedFor(position, type)) {
			b.unassignWorker();
			break;
		}
	}
}

void ReserveBuilding::reordering(TilePosition tiles[], int strIdx, int endIdx, UnitType unitType) {
	vector<int> order;

	for (int i = strIdx; i < endIdx; i++) {
		TilePosition tile = tiles[i];

		for (Building &b : _reserveList) {
			if (b.isReservedFor(tile, unitType)) {
				order.push_back(b.getOrder());
				break;
			}
		}
	}

	sort(order.begin(), order.end());

	for (int i = strIdx; i < endIdx; i++) {
		TilePosition tile = tiles[i];

		for (Building &b : _reserveList) {
			if (b.isReservedFor(tile, unitType)) {
				b.setOrder(order.at(i - strIdx));
				break;
			}
		}
	}

	sort(_reserveList.begin(), _reserveList.end(), [](Building a, Building b) {
		return a.getOrder() < b.getOrder();
	});
}

void ReserveBuilding::addTypes(Building building, vector<UnitType> unitTypes) {
	for (auto &reserve : _reserveList) {
		if (reserve == building) {
			reserve.addType(unitTypes);

			break;
		}
	}
}

const vector< Building > ReserveBuilding::getReserveList() {
	return _reserveList;
}

const vector< Building > ReserveBuilding::getAvoidList() {
	return _avoidList;
}
