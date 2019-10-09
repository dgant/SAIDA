#pragma once
#include "Common.h"
#include "UnitData.h"
#include "GridArea.h"
#include "ReserveBuilding.h"
#include "StormbreakerUtil.h"

#define INFO	InformationManager::Instance()
#define MYBASE INFO.getMainBaseLocation(S)->Center()
#define ENBASE INFO.getMainBaseLocation(E)->Center()

namespace MyBot
{
	enum TypeKind
	{
		AllUnitKind,
		AirUnitKind,
		GroundCombatKind,
		GroundUnitKind,
		BuildingKind,
		AllDefenseBuildingKind,
		AirDefenseBuildingKind,
		GroundDefenseBuildingKind,
		AllKind
	};

	class InformationManager
	{
		InformationManager();
		~InformationManager();

		void updateStartAndBaseLocation();


		int															mapPlayerLimit;

		map<Player, UnitData>							_unitData;


		map<Player, const BWEM::Base *>				_mainBaseLocations;

		map<Player, bool>								_mainBaseLocationChanged;

		map<Player, list<const BWEM::Base *> >	_occupiedBaseLocations;

		map<Player, set<BWEM::Area *> >			_occupiedAreas;

		map<Player, const ChokePoint *>					_firstChokePoint;
		map<Player, BWEM::Base *>				_firstExpansionLocation;
		map<Player, BWEM::Base *>				_secondExpansionLocation;
		map<Player, BWEM::Base *>				_thirdExpansionLocation;
		map<Player, BWEM::Base *>				_islandExpansionLocation;
		vector<Base *>                          _additionalExpansions;
		map<Base *, int>                        _expansionMap;
		map<Player, const ChokePoint *>					_secondChokePoint;

		map<BWEM::Base *, BWEM::Base *> _firstExpansionOfAllStartposition;

		map<const BWEM::Area *, const BWEM::Area *> _mainBaseAreaPair;


		vector<BWEM::Base *>	_startBaseLocations;//存放开局基地
		vector<BWEM::Base *>	_allBaseLocations;//存放所有基地

		uList _enemyInMyArea;
		uList _enemyInMyYard;

		void                    updateUnitsInfo();

		void					updateBaseLocationInfo();
		void					updateSelfBaseLocationInfo();
		void					updateEnemyBaseLocationInfo();
		void					updateChokePointInfo();

		void					updateChangedMainBaseInfo(Player player);
		bool					updateChokePointAndExpansionLocation(const Area &baseArea, Player player, vector<const Area *> passedAreas = {});
		bool					checkPassedArea(const Area &area, vector<const Area *> passedAreas);
		void					updateOccupiedAreas(Player player);

		void					updateExpectedMultiBases();

		void					updateAllFirstExpansions();
		void                    updateSecondExpansion();
		void                    updateSecondExpansionForEnemy();
		void                    updateThirdExpansion();
		void                    updateIslandExpansion();

		bool					isEnemyScanResearched;
		void					checkEnemyScan();

		int						availableScanCount;

		void					updateBaseInfo();
		set<TechType>			researchedSet;

		Position                _firstWaitLinePosition = Positions::Unknown;
		void                    setFirstWaitLinePosition();
		map<TilePosition, Unit> baseSafeMineralMap;

		int                     activationMineralBaseCount;
		int                     activationGasBaseCount;
		void                    checkActivationBaseCount();

		void                    addAdditionalExpansion(Unit depot);
		void                    removeAdditionalExpansion(Unit depot);

		vector<TilePosition>		scanPositionOfMainBase;
		word						scanIndex;

	public:
		/*
		//人族单位
		int enemy_proxy_building_count = 0;
		int enemy_attacking_worker_count = 0;
		int enemy_cloaked_unit_count = 0;

		//通用建筑和单位
		int enemy_base_count = 0;
		int enemy_worker_count = 0;
		
		*/
		//人族建筑
		int enemy_supply_depot = 0;					//基地
		int enemy_refinery = 0;						//气矿
		int enemy_bunker_count = 0;					//地堡
		int enemy_barrack_count = 0;				//兵营
		int enemy_factory_count = 0;				//工厂
		int enemy_starport_count = 0;				//飞机场

