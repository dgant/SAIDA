#include "WraithManager.h"
#include "VultureManager.h"
#include "../InformationManager.h"

using namespace MyBot;

WraithManager::WraithManager()
{
}

WraithManager &WraithManager::Instance()
{
	static WraithManager instance;
	return instance;
}

void WraithManager::update()
{
	if (TIME < 300 || TIME % 2 != 0)
		return;

	if (canCloak == false)
	{
		if (S->hasResearched(TechTypes::Cloaking_Field))
			canCloak = true;
	}

	uList wraithList = INFO.getUnits(Terran_Wraith, S);
	uList enemyWraithList = INFO.getUnits(Terran_Wraith, E);
	uList enemyValkyrie = INFO.getUnits(Terran_Valkyrie, E);

	if (wraithList.empty())
		return;

	if (SM.getMainStrategy() == Eliminate)
		return;

	if (enemyValkyrie.size() > 0)
	{
		for (auto w : wraithList)
		{
			CommandUtil::move(w->unit(), MYBASE);
			w->setState(new WraithIdleState());
		}

		return;
	}

	



	if (!enemyWraithList.empty() && (oneTargetUnit == nullptr || !oneTargetUnit->exists()))
	{
		int dist = INT_MAX;

		for (auto e : enemyWraithList)
		{
			int eDist = INFO.getMainBaseLocation(S)->getPosition().getApproxDistance(e->unit()->getPosition());

			if (eDist < dist && !e->isHide() && wraithList.size() >= INFO.getTypeUnitsInRadius(Terran_Wraith, E, e->pos(), TILE_SIZE * 8).size()
					&& INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, e->pos(), TILE_SIZE * 5).empty())
			{
				if (!e->unit()->isCloaked())
				{
					oneTargetUnit = e->unit();
					dist = eDist;
				}
				else if (INFO.getAvailableScanCount() >= 1)
				{
					oneTargetUnit = e->unit();
					dist = eDist;
				}
			}
		}
	}

	Position killScvPosition = getKillScvTargetBase();

	for (auto w : wraithList)
	{
		string state = w->getState();

		if (state == "New" && w->isComplete())
		{
			w->setState(new WraithIdleState());
			w->action();
		}

		if (state == "Idle")
		{
			if (w->hp() < 50 || isBeingRepaired(w->unit()))
			{
				w->action();
				continue;
			}

			if (oneTargetUnit != nullptr)
			{
				w->setState(new WraithAttackWraithState());
				w->action(oneTargetUnit);
				continue;
			}

			if (killScvPosition != Positions::Unknown && !w->unit()->isUnderAttack())
			{
				w->setState(new WraithKillScvState());
				w->action(killScvPosition);
				continue;
			}

			w->setState(new WraithFollowTankState());
			w->action();
		}
		else if (state == "AttackWraith")
		{
			bool targetExist = false;

			if (oneTargetUnit != nullptr)
			{
				for (auto eWraith : enemyWraithList)
				{
					if (eWraith->id() == oneTargetUnit->getID())
					{
						targetExist = true;

						if (eWraith->isHide())
						{
							oneTargetUnit = nullptr;
						}

						break;
					}
				}
			}

			if (!targetExist)
			{
				oneTargetUnit = nullptr;
			}

			if (oneTargetUnit == nullptr)
			{
				w->setState(new WraithIdleState());
				w->action();
				continue;
			}
			else if (!INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, oneTargetUnit->getPosition(), TILE_SIZE * 10, false).empty())
			{
				oneTargetUnit = nullptr;
				w->setState(new WraithIdleState());
				w->action();
				continue;
			}

			w->action(oneTargetUnit);
		}
		else if (state == "KillScv")
		{
			if (killScvPosition == Positions::Unknown)
			{
				w->setState(new WraithIdleState());
				w->action();
				continue;
			}

			

			w->action(killScvPosition);
		}
		else
		{

		}
	}

	if (SM.getMainStrategy() == AttackAll)
	{
		for (auto w : wraithList)
		{
			string state = w->getState();

			if (state == "Idle")
			{
			
				if (!INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S).empty())
				{
					w->setState(new WraithFollowTankState());
				}
			}

			if (state == "FollowTank")
			{
				if (oneTargetUnit != nullptr)
				{
					w->setState(new WraithIdleState());
				}

				w->action();
			}
		}
	}
	else
	{
		for (auto w : wraithList)
		{
			
			w->action();
		}
	}
}

Position WraithManager::getKillScvTargetBase()
{
	list<const Base *> &enemyOccupiedBaseLocation = INFO.getOccupiedBaseLocations(E);
	Position myFirstExpantionPosition = INFO.getFirstExpansionLocation(S)->getPosition();
	Base *enemyFirstExpantionLocation = INFO.getFirstExpansionLocation(E);
	Position targetBasePosition = Positions::Unknown;
	int closestDistance = INT_MAX;
	int tempDistance = 0;
	
	if (enemyOccupiedBaseLocation.empty())
		return targetBasePosition;


	for (auto enemyBase : enemyOccupiedBaseLocation)
	{
		if (enemyBase->GetEnemyAirDefenseBuildingCount() > 0) 
		{
			continue;
		}

		if (enemyBase->GetEnemyAirDefenseUnitCount() > 0 && !canCloak) 
		{
			continue;
		}

		if (enemyBase->GetWorkerCount() == 0) 
		{
			continue;
		}

		tempDistance = myFirstExpantionPosition.getApproxDistance(enemyBase->getPosition());

		if (tempDistance < closestDistance)
		{
			targetBasePosition = enemyBase->getPosition();
			closestDistance = tempDistance;
		}
	}

	return targetBasePosition;
}