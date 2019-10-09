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
	/// ���� ���� �Ŵ���
	class TrainManager
	{
		TrainManager();

	public:
		/// static singleton ��ü�� �����մϴ�
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
		bool saveGas;         ///< Ư�� ���� ����(ex. ����)�� �ʿ��� �� ���� ���̺��ϱ� ���� �߰�
		int nextVessleTime;   /// ������ ������ ���� ������ 1�и��� �ѹ��� ����

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

		// �Ļ� ��ġ�� �����κ��� �������� �Ǵ�
		bool isSafeComsatPosition(UnitInfo *depot);

		// �ʹ� ���� ������ ���� ����� �ϴ��� �Ǵ�
		bool needStopTrainToFactory();
		bool needStopTrainToBarracks();
	};
}