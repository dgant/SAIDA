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

	/*else if (enemyValkyrie.size() == 1 && !enemyValkyrie.at(0)->isHide())
	{
		for (auto w : wraithList)
		{
			CommandUtil::attackUnit(w->unit(), enemyValkyrie.at(0)->unit());
		}
		return;
	}*/


	//일점사를 위한 타겟 Wraith 설정 ->base에서 가장 가까운 Wraith부터 타겟으로 설정
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

	//WraithList를 가져와서 State별로 Job을 할당한다
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
			if (w->hp() < 50 || isBeingRepaired(w->unit()) /* || w->unit()->getEnergy() < 40*/) //HP가 50보다 작거나 수리중인 경우 JOB을 할당하지 않는다..
			{
				w->action();
				continue;
			}

			/////테란전START/////
			if (oneTargetUnit != nullptr)
			{
				w->setState(new WraithAttackWraithState());
				w->action(oneTargetUnit);
				continue;
			}

			// 상대 scv 사냥하러
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

			////벙커가 있는데 클로킹 못 쓰는 경우는 그곳으로 가지 않음..
			//if (INFO.getNearestBaseLocation(killScvPosition)->GetEnemyBunkerCount() > 0
			//		&& !canCloak && w->unit()->getEnergy() < 40)
			//{
			//	w->setState(new WraithIdleState());
			//	w->action();
			//	continue;
			//}

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
				// 할일 없으면 탱크 따라다니기(AttackAll모드)
				if (!INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S).empty())
				{
					w->setState(new WraithFollowTankState());
				}
			}

			if (state == "FollowTank")
			{
				if (oneTargetUnit != nullptr)// || killScvPosition != Positions::Unknown)
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
			//string state = w->getState();

			//if (state == "FollowTank")
			//{
			//	w->setState(new WraithIdleState());
			//}
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
	//bool baseExists = false;

	if (enemyOccupiedBaseLocation.empty())
		return targetBasePosition;

	// 상대 본진밖에 없거나 앞마당 체크가 안된 경우 앞마당으로 이동
	/*if (enemyOccupiedBaseLocation.size() == 1 && enemyMainBase->Starting())
	{
		cout << "building:" << enemyMainBase->GetEnemyAirDefenseBuildingCount() << " / unit:" << enemyMainBase->GetEnemyAirDefenseUnitCount() << endl;

		if (enemyMainBase->GetEnemyAirDefenseBuildingCount() || enemyMainBase->GetEnemyAirDefenseUnitCount())
		{
			targetBasePosition = INFO.getFirstExpansionLocation(E)->getPosition();
		}
		else
		{
			targetBasePosition = enemyMainBase->getPosition();
		}
	}
	else
	{*/
	// 본진/앞마당 둘 다 터렛 있는 경우 레이스 Idle 로 변경되도록 return Positions::Unknown
	for (auto enemyBase : enemyOccupiedBaseLocation)
	{
		if (enemyBase->GetEnemyAirDefenseBuildingCount() > 0) //공중방어건물이 있는 곳은 가지 않음
		{
			continue;
		}

		if (enemyBase->GetEnemyAirDefenseUnitCount() > 0/* || enemyBase->GetEnemyBunkerCount() > 0)*/ && !canCloak) //공중방어유닛이 있고, 클로킹업 안된경우 가지 않음
		{
			continue;
		}

		if (enemyBase->GetWorkerCount() == 0) //Worker가 한 마리도 없는 경우 가지 않음
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
