//////////////////////////////////////////////////////////////////////////
//
// This file is part of the BWEM Library.
// BWEM is free software, licensed under the MIT/X11 License.
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2015, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#ifndef BWEM_BASE_H
#define BWEM_BASE_H

#include <BWAPI.h>
#include "utils.h"
#include "defs.h"
#include <iostream>
#include <utility>

using namespace std;
namespace BWEM {

	class Ressource;
	class Mineral;
	class Geyser;
	class Area;
	class Map;

	//[현진] base관리를 위한 멤버 변수 추가
	enum baseOccupiedInfo { emptyBase, myBase, enemyBase, shareBase };

	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class Base
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////
	//
	// After Areas and ChokePoints, Bases are the third kind of object BWEM automatically computes from Brood War's maps.
	// A Base is essentially a suggested location (intended to be optimal) to put a Command Center, Nexus, or Hatchery.
	// It also provides information on the ressources available, and some statistics.
	// A Base alway belongs to some Area. An Area may contain zero, one or several Bases.
	// Like Areas and ChokePoints, the number and the addresses of Base instances remain unchanged.
	//
	// Bases inherit utils::UserData, which provides free-to-use data.

	class Base : public utils::UserData
	{
	public:


		// Tells whether this Base's location is contained in Map::StartingLocations()
		// Note: all players start at locations taken from Map::StartingLocations(),
		//       which doesn't mean all the locations in Map::StartingLocations() are actually used.
		bool							Starting() const			{
			return m_starting;
		}

		// Returns the Area this Base belongs to.
		const Area 					*GetArea() const				{
			return m_pArea;
		}

		// Returns the location of this Base (top left Tile position).
		// If Starting() == true, it is guaranteed that the loction corresponds exactly to one of Map::StartingLocations().
		const BWAPI::TilePosition 		&Location() const			{
			return m_location;
		}
		const BWAPI::TilePosition 		&getTilePosition() const {
			return m_location;
		}

		// Returns the location of this Base (center in pixels).
		const BWAPI::Position 			&Center() const				{
			return m_center;
		}
		const BWAPI::Position 			&getPosition() const {
			return m_center;
		}

		// Returns the available Minerals.
		// These Minerals are assigned to this Base (it is guaranteed that no other Base provides them).
		// Note: The size of the returned list may decrease, as some of the Minerals may get destroyed.
		const std::vector<Mineral *> 	&Minerals() const			{
			return m_Minerals;
		}

		// Returns the available Geysers.
		// These Geysers are assigned to this Base (it is guaranteed that no other Base provides them).
		// Note: The size of the returned list may NOT decrease, as Geysers never get destroyed.
		const std::vector<Geyser *> 	&Geysers() const				{
			return m_Geysers;
		}

		// Returns the blocking Minerals.
		// These Minerals are special ones: they are placed at the exact location of this Base (or very close),
		// thus blocking the building of a Command Center, Nexus, or Hatchery.
		// So before trying to build this Base, one have to finish gathering these Minerals first.
		// Fortunately, these are guaranteed to have their InitialAmount() <= 8.
		// As an example of blocking Minerals, see the two islands in Andromeda.scx.
		// Note: if Starting() == true, an empty list is returned.
		// Note Base::BlockingMinerals() should not be confused with ChokePoint::BlockingNeutral() and Neutral::Blocking():
		//      the last two refer to a Neutral blocking a ChokePoint, not a Base.
		const std::vector<Mineral *> 	&BlockingMinerals() const	{
			return m_BlockingMinerals;
		}

		Base 							&operator=(const Base &) = delete;

		////////////////////////////////////////////////////////////////////////////
		//	Details: The functions below are used by the BWEM's internals

		Base(Area *pArea, const BWAPI::TilePosition &location, const std::vector<Ressource *> &AssignedRessources, const std::vector<Mineral *> &BlockingMinerals);
		Base(const Base &Other);
		void							SetStartingLocation(const BWAPI::TilePosition &actualLocation);
		void							OnMineralDestroyed(const Mineral *pMineral);