		//神族单位
		int enemy_zealot_count = 0;					//狂热者
		int enemy_dragoon_count = 0;				//龙骑
		int enemy_ht_count = 0;						//光明圣堂
		int enemy_dt_count = 0;						//黑暗圣堂
		int enemy_reaver_count = 0;					//金甲虫
		int enemy_shuttle_count = 0;				//运输机
		int enemy_carrier_count = 0;				//航母
		int enemy_arbiter_count = 0;				//仲裁者
		int enemy_corsair_count = 0;				//海盗船
		int enemy_scout_count = 0;					//侦察机
		int enemy_archon_count = 0;					//光明执政官
		int enemy_dark_archon_count = 0;			//黑暗执政官

		//神族建筑
		int enemy_cannon_count = 0;					//光子炮塔
		int enemy_assimilator = 0;					//气矿
		int enemy_forge = 0;						//锻炉
		int enemy_gateway_count = 0;				//兵营
		int enemy_stargate_count = 0;				//星门
		int enemy_robotics_facility_count = 0;		//机械工厂

		//虫族单位
		int enemy_zergling_count = 0;				//小狗
		int enemy_hydralisk_count = 0;				//刺蛇
		int enemy_lurker_count = 0;					//地刺
		int getActivationMineralBaseCount();
		int getActivationGasBaseCount();

		int getAverageMineral(Position basePosition);

		Base *getFirstMulti(Player p, bool existGas = false, bool notCenter = false, int multiRank = 1);
		Position                getFirstWaitLinePosition();
		Base *getBaseLocation(TilePosition pos);
		Base *getStartLocation(Player p);
		Base *getNearestBaseLocation(Position pos, bool groundDist = false);
		const Base 				*getNextSearchBase(bool reverse = false);

		set<ChokePoint *> getMineChokePoints();

		vector<BWEM::Base *> getStartLocations() {
			return _startBaseLocations;
		}
		vector<BWEM::Base *> getBaseLocations() {
			return _allBaseLocations;
		}

		bool isConnected(Position a, Position b);

		static InformationManager &Instance();

		Player       selfPlayer;
		Race			selfRace;
		Player       enemyPlayer;
		Race			enemyRace;

		bool	isEnemyBaseFound = false;

		void                    update();

		void initReservedMineCount();

		void					onUnitShow(Unit unit);
		void					onUnitCreate(Unit unit);

		void					onUnitComplete(Unit unit);

		void					onUnitDestroy(Unit unit);

		int						getMapPlayerLimit() {
			return mapPlayerLimit;
		}
		Unit getSafestMineral(Unit scv);


		UnitData 				&getUnitData(Player player) {
			return _unitData[player];
		}


		bool					hasBuildingAroundBaseLocation(BWEM::Base *baseLocation, Player player, int radius = 10);


		bool					existsPlayerBuildingInArea(const BWEM::Area *area, Player player);

		bool					isInFirstExpansionLocation(BWEM::Base *startLocation, BWAPI::TilePosition pos);

		set<BWEM::Area *>   &getOccupiedAreas(Player player);

		list<const BWEM::Base *> &getOccupiedBaseLocations(Player player);

		const Base 	*getMainBaseLocation(Player player);

		const ChokePoint       *getFirstChokePoint(Player player);
		Position               getFirstChokePosition(Player player, ChokePoint::node node = ChokePoint::node::middle) {
			return _firstChokePoint[player] == nullptr ? Positions::None : (Position)_firstChokePoint[player]->Pos(node);
		}

		Base            *getFirstExpansionLocation(Player player);
		Base			*getSecondExpansionLocation(Player player);
		Base			*getThirdExpansionLocation(Player player);
		Base			*getIslandExpansionLocation(Player player);
		vector<Base *>  getAdditionalExpansions();

