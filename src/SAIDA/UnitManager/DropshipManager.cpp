#include "DropshipManager.h"
#include "../UnitManager/TankManager.h"
#include "../UnitManager/GoliathManager.h"

using namespace MyBot;

DropshipManager::DropshipManager()
{
}

DropshipManager &DropshipManager::Instance()
{
	static DropshipManager instance;
	return instance;
}

void DropshipManager::update()
{
	if (TIME < 300 || TIME % 2 != 0)
		return;

	uList dropshipList = INFO.getUnits(Terran_Dropship, S);

	if (dropshipList.empty())
		return;

	int boardingSuccessCnt = 0;
	int waitToBoardCnt = 0;

	for (auto d : dropshipList)
	{
		string state = d->getState();

		if (state == "Boarding")
		{
			waitToBoardCnt++;

			if (d->unit()->getSpaceRemaining() != Terran_Dropship.spaceProvided() && d->pos().getApproxDistance(MYBASE) <= TILE_SIZE)
				boardingSuccessCnt++;
		}

		if (state == "Back" && 	d->pos().getApproxDistance(MYBASE) < 50 * TILE_SIZE)
			waitToBoardCnt++;
	}

	if (waitToBoardCnt >= 1)
		waitToBoarding = true;
	if (INFO.enemyRace == Races::Protoss)
	{
		if (boardingSuccessCnt == 2) // 이번 Frame에 Boarding으로 간다.
		{
			TM.initDropshipSet();
			GM.initDropshipSet();
			waitToBoarding = false;
			reverse = !reverse;
		}
	}
	else 
	{
		if (boardingSuccessCnt == 3) // 이번 Frame에 Boarding으로 간다.
		{
			TM.initDropshipSet();
			GM.initDropshipSet();
			waitToBoarding = false;
			reverse = !reverse;
		}
	}
	for (auto d : dropshipList)
	{
		string state = d->getState();

		if (state == "New" && d->isComplete()) {
			d->setState(new DropshipIdleState());
		}

		if (state == "Idle")
		{
			if (SM.getDropshipMode())
			{
				d->setState(new DropshipBoardingState());
			}
		}

		if (state == "Boarding")
		{
			// 예외처리
			if (TIME % (24 * 5) == 0)
				d->initspaceRemaining();
          if(INFO.enemyRace == Races::Protoss)
		  { 
			if (boardingSuccessCnt >= 2 && SM.getMainStrategy() == AttackAll)
			{
				d->setState(new DropshipGoState(INFO.getMainBaseLocation(E)->Center(), reverse));
				d->action();
			}
		  }
		  else 
		  
		  
		  {
			  if (boardingSuccessCnt >= 3)
			  {
				  d->setState(new DropshipGoState(INFO.getMainBaseLocation(E)->Center(), reverse));
				  d->action();
			  }
		  }
		}

		d->action();
	}
}