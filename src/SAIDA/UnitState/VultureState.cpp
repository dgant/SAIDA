#include "VultureState.h"
#include "../InformationManager.h"

using namespace MyBot;

////////////////////////////////////Idle////////////////////////////////////////////
State *VultureIdleState::action()
{
	Position movePosition = (INFO.getSecondChokePosition(S) + INFO.getFirstExpansionLocation(S)->getPosition()) / 2;

	//if (unit->getPosition().getApproxDistance(movePosition) > 6 * TILE_SIZE)
	//	CommandUtil::move(unit, movePosition);
	//else
	// 나중에...
	if (unit->getDistance(movePosition) > 3 * TILE_SIZE)
	{
		unit->attack(movePosition);
		unit->holdPosition(true);
	}

	//CommandUtil::attackMove(unit, movePosition, true);

	return nullptr;
}

State *VultureScoutState::action()
{
	if (!me)
		me = INFO.getUnitInfo(unit, S);

	if ((TIME - unit->getLastCommandFrame() < 24) &&
			unit->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position)
		return nullptr;

	if (targetBase == nullptr)
	{
		targetBase = getMinePosition(targetBase);

		if (targetBase == nullptr)
			return new VultureIdleState();
	}

	Position targetPosition = targetBase->Center();

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(me->pos(), &dangerPoint, false);

	if (me->pos().getApproxDistance(targetBase->Center()) < 8 * TILE_SIZE && dangerUnit)
	{
		if (isNeedKitingUnitType(dangerUnit->type()))
		{
			UnitInfo *closestAttack = INFO.getClosestUnit(E, me->pos(), GroundUnitKind, 10 * TILE_SIZE, true, false, true);

			if (closestAttack) {
				if (VM.getNeedPcon())
					pControl(me, closestAttack);
				else
					kiting(me, closestAttack, dangerPoint, 3 * TILE_SIZE);

				return nullptr;
			}
		}
		else
		{
			if (S->hasResearched(TechTypes::Spider_Mines) && unit->getSpiderMineCount())
			{
				unit->useTech(TechTypes::Spider_Mines, findRandomeSpot(me->pos()));
				return nullptr;
			}
		}
	}

	UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, unit->getPosition(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

	if (closestWorker != nullptr)
	{
		if (VM.getNeedPcon())
			pControl(me, closestWorker);
		else
			kiting(me, closestWorker, dangerPoint, 3 * TILE_SIZE);

		return nullptr;
	}

	//타겟 지점에 이미 상대방 건물이 지어진 경우
	uList eBuildings = INFO.getBuildingsInRadius(E, targetPosition, 10 * TILE_SIZE, true, false, true);

	if (eBuildings.size()) {

		// 지상 방어 Tower가 있는 경우 바로 빼
		for (auto eb : eBuildings)
			if (eb->type().groundWeapon().targetsGround())
			{
				targetBase = getMinePosition(targetBase);
				return nullptr;
			}

		if (unit->getPosition().getApproxDistance(targetPosition) > 3 * TILE_SIZE)
			unit->move(targetPosition);
		else
			targetBase = getMinePosition(targetBase);

		return nullptr;
	}

	if (INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, targetPosition, 2 * TILE_SIZE).size())
	{
		targetBase = getMinePosition(targetBase);
		return nullptr;
	}

	if (unit->getDistance(targetPosition) < 2 * TILE_SIZE)
	{
		if (INFO.getFirstMulti(S) == targetBase || INFO.getFirstMulti(S, true) == targetBase)
		{
			targetBase = getMinePosition(targetBase);
			return nullptr;
		}

		if (S->hasResearched(TechTypes::Spider_Mines) && unit->getSpiderMineCount() > 0)
			unit->useTech(TechTypes::Spider_Mines, findRandomeSpot(targetPosition));
		else
			targetBase = getMinePosition(targetBase);
	}
	else
		CommandUtil::rightClick(unit, targetPosition);

	return nullptr;
}