		void            setAdditionalExpansions(Base *base);
		void            removeAdditionalExpansionData(Unit depot);
		const ChokePoint       *getSecondChokePoint(Player player);
		Position               getSecondChokePosition(Player player, ChokePoint::node node = ChokePoint::node::middle) {
			return _secondChokePoint[player] == nullptr ? Positions::None : (Position)_secondChokePoint[player]->Pos(node);
		}
		Position               getSecondAverageChokePosition(Player player, int m = 1, int n = 1, int l = 204) {
			if (_secondChokePoint[player] == nullptr)
				return Positions::None;

			const pair<const Area *, const Area *> &areas = getSecondChokePoint(player)->GetAreas();
			const vector<ChokePoint> &secondCP = areas.first->ChokePoints(areas.second);
			Position pos = Positions::Origin;

			for (auto &cp : secondCP) {
				pos += (Position)cp.Center();
			}


			if (INFO.getFirstExpansionLocation(player)->getPosition().getApproxDistance((Position)INFO.getSecondChokePoint(player)->Center()) > l)
				pos = (pos * n + INFO.getFirstExpansionLocation(player)->getPosition() * m * secondCP.size()) / (secondCP.size() * (m + n));
			else
				pos /= secondCP.size();

			return pos;
		}

		TilePosition			getSecondChokePointBunkerPosition() {
			if (INFO.getSecondChokePoint(S) == nullptr)
				return TilePositions::None;

			const pair<const Area *, const Area *> &areas = INFO.getSecondChokePoint(S)->GetAreas();
			const vector<ChokePoint> &secondCP = areas.first->ChokePoints(areas.second);
			Position pos = Positions::Origin;

			for (auto &cp : secondCP) {
				pos += (Position)cp.Center();
			}

			Position commandPos = INFO.getFirstExpansionLocation(S)->getPosition() + Position(32, 0);

			int dist = commandPos.getApproxDistance((Position)INFO.getSecondChokePoint(S)->Center());

			bw->drawDotMap(pos / secondCP.size(), Colors::Red);
			bw->drawDotMap(commandPos, Colors::Red);

			Position p1 = commandPos;
			Position p2 = pos / secondCP.size();

			bool up = p1.y - 2 * 32 >= p2.y;
			bool down = p1.y + 2 * 32 <= p2.y;
			bool left = p1.x - 6 * 32 >= p2.x;
			bool right = p1.x + 6 * 32 <= p2.x;

			if (TIME == 1) {

			}

			int m = 0;
			int n = 0;

			if (left && down) {
				m = 3;
				n = 7;
			}
			else if (down && dist > 300) {
				m = 3;
				n = 7;
			}
			else if (dist <= 260) {
				m = 1;
				n = 1;
			}
			else {
				m = 3;
				n = 5;
			}

			pos = (pos * n + commandPos * m * secondCP.size()) / (secondCP.size() * (m + n));

			bw->drawDotMap(pos, Colors::Blue);

			return (TilePosition)pos;
		}

		int					getAltitudeDifference(TilePosition p1 = INFO.getMainBaseLocation(S)->getTilePosition(), TilePosition p2 = TilePositions::None);

		Position getWaitingPositionAtFirstChoke(int min, int max);
		int getFirstChokeDefenceCnt();

		bool					isCombatUnitType(UnitType type) const;

		UnitType			getBasicResourceDepotBuildingType(Race race = Races::None);

		UnitType			getRefineryBuildingType(Race race = Races::None);

		UnitType			getBasicSupplyProviderUnitType(Race race = Races::None);

		UnitType			getWorkerType(Race race = Races::None);

		UnitType			getBasicCombatUnitType(Race race = Races::None);

		UnitType			getBasicCombatBuildingType(Race race = Races::None);

		UnitType			getAdvancedCombatUnitType(Race race = Races::None);

		UnitType			getObserverUnitType(Race race = Races::None);
		UnitType			getAdvancedDefenseBuildingType(Race race = Races::None, bool isAirDefense = false);


		UnitInfo *getUnitInfo(Unit unit, Player p);
		uList getUnits(UnitType t, Player p) {
			return _unitData[p].getUnitVector(t);
		}
		uList getBuildings(UnitType t, Player p) {
			return _unitData[p].getBuildingVector(t);
		}
		uMap &getUnits(Player p) {
			return _unitData[p].getAllUnits();
		}
		uMap &getBuildings(Player p) {
			return _unitData[p].getAllBuildings();
		}
		int			getCompletedCount(UnitType t, Player p);
		int			getDestroyedCount(UnitType t, Player p);
		map<UnitType, int> getDestroyedCountMap(Player p);
		int			getAllCount(UnitType t, Player p);

