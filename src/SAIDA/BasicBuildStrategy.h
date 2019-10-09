#pragma once

#include "Common.h"
#include "ConstructionManager.h"
#include "InformationManager.h"
#include "ReserveBuilding.h"
#include "BuildManager.h"

#define EMB ESM.getEnemyMainBuild()
#define EIB ESM.getEnemyInitialBuild()

namespace MyBot
{
	class BasicBuildStrategy
	{
		/// 초반 빌드 실행을 위한 클래스
	private :
		// 이 프레임 카운트에 이르기 전까지는
		// SupppyUsed 값으로 빌드 타이밍을 계산한다.
		int frameCountFlag = 24 * 420;
		// 업그레이드 취소 금지
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

		// 초반 빌드 수행을 위한 함수
		// 초반 빌드는 주로 인구수를 Flag로 해서 수행 시점을 정한다.
		// 특정 건물의 첫번째를 지을지 말지 판단.
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
		// 본진 가스, 앞마당 가스를 지을 지 말지 결정하는 함수.
		bool canBuildRefinery(int supplyUsed, BuildOrderItem::SeedPositionStrategy seedPosition);
		bool canBuildRefinery(int supplyUsed, Base *base);
		// 앞마당 벙커 건설
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
		// until_frameCount 이전에 일정 supplyUsed 가 되었는지 체크
		bool checkSupplyUsedForInitialBuild(int supplyUsed, int until_frameCount);
		void buildBunkerDefence();
		bool isResearchingOrResearched(TechType techType, UpgradeType = UpgradeTypes::None);
		int getUnderConstructionCount(UnitType u, TilePosition p);
		int getExistOrUnderConstructionCount(UnitType u, Position p);
		void buildMissileTurrets(int count = 2);
		// 세컨트초크포인트에 터렛을 지을 기준 위치를 가져온다.
		TilePosition getSecondChokePointTurretPosition();

		void onInitialBuildChange(InitialBuildType pre, InitialBuildType cur);
		void onMainBuildChange(MainBuildType pre, MainBuildType cur);
	};

}