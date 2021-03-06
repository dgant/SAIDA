#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "../SaidaUtil.h"
#include "../InformationManager.h"

#define SUPER	Supervised::Instance()

namespace MyBot
{
	class Supervised
	{
		Supervised();
		~Supervised();

	public:
		static Supervised &Instance();
		void update(Player p);

		int getUnitAndBuildingCount(Player p);
		map<std::string, int **> getFeatureMap(Player p);
		std::list<UnitType> getUnitList(Player p);
		std::list<UnitType> getBuildingList(Player p);

	private:
		void updateMyFeatureMap();
		void updateEnemyFeatureMap();
		void makeMyFeatureMap();
		void makeEnemyFeatureMap();
		void clearAllFeatureMap();

		map<std::string, int **> featureMap_E;
		map<std::string, int **> featureMap_S;

		std::list<UnitType> unitListsTerran;
		std::list<UnitType> unitListsProtoss;
		std::list<UnitType> unitListsZerg;

		std::list<UnitType> buildingListsTerran;
		std::list<UnitType> buildingListsProtoss;
		std::list<UnitType> buildingListsZerg;
	};
}
