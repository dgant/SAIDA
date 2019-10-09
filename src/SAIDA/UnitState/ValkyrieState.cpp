#include "ValkyrieState.h"
#include "../InformationManager.h"
#include "../UnitManager/TankManager.h"

using namespace MyBot;



//////////////////////////////////// Idle ///////////////////////////////////////////////////
State *ValkyrieIdleState::action()
{
	

	if (isBeingRepaired(unit))
		return nullptr;
	Unit mineral = bw->getClosestUnit(MYBASE, Filter::IsMineralField);
	Unit mineral2 = bw->getClosestUnit(INFO.getFirstExpansionLocation(S)->Center(), Filter::IsMineralField);
	Position movePosition = (MYBASE + mineral->getPosition()) / 2;
	Position movePosition2 = (INFO.getFirstExpansionLocation(S)->Center() + mineral2->getPosition()) / 2;


	if (TIME<15*60*24)
	{ 
	if (unit->getDistance(movePosition) > 6 * TILE_SIZE)
		CommandUtil::move(unit, movePosition);
	}
	else if (TIME>=15 * 60 * 24)
	{
		if (unit->getDistance(movePosition2) > 6 * TILE_SIZE)
			CommandUtil::move(unit, movePosition2);
	}
	return nullptr;
}
//////////////////////////////////// AttackWraith ///////////////////////////////////////////////////
State *ValkyrieAttackWraithState::action()
{
	if (tU != nullptr)
	{
		//cout << "action tU" << endl;
		action(tU);
	}

	return nullptr;
}
State *ValkyrieAttackWraithState::action(Unit targetUnit)
{


	if (targetUnit == nullptr) return nullptr;

	tU = targetUnit;

	//주변에 다른 Wraith가 있으면 타겟 대상을 바꿔줍니다
	for (auto eWraith : INFO.getTypeUnitsInRadius(AirUnitKind, E, unit->getPosition(), TILE_SIZE * 8))
	{
		targetUnit = eWraith->unit();
		break;
	}

	//흔벎닸瞳檢，낮쀼돕뒤寧뗏쨌듐
	if (!INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, unit->getPosition(), TILE_SIZE * 10).empty())
	{
		//일단 Idle상태일 때 가는 첫번째 초크포인트로 도망가기..
		Position movePosition = INFO.getFirstChokePosition(S);
		CommandUtil::move(unit, movePosition);
		tU = nullptr;

		return new ValkyrieIdleState();
	}
	else if (!INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, unit->getPosition(), TILE_SIZE * 10).empty())
	{
		//일단 Idle상태일 때 가는 첫번째 초크포인트로 도망가기..
		Position movePosition = INFO.getFirstChokePosition(S);
		CommandUtil::move(unit, movePosition);
		tU = nullptr;

		return new ValkyrieIdleState();
	}
	else if (!INFO.getTypeBuildingsInRadius(Zerg_Spore_Colony, E, unit->getPosition(), TILE_SIZE * 10).empty())
	{
		//일단 Idle상태일 때 가는 첫번째 초크포인트로 도망가기..
		Position movePosition = INFO.getFirstChokePosition(S);
		CommandUtil::move(unit, movePosition);
		tU = nullptr;

		return new ValkyrieIdleState();
	}

	//상대가 클로킹 했는데 스캔 없으면 공격 해제
	if (targetUnit->isCloaked() && INFO.getAvailableScanCount() < 1)
	{
		tU = nullptr;
		return new ValkyrieIdleState();
	}

	
	if (targetUnit->getTarget() == unit || targetUnit->getOrderTarget() == unit) //상대가 나를 타겟으로 잡고 있는 경우
	{
		CommandUtil::attackUnit(unit, targetUnit);
	}
	else //상대가 도망치고 있는 경우
	{
		////무빙샷 START////
		if (unit->getDistance(targetUnit) > unit->getType().airWeapon().maxRange())
		{
			if (unit->getTarget() != targetUnit)
			{
				CommandUtil::attackUnit(unit, targetUnit);
			}

			return nullptr;
		}

		if (unit->getAirWeaponCooldown() == 0)
		{
			unit->attack(targetUnit);
		}

		Position movePosition = targetUnit->getPosition() + Position((int)(targetUnit->getVelocityX() * 6), (int)(targetUnit->getVelocityY() * 6));

		unit->move(movePosition);
		
	}

	return nullptr;
}
State *ValkyrieFollowTankState::action()
{
	UnitInfo *w = INFO.getUnitInfo(unit, S);//w角unitinfo
	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(w->pos(), &dangerPoint, true);

	UnitInfo *frontTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());
	
	if (frontTank != nullptr)
	{
		if (getGroundDistance(frontTank->pos(), w->pos()) > 10 * TILE_SIZE)

		{
			CommandUtil::attackMove(unit, frontTank->pos());
			return nullptr;
		}
	}
	if (dangerPoint < 6 * TILE_SIZE)
	{
		
		CommandUtil::move(unit, INFO.getFirstExpansionLocation(S)->Center());
		return nullptr;
	}
	else // DangerUnit > 6 * TILE_SIZE
	{
		if (INFO.enemyRace == Races::Terran)
		{
			if (w->unit()->exists() && w->unit()->getTarget() != nullptr)
			{
				if (w->unit()->getTarget()->getType() == Terran_Battlecruiser) return nullptr;
			}

			for (auto et : INFO.getTypeUnitsInRadius(Terran_Battlecruiser, E, w->pos(), 20 * TILE_SIZE, false))
			{
				w->unit()->attack(et->unit());
				break;
			}
		}
		if (INFO.enemyRace == Races::Protoss)
		{
			if (w->unit()->exists() && w->unit()->getTarget() != nullptr)
			{
				if (w->unit()->getTarget()->getType() == Protoss_Carrier)
					return nullptr;
			}

			for (auto et : INFO.getTypeUnitsInRadius(Protoss_Carrier, E, w->pos(), 30 * TILE_SIZE, false))
			{
				w->unit()->attack(et->unit());
				break;
			}
		}
		UnitInfo *enemyBarrack = INFO.getClosestTypeUnit(E, w->pos(), Terran_Barracks);
		UnitInfo *Overlord = INFO.getClosestTypeUnit(E, w->pos(), Zerg_Overlord);
		// 떠있지 않은것은 타겟팅 하지 않는다.
		if (enemyBarrack != nullptr && !enemyBarrack->getLift())
			enemyBarrack = nullptr;

		UnitInfo *enemyEngineering = INFO.getClosestTypeUnit(E, w->pos(), Terran_Engineering_Bay);

		// 떠있지 않은것은 타겟팅 하지 않는다.
		if (enemyEngineering != nullptr && !enemyEngineering->getLift())
			enemyEngineering = nullptr;

		if (INFO.enemyRace == Races::Terran)
		{
		
		 if (enemyBarrack != nullptr && enemyEngineering != nullptr)
			{
				if (enemyBarrack->pos().getApproxDistance(w->pos()) > enemyEngineering->pos().getApproxDistance(w->pos()))
					CommandUtil::attackUnit(unit, enemyEngineering->unit());
				else
					CommandUtil::attackUnit(unit, enemyBarrack->unit());
			}
			else if (enemyBarrack == nullptr)
			{
				CommandUtil::attackUnit(unit, enemyEngineering->unit());
			}
			else
				CommandUtil::attackUnit(unit, enemyBarrack->unit());
		}
		if (INFO.enemyRace == Races::Zerg)
		{
			if (Overlord != nullptr)
				CommandUtil::attackUnit(unit, Overlord->unit());
		}





		if (!unit->isIdle()) return nullptr;

		for (auto eFloatingB : INFO.getBuildingsInRadius(E, w->pos(), 12 * TILE_SIZE, false, true, false))
		{
			CommandUtil::attackUnit(unit, eFloatingB->unit());
			break;
		}
	}

	/*
	if (unit->getHitPoints() < 50)
	{
	return new WraithIdleState();
	}

	if (!INFO.getUnitInfo(unit, S)->getEnemiesTargetMe().empty())
	{
	if (unit->isCloaked())
	{
	return new WraithIdleState();
	}
	else
	{
	if (unit->canCloak() && unit->getEnergy() > 40 && unit->getHitPoints() > 50)
	{
	unit->cloak();
	}
	else
	{
	return new WraithIdleState();
	}
	}
	}

	UnitInfo *fTank = nullptr;

	//fTank = TankManager::Instance().getCloseTankFromPosition(INFO.getMainBaseLocation(E)->Center());
	fTank = TankManager::Instance().getCloseTankFromPosition(SM.getMainAttackPosition());

	if (fTank != nullptr && unit->getDistance(fTank->unit()) > 3 * TILE_SIZE)
	{
	GoWithoutDamage(unit, fTank->pos());
	}
	*/
	return nullptr;
}
State *ValkyrieRetreatState::action()
{
	

	if (unit->getDistance(MYBASE) > 25 * TILE_SIZE)
	{
		CommandUtil::move(unit, MYBASE);
		return nullptr;
	}
	else
	{
		return new ValkyrieIdleState();
	}


	return nullptr;

}

State *ValkyrieDefenceState::action()
{

	if (INFO.getTypeUnitsInRadius(Protoss_Carrier, E, MYBASE, 20 * TILE_SIZE).size())
	{
		if (unit->getDistance(MYBASE) > 12 * TILE_SIZE)
		{
			CommandUtil::move(unit, MYBASE);
			return nullptr;
		}
	}
	else if (INFO.getTypeUnitsInRadius(Protoss_Carrier, E, INFO.getFirstExpansionLocation(S)->Center(), 20 * TILE_SIZE).size())
	{
		if (unit->getDistance(INFO.getFirstExpansionLocation(S)->Center()) > 13 * TILE_SIZE)

		{
			CommandUtil::move(unit, INFO.getFirstExpansionLocation(S)->Center());
			return nullptr;
		}

	}


	for (auto et : INFO.getTypeUnitsInRadius(Protoss_Carrier, E, unit->getPosition(), 30 * TILE_SIZE, false))
	{
		unit->attack(et->unit());
		break;
	}
		
	
	return nullptr;

}