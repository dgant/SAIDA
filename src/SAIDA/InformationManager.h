#pragma once

#include "Common.h"
#include "UnitData.h"
#include "GridArea.h"
#include "ReserveBuilding.h"
#include "SaidaUtil.h"

#define INFO	InformationManager::Instance()
#define MYBASE INFO.getMainBaseLocation(S)->Center()
#define ENBASE INFO.getMainBaseLocation(E)->Center()

namespace MyBot
{
	enum TypeKind
	{
		AllUnitKind,
		AirUnitKind,
		GroundCombatKind, //나중에..
		GroundUnitKind,
		BuildingKind,
		AllDefenseBuildingKind,
		AirDefenseBuildingKind,
		GroundDefenseBuildingKind,
		AllKind
	};
	/// 게임 상황정보 중 일부를 자체 자료구조 및 변수들에 저장하고 업데이트하는 class<br>
	/// 현재 게임 상황정보는 Broodwar 를 조회하여 파악할 수 있지만, 과거 게임 상황정보는 Broodwar 를 통해 조회가 불가능하기 때문에 InformationManager에서 별도 관리하도록 합니다<br>
	/// 또한, Broodwar 나 BWEM 등을 통해 조회할 수 있는 정보이지만 전처리 / 별도 관리하는 것이 유용한 것도 InformationManager에서 별도 관리하도록 합니다
	class InformationManager
	{
		InformationManager();
		~InformationManager();

		void updateStartAndBaseLocation();

		/// 맵 플레이어수 (2인용 맵, 3인용 맵, 4인용 맵, 8인용 맵)
		int															mapPlayerLimit;

		/// Player - UnitData(각 Unit 과 그 Unit의 UnitInfo 를 Map 형태로 저장하는 자료구조) 를 저장하는 자료구조 객체<br>
		map<Player, UnitData>							_unitData;

		/// 해당 Player의 주요 건물들이 있는 BaseLocation. <br>
		/// 처음에는 StartLocation 으로 지정. mainBaseLocation 내 모든 건물이 파괴될 경우 재지정<br>
		/// 건물 여부를 기준으로 파악하기 때문에 부적절하게 판단할수도 있습니다
		map<Player, const BWEM::Base *>				_mainBaseLocations;

		/// 해당 Player의 mainBaseLocation 이 변경되었는가 (firstChokePoint, secondChokePoint, firstExpansionLocation 를 재지정 했는가)
		map<Player, bool>								_mainBaseLocationChanged;

		/// 해당 Player가 점령하고 있는 Area 이 있는 BaseLocation<br>
		/// 건물 여부를 기준으로 파악하기 때문에 부적절하게 판단할수도 있습니다
		map<Player, list<const BWEM::Base *> >	_occupiedBaseLocations;

		/// 해당 Player가 점령하고 있는 Area<br>
		/// 건물 여부를 기준으로 파악하기 때문에 부적절하게 판단할수도 있습니다
		map<Player, set<BWEM::Area *> >			_occupiedAreas;

		/// 해당 Player의 mainBaseLocation 에서 가장 가까운 ChokePoint
		map<Player, const ChokePoint *>					_firstChokePoint;
		/// 해당 Player의 mainBaseLocation 에서 가장 가까운 BaseLocation
		map<Player, BWEM::Base *>				_firstExpansionLocation;
		/// 해당 Player의 두번째 멀티
		map<Player, BWEM::Base *>				_secondExpansionLocation;
		/// 해당 Player의 세번째 멀티
		map<Player, BWEM::Base *>				_thirdExpansionLocation;
		/// 해당 Player의 섬 멀티
		map<Player, BWEM::Base *>				_islandExpansionLocation;
		/// 추가 멀티들
		vector<Base *>                          _additionalExpansions;
		/// 추가 멀티를 가져가기 위한 관리 map
		map<Base *, int>                        _expansionMap;
		/// 해당 Player의 mainBaseLocation 에서 두번째로 가까운 (firstChokePoint가 아닌) ChokePoint<br>
		/// 게임 맵에 따라서, secondChokePoint 는 일반 상식과 다른 지점이 될 수도 있습니다
		map<Player, const ChokePoint *>					_secondChokePoint;

		//맵의 모든 앞마당 베이스 로케이션
		map<BWEM::Base *, BWEM::Base *> _firstExpansionOfAllStartposition;

		// 베이스와 확장 사이에 Area 가 있는 경우 동일하게 취급하기 위해 저장.
		map<const BWEM::Area *, const BWEM::Area *> _mainBaseAreaPair;

		// Center Area를 N 등분 한 영역
		//		GridArea	*centerGridArea = nullptr;

		// Start & Base Location 관리
		vector<BWEM::Base *>	_startBaseLocations;
		vector<BWEM::Base *>	_allBaseLocations;

