#pragma once

#include "../Common.h"

namespace MyBot
{
	
	namespace ConstructionStatus
	{
		enum {
			Unassigned = 0,				
			Assigned = 1,				
			UnderConstruction = 2,		
			WaitToAssign = 3,				
		};
	}

	
	class ConstructionTask
	{
	public:

		
		UnitType         type;

		
		TilePosition     desiredPosition;

		
		TilePosition     finalPosition;

		
		size_t                  status;

		
		Unit             constructionWorker;

		
		Unit             buildingUnit;

		
		bool                    buildCommandGiven;

	
		int                     lastBuildCommandGivenFrame;

		
		int                     lastConstructionWorkerID;

		
		int                     startWaitingFrame;

		
		int                     assignFrame;

		
		int						retryCnt;

		
		double minDist;
		
		int minDistTime;
		
		Position minDistPosition;
		
		int minDistVisitCnt;
		bool removeBurrowedUnit;


		ConstructionTask()
			: desiredPosition   (TilePositions::None)
			, finalPosition     (TilePositions::None)
			, type              (UnitTypes::Unknown)
			, buildingUnit      (nullptr)
			, constructionWorker       (nullptr)
			, lastBuildCommandGivenFrame(0)
			, lastConstructionWorkerID(0)
			, status            (ConstructionStatus::Unassigned)
			, buildCommandGiven (false)
			, startWaitingFrame(0)
			, assignFrame(0)
			, retryCnt(0)
			, minDist(INT_MAX)
			, minDistTime(0)
			, minDistPosition (Positions::None)
			, minDistVisitCnt(0)
			, removeBurrowedUnit(false)
		{}

		ConstructionTask(UnitType t, TilePosition _desiredPosition, Unit _constructionWorker = nullptr)
			: desiredPosition   (_desiredPosition)
			, finalPosition     (TilePositions::None)
			, type              (t)
			, buildingUnit      (nullptr)
			, constructionWorker       (_constructionWorker)
			, lastBuildCommandGivenFrame(0)
			, lastConstructionWorkerID(0)
			, status            (ConstructionStatus::Unassigned)
			, buildCommandGiven (false)
			, startWaitingFrame(0)
			, assignFrame(0)
			, retryCnt(0)
			, minDist(INT_MAX)
			, minDistTime(0)
			, minDistPosition(Positions::None)
			, minDistVisitCnt(0)
			, removeBurrowedUnit(false)
		{
			if (_constructionWorker != nullptr) {
				finalPosition = _desiredPosition;
				lastConstructionWorkerID = _constructionWorker->getID();
				status = ConstructionStatus::Assigned;
			}
		}

		bool operator==(const ConstructionTask &b)
		{
			if (b.type == this->type
					&& b.desiredPosition.x == this->desiredPosition.x && b.desiredPosition.y == this->desiredPosition.y) {
				
				return (b.buildingUnit == buildingUnit) || (b.constructionWorker == constructionWorker);
			}
			else {
				return false;
			}
		}
	};
}