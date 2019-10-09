#pragma once

#include "Common.h"
#include "ScoutManager.h"
#include "ConstructionPlaceFinder.h"
#include "InformationManager.h"
#include "StrategyManager.h"
#include "UnitManager\ScvManager.h"

#define CM ConstructionManager::Instance()

namespace MyBot
{
	/// 건물 건설 Construction 명령 목록을 리스트로 관리하고, 건물 건설 명령이 잘 수행되도록 컨트롤하는 class
	class ConstructionManager
	{
		ConstructionManager();

		/// 건설 필요 자원을 미리 예약해놓고, <br>
		/// 건설 대상 장소가 미개척 장소인 경우 건설 일꾼을 이동시켜 결국 건설이 시작되게 하고, <br>
		/// 건설 일꾼이 도중에 죽는 경우 다른 건설 일꾼을 지정하여 건설을 수행하게 하기 위해<br>
		/// Construction Task 들의 목록을 constructionQueue 로 유지합니다
		vector<ConstructionTask> constructionQueue;

		int             reservedMinerals;				///< minerals reserved for planned buildings
		int             reservedGas;					///< gas reserved for planned buildings

		bool            isBuildingPositionExplored(const ConstructionTask &b) const;

		/// constructionQueue 에서 건설 상태가 UnderConstruction 인 ConstructionTask 여러개를 삭제합니다<br>
		/// 건설을 시작했었던 ConstructionTask 이기 때문에 _reservedMinerals, _reservedGas 는 건드리지 않는다
		void            removeCompletedConstructionTasks(const vector<ConstructionTask> &toRemove);
		/// constructionQueue 에서 건설 시작하지 않은 ConstructionTask 를 취소합니다
		void			cancelNotUnderConstructionTasks(const vector<ConstructionTask> &toRemove);

		/// DO BOOK KEEPING ON WORKERS WHICH MAY HAVE DIED
		void            validateWorkersAndBuildings();
		/// ASSIGN WORKERS TO BUILDINGS WITHOUT THEM
		void            assignWorkersToUnassignedBuildings();
		/// ISSUE CONSTRUCTION ORDERS TO ASSIGN BUILDINGS AS NEEDED
		void            constructAssignedBuildings();
		/// UPDATE DATA STRUCTURES FOR BUILDINGS STARTING CONSTRUCTION
		void            checkForStartedConstruction();
		/// IF WE ARE TERRAN, THIS MATTERS<br>
		/// 테란은 건설을 시작한 후, 건설 도중에 일꾼이 죽을 수 있습니다. 이 경우, 건물에 대해 다시 다른 SCV를 할당합니다<br>
		/// 참고로, 프로토스 / 저그는 건설을 시작하면 일꾼 포인터를 null 로 만들기 때문에 (constructionWorker = null) 건설 도중에 죽은 일꾼을 신경쓸 필요 없습니다
		void            checkForDeadTerranBuilders();
		/// STEP 6: CHECK FOR COMPLETED BUILDINGS
		void            checkForCompletedBuildings();

		/// 건설 데드락을 체크하고, 해결합니다
		void            checkForDeadlockConstruction();
		clock_t time[10];
	public:
		/// static singleton 객체를 리턴합니다
		static ConstructionManager 	&Instance();

		/// constructionQueue 에 대해 Dead lock 이 있으면 제거하고, constructionQueue 내 ConstructionTask 들이 진행되도록 관리합니다
		void                update();

		/// constructionQueue 를 리턴합니다
		const vector<ConstructionTask> *getConstructionQueue();

		/// constructionQueue 에 ConstructionTask 를 추가합니다
		void                addConstructionTask(UnitType type, TilePosition desiredPosition, Unit builder = nullptr);
		/// ConstructionTask 하나를 삭제한다. 건설중인 것은 건설 취소. for 문 안에서 삭제하는 경우 주의해서 사용해야 함.
		void				cancelConstructionTask(ConstructionTask ct);

		/// constructionQueue 내 ConstructionTask 갯수를 리턴합니다<br>
		/// queryTilePosition 을 입력한 경우, 위치간 거리까지도 고려합니다
		int                 getConstructionQueueItemCount(UnitType queryType, TilePosition queryTilePosition = TilePositions::None, int maxRange = 16);
		/// 갯수는 상관없이 존재 여부만 체크한다.
		bool                existConstructionQueueItem(UnitType queryType, TilePosition queryTilePosition = TilePositions::None, int maxRange = 16);

		/// Construction 을 위해 예비해둔 Mineral 숫자를 리턴합니다
		int                 getReservedMinerals();
		/// Construction 을 위해 예비해둔 Gas 숫자를 리턴합니다
		int                 getReservedGas();
		/// 건설중이지 않은 Queue 를 모두 비워준다.
		void				clearNotUnderConstruction();
	};
}