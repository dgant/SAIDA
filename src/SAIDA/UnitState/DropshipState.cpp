#include "DropshipState.h"
#include "../InformationManager.h"

using namespace MyBot;

State *DropshipIdleState::action()
{
	CommandUtil::move(unit, MYBASE);
	return nullptr;
}

State *DropshipGoState::action()
{
	if (myRoute.size() == 0)
		myRoute = makeDropshipRoute(MYBASE, dropTarget, reverse);

	if (unit->getLoadedUnits().empty())
	{
		INFO.getUnitInfo(unit, S)->initspaceRemaining();

		vector<Position> backRoute;

		bool flag = false;

		for (int i = myRoute.size(); i > 0; i--)
		{
			if (flag)
				backRoute.push_back(myRoute[i - 1]);

			if (flag == false && myRoute[i - 1] == m_targetPos)
				flag = true;
		}

		backRoute.push_back(MYBASE);

		return new DropshipBackState(MYBASE, backRoute);
	}

	//Dropship HP가 10프로 미만으로 떨어지면 유닛을 모두 내려놓습니다.
	if (unit->getHitPoints() < Terran_Dropship.maxHitPoints() * 0.5 && unit->isUnderAttack())
	{
		for (auto u : unit->getLoadedUnits())
		{
			unit->unload(u);
		}

		CommandUtil::move(unit, m_targetPos);
		return nullptr;
	}

	//Dropship근처에 wraith잇으면
	vector<UnitType> types = { Terran_Wraith, Terran_Battlecruiser, Terran_Valkyrie };
	UnitInfo *closestAirEnemy = INFO.getClosestTypeUnit(E, unit->getPosition(), types, 8 * TILE_SIZE, true, false, false);
	// 가다가 건설중인 Command를 만났을 때
	UnitInfo *closestCommand = INFO.getClosestTypeUnit(E, unit->getPosition(), Terran_Command_Center, 10 * TILE_SIZE, true);

	if (closestAirEnemy != nullptr || (closestCommand && !closestCommand->isComplete()))
	{
		Position pos = closestAirEnemy ? closestAirEnemy->pos() : closestCommand->pos();
		int range = closestAirEnemy ? 8 : 10;

		for (auto g : INFO.getTypeUnitsInRadius(Terran_Goliath, S, pos, range * TILE_SIZE)) {
			if (!g->unit()->isLoaded())
				return nullptr;
		}

		for (auto u : unit->getLoadedUnits())
		{
			if (u->getType() == Terran_Goliath)
			{
				unit->unload(u);
				return nullptr;
			}
		}

		return nullptr;
	}

	//가다가 멀티를 만나면
	UnitInfo *closestRefinery = INFO.getClosestTypeUnit(E, unit->getPosition(), Terran_Refinery, 10 * TILE_SIZE, true);

	if (closestCommand || (closestRefinery && INFO.getTypeUnitsInRadius(Terran_SCV, E, closestRefinery->pos(), 7 * TILE_SIZE).size() >= 2) )
	{
		for (auto u : unit->getLoadedUnits())
		{
			unit->unload(u);
		}

		CommandUtil::move(unit, m_targetPos);
		return nullptr;
	}

	//정상 루트로 가는 경우(멀티 안만난경우)
	int routeSize = myRoute.size();

	if (m_targetPos == Positions::None)
	{
		m_targetPos = myRoute[0];
		CommandUtil::move(unit, m_targetPos);
		return nullptr;
	}
	else
	{
		if (m_targetPos != dropTarget) // 마지막 목적지가 아닌경우
		{
			if (unit->getPosition().getApproxDistance(m_targetPos) < 3 * TILE_SIZE)
			{
				bool nextPos = false;

				for (auto p : myRoute)
				{
					if (nextPos)
					{
						m_targetPos = p;
						break;
					}

					if (p == m_targetPos)
					{
						nextPos = true;
					}
				}

				return nullptr;
			}
		}
	}

	if (unit->getDistance(dropTarget) < 30 * TILE_SIZE)
	{
		if (unit->isUnderAttack())
		{
			for (auto u : unit->getLoadedUnits())
			{
				unit->unload(u);
			}

			CommandUtil::move(unit, m_targetPos);
		}
		else
		{
			if (unit->getDistance(dropTarget) < 10 * TILE_SIZE)
			{
				for (auto u : unit->getLoadedUnits())
				{
					unit->unload(u);
				}

				CommandUtil::move(unit, m_targetPos);
			}
			else
			{
				CommandUtil::move(unit, m_targetPos);
			}
		}
	}
	else
	{
		if (unit->getPosition().getApproxDistance(MYBASE) < 50 * TILE_SIZE)
		{
			if (goWithoutDamage(unit, m_targetPos, direction) == false)
				direction *= -1;
		}
		else
			CommandUtil::move(unit, m_targetPos);

		/*
		if (unit->getSpaceRemaining() == 0)
		{
			if (goWithoutDamage(unit, m_targetPos, direction) == false)
				direction *= -1;
		}
		*/
	}

	return nullptr;
}

State *DropshipBackState::action()
{
	//	if (myRoute.size() == 0)
	//		myRoute = makeDropshipRoute(unit->getPosition(), dropTarget, reverse);
	//
	int routeSize = myRoute.size();

	if (m_targetPos == Positions::None)
	{
		m_targetPos = myRoute[0];
		CommandUtil::move(unit, m_targetPos);
		return nullptr;
	}
	else
	{
		if (m_targetPos != dropTarget) // 마지막 목적지가 아닌경우
		{
			if (unit->getPosition().getApproxDistance(m_targetPos) < 3 * TILE_SIZE)
			{
				bool nextPos = false;

				for (auto p : myRoute)
				{
					if (nextPos)
					{
						m_targetPos = p;
						break;
					}

					if (p == m_targetPos)
					{
						nextPos = true;
					}
				}

				return nullptr;
			}
		}
	}

	if (unit->getDistance(dropTarget) > 5 * TILE_SIZE)
	{
		CommandUtil::move(unit, m_targetPos);
	}
	else
	{
		return new DropshipBoardingState();
	}

	return nullptr;
}

State *DropshipBoardingState::action()
{
	if (waitBoardTime == 0 && SM.getDropshipMode())
		waitBoardTime = TIME;

	if (preRemainSpace != unit->getSpaceRemaining())
		waitBoardTime = TIME;

	preRemainSpace = unit->getSpaceRemaining();

	if (unit->getHitPoints() < Terran_Dropship.maxHitPoints())
	{
		// 보드 State된지 15초 안되었거나 수리중이라면 해당 위치에 대기한다.
		if (waitBoardTime + (24 * 15) > TIME || isBeingRepaired(unit))
		{
			CommandUtil::move(unit, (MYBASE + Position(0, -3 * TILE_SIZE)), true);
			return nullptr;
		}
	}

	// empty Dropship이 있으면 모이지 말자
	bool emptyDropship = false;

	for (auto d : INFO.getUnits(Terran_Dropship, S))
	{
		if (d->isComplete() && d->unit()->getSpaceRemaining() == Terran_Dropship.spaceProvided())
		{
			emptyDropship = true;
			break;
		}
	}

	if (unit->getSpaceRemaining() == 0
			|| (waitBoardTime + (24 * 30) < TIME && emptyDropship == false)) // 30초 지났는데 뭐라도 타있어
	{
		CommandUtil::move(unit, MYBASE, true);
	}

	return nullptr;
}
