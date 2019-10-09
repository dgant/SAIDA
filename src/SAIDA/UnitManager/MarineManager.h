#pragma once

#include "../UnitState/MarineState.h"
#include "TankManager.h"
#include "../StrategyManager.h"
#include "../InformationManager.h"
#define MM MarineManager::Instance()

namespace MyBot
{
	class MarineKiting : public UListSet
	{
	public:
		void action();
		void actionVsT();
		void initParam()
		{
			direction = 1;
			needWaiting = false;
			needCheck = false;
			waitingTime = 0;
			needFight = false;
			preAmountOfEnemy = 0;

			target = INFO.getMainBaseLocation(E)->Center();

			
		}
		void setTarget(Position p) {
			target = p;
		}
		int isWaiting() {
			return needWaiting;
		}
		void clearSet();
		bool canFight(Position);
		bool needBack(Position);
	
	private:
		Position target;
		int needWaiting; // 0 : no Wait, 1 : defence Build, 2 : range Unit
		bool needCheck;
		int direction;
		int waitingTime;
		word preAmountOfEnemy;

		vector<Position> defenceMinePos;
		bool needFight;
		// 테란전
		int preStage = WaitToBase;
		int changedTime = 0;
	};
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
		MarineKiting kitingSet;
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
		Position waitnearsecondchokepoint = Positions::None;
		//==================================
		bool moveAsideForCarrierDefence(UnitInfo *v);
		void setMarineDefence(uList &vList);
		void checkDiveMarine();
		bool needKeepMulti();
		Position checkForwardPylon();

		bool diveDone = false;
		word needScoutCnt = 0;
		bool scoutDone = false;
		int lastScoutTime = 0;
		Position forwardBuildingPosition = Positions::Unknown;
		bool checkedForwardPylon = false;

		UnitInfo *getFrontMarineFromPos(Position pos);
		bool needPcontrol = false;
		bool getNeedPcon() {
			return needPcontrol;
		}
	private:
		bool canStim = false;
		int lastSawScouterTime = 0;
		Position lastScouterPosition = Positions::None;
		uList enemyInMyYard;
		uList enemyWorkersInMyYard;
		uList enemyBuildingsInMyYard;
		UListSet MarineDefenceSet;
		
		MarineKiting kitingSecondSet;
		MarineKiting keepMultiSet;

		map<Unit, Unit> removeMineMap;

	};
}