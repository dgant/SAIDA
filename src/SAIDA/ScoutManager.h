#pragma once

#include "Common.h"
#include "InformationManager.h"
#include "UnitState/ScvState.h"
#include "BuildManager.h"

namespace MyBot
{
	namespace ScoutStatus
	{
		enum {
			NoScout = 0,						///< 정찰 유닛을 미지정한 상태
			AssignScout,						///< 정찰 유닛을 할당한 상태
			LookAroundMyMainBase,				///< 정찰 출발 전 아군의 기지를 확인해본다.
			WaitAtMySecondChokePoint,			///< 아군의 세컨드초크포인트에서 대기한다.
			MovingToAnotherBaseLocation,		///< 적군의 BaseLocation 이 미발견된 상태에서 정찰 유닛을 이동시키고 있는 상태
			MoveAroundEnemyBaseLocation,		///< 적군의 BaseLocation 이 발견된 상태에서 정찰 유닛을 이동시키고 있는 상태
			CheckEnemyFirstExpansion,			///< 적군 앞마당을 확인한다.
			WaitAtEnemyFirstExpansion,			///< 적군 앞마당을 확인하기 위해 대기하는 상태
			FinishFirstScout					/// < 첫 스카우트가 완료된 상태
		};
	}

	/// 게임 초반에 일꾼 유닛 중에서 정찰 유닛을 하나 지정하고, 정찰 유닛을 이동시켜 정찰을 수행하는 class<br>
	/// 적군의 BaseLocation 위치를 알아내는 것까지만 개발되어있습니다
	class ScoutManager
	{
		ScoutManager();

		int								currentScoutStatus;
		int							currentScoutUnitTime;
		Unit						currentScoutUnit;
		UnitInfo 					*currentScoutUnitInfo;
		Unit						dualScoutUnit;

		int								currentScoutTargetDistance;

		Position					hidingPosition = Positions::Unknown; // 앞마당 확인을 하기 위해 SCV가 숨어있는 위치
		int								startHidingframe;

		/// 정찰 유닛을 필요하면 새로 지정한다. 정찰할지 여부 리턴
		bool							assignScoutIfNeeded();
		/// 정찰이 한번더 필요한지 확인해서 다시 정찰 유닛 할당.
		//bool							assignScoutIfNeededAgain();
		// 앞마당 구석에 숨어있다가 앞마당을 다시 확인하기 위한 로직
		void waitTillFindFirstExpansion();

		/// 정찰 유닛을 이동시킵니다
		void                            moveScoutUnit();

		void                            calculateEnemyAreaVertices();
		int                             getClosestVertexIndex(Unit unit);

		/// 아군 초반 일꾼 정찰 종료 여부
		bool					isToFinishScvScout;
		int						scoutDeadCount;

		/// 정찰 방문 순서
		BWEM::Base 			*cardinalPoints[4];
		BWEM::Base 			*explorePoints[4];

		bool							checkCardinalPoints;
		BWEM::Base 			*closestBaseLocation;
		bool							isPassThroughChokePoint;



	public:
		/// static singleton 객체를 리턴합니다
		static ScoutManager &Instance();

		/// 정찰 유닛을 지정하고, 정찰 상태를 업데이트하고, 정찰 유닛을 이동시킵니다
		void update();

		/// 정찰 유닛을 리턴합니다
		Unit getScoutUnit();

		// 정찰 상태를 리턴합니다
		int getScoutStatus();
		void onUnitDestroyed(Unit scouter);

	private:
		// 적 베이스 도착해서 스카우터 움직이기
		void		moveScoutInEnemyBaseArea(UnitInfo   *scouter);
		// 적 베이스 까지 가는길에 스카우터 움직이기
		void		moveScoutToEnemBaseArea(UnitInfo   *scouter, Position targetPos);
		// 내 주위에 적 공격건물 가져오기
		void		checkEnemyBuildingsFromMyPos(UnitInfo *scouter);
		// 내 주위에 적 원거리 공격 유닛 가져오기
		void		checkEnemyRangedUnitsFromMyPos(UnitInfo *scouter);

		Position	scouterNextPosition = Positions::None;
		UnitInfo	*enemAttackBuilding = nullptr;
		bool	isExistDangerUnit = false;
		int		attackBuildingDist = INT_MAX;
		int		currentScoutArea = -1;
		// 주기적으로 본진과 첫번째 확장 사이를 왔다갔다 한다.
		int		changeScoutingAreaTime = 0;
		int		goSpecialPosition = 0;
		int		immediateRangedDamage = 0;
		map<Position, int> enemBasePositions;
		int direction = 1;
	};
}