State *VultureDefenceState::action()
{
	if ((TIME - unit->getLastCommandFrame() < 24) &&
			unit->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position)
		return nullptr;

	UnitInfo *me = INFO.getUnitInfo(unit, S);

	uList enemy = INFO.enemyInMyYard();

	UnitInfo *closest = nullptr;
	UnitInfo *shuttle = nullptr;
	int dist = INT_MAX;

	for (auto eu : enemy)
	{
		if (me->pos().getApproxDistance(eu->pos()) < dist)
		{
			if (eu->type() == Terran_Dropship || eu->type() == Protoss_Shuttle)
			{
				shuttle = eu;
				continue;
			}

			if (!eu->type().isFlyer())
			{
				closest = eu;
				dist = me->pos().getApproxDistance(eu->pos());
			}
		}
	}

	if (closest != nullptr) {
		if (S->hasResearched(TechTypes::Spider_Mines) && unit->getSpiderMineCount() > 0)
		{
			// Command Center와 근처에 마인 심지 않기 위해
			if (me->pos().getApproxDistance(MYBASE) > 7 * TILE_SIZE &&
					me->pos().getApproxDistance(closest->pos()) >= closest->type().groundWeapon().maxRange() + 2 * TILE_SIZE &&
					me->pos().getApproxDistance(closest->pos()) <= closest->type().groundWeapon().maxRange() + 4 * TILE_SIZE)
			{
				unit->useTech(TechTypes::Spider_Mines, findRandomeSpot(unit->getPosition()));
				return nullptr;
			}
		}

		///////////////////////////////////////////////////////
		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(me->pos(), &dangerPoint, false);

		if (closest->isBurrowed()) // 러커
		{
			if (!closest->isHide() && closest->unit()->isDetected())
			{
				CommandUtil::attackUnit(unit, closest->unit());
			}
			else // 안보이거나 Detection이 안된 상황
			{
				if (INFO.getAvailableScanCount() &&
						INFO.getCompletedCount(Terran_Goliath, S) + INFO.getCompletedCount(Terran_Vulture, S) + INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 3)
				{
					CommandUtil::attackMove(unit, closest->pos());
				}
				else
				{
					if (dangerPoint > 4 * TILE_SIZE)
					{
						CommandUtil::attackMove(unit, closest->pos());
					}
					else
						moveBackPostion(me, closest->pos(), 3 * TILE_SIZE);
				}
			}

			return nullptr;
		}

		//럴커 아님
		if (closest->isHide()) // 시야에서 사라짐
		{
			CommandUtil::attackMove(unit, closest->pos());
		}
		else
		{
			if (closest->unit()->isDetected())
			{
				if (dangerUnit && closest->type() == dangerUnit->type()) // 후속 공격유닛 없음.
				{
					if (isNeedKitingUnitType(closest->type()))
					{
						if (VM.getNeedPcon())
							pControl(me, closest);
						else
							kiting(me, closest, dangerPoint, 3 * TILE_SIZE);
					}
					else
					{
						if (unit->getGroundWeaponCooldown() == 0)
							CommandUtil::attackMove(unit, closest->pos());
						else if (dangerPoint < 3 * TILE_SIZE)
							moveBackPostion(me, closest->pos(), 2 * TILE_SIZE);
					}
				}
				else // 후속 공격 있음.
				{
					kiting(me, closest, dangerPoint, 3 * TILE_SIZE);
				}
			}
			else // 다템한테 안전하게
			{
				kiting(me, closest, dangerPoint, 4 * TILE_SIZE);
			}
		}
	}
	else if (shuttle != nullptr) // 드랍 뱅기가 있을 경우
	{
		CommandUtil::move(unit, shuttle->pos());
	}
	else {
		if (ESM.getWaitTimeForDrop() > 0 &&
				TIME - ESM.getWaitTimeForDrop() <= 24 * 60 * 3) {

			if (unit->getPosition().getApproxDistance(MYBASE) > 7 * TILE_SIZE && S->hasResearched(TechTypes::Spider_Mines) && unit->getSpiderMineCount() > 0)
			{
				unit->useTech(TechTypes::Spider_Mines, findRandomeSpot(unit->getPosition()));
			} else
				CommandUtil::move(unit, MYBASE - 3 * TILE_SIZE);
		}
	}

	return nullptr;
}