		uList _enemyInMyArea;
		uList _enemyInMyYard;

		/// 전체 unit 의 정보를 업데이트 합니다 (UnitType, lastPosition, HitPoint 등. 프레임당 1회 실행)
		void                    updateUnitsInfo();
		// enemy 전용 ( Hide, Show )

		void					updateBaseLocationInfo();
		void					updateSelfBaseLocationInfo();
		void					updateEnemyBaseLocationInfo();
		// ChokePoint 의 정보 업데이트 (적군 기지를 찾은 경우)
		void					updateChokePointInfo();

		// 초크포인트와 확장기지 정보 업데이트 (_mainBaseLocationChanged 가 true 인 경우 _firstChokePoint, _firstExpansionLocation, _secondChokePoint 정보를 업데이트한다.
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

		//		void					updateCenterGridArea();
		void					updateBaseInfo();
		set<TechType>			researchedSet;

		// FirstWaitLinePosition 설정, setFirstWaitLinePosition() 은 앞마당 먹으면 최초 1번만 호출
		Position                _firstWaitLinePosition = Positions::Unknown;
		void                    setFirstWaitLinePosition();
		map<TilePosition, Unit> baseSafeMineralMap;

		// 실제 채취중인 미네랄, 가스 Base 개수
		int                     activationMineralBaseCount;
		int                     activationGasBaseCount;
		void                    checkActivationBaseCount();

		// 추가 멀티들 커맨드 완성 되었을 때 리스트에 추가/삭제
		void                    addAdditionalExpansion(Unit depot);
		void                    removeAdditionalExpansion(Unit depot);

		// 적 본진의 스캔 위치
		vector<TilePosition>		scanPositionOfMainBase;
		word						scanIndex;

	public:
		// 현재 채취중인 미네랄/가스 멀티 개수
		int getActivationMineralBaseCount();
		int getActivationGasBaseCount();

		// 해당 위치 베이스의 남은 평균 미네랄
		int getAverageMineral(Position basePosition);

		Base *getFirstMulti(Player p, bool existGas = false, bool notCenter = false, int multiRank = 1);
		Position                getFirstWaitLinePosition();
		// BaseLocation 관련 함수
		Base *getBaseLocation(TilePosition pos);
		Base *getStartLocation(Player p);
		Base *getNearestBaseLocation(Position pos, bool groundDist = false);
		const Base 				*getNextSearchBase(bool reverse = false);

		//ChokePoint 관련 함수
		set<ChokePoint *> getMineChokePoints();

		vector<BWEM::Base *> getStartLocations() {
			return _startBaseLocations;
		}
		vector<BWEM::Base *> getBaseLocations() {
			return _allBaseLocations;
		}

		bool isConnected(Position a, Position b);

		/// static singleton 객체를 리턴합니다
		static InformationManager &Instance();

		Player       selfPlayer;		///< 아군 Player
		Race			selfRace;		///< 아군 Player의 종족
		Player       enemyPlayer;	///< 적군 Player
		Race			enemyRace;		///< 적군 Player의 종족
		Race		enemySelectRace;		///< 적군 Player가 선택한 종족

		//// 적 Base를 최종적으로 찾았는지 여부 판단
		bool	isEnemyBaseFound = false;

		/// Unit 및 BaseLocation, ChokePoint 등에 대한 정보를 업데이트합니다
		void                    update();

		void initReservedMineCount();

		/// Unit 에 대한 정보를 업데이트합니다
		void					onUnitShow(Unit unit);
		/// Unit 에 대한 정보를 업데이트합니다
		//		void					onUnitHide(Unit unit)        { updateUnitHide(unit, true); }
		/// Unit 에 대한 정보를 업데이트합니다
		void					onUnitCreate(Unit unit);
		/// Unit 에 대한 정보를 업데이트합니다
		void					onUnitComplete(Unit unit);
		/// 유닛이 파괴/사망한 경우, 해당 유닛 정보를 삭제합니다
		void					onUnitDestroy(Unit unit);

		/// 현재 맵의 최대 플레이어수 (2인용 맵, 3인용 맵, 4인용 맵, 8인용 맵) 을 리턴합니다
		int						getMapPlayerLimit() {
			return mapPlayerLimit;
		}
		Unit getSafestMineral(Unit scv);

		/// 해당 Player (아군 or 적군) 의 모든 유닛 통계 UnitData 을 리턴합니다
		UnitData 				&getUnitData(Player player) {
			return _unitData[player];
		}

		/// 해당 BaseLocation 에 player의 건물이 존재하는지 리턴합니다
		/// @param baseLocation 대상 BaseLocation
		/// @param player 아군 / 적군
		/// @param radius TilePosition 단위
		bool					hasBuildingAroundBaseLocation(BWEM::Base *baseLocation, Player player, int radius = 10);

