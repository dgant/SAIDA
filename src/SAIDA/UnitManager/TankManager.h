#pragma once

#include "../InformationManager.h"
#include "DropshipManager.h"
#include "../UnitManager/TankState.h"
#include "../StrategyManager.h"
#include "../UnitData.h"

#define TM TankManager::Instance()

namespace MyBot
{
	class KeepMultiSet : public UListSet {

	public:
		KeepMultiSet() {
			myScv = nullptr;
			middlePos = Positions::None;
		}
		void init() {
			myScv = nullptr;
		}
		void action(int num, bool needScv = true);
		void assignSCV(Unit scv) {
			myScv = scv;
		}
		
		void setMiddlePos(Position pos)
		{
			middlePos = pos;
		}
	private:
		Unit myScv;
		Position middlePos;
	};

	class TankExpandSet : public UListSet
	{
	public :
		void action();
		bool gathering(int Size)
		{
			if (isGathered || size() == 0)
				return true;

			bool gathered = true;

			

			for (auto t : getUnits())
			{
				if (t->pos().getApproxDistance(getPos()) > Size)
				{
					gathered = false;

					if (!t->unit()->canSiege() && !t->unit()->canUnsiege())
						continue;

					if (t->unit()->isSieged()) {
						t->unit()->unsiege();
						continue;
					}

					t->unit()->move(getPos());
				}
			}

			if (gathered) isGathered = true;

			return gathered;
		}
		
		void siegeAll()
		{
			for (auto t : getUnits())
			{
				if (!t->unit()->canSiege() && !t->unit()->canUnsiege())
					continue;

				if (!t->unit()->isSieged()) {
					t->unit()->siege();
				}
			}
		}

		void unSiegeAll()
		{
			for (auto t : getUnits())
			{
				if (!t->unit()->canSiege() && !t->unit()->canUnsiege())
					return;

				if (t->unit()->isSieged()) {
					t->unit()->unsiege();
				}
			}

			isGathered = false;
		}
	private :
		bool isGathered = false;
	};

	class TankGatheringSet : public UListSet
	{
	public:
		TankGatheringSet() {
			firstTank = nullptr;
		}
		void init();
		void action();
		UnitInfo *getFrontUnit() {
			if (!firstTank)
				firstTank = getFrontUnitFromPosition(SM.getMainAttackPosition());

			return firstTank;
		}
	private:
		UnitInfo *firstTank;
	};

	class TankFirstExpansionSecureSet : public UListSet
	{
	public:
		TankFirstExpansionSecureSet() {
			if (waitingPositionAtFirstChoke == Positions::None) {
				waitingPositionAtFirstChoke = INFO.getWaitingPositionAtFirstChoke(5, 8);
			}
		}
		void init();
		void action();
		void moveStepByStepAsOneTeam(vector<UnitInfo *> &tanks, Position targetPosition);
	private:
		Position waitingPositionAtFirstChoke = Positions::None;
	};

	class TankManager
	{
	public:
		TankManager();
		bool checkAllSiegeNeed();
		void getClosestPosition();
		UnitInfo *frontTankOfNotDefenceTank = nullptr;
		
		map<Position, UnitInfo *> firstDefencePositions;
		Position waitingPosition;
		Position defencePositionOnTheHill = Positions::Unknown;
		Position defencePositionNearMineral = Positions::Unknown;
		void setStateToTank(UnitInfo *t, State *state);
		void onUnitCompleted(Unit u);
		void onUnitDestroyed(Unit u);
		bool notAttackableByTank(UnitInfo *targetInfo);
		void closeUnitAttack(Unit u);
		void commonAttackActionByTank(const Unit &unit, const UnitInfo *targetInfo);
		UnitInfo *getDefenceTargetUnit(const uList &enemy, const Unit &unit, int defenceRange);
		void defenceInvisibleUnit(const Unit &unit, const UnitInfo *targetInfo);
		static TankManager &Instance();
		void update();
		void useScanForCannonOnTheHill();
		bool needWaitAtFirstenemyExapnsion();
		void setFirstExpansionSecure();
		void setAllSiegeMode(const uList &tankList);
		void setGatheringMode(const uList &tankList);

