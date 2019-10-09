#include "Common.h"
#include "InformationManager.h"
#include "UnitData.h"

using namespace MyBot;

UnitData::UnitData()
{
}

UnitData::~UnitData()
{
}

vector<UnitInfo *> &UnitData::getUnitVector(UnitType uType)
{
	if (unitTypeMap.find(uType) == unitTypeMap.end())
	{
		vector<UnitInfo *> newUnitType;
		unitTypeMap[uType] = newUnitType;
	}

	return unitTypeMap[uType];
}

vector<UnitInfo *> &UnitData::getBuildingVector(UnitType uType)
{
	if (buildingTypeMap.find(uType) == buildingTypeMap.end())
	{
		vector<UnitInfo *> newUnitType;
		buildingTypeMap[uType] = newUnitType;
	}

	return buildingTypeMap[uType];
}
int UnitData::getCompletedCount(UnitType uType)
{
	if (completedCount.find(uType) == completedCount.end())
		completedCount[uType] = 0;

	return completedCount[uType];
}
int UnitData::getDestroyedCount(UnitType uType)
{
	if (destroyedCount.find(uType) == destroyedCount.end())
		destroyedCount[uType] = 0;

	return destroyedCount[uType];
}
map<UnitType, int> UnitData::getDestroyedCountMap() {
	return destroyedCount;
}
int UnitData::getAllCount(UnitType uType)
{
	if (allCount.find(uType) == allCount.end())
		allCount[uType] = 0;

	return allCount[uType];
}
void UnitData::increaseCompleteUnits(UnitType uType)
{
	UnitType type = getUnitTypeDB(uType);

	if (completedCount.find(type) == completedCount.end())
		completedCount[type] = 0;

	completedCount[type]++;
}
void UnitData::increaseDestroyUnits(UnitType uType)
{
	UnitType type = getUnitTypeDB(uType);

	if (destroyedCount.find(type) == destroyedCount.end())
		destroyedCount[type] = 0;

	destroyedCount[type]++;
}
void UnitData::increaseCreateUnits(UnitType uType)
{
	UnitType type = getUnitTypeDB(uType);

	if (allCount.find(type) == allCount.end())
		allCount[type] = 0;

	allCount[type]++;
}
void UnitData::decreaseCompleteUnits(UnitType uType)
{
	UnitType type = getUnitTypeDB(uType);

	completedCount[type]--;
}
void UnitData::decreaseCreateUnits(UnitType uType)
{
	UnitType type = getUnitTypeDB(uType);

	allCount[type]--;
}

bool UnitData::addUnitNBuilding(Unit u)
{
	UnitType type = getUnitTypeDB(u->getType());

	map<Unit, UnitInfo *> &unitMap = type.isBuilding() ? getAllBuildings() : getAllUnits();
	vector<UnitInfo *> &unitVector = type.isBuilding() ? getBuildingVector(type) : getUnitVector(type);

	if (unitMap.find(u) == unitMap.end())
	{
		UnitInfo *pUnit = new UnitInfo(u);
		unitMap[u] = pUnit;
		unitVector.push_back(pUnit);

		return true;
	}

	return false;
}


void UnitData::removeUnitNBuilding(Unit u)
{
	UnitType type = getUnitTypeDB(u->getType());

	map<Unit, UnitInfo *> &unitMap = type.isBuilding() ? getAllBuildings() : getAllUnits();
	vector<UnitInfo *> &unitVector = type.isBuilding() ? getBuildingVector(type) : getUnitVector(type);

	if (unitMap.find(u) != unitMap.end())
	{
		auto del_unit = find_if(unitVector.begin(), unitVector.end(), [u](UnitInfo * up) {
			return up->unit() == u;
		});

		if (del_unit != unitVector.end()) {
			BWEM::utils::fast_erase(unitVector, distance(unitVector.begin(), del_unit));
		}
		else {
			cout << "remove Unit Error" << endl;
		}

		delete unitMap[u];
		unitMap.erase(u);

		
		if (u->getPlayer() == S)
		{
			if (u->isCompleted())
				decreaseCompleteUnits(type);

			decreaseCreateUnits(type);
		}

		increaseDestroyUnits(type);
	}
}

void UnitData::initializeAllInfo() {
	for (auto &u : allUnits)
		u.second->initFrame();

	for (auto &u : allBuildings)
		u.second->initFrame();
}


void UnitData::updateAllInfo()
{
	for (auto &u : allUnits)
		u.second->Update();

	for (auto &u : allBuildings)
		u.second->Update();

	map<int, pair<int, Position>> &spellMap = getAllSpells();

	for (auto &b : Broodwar->getBullets()) {
		
		if (b->getType() == BulletTypes::Fusion_Cutter_Hit && b->getSource() != nullptr && b->getSource()->getPlayer() == S && b->getTarget() != nullptr && b->getTarget()->isBeingGathered()) {
			uMap m = getAllUnits();
			m[b->getSource()]->setGatheringMinerals();
		}

		
		if (b->getType() == BulletTypes::EMP_Missile && b->getSource() && b->getSource()->getPlayer() == S)
			spellMap[b->getID()] = make_pair(TIME, b->getTargetPosition());

		if (TIME % 12 == 0 && b->getType() == BulletTypes::Gauss_Rifle_Hit && b->getSource() == nullptr)
		{
			int gap;

			if (E->getUpgradeLevel(UpgradeTypes::U_238_Shells))
				gap = 8;
			else
				gap = 7;

			if (b->getTarget() != nullptr && b->getTarget()->getPlayer() == S)
			{
				uList Bunkers = INFO.getTypeBuildingsInRadius(Terran_Bunker, E, b->getTarget()->getPosition(), gap * TILE_SIZE, true, false);

				for (auto b : Bunkers)
				{
					b->setMarineInBunker();
				}
			}
		}
	}

	vector<pair<int, Position>> del_spells;

	for (auto &s : spellMap)
		if (s.second.first < TIME)
			del_spells.push_back(make_pair(s.first, s.second.second));

	for (auto &u : del_spells) {
		getAllSpells().erase(u.first);

		for (auto e : INFO.getUnitsInRadius(E, u.second, 2 * TILE_SIZE))
			e->recudeEnergy(INT_MAX);
	}
}

void UnitData::updateNcheckTypeAllInfo()
{
	
	vector<Unit> del_units;
	vector<Unit> del_buildings;
	vector<int> del_spells;

	for (auto &u : allUnits)
	{
		if (!u.first->exists())
		{
			u.second->Update();
			continue;
		}

		UnitType preType = getUnitTypeDB(u.second->type());
		UnitType curType = getUnitTypeDB(u.second->unit()->getType());

		INFO.setRearched(u.second->unit()->getType());

		if (preType != curType)
		{
			if (curType.isBuilding()) 
			{
				
				vector<UnitInfo *> &Drones = (preType == Zerg_Drone ? getUnitVector(Zerg_Drone) :
											  (preType == Zerg_Larva ? getUnitVector(Zerg_Larva) : getUnitVector(Zerg_Egg)));

				Unit unit = u.first;

				auto del_unit = find_if(Drones.begin(), Drones.end(), [unit](UnitInfo * up) {
					return up->unit() == unit;
				});

				if (del_unit != Drones.end()) {
					BWEM::utils::fast_erase(Drones, distance(Drones.begin(), del_unit));

					map<Unit, UnitInfo *> &unitMap = getAllUnits();
					delete unitMap[unit];
					del_units.push_back(unit);
				}
				else {
					cout << "remove for change Unit Error Drone to building - " << unit->getID() << preType << curType << endl;
				}

				
				addUnitNBuilding(unit);
				continue;
			}
			else 
			{
				

				vector<UnitInfo *> &preUnitVector = getUnitVector(preType);
				vector<UnitInfo *> &curUnitVector = getUnitVector(curType);

				Unit unit = u.first;

				auto del_unit = find_if(preUnitVector.begin(), preUnitVector.end(), [unit](UnitInfo * up) {
					return up->unit() == unit;
				});

				if (del_unit != preUnitVector.end()) {
					BWEM::utils::fast_erase(preUnitVector, distance(preUnitVector.begin(), del_unit));
				}
				else {
					cout << "remove for change Unit Error unit to unit - " << unit->getID() << preType << curType << endl;
				}

				curUnitVector.push_back(u.second);
			}
		}

		u.second->Update();
	}

	for (auto &u : allBuildings)  
	{
		if (!u.first->exists())
		{
			u.second->Update();
			continue;
		}

		UnitType preType = u.second->type();
		UnitType curType = u.second->unit()->getType();

		if (preType != curType)
		{
			if (!curType.isBuilding()) 
			{
				
				vector<UnitInfo *> &Buildings = getBuildingVector(preType);

				Unit unit = u.first;

				auto del_unit = find_if(Buildings.begin(), Buildings.end(), [unit](UnitInfo * up) {
					return up->unit() == unit;
				});

				if (del_unit != Buildings.end()) {
					BWEM::utils::fast_erase(Buildings, distance(Buildings.begin(), del_unit));

					map<Unit, UnitInfo *> &buildingMap = getAllBuildings();
					delete buildingMap[unit];
					del_buildings.push_back(unit);
				}
				else {
					cout << "remove for change Unit Error building to unit" << endl;
				}

				
				addUnitNBuilding(unit);
				continue;
			}
			else 
			{
				
				vector<UnitInfo *> &preBuildingVector = getBuildingVector(preType);
				vector<UnitInfo *> &curBuildingVector = getBuildingVector(curType);

				Unit unit = u.first;

				auto del_unit = find_if(preBuildingVector.begin(), preBuildingVector.end(), [unit](UnitInfo * up) {
					return up->unit() == unit;
				});

				if (del_unit != preBuildingVector.end()) {
					BWEM::utils::fast_erase(preBuildingVector, distance(preBuildingVector.begin(), del_unit));

					if (curType == Resource_Vespene_Geyser) {
						map<Unit, UnitInfo *> &buildingMap = getAllBuildings();
						delete buildingMap[unit];
						del_buildings.push_back(unit);

						continue;
					}
				}
				else {
					cout << "remove for change Unit Error building to building" << endl;
				}

				if (curType != Resource_Vespene_Geyser) 
					curBuildingVector.push_back(u.second);
			}
		}

		u.second->Update();
	}

	
	for (auto u : del_units)
		getAllUnits().erase(u);

	for (auto u : del_buildings)
		getAllBuildings().erase(u);

	map<int, pair<int, Position>> &spellMap = getAllSpells();

	for (auto &b : bw->getBullets()) {
		
		if (b->getType() == BulletTypes::Invisible || b->getType() == BulletTypes::Psionic_Storm) {
			if (spellMap.find(b->getID()) != spellMap.end()) {
				spellMap[b->getID()] = make_pair(TIME, Positions::None);
			}
			
			else if (b->getSource() != nullptr && b->getSource()->getPlayer() == E) {
				uMap m = getAllUnits();
				int energy = b->getType() == (b->getSource()->getType() == Protoss_Arbiter && BulletTypes::Invisible) ? 100 : 75;
				m[b->getSource()]->recudeEnergy(energy);
				spellMap[b->getID()] = make_pair(TIME, Positions::None);
			}
		}
	}

	for (auto &s : spellMap) {
		if (s.second.first < TIME) {
			del_spells.push_back(s.first);
		}
	}

	for (auto u : del_spells)
		getAllSpells().erase(u);
}

UnitType UnitData::getUnitTypeDB(UnitType uType)
{
	if (uType == Terran_Siege_Tank_Siege_Mode)
		return Terran_Siege_Tank_Tank_Mode;

	if (uType == Zerg_Lurker_Egg)
		return Zerg_Lurker;

	return uType;
}