#pragma once

#include "../Common.h"
#include "ConstructionManager.h"
#include "../InformationManager.h"
#include "../ReserveBuilding.h"
#include "BuildManager.h"

#define EMB ESM.getEnemyMainBuild()
#define EIB ESM.getEnemyInitialBuild()

namespace MyBot
{
	class BasicBuildStrategy
	{
	private :
		int frameCountFlag = 24 * 420;
	
		bool notCancel;
		TilePosition secondCommandPosition = TilePositions::None;
		TilePosition turretposOnTheHill = TilePositions::None;

		Base *preBase = nullptr;

	public:
		BasicBuildStrategy();
		~BasicBuildStrategy();

		TilePosition getSecondCommandPosition(bool force = false);
		TilePosition getTurretPositionOnTheHill();
		bool cancelSupplyDepot;
		bool canceledBunker;
		int getFrameCountFlag();
		void setFrameCountFlag(int frameCountFlag);
		static BasicBuildStrategy &Instance();
		void executeBasicBuildStrategy();
		void cancelBunkerAtMainBase();
		void cancelFirstExpansion();
		void cancelFirstMachineShop(InitialBuildType eib, MainBuildType emb);
		void buildTurretFromAtoB(Position a, Position b);
		void TerranVulutreTankWraithBuild();
		void TerranTankGoliathBuild();

		bool canBuildFirstBarrack(int supplyUsed);
		bool canBuildFirstFactory(int supplyUsed);
		bool canBuildFirstExpansion(int supplyUsed);
		bool canBuildSecondExpansion(int supplyUsed);
		bool canBuildThirdExpansion(int supplyUsed);
		bool canBuildAdditionalExpansion(int supplyUsed);
		bool canBuildFirstStarport(int supplyUsed);
		bool canBuildFirstTurret(int supplyUsed);
		bool canBuildFirstArmory(int supplyUsed);
		bool canBuildSecondArmory(int supplyUsed);
		bool canBuildFirstBunker(int supplyUsed);
		bool canBuildFirstEngineeringBay(int supplyUsed);
		bool canBuildFirstAcademy(int supplyUsed);
		bool canBuildFirstScienceFacility(int supplyUsed);
		bool canBuildadditionalFactory();
		bool canBuildMissileTurret(TilePosition p, int buildCnt = 1);

		bool canBuildRefinery(int supplyUsed, BuildOrderItem::SeedPositionStrategy seedPosition);
		bool canBuildRefinery(int supplyUsed, Base *base);
		bool canBuildSecondChokePointBunker(int supplyUsed);

		bool hasRefinery(BuildOrderItem::SeedPositionStrategy seedPosition);
		bool hasRefinery(Base *base);
		bool hasEnoughResourcesToBuild(UnitType unitType);
		bool hasSecondCommand();
		bool isExistOrUnderConstruction(UnitType unitType);
		bool isUnderConstruction(UnitType unitType);
		bool isTimeToMachineShopUpgrade(TechType techType, UpgradeType = UpgradeTypes::None);
		bool isTimeToControlTowerUpgrade(TechType techType);
		bool isTimeToScienceFacilityUpgrade(TechType techType);
		bool isTimeToArmoryUpgrade(UpgradeType upgradeType, int level = 1);
		bool checkSupplyUsedForInitialBuild(int supplyUsed, int until_frameCount);
		void buildBunkerDefence();
		bool isResearchingOrResearched(TechType techType, UpgradeType = UpgradeTypes::None);
		int getUnderConstructionCount(UnitType u, TilePosition p);
		int getExistOrUnderConstructionCount(UnitType u, Position p);
		void buildMissileTurrets(int count = 2);
		TilePosition getSecondChokePointTurretPosition();

		void onInitialBuildChange(InitialBuildType pre, InitialBuildType cur);
		void onMainBuildChange(MainBuildType pre, MainBuildType cur);
	};

}