		////////////////////////////////////////////////////////////////////////////
		// SAIDA 추가
		bool							isIsland() const;
		// (starting point only) 미네랄 뒤쪽에 공간이 있으면 true
		bool							isExistBackYard() const {
			return m_isExistBackYard;
		}
		void							setIsExistBackYard(bool isExist) const {
			m_isExistBackYard = isExist;
		}

		std::pair<BWAPI::Unit, BWAPI::Unit>	&getEdgeMinerals() const {
			return m_edgeMinerals;
		}

		bool							isMineralsPlacedVertical() const {
			return m_isMineralsPlacedVertical;
		}
		void							setIsMineralsPlacedVertical() const {
			m_isMineralsPlacedVertical = m_leftMineral->getPosition().x + m_bottomMineral->getPosition().y > m_rightMineral->getPosition().x + m_topMineral->getPosition().y;

			if (m_isMineralsPlacedVertical)
				m_edgeMinerals = make_pair(m_topMineral, m_bottomMineral);
			else
				m_edgeMinerals = make_pair(m_leftMineral, m_rightMineral);
		}

		// are there minerals on the left/right/top/bottom side of the base
		bool							isMineralsLeftOfBase() const {
			return m_isMineralsLeftOfBase;
		}
		bool							isMineralsRightOfBase() const {
			return !m_isMineralsLeftOfBase;
		}
		bool							isMineralsTopOfBase() const {
			return m_isMineralsTopOfBase;
		}
		bool							isMineralsBottomOfBase() const {
			return !m_isMineralsTopOfBase;
		}

		// is there base on the left/right/top/bottom side of the area
		bool							isBaseLeftOfArea() const {
			return m_isBaseLeftOfArea;
		}
		bool							isBaseRightOfArea() const {
			return !m_isBaseLeftOfArea;
		}
		bool							isBaseTopOfArea() const {
			return m_isBaseTopOfArea;
		}
		bool							isBaseBottomOfArea() const {
			return !m_isBaseTopOfArea;
		}
		void							setBaseLocationInfo() const;

		BWAPI::Unit						getTopMineral() const {
			return m_topMineral;
		}
		void							setTopMineral(BWAPI::Unit mineral) const {
			m_topMineral = mineral;
		}
		BWAPI::Unit						getLeftMineral() const {
			return m_leftMineral;
		}
		void							setLeftMineral(BWAPI::Unit mineral) const {
			m_leftMineral = mineral;
		}
		BWAPI::Unit						getRightMineral() const {
			return m_rightMineral;
		}
		void							setRightMineral(BWAPI::Unit mineral) const {
			m_rightMineral = mineral;
		}
		BWAPI::Unit						getBottomMineral() const {
			return m_bottomMineral;
		}
		void							setBottomMineral(BWAPI::Unit mineral) const {
			m_bottomMineral = mineral;
		}

		// 미네랄과 베이스의 중심 지점을 가져온다.
		BWAPI::Position					getMineralBaseCenter() const {
			return m_mineral_center;
		}
		void							setMineralBaseCenter(BWAPI::Position center) const {
			m_mineral_center = center;
		}

