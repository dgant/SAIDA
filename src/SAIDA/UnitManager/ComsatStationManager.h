#pragma once

#include "../UnitState/ComsatStationState.h"
#include "../StrategyManager.h"

namespace MyBot
{
	class ComsatStationManager
	{
		ComsatStationManager();

	public:
		static ComsatStationManager &Instance();
		void update();

		bool useScan(Position targetPosition);
		bool inDetectedArea(Unit targetUnit);
		bool inDetectedArea(UnitType targetType, Position targetPosition);
		bool inTheScanArea(Position targetPosition);
		int getAvailableScanCount() {
			return availableScanCount;
		}
	private:
		void updateReserveScanList();
		bool isReservedPosition(Position targetPosition);
		bool checkScan(Unit targetUnit);
		bool inArea(Position targetPosition, Position centerPosition, int range);

		vector<Position> reserveScanList;
		int availableScanCount;
	};
}