		uList getUnitsInRadius(Player p, Position pos = Positions::Origin, int radius = 0, bool ground = true, bool air = true, bool worker = true, bool hide = false, bool groundDist = false);
		uList getBuildingsInRadius(Player p, Position pos = Positions::Origin, int radius = 0, bool ground = true, bool air = true, bool hide = false, bool groundDist = false);
		uList getAllInRadius(Player p, Position pos = Positions::Origin, int radius = 0, bool ground = true, bool air = true, bool hide = false, bool groundDist = false);
		uList getUnitsInRectangle(Player p, Position leftTop, Position rightDown, bool ground = true, bool air = true, bool worker = true, bool hide = false);
		uList getBuildingsInRectangle(Player p, Position leftTop, Position rightDown, bool ground = true, bool air = true, bool hide = false);
		uList getAllInRectangle(Player p, Position leftTop, Position rightDown, bool ground = true, bool air = true, bool hide = false);
		uList getTypeUnitsInRadius(UnitType t, Player p, Position pos = Positions::Origin, int radius = 0, bool hide = false);
		uList getTypeBuildingsInRadius(UnitType t, Player p, Position pos = Positions::Origin, int radius = 0, bool incomplete = true, bool hide = true);
		uList getDefenceBuildingsInRadius(Player p, Position pos = Positions::Origin, int radius = 0, bool incomplete = true, bool hide = true);
		uList getTypeUnitsInRectangle(UnitType t, Player p, Position leftTop, Position rightDown, bool hide = false);
		uList getTypeBuildingsInRectangle(UnitType t, Player p, Position leftTop, Position rightDown, bool incomplete = true, bool hide = true);
		uList getUnitsInArea(Player p, Position pos, bool ground = true, bool air = true, bool worker = true, bool hide = true);
		uList getBuildingsInArea(Player p, Position pos, bool ground = true, bool air = true, bool hide = true);
		uList getTypeUnitsInArea(UnitType t, Player p, Position pos, bool hide = false);
		uList getTypeBuildingsInArea(UnitType t, Player p, Position pos, bool incomplete = true, bool hide = true);

		UnitInfo *getClosestUnit(Player p, Position pos, TypeKind kind = TypeKind::AllKind, int radius = 0, bool worker = false, bool hide = false, bool groundDist = false, bool detectedOnly = true);
		UnitInfo *getFarthestUnit(Player p, Position pos, TypeKind kind = TypeKind::AllKind, int radius = 0, bool worker = false, bool hide = false, bool groundDist = false, bool detectedOnly = true);
		UnitInfo *getClosestTypeUnit(Player p, Position pos, UnitType type, int radius = 0, bool hide = false, bool groundDist = false, bool detectedOnly = true);
		UnitInfo *getClosestTypeUnit(Player p, Unit my, UnitType type, int radius = 0, bool hide = false, bool groundDist = false, bool detectedOnly = true);
		UnitInfo *getClosestTypeUnit(Player p, Position pos, vector<UnitType> &types, int radius = 0, bool hide = false, bool groundDist = false, bool detectedOnly = true);
		UnitInfo *getFarthestTypeUnit(Player p, Position pos, UnitType type, int radius = 0, bool hide = false, bool groundDist = false, bool detectedOnly = true);

		bool getEnemyScanResearched() {
			return isEnemyScanResearched;
		}
		int getAvailableScanCount() {
			return availableScanCount;
		}
		void setAvailableScanCount(int scanCount) {
			availableScanCount = scanCount;
		}
		bool isBaseSafe(const Base *base);
		bool isBaseHasResourses(const Base *base, int mineral, int gas);
		Base *getClosestSafeEmptyBase(UnitInfo *c);
		bool isMainBasePairArea(const Area *a1, const Area *a2);
		const Area *getMainBasePairArea(const Area *area) {
			return _mainBaseAreaPair[area];
		}
		bool isTimeToMoveCommandCenterToFirstExpansion();
		bool hasRearched(TechType tech);
		void setRearched(UnitType unitType);
		void checkEnemyInMyArea();
		uList enemyInMyArea() {
			return _enemyInMyArea;
		}
		uList enemyInMyYard() {
			return _enemyInMyYard;
		}
		void checkMoveInside();
		bool needMoveInside() {
			return _needMoveInside;
		}
		bool _needMoveInside = false;
		void setNextScanPointOfMainBase();
		TilePosition getScanPositionOfMainBase();

		int nucLaunchedTime = 0;
	};
}