State *VultureDiveState::action()
{
	if ((TIME - unit->getLastCommandFrame() < 24) &&
			unit->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position)
		return nullptr;

	UnitInfo *me = INFO.getUnitInfo(unit, S);

	Position target = INFO.getMainBaseLocation(E)->Center();

	if (!isSameArea(me->pos(), target))
	{
		unit->move(target);
		return nullptr;
	}

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(me->pos(), &dangerPoint, false);
	bool mineUsable = S->hasResearched(TechTypes::Spider_Mines) && unit->getSpiderMineCount();

	if (dangerUnit == nullptr)
	{
		UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, me->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

		if (closestWorker != nullptr && isSameArea(closestWorker->pos(), target) )
		{
			if (VM.getNeedPcon())
				pControl(me, closestWorker);
			else
				kiting(me, closestWorker, dangerPoint, 2 * TILE_SIZE);
		}
		else if (me->pos().getApproxDistance(target) > 5 * TILE_SIZE)
		{
			CommandUtil::move(unit, target);
		}
		else
		{
			if (INFO.enemyRace == Races::Zerg)
			{
				UnitInfo *larva = INFO.getClosestTypeUnit(E, me->pos(), Zerg_Larva, 8 * TILE_SIZE);

				if (larva) {
					CommandUtil::attackUnit(me->unit(), larva->unit());
					return nullptr;
				}

				UnitInfo *egg = INFO.getClosestTypeUnit(E, me->pos(), Zerg_Egg, 8 * TILE_SIZE);

				if (egg) {
					CommandUtil::attackUnit(me->unit(), egg->unit());
					return nullptr;
				}
			}
		}
	}
	else //dangerUnit 있음
	{
		// danger가 앞마당 유닛이면 일단 본진으로 뛴다.
		if (!isSameArea(dangerUnit->pos(), target))
		{
			if (me->pos().getApproxDistance(target) > 3 * TILE_SIZE)
				CommandUtil::move(unit, target);

			return nullptr;
		}

		UnitInfo *closestAttack = INFO.getClosestUnit(E, me->pos(), GroundUnitKind, 10 * TILE_SIZE, true, false, true);

		if (dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
		{
			if (closestAttack == nullptr)
			{
				if (goWithoutDamage(unit, target, direction) == false)
					direction *= -1;
			}
			else // closest Attacker 존재
			{
				kiting(me, closestAttack, dangerPoint, 3 * TILE_SIZE);
			}
		}
		else if (isNeedKitingUnitType(dangerUnit->type()))
		{
			if (closestAttack == nullptr)
			{
				if (me->pos().getApproxDistance(target) > 3 * TILE_SIZE)
					CommandUtil::move(unit, target);
			}
			else // closest Attacker 존재
			{
				if (VM.getNeedPcon())
					pControl(me, closestAttack);
				else
					kiting(me, closestAttack, dangerPoint, 3 * TILE_SIZE);
			}
		}
		else
		{
			if (mineUsable) {
				unit->useTech(TechTypes::Spider_Mines, findRandomeSpot(me->pos()));
				return nullptr;
			}

			// 적의 사거리 유닛이 있으면 벌쳐는 그냥 일꾼만 죽여
			return new VultureKillWorkerFirstState();
		}
	}

	return nullptr;
}

State *VultureRemoveMineState::action()
{
	if (!m_targetUnit->exists())
		return new VultureIdleState();

	CommandUtil::attackUnit(unit, m_targetUnit, true);

	return nullptr;
}

State *VultureKillWorkerFirstState::action()
{
	UnitInfo *me = INFO.getUnitInfo(unit, S);

	uList workers = INFO.getTypeUnitsInRadius(INFO.getWorkerType(INFO.enemyRace), E, me->pos(), 15 * TILE_SIZE, true);

	for (auto w : workers)
	{
		if (!unit->isInWeaponRange(w->unit()))
			continue;

		if (me->hp() < me->expectedDamage())
			continue;

		if (unit->getGroundWeaponCooldown() == 0)
		{
			w->setDamage(unit);
			break;
		}

		kiting(me, w, unit->getDistance(w->unit()), 2 * TILE_SIZE);
		//CommandUtil::attackUnit(unit, w->unit());
		return nullptr;
	}

	UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, me->pos(), INFO.getWorkerType(INFO.enemyRace), 0, true, true);

	if (closestWorker)
		kiting(me, closestWorker, unit->getDistance(closestWorker->unit()), 2 * TILE_SIZE);
	else
	{
		if (checkBase)
			CommandUtil::attackMove(unit, INFO.getMainBaseLocation(E)->Center());
		else
		{
			if (me->pos().getApproxDistance(INFO.getMainBaseLocation(E)->Center()) < 5 * TILE_SIZE)
				checkBase = true;
			else
				CommandUtil::move(unit, INFO.getMainBaseLocation(E)->Center());
		}
	}

	return nullptr;
}