		int								GetWorkerCount() const {
			return m_workerCount;
		}
		void							SetWorkerCount(int count) const {
			m_workerCount = count;
		}
		int								GetMineCount() const {
			return m_mineCount;
		}
		void							SetMineCount(int count) const {
			m_mineCount = count;
		}
		int								GetReservedMineCount() const {
			return m_reservedMineCount;
		}
		void							SetReservedMineCount(int count) const {
			m_reservedMineCount = count;
		}
		void							AddReservedMineCount() const {
			m_reservedMineCount++;
		}
		enum baseOccupiedInfo			GetOccupiedInfo() const {
			return m_occupiedInfo;
		}
		void							SetOccupiedInfo(enum baseOccupiedInfo info) const {
			m_occupiedInfo = info;
		}
		int								GetExpectedEnemyMultiRank() {
			return m_expectedEnemyMultiRank;
		}
		void							SetExpectedEnemyMultiRank(int rank) const {
			m_expectedEnemyMultiRank = rank;
		}
		int								GetExpectedMyMultiRank() {
			return m_expectedMyMultiRank;
		}
		void							SetExpectedMyMultiRank(int rank) const {
			m_expectedMyMultiRank = rank;
		}
		bool							GetIsEnemyAround() const {
			return m_isEnemyAround;
		}
		void							SetIsEnemyAround(bool flag) const {
			m_isEnemyAround = flag;
		}
		bool							GetDangerousAreaForWorkers() const {
			return m_dangerousAreaForWorkers;
		}
		void							SetDangerousAreaForWorkers(bool flag) const {
			m_dangerousAreaForWorkers = flag;
		}
		int								GetSelfAirDefenseBuildingCount() const {
			return m_s_AirDefenseBuildingCount;
		}
		void							SetSelfAirDefenseBuildingCount(int count) const {
			m_s_AirDefenseBuildingCount = count;
		}
		int								GetEnemyAirDefenseBuildingCount() const {
			return m_e_AirDefenseBuildingCount;
		}
		void							SetEnemyAirDefenseBuildingCount(int count) const {
			m_e_AirDefenseBuildingCount = count;
		}
		int								GetSelfGroundDefenseBuildingCount() const {
			return m_s_GroundDefenseBuildingCount;
		}
		void							SetSelfGroundDefenseBuildingCount(int count) const {
			m_s_GroundDefenseBuildingCount = count;
		}
		int								GetEnemyGroundDefenseBuildingCount() const {
			return m_e_GroundDefenseBuildingCount;
		}
		void							SetEnemyGroundDefenseBuildingCount(int count) const {
			m_e_GroundDefenseBuildingCount = count;
		}
		int								GetSelfAirDefenseUnitCount() const {
			return m_s_AirDefenseUnitCount;
		}
		void							SetSelfAirDefenseUnitCount(int count) const {
			m_s_AirDefenseUnitCount = count;
		}
		int								GetEnemyAirDefenseUnitCount() const {
			return m_e_AirDefenseUnitCount;
		}
		void							SetEnemyAirDefenseUnitCount(int count) const {
			m_e_AirDefenseUnitCount = count;
		}
		int								GetSelfGroundDefenseUnitCount() const {
			return m_s_GroundDefenseUnitCount;
		}
		void							SetSelfGroundDefenseUnitCount(int count) const {
			m_s_GroundDefenseUnitCount = count;
		}
		int								GetEnemyGroundDefenseUnitCount() const {
			return m_e_GroundDefenseUnitCount;
		}
		void							SetEnemyGroundDefenseUnitCount(int count) const {
			m_e_GroundDefenseUnitCount = count;
		}
		int								GetEnemyBunkerCount() const {
			return m_e_BunkerCount;
		}
		void							SetEnemyBunkerCount(int count) const {
			m_e_BunkerCount = count;
		}
		int								GetLastVisitedTime() const {
			return m_lastVisitedTime;
		}
		void							SetLastVisitedTime(int visitTime) const {
			m_lastVisitedTime = visitTime;
		}
		// creep 은 초반 정찰을 위해서만 사용이 됨. (event 가 발생하지 않기 때문에 매 프레임 체크하는것은 비효율적이기 때문)
		int								GetLastCreepFoundTime() const {
			return m_lastCreepFoundTime;
		}
		bool							GetHasCreepAroundBase() const {
			return m_hasCreepAroundBase;
		}
		void							SetHasCreepAroundBase(bool hasCreep, int time = 0) const {
			m_hasCreepAroundBase = m_hasCreepAroundBase || hasCreep;

			if (hasCreep)
				m_lastCreepFoundTime = time;
		}
		void							ClearHasCreepAroundBase() const {
			m_hasCreepAroundBase = false;
		}

