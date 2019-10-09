#pragma once

#include "Common.h"

#include "UnitData.h"
#include "InformationManager.h"
#include "StrategyManager.h"
#include "UnitManager\CommandCenterState.h"
#include "UnitManager\BarrackState.h"
#include "UnitManager\FactoryState.h"
#include "UnitManager\StarportState.h"

namespace MyBot
{
	
	class TrainManager
	{
		TrainManager();

	public:
		
		static TrainManager &Instance();

		void update();
		Position getBarricadePosition();
		void setBarricadePosition(Position p);

		int getAvailableMinerals();
		int getAvailableGas();
		int getBaseVultureCount(InitialBuildType initialBuild, MainBuildType mainBuild);
	private:
		int reservedMinerals; 
		int reservedGas;      
		bool saveGas;        
		int nextVessleTime;   

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

		
		bool isSafeComsatPosition(UnitInfo *depot);


		bool needStopTrainToFactory();
		bool needStopTrainToBarracks();
	};
}