		/// 해당 Area 에 해당 Player의 건물이 존재하는지 리턴합니다
		bool					existsPlayerBuildingInArea(const BWEM::Area *area, Player player);

		bool					isInFirstExpansionLocation(BWEM::Base *startLocation, BWAPI::TilePosition pos);

		/// 해당 Player (아군 or 적군) 가 건물을 건설해서 점령한 Area 목록을 리턴합니다
		set<BWEM::Area *>   &getOccupiedAreas(Player player);

		/// 해당 Player (아군 or 적군) 의 건물을 건설해서 점령한 BaseLocation 목록을 리턴합니다
		list<const BWEM::Base *> &getOccupiedBaseLocations(Player player);

		/// 해당 Player (아군 or 적군) 의 Main BaseLocation 을 리턴합니다
		const Base 	*getMainBaseLocation(Player player);

		/// 해당 Player (아군 or 적군) 의 Main BaseLocation 에서 가장 가까운 ChokePoint 를 리턴합니다
		const ChokePoint       *getFirstChokePoint(Player player);
		/// 해당 Player (아군 or 적군) 의 Main BaseLocation 에서 가장 가까운 ChokePoint 의 Position 을 리턴합니다. default middle
		Position               getFirstChokePosition(Player player, ChokePoint::node node = ChokePoint::node::middle) {
			return _firstChokePoint[player] == nullptr ? Positions::None : (Position)_firstChokePoint[player]->Pos(node);
		}

		/// 해당 Player (아군 or 적군) 의 Main BaseLocation 에서 가장 가까운 Expansion BaseLocation 를 리턴합니다
		Base            *getFirstExpansionLocation(Player player);
		Base			*getSecondExpansionLocation(Player player);
		Base			*getThirdExpansionLocation(Player player);
		Base			*getIslandExpansionLocation(Player player);
		vector<Base *>  getAdditionalExpansions();

		void            setAdditionalExpansions(Base *base);
		void            removeAdditionalExpansionData(Unit depot);
		/// 해당 Player (아군 or 적군) 의 Main BaseLocation 에서 두번째로 가까운 ChokePoint 를 리턴합니다
		/// 게임 맵에 따라서, secondChokePoint 는 일반 상식과 다른 지점이 될 수도 있습니다
		const ChokePoint       *getSecondChokePoint(Player player);
		/// 해당 Player (아군 or 적군) 의 Main BaseLocation 에서 가장 가까운 ChokePoint 의 Position 을 리턴합니다. default middle
		Position               getSecondChokePosition(Player player, ChokePoint::node node = ChokePoint::node::middle) {
			return _secondChokePoint[player] == nullptr ? Positions::None : (Position)_secondChokePoint[player]->Pos(node);
		}
		// 초크포인트 : 확장기지 거리의 m:n 의 지점을 리턴한다. (거리가 먼 경우만)
		Position               getSecondAverageChokePosition(Player player, int m = 1, int n = 1, int l = 204) {
			if (_secondChokePoint[player] == nullptr)
				return Positions::None;

			const pair<const Area *, const Area *> &areas = getSecondChokePoint(player)->GetAreas();
			const vector<ChokePoint> &secondCP = areas.first->ChokePoints(areas.second);
			Position pos = Positions::Origin;

			for (auto &cp : secondCP) {
				pos += (Position)cp.Center();
			}

			//cout << " 거리 =" << INFO.getFirstExpansionLocation(player)->getPosition().getApproxDistance((Position)INFO.getSecondChokePoint(player)->Center()) << endl;

			if (INFO.getFirstExpansionLocation(player)->getPosition().getApproxDistance((Position)INFO.getSecondChokePoint(player)->Center()) > l)
				pos = (pos * n + INFO.getFirstExpansionLocation(player)->getPosition() * m * secondCP.size()) / (secondCP.size() * (m + n));
			else
				pos /= secondCP.size();

			return pos;
		}

		TilePosition			getSecondChokePointBunkerPosition() 
		{
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

				cout << " 거리 = " << dist << endl;
				cout << " 초크포인트 위치 " << (right ? "우" : left ? "좌" : "") + (string)(down ? "하" : up ? "상" : "") << endl;
			}

			// 초크포인트 : 확장기지 거리의 m:n 의 지점을 리턴한다. (거리가 먼 경우만)
			int m = 0;
			int n = 0;

			// 좌 하 방향인 경우. (데스티네이션 12시, 포트리스 12시)
			if (left && down) {
				m = 3;
				n = 7;
			}
			// 하 방향 거리 먼 경우 (엠파이어 1시, 11시)
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