		void toScreen() const
		{
			cout << "[ **base정보** ]" << endl;

			cout << "섬:" << isIsland() << endl;
			cout << "점령정보(0:empty/1:my/2:enemy/3:share):" << m_occupiedInfo << endl;

			cout << "(" << m_location.x << "," << m_location.y << ")" << "Base 적 멀티순위:" << m_expectedEnemyMultiRank << endl;
			cout << "(" << m_location.x << "," << m_location.y << ")" << "Base의 내 멀티순위:" << m_expectedMyMultiRank << endl;

			cout << "일꾼 수:" << m_workerCount << endl;
			cout << "일꾼위험지역:" << m_dangerousAreaForWorkers << endl;
			cout << "근처 적 공격유닛 수:" << m_isEnemyAround << endl;

			cout << "//////////////건물////////////////" << endl;
			cout << "내 공중 방어건물 수:" << m_s_AirDefenseBuildingCount << endl;
			cout << "적 공중 방어건물 수:" << m_e_AirDefenseBuildingCount << endl;
			cout << "내 지상 방어건물 수:" << m_s_GroundDefenseBuildingCount << endl;
			cout << "적 지상 방어건물 수:" << m_e_GroundDefenseBuildingCount << endl;
			cout << "//////////////유닛////////////////" << endl;
			cout << "내 공중 방어유닛 수:" << m_s_AirDefenseUnitCount << endl;
			cout << "적 공중 방어유닛 수:" << m_e_AirDefenseUnitCount << endl;
			cout << "내 지상 방어유닛 수:" << m_s_GroundDefenseUnitCount << endl;
			cout << "적 지상 방어유닛 수:" << m_e_GroundDefenseUnitCount << endl;
			cout << "//////////////마인////////////////" << endl;
			cout << "마인 수:" << m_mineCount << endl;
			cout << "예약된 마인 수:" << m_reservedMineCount << endl;
		}

	private:
		Map 							*GetMap() const				{
			return m_pMap;
		}
		Map 							*GetMap()					{
			return m_pMap;
		}

		Map *const						m_pMap;
		Area *const						m_pArea;
		BWAPI::TilePosition				m_location;
		BWAPI::Position					m_center;
		std::vector<Mineral *>			m_Minerals;
		std::vector<Geyser *>			m_Geysers;
		std::vector<Mineral *>			m_BlockingMinerals;
		bool							m_starting = false;

		// SAIDA 추가
		mutable std::pair<BWAPI::Unit, BWAPI::Unit>	m_edgeMinerals;

		// 미네랄 뒤쪽에 공간이 있으면 true
		mutable bool							m_isExistBackYard;
		mutable bool							m_isMineralsPlacedVertical;

		// are there minerals on the left/top side of the base
		mutable bool							m_isMineralsLeftOfBase;
		mutable bool							m_isMineralsTopOfBase;

		// is there base on the left/right/top/bottom side of the area
		mutable bool							m_isBaseLeftOfArea;
		mutable bool							m_isBaseTopOfArea;

		mutable BWAPI::Unit						m_topMineral;
		mutable BWAPI::Unit						m_leftMineral;
		mutable BWAPI::Unit						m_rightMineral;
		mutable BWAPI::Unit						m_bottomMineral;

		mutable BWAPI::Position					m_mineral_center;

		mutable int								m_workerCount = 0;
		mutable int								m_mineCount = 0;
		mutable int								m_reservedMineCount = 0;
		mutable enum baseOccupiedInfo			m_occupiedInfo = emptyBase; //Base점령상태
		mutable int								m_expectedEnemyMultiRank = INT_MAX; //적 예상멀티지역
		mutable int								m_expectedMyMultiRank = INT_MAX; //내 예상멀티지역

		mutable bool							m_isEnemyAround = false;
		mutable bool							m_dangerousAreaForWorkers = false; //공격받고있음 -> 미네랄,가스 캐는 일꾼만!

		mutable int								m_s_AirDefenseBuildingCount = 0; //Self 터렛
		mutable int								m_e_AirDefenseBuildingCount = 0; //Enemy 터렛/스포어/포톤
		mutable int								m_s_GroundDefenseBuildingCount = 0; //Self
		mutable int								m_e_GroundDefenseBuildingCount = 0; //Enemy 성큰/포톤

		mutable int								m_s_AirDefenseUnitCount = 0; //Self
		mutable int								m_e_AirDefenseUnitCount = 0; //Enemy
		mutable int								m_s_GroundDefenseUnitCount = 0; //Self
		mutable int								m_e_GroundDefenseUnitCount = 0; //Enemy
		mutable int								m_lastVisitedTime = 0;
		mutable int								m_lastCreepFoundTime = 0;
		mutable bool							m_hasCreepAroundBase = false;

		//Terran전
		mutable int								m_e_BunkerCount = 0;

	};

} // namespace BWEM


#endif

