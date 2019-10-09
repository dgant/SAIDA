#pragma once

#include "Common.h"

#include "UnitData.h"
#include "InformationManager.h"
#include "StrategyManager.h"
#include "UnitState\CommandCenterState.h"
#include "UnitState\BarrackState.h"
#include "UnitState\FactoryState.h"
#include "UnitState\StarportState.h"

namespace MyBot
{
	/// 유닛 생산 매니저
	class TrainManager
	{
		TrainManager();

	public:
		/// static singleton 객체를 리턴합니다
		static TrainManager &Instance();

		void update();
		Position getBarricadePosition();
		void setBarricadePosition(Position p);

		int getAvailableMinerals();
		int getAvailableGas();
		int getBaseVultureCount(InitialBuildType initialBuild, MainBuildType mainBuild);
	private:
		int reservedMinerals; ///< minerals reserved for planned trains
		int reservedGas;      ///< gas reserved for planned trains
		bool saveGas;         ///< 특정 가스 유닛(ex. 베슬)이 필요할 때 가스 세이브하기 위해 추가
		int nextVessleTime;   /// 베슬은 가스가 많지 않으면 1분마다 한번씩 생산

		bool waitToProduce;

		bool hasEnoughResources(UnitType unitType);
		void addReserveResources(UnitType unitType);

		const Base *getEscapeBase(UnitInfo *c);

		void commandCenterTraining();
		void barracksTraining();
		void factoryTraining();
		void starportTraining();

		int getMaxVultureCount();
		int getMaxScvNeedCount();

		bool isFirstFactory();
		void findAndSaveFirstFactoryPos(Unit factory);
		TilePosition moveFirstFactoryPos = TilePositions::None;
		bool isTimeToMoveCommandCenter(UnitInfo *c);

		Position barricadePosition = Positions::None;
		void setBarricadeBarrack(uList &bList);

		// 컴샛 위치가 적으로부터 안전한지 판단
		bool isSafeComsatPosition(UnitInfo *depot);

		// 초반 병력 생산을 위해 쉬어야 하는지 판단
		bool needStopTrainToFactory();
		bool needStopTrainToBarracks();
	};
}