		TilePosition			getEnemySecondChokePointBunkerPosition()
		{
			if (INFO.getSecondChokePoint(E) == nullptr)
				return TilePositions::None;

			const pair<const Area *, const Area *> &areas = INFO.getSecondChokePoint(E)->GetAreas();
			const vector<ChokePoint> &secondCP = areas.first->ChokePoints(areas.second);
			Position pos = Positions::Origin;

			for (auto &cp : secondCP) {
				pos += (Position)cp.Center();
			}

			Position commandPos = INFO.getFirstExpansionLocation(E)->getPosition() + Position(32, 0);

			int dist = commandPos.getApproxDistance((Position)INFO.getSecondChokePoint(E)->Center());

			bw->drawDotMap(pos / secondCP.size(), Colors::Red);
			bw->drawDotMap(commandPos, Colors::Red);

			Position p1 = commandPos;
			Position p2 = pos / secondCP.size();

			bool up = p1.y - 2 * 32 >= p2.y;
			bool down = p1.y + 2 * 32 <= p2.y;
			bool left = p1.x - 6 * 32 >= p2.x;
			bool right = p1.x + 6 * 32 <= p2.x;

			if (TIME == 1) {

				cout << " 거리 = " << dist << endl;
				cout << " 초크포인트 위치 " << (right ? "우" : left ? "좌" : "") + (string)(down ? "하" : up ? "상" : "") << endl;
			}

			// 초크포인트 : 확장기지 거리의 m:n 의 지점을 리턴한다. (거리가 먼 경우만)
			int m = 0;
			int n = 0;

			// 좌 하 방향인 경우. (데스티네이션 12시, 포트리스 12시)
			if (left && down) {
				m = 3;
				n = 7;
			}
			// 하 방향 거리 먼 경우 (엠파이어 1시, 11시)
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


		// 두 지점이 같으면 0, p1 이 높으면 1, p2 가 높으면 -1, 유효하지 않은 인풋이면 -1.
		int					getAltitudeDifference(TilePosition p1 = INFO.getMainBaseLocation(S)->getTilePosition(), TilePosition p2 = TilePositions::None);

		Position getWaitingPositionAtFirstChoke(int min, int max);
		int getFirstChokeDefenceCnt();
		// Center Grid Area
		//GridArea *getCenterArea() {
		//	return centerGridArea;
		//};

		// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
		// getUnitAndUnitInfoMap 메소드에 대해 const 제거

		/// 해당 Player (아군 or 적군) 의 모든 유닛 목록 (가장 최근값) UnitAndUnitInfoMap 을 리턴합니다<br>
		/// 파악된 정보만을 리턴하기 때문에 적군의 정보는 틀린 값일 수 있습니다
		//		UnitAndUnitInfoMap &    getUnitAndUnitInfoMap(Player player);

		// BasicBot 1.1 Patch End //////////////////////////////////////////////////

		/// 해당 UnitType 이 전투 유닛인지 리턴합니다
		bool					isCombatUnitType(UnitType type) const;

		// 해당 종족의 UnitType 중 ResourceDepot 기능을 하는 UnitType을 리턴합니다
		UnitType			getBasicResourceDepotBuildingType(Race race = Races::None);

		// 해당 종족의 UnitType 중 Refinery 기능을 하는 UnitType을 리턴합니다
		UnitType			getRefineryBuildingType(Race race = Races::None);

		// 해당 종족의 UnitType 중 SupplyProvider 기능을 하는 UnitType을 리턴합니다
		UnitType			getBasicSupplyProviderUnitType(Race race = Races::None);

		// 해당 종족의 UnitType 중 Worker 에 해당하는 UnitType을 리턴합니다
		UnitType			getWorkerType(Race race = Races::None);

		// 해당 종족의 UnitType 중 Basic Combat Unit 에 해당하는 UnitType을 리턴합니다
		UnitType			getBasicCombatUnitType(Race race = Races::None);

		// 해당 종족의 UnitType 중 Basic Combat Unit 을 생산하기 위해 건설해야하는 UnitType을 리턴합니다
		UnitType			getBasicCombatBuildingType(Race race = Races::None);

		// 해당 종족의 UnitType 중 Advanced Combat Unit 에 해당하는 UnitType을 리턴합니다
		UnitType			getAdvancedCombatUnitType(Race race = Races::None);

		// 해당 종족의 UnitType 중 Observer 에 해당하는 UnitType을 리턴합니다
		UnitType			getObserverUnitType(Race race = Races::None);

		// 해당 종족의 UnitType 중 Advanced Depense 기능을 하는 UnitType을 리턴합니다
		UnitType			getAdvancedDefenseBuildingType(Race race = Races::None, bool isAirDefense = false);


		// UnitData 관련 API는 아래에서 정의한다.
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