		void setIdleState(UnitInfo *t);
		void setTankDefence();
		void checkMultiBreak();
		void checkKeepMulti();
		void checkKeepMulti2();
		void checkZealotAllinRush(const uList &tankList);
		void checkProtectAdditionalMulti();

		word getEnemyNeerMyAllTanks(uList tankList);

		void checkDropship();
		bool enoughTankForDrop();
		void stepByStepMove();
		void siegeWatingMode();
		bool amIMoreCloseToTarget(Unit me, Position target, int dFromFrontToTarget);
		void moveForward(Unit t, Position pos);
		bool attackFirstTargetOfTank(Unit me, uList &e);
		bool isSiegeTankNeerMe(Unit u);
		void siege(Unit u);
		void unsiege(Unit u, bool imme = true);
		void defenceOnFixedPosition(UnitInfo *t, Position posOnHill);
		bool needGathering();
		void initDropshipSet() {
			dropshipSet.clear();
		}
		Position getPositionOnTheHill();
		Position getPositionNearMineral();

		UnitInfo *getFirstAttackTargetOfTank(Unit me, uList &eList);

		TankFirstExpansionSecureSet getTankFirstExpansionSecureSet() {
			return tankFirstExpansionSecureSet;
		}

		UnitInfo *getFrontTankFromPos(Position pos);
		bool getZealotAllinRush() {
			return zealotAllinRush;
		}

		int getUsableTankCnt() {
			if (INFO.enemyRace == Races::Terran)
				return siegeLineSet.size();
			else
				return notDefenceTankList.size();
		}

		Position getNextMovingPoint() {
			return nextMovingPoint;
		}

		void setNextMovingPoint();

		void setSiegeNeedTank(uList &tanks);

		void setWaitAtChokePoint();

		uList getKeepMultiTanks(int num) {
			if (num == 1)
				return keepMultiSet.getUnits();

			return keepMultiSet2.getUnits();
		}

		void assignSCV(int num, Unit scv) {
			if (num == 1)
				keepMultiSet.assignSCV(scv);
			else if (num == 2)
				keepMultiSet2.assignSCV(scv);
		}

		UListSet getBaseDefenceTankSet() {
			return baseDefenceTankSet;
		}

		int getSiegeModeDefenceTank() {
			return siegeModeDefenceTank;
		}

		int getDropshipTankNum()
		{
			return dropshipSet.size();
		}

		void removeEgg();

	private:
		map<WalkPosition, vector<int>> chokeInfo;
		map<int, int> chokeMovingTankMap;
		UListSet allTanks;
		UListSet defenceTankSet;
		UListSet notDefenceTankList;
		UListSet baseDefenceTankSet;
		UListSet multiBreakSet;
		TankGatheringSet gatheringSet;
		set<UnitInfo *> siegeAllSet;
		TankFirstExpansionSecureSet tankFirstExpansionSecureSet;

		KeepMultiSet keepMultiSet;
		KeepMultiSet keepMultiSet2;
		UListSet dropshipSet;
		Position multiBase = Positions::Unknown;
		Position multiBase2 = Positions::Unknown;
		bool setMiddlePosition = false;
		Position middlePos = Positions::Unknown;
		UListSet siegeLineSet;
		TankExpandSet expandTankSet;
		bool zealotAllinRush;
		Position nextMovingPoint = Positions::Unknown;
		bool waitAtChokePoint = false;
		int siegeModeDefenceTank = 0;

		map<Position, Unit> protectAdditionalMultiMap;
		bool eggRemoved = false;

		bool exceedMaximumSupply = false;
	};
}