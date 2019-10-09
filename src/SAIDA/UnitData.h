#pragma once

#include "Common.h"
#include "UnitInfo.h"

#define uList vector<UnitInfo*>
#define uMap map<Unit, UnitInfo*>

namespace MyBot
{
	class UnitData
	{
	public:
		UnitData();
		~UnitData();

		vector<UnitInfo *> &getUnitVector(UnitType);
		vector<UnitInfo *> &getBuildingVector(UnitType);

		map<Unit, UnitInfo *> &getAllUnits() {
			return allUnits;
		}
		map<Unit, UnitInfo *> &getAllBuildings() {
			return allBuildings;
		}
		map<int, pair<int, Position>> &getAllSpells() {
			return allSpells;
		}

		map<UnitType, vector<UnitInfo *> > &getUnitTypeMap() {
			return unitTypeMap;
		}
		map<UnitType, vector<UnitInfo *> > &getBuildingTypeMap() {
			return buildingTypeMap;
		}
		map<UnitType, int> &getCompletedCount() {
			return completedCount;
		}
		map<UnitType, int> &getDestroyedCount() {
			return destroyedCount;
		}
		map<UnitType, int> &getAllCount() {
			return allCount;
		}

		int getCompletedCount(UnitType);
		int getDestroyedCount(UnitType);
		map<UnitType, int> getDestroyedCountMap();
		int getAllCount(UnitType);

		void increaseCompleteUnits(UnitType);
		void increaseDestroyUnits(UnitType);
		void increaseCreateUnits(UnitType);
		void decreaseCompleteUnits(UnitType);
		void decreaseCreateUnits(UnitType);

		bool addUnitNBuilding(Unit);
		void removeUnitNBuilding(Unit);
		
		void initializeAllInfo();
		void updateAllInfo();
		void updateNcheckTypeAllInfo();

	private:
		map<UnitType, vector<UnitInfo *> > unitTypeMap;
		map<UnitType, vector<UnitInfo *> > buildingTypeMap;

		map<Unit, UnitInfo *> allUnits;
		map<Unit, UnitInfo *> allBuildings;
		map<int, pair<int, Position>> allSpells;

		
		map<UnitType, int> completedCount;
		
		map<UnitType, int> destroyedCount;
		
		map<UnitType, int> allCount;

		UnitType getUnitTypeDB(UnitType);
	};

	class UListSet
	{
	public:
		Position getPos() {
			Position avgPos(Positions::Origin);

			if (units.size() != 0)
			{
				for (auto u : units) {
					avgPos += u->pos();
				}

				avgPos /= units.size();
			}

			return avgPos;
		}

		void add(UnitInfo *uInfo) {
			if (units.size()) {
				auto add_unit = find_if(units.begin(), units.end(), [uInfo](UnitInfo * up) {
					return up == uInfo;
				});

				if (add_unit == units.end())
					units.push_back(uInfo);
			}
			else
				units.push_back(uInfo);
		}

		void del(Unit u) {
			if (units.size()) {
				auto del_unit = find_if(units.begin(), units.end(), [u](UnitInfo * up) {
					return up->unit() == u;
				});

				if (del_unit != units.end()) {
					utils::fast_erase(units, distance(units.begin(), del_unit));
				}
			}
		}
		void del(UnitInfo *uInfo) {
			if (units.size()) {
				auto del_unit = find_if(units.begin(), units.end(), [uInfo](UnitInfo * up) {
					return up == uInfo;
				});

				if (del_unit != units.end()) {
					utils::fast_erase(units, distance(units.begin(), del_unit));
				}
			}
		}

		word size() {
			return units.size();
		}
		uList &getUnits() {
			return units;
		}
		void clear() {
			units.clear();
		}
		bool isEmpty() {
			return units.empty();
		}

		UnitInfo *getFrontUnitFromPosition(Position t) {
			int distance = INT_MAX;
			int temp = 0;
			UnitInfo *frontUnit = nullptr;

			for (auto &u : units) {
				theMap.GetPath(u->pos(), t, &temp);

				if (temp >= 0 && temp < distance) {
					frontUnit = u;
					distance = temp;
				}
			}

			return frontUnit;
		}

		
		uList getSortedUnitList(Position targetPos, bool reverseOrder = false) {

			vector<pair<int, UnitInfo * >> sortList;

			for (auto t : units)
			{
				int tempDist = 0;
				theMap.GetPath(t->pos(), targetPos, &tempDist);

				
				if (tempDist < 0)
					continue;

				sortList.push_back(pair<int, UnitInfo * >(tempDist, t));
			}

			if (reverseOrder) {
				sort(sortList.begin(), sortList.end(), [](pair<int, UnitInfo *> a, pair<int, UnitInfo *> b) {
					return a.first > b.first;
				});
			}
			else {
				sort(sortList.begin(), sortList.end(), [](pair<int, UnitInfo *> a, pair<int, UnitInfo *> b) {
					return a.first < b.first;
				});
			}

			uList sortedList;

			for (word i = 0; i < sortList.size(); i++) {
				sortedList.push_back(sortList[i].second);
			}

			return sortedList;
		}

	private:
		uList units;
	};
}