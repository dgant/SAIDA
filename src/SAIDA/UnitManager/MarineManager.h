#pragma once

#include "../UnitState/MarineState.h"
#include "TankManager.h"
#include "../StrategyManager.h"

#define MM MarineManager::Instance()

namespace MyBot
{
	class MarineManager
	{
		MarineManager();

		void getFirstDefensePos();
		Position FirstDefensePos;
		Unit bunker;
		Unit firstBarrack;
		Unit nextBarrackSupply;
		Unit zealotDefenceMarine;
		bool rangeUnitNearBunker;
	public:
		Unit getBunker() {
			return (bunker && bunker->exists()) ? bunker : nullptr;
		}
		void setFirstBunker();
		Unit getFirstBarrack() {
			return firstBarrack;
		};
		Unit getNextBarrackSupply() {
			return nextBarrackSupply;
		};
		Unit getzealotDefenceMarine() {
			return nextBarrackSupply;
		};
		void setZealotDefenceMarine(Unit &m) {
			zealotDefenceMarine = m;
		}
		bool isRangeUnitClose() {
			return rangeUnitNearBunker;
		}
		bool isZealotDefenceNeed();
		void setDefenceMarine(uList &marineList);
		bool findFirstBarrackAndSupplyPosition(); // 질럿 방어를 위한 서로 붙어 있는 배럭과 서플라이
		static MarineManager &Instance();
		void update();
		//		void checkKitingMarine(UnitInfo *&m);
		void setZealotDefenceMarine(uList &marineList);
		void onUnitDestroyed(Unit u);
		bool checkZerglingDefenceNeed();
		void checkRangeUnitNearBunker();

		void doKiting(Unit me);
		void setFirstChokeDefenceMarine(uList &marineList);
		Position waitingNearCommand = Positions::None;
	private:
		int lastSawScouterTime = 0;
		Position lastScouterPosition = Positions::None;
		uList enemyInMyYard;
		uList enemyWorkersInMyYard;
		uList enemyBuildingsInMyYard;

	};
}