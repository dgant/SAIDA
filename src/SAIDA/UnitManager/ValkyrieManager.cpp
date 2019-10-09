#include "ValkyrieManager.h"
#include "VultureManager.h"
#include "../InformationManager.h"


using namespace MyBot;

ValkyrieManager::ValkyrieManager()
{
	
}

ValkyrieManager &ValkyrieManager::Instance()
{
	static ValkyrieManager instance;
	return instance;
}

void ValkyrieManager::update()
{
	if (TIME < 300 || TIME % 2 != 0)
		return;

	
	uList enemyAirArmyList = INFO.getUnits(AirUnitKind, E);
	uList enemyScoutList = INFO.getUnits(Protoss_Scout, E);
	uList enemyCorsairList = INFO.getUnits(Protoss_Corsair, E);
	uList enemyMutaList = INFO.getUnits(Zerg_Mutalisk, E);
	uList wraithList = INFO.getUnits(Terran_Wraith, S);
	uList valkyrieList = INFO.getUnits(Terran_Valkyrie, S);
	uList enemyWraithList = INFO.getUnits(Terran_Wraith, E);
	uList enemyValkyrie = INFO.getUnits(Terran_Valkyrie, E);
	int idleValkyrienum = 0;
	if (valkyrieList.empty())
		return;

	if (SM.getMainStrategy() == Eliminate)
		return;

	if (!enemyAirArmyList.empty() && (oneTargetUnit == nullptr || !oneTargetUnit->exists()))
	{
		int dist = INT_MAX;

		for (auto e : enemyAirArmyList)
		{
			int eDist = INFO.getMainBaseLocation(S)->getPosition().getApproxDistance(e->unit()->getPosition());

			if (eDist < dist && !e->isHide() && wraithList.size() >= INFO.getTypeUnitsInRadius(AirUnitKind, E, e->pos(), TILE_SIZE * 8).size()
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


	for (auto w : valkyrieList)
	{
		string state = w->getState();

		if (state == "New" && w->isComplete())
		{
			w->setState(new ValkyrieIdleState());
			w->action();
		}

		if (state == "Idle")
		{
			if (INFO.getTypeUnitsInRadius(Protoss_Carrier, E, MYBASE, 20 * TILE_SIZE).size() || INFO.getTypeUnitsInRadius(Protoss_Carrier, E, INFO.getFirstExpansionLocation(S)->Center(), 20 * TILE_SIZE).size())
			{

				w->setState(new ValkyrieDefenceState());
				w->action();

				continue;
			}
			
			else if (w->hp() < 100 || isBeingRepaired(w->unit()) /* || w->unit()->getEnergy() < 40*/) //HP가 50보다 작거나 수리중인 경우 JOB을 할당하지 않는다..
			{
				w->action();
				continue;
			}

			//흔벎唐target
			else if (oneTargetUnit != nullptr)
			{
				w->setState(new ValkyrieAttackWraithState());
				w->action(oneTargetUnit);
				continue;
			}
			else if (!INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S).empty())
			{
				w->setState(new  ValkyrieFollowTankState());
				w->action();
				continue;
			}
		}
		else if (state == "AttackWraith")
		{
			bool targetExist = false;

			if (oneTargetUnit != nullptr)
			{
				for (auto AirEnemy : enemyAirArmyList)
				{
					if (AirEnemy->id() == oneTargetUnit->getID())
					{
						targetExist = true;

						if (AirEnemy->isHide())
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
				w->setState(new ValkyrieIdleState());
				w->action();
				continue;
			}
			else if (!INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, oneTargetUnit->getPosition(), TILE_SIZE * 10, false).empty())
			{
				oneTargetUnit = nullptr;
				w->setState(new ValkyrieIdleState());
				w->action();
				continue;
			}

			w->action(oneTargetUnit);
		}
		if (state == "FollowTank")
		{

			if (w->hp() < 50)
			{
			
				w->setState(new ValkyrieRetreatState());
				w->action();
				
				continue;
			}
			else if (INFO.getTypeUnitsInRadius(Protoss_Carrier, E, MYBASE, 20 * TILE_SIZE).size() || INFO.getTypeUnitsInRadius(Protoss_Carrier, E, INFO.getFirstExpansionLocation(S)->Center(), 20 * TILE_SIZE).size())
			
			{

					w->setState(new ValkyrieDefenceState());
					w->action();

					continue;
			}
		}
		if (state == "Defence")
		{
			if (w->hp() < 50)
			{

				w->setState(new ValkyrieRetreatState());
				w->action();

				continue;
			}
			else if (INFO.getTypeUnitsInRadius(Protoss_Carrier, E, MYBASE, 20 * TILE_SIZE).size() == 0 && INFO.getTypeUnitsInRadius(Protoss_Carrier, E, INFO.getFirstExpansionLocation(S)->Center(), 20 * TILE_SIZE).size()==0)

			{

				w->setState(new ValkyrieFollowTankState());
				w->action();

				continue;
			}



		}

		else
		{
			w->action();

		}
	}
}