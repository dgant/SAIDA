#include "WraithState.h"
#include "../InformationManager.h"
#include "../UnitManager/TankManager.h"
#include "../UnitManager/VultureManager.h"

using namespace MyBot;

bool defaultAction(Unit me)
{
	vector<Unit> enemiesTargetMe = INFO.getUnitInfo(me, S)->getEnemiesTargetMe();
	Unit enemy = nullptr;
	Unit closestTarget = nullptr;

	// ´©°¡ ³¯ °ø°ÝÇÏ³ª
	enemy = UnitUtil::GetClosestEnemyTargetingMe(me, enemiesTargetMe);

	//°¡Àå °¡±î¿î °ø°Ý°¡´ÉÇÑ Àû
	int range = max(me->getType().airWeapon().maxRange(), me->getType().groundWeapon().maxRange());
	closestTarget = me->getClosestUnit(BWAPI::Filter::IsEnemy, range);

	if (enemy != nullptr)	// °ø°Ý ¹ÞÀ¸¸é È¸ÇÇ
	{
		Position dir = me->getPosition() - enemy->getPosition();
		Position pos = getCirclePosFromPosByDegree(me->getPosition(), dir, 15);
		goWithoutDamage(me, pos, 1);
		return true;
	}
	else if (closestTarget != nullptr && closestTarget->getType() == UnitTypes::Zerg_Overlord)	// ¿À¹ö·Îµå ¹ß°ß½Ã °ø°Ý
	{
		//me->rightClick(closestTarget);
		CommandUtil::attackUnit(me, closestTarget);

		return true;
	}

	return false;
}

//////////////////////////////////// Scout ///////////////////////////////////////////////////
State *WraithScoutState::action(Position targetPos)
{
	if (unit == nullptr || unit->exists() == false)	return nullptr;

	if (!defaultAction(unit))
		goWithoutDamage(unit, targetPos, 1);

	return nullptr;
}

//////////////////////////////////// Idle ///////////////////////////////////////////////////
State *WraithIdleState::action()
{
	if (unit->isCloaked() && !unit->isUnderAttack())
	{
		unit->decloak();
	}

	if (!INFO.getUnitInfo(unit, S)->getEnemiesTargetMe().empty() && unit->canCloak() && unit->getEnergy() > 30)
	{
		unit->cloak();
	}

	if (isBeingRepaired(unit))
		return nullptr;
	Unit mineral = bw->getClosestUnit(MYBASE, Filter::IsMineralField);
	Position movePosition = (MYBASE + mineral->getPosition()) / 2;
	if (unit->getDistance(movePosition) > 4 * TILE_SIZE)
	CommandUtil::move(unit, movePosition);
		

	return nullptr;
}
//////////////////////////////////// AttackWraith ///////////////////////////////////////////////////
State *WraithAttackWraithState::action()
{
	if (tU != nullptr)
	{
		//cout << "action tU" << endl;
		action(tU);
	}

	return nullptr;
}

State *WraithAttackWraithState::action(Unit targetUnit)
{
	

	if (targetUnit == nullptr) return nullptr;

	tU = targetUnit;

	//ÁÖº¯¿¡ ´Ù¸¥ Wraith°¡ ÀÖÀ¸¸é Å¸°Ù ´ë»óÀ» ¹Ù²ãÁÝ´Ï´Ù
	for (auto eWraith : INFO.getTypeUnitsInRadius(Terran_Wraith, E, unit->getPosition(), TILE_SIZE * 8))
	{
		targetUnit = eWraith->unit();
		break;
	}

	//Èç¹û´æÔÚËþ£¬³·»Øµ½µÚÒ»¶ÂÂ·µã
	if (!INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, unit->getPosition(), TILE_SIZE * 10).empty())
	{
		//ÀÏ´Ü Idle»óÅÂÀÏ ¶§ °¡´Â Ã¹¹øÂ° ÃÊÅ©Æ÷ÀÎÆ®·Î µµ¸Á°¡±â..
		Position movePosition = INFO.getFirstChokePosition(S);
		CommandUtil::move(unit, movePosition);
		tU = nullptr;

		return new WraithIdleState();
	}

	//»ó´ë°¡ Å¬·ÎÅ· Çß´Âµ¥ ½ºÄµ ¾øÀ¸¸é °ø°Ý ÇØÁ¦
	if (targetUnit->isCloaked() && INFO.getAvailableScanCount() < 1)
	{
		//ÀÏ´Ü Idle»óÅÂÀÏ ¶§ °¡´Â Ã¹¹øÂ° ÃÊÅ©Æ÷ÀÎÆ®·Î µµ¸Á°¡±â..
		//Position movePosition = INFO.getFirstChokePosition(S);
		//CommandUtil::move(unit, movePosition);
		tU = nullptr;

		return new WraithIdleState();
	}

	//Å¬·ÎÅ· Ã³¸®
	if (targetUnit->getDistance(unit) <= targetUnit->getType().sightRange() && !unit->isCloaked())
	{
		unit->cloak();
	}
	else if (targetUnit->getDistance(unit) > targetUnit->getType().sightRange() && unit->isCloaked())
	{
		unit->decloak();
	}

	if (targetUnit->getTarget() == unit || targetUnit->getOrderTarget() == unit) //»ó´ë°¡ ³ª¸¦ Å¸°ÙÀ¸·Î Àâ°í ÀÖ´Â °æ¿ì
	{
		CommandUtil::attackUnit(unit, targetUnit);
	}
	else //»ó´ë°¡ µµ¸ÁÄ¡°í ÀÖ´Â °æ¿ì
	{
		////¹«ºù¼¦ START////
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
		/////¹«ºù¼¦ END///////
	}

	return nullptr;
}

State *WraithKillScvState::action(Position targetPosition)
{
	//Èç¹ûÕÒ²»µ½Ä¿±ê
	if (targetPosition == Positions::Unknown)
	{
		return new WraithIdleState();
	}
	//Èç¹ûÊÕµ½¹¥»÷
	if (!INFO.getUnitInfo(unit, S)->getEnemiesTargetMe().empty())
	{
		//Èç¹ûÔÚÒþÐÎ×´Ì¬£¬³·ÍË
		if (unit->isCloaked())
		{
			return new WraithIdleState();
		}
		else
		{//ÇÐ»»µ½ÒþÐÎ×´Ì¬
			if (unit->canCloak() && unit->getEnergy() > 40 && unit->getHitPoints() > 50)
			{
				unit->cloak();
			}
			else
			{//²»ÐÐµÄ»°¾Í³·ÁË
				return new WraithIdleState();
			}
		}
	}
	//Èç¹û²»ÔÚ´ÌÉ±µã£¬µ½ÄÇ±ßÈ¥
	if (unit->getPosition().getDistance(targetPosition) > 6 * TILE_SIZE)
	{
		goWithoutDamage(unit, targetPosition, 1);
		return nullptr;
	}

	
		UnitInfo *closestDefense = INFO.getClosestTypeUnit(E, unit->getPosition(), Terran_Bunker, 10 * TILE_SIZE, false);

	if (closestDefense != nullptr && unit->getDistance(closestDefense->pos()) < 8 * TILE_SIZE)
	{
		if (!unit->isCloaked())
		{
			//printf("[%d] µÚ·Î µµ¸Á°¡¾ßÇØ (%d, %d)\n", unit->getID(), unit->getPosition().x / TILE_SIZE, unit->getPosition().y / TILE_SIZE);
			moveBackPostion(INFO.getUnitInfo(unit, S), closestDefense->pos(), 1 * TILE_SIZE);
			//CommandUtil::backMove(unit, getBackPostion(unit->getPosition(), closestBunker->pos(), 2));
			return nullptr;
		}

		else if ((unit->isCloaked() && unit->getEnergy() < 10) && (TIME % 24 * 2 == 0))
		{
			
			moveBackPostion(INFO.getUnitInfo(unit, S), closestDefense->pos(), 6 * TILE_SIZE);
			return nullptr;
		}
	}
	uList enemyWorkerList;
	if (INFO.enemyRace == Races::Terran)
	 enemyWorkerList = INFO.getTypeUnitsInRadius(Terran_SCV, E, targetPosition, TILE_SIZE * 18, false);


	if (INFO.enemyRace == Races::Protoss)
	enemyWorkerList = INFO.getTypeUnitsInRadius(Protoss_Probe, E, targetPosition, TILE_SIZE * 18, false);

	if (INFO.enemyRace == Races::Zerg)
	enemyWorkerList = INFO.getTypeUnitsInRadius(Zerg_Drone, E, targetPosition, TILE_SIZE * 18, false);

	uList enemyMedicList = INFO.getTypeUnitsInRadius(Terran_Medic, E, targetPosition, TILE_SIZE * 8, false);
	uList turretList = INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, targetPosition, TILE_SIZE * 12, true, true);
	uList CannonList = INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, targetPosition, TILE_SIZE * 12, true, true);
	uList ColonyList = INFO.getTypeBuildingsInRadius(Zerg_Spore_Colony, E, targetPosition, TILE_SIZE * 12, true, true);
	
	Unit targetUnit = nullptr;
	int targetId = INT_MAX;

	if (INFO.enemyRace == Races::Terran)
	{
		for (auto turret : turretList)
		{
			if (turret->isComplete())
			{
				return new WraithIdleState();
			}
			else if (turret->hp() > 150)
			{
				return new WraithIdleState();
			}
			else
			{
				targetUnit = turret->unit()->getBuildUnit();
			}
		}
	}
	if (INFO.enemyRace == Races::Protoss)
	{
		for (auto Cannon : CannonList)
		{
			if (Cannon->isComplete())
			{
				return new WraithIdleState();
			}
			else if (Cannon->hp() > 150)
			{
				return new WraithIdleState();
			}
			else
			{
				targetUnit = Cannon->unit()->getBuildUnit();
			}
		}
	}

	if (INFO.enemyRace == Races::Zerg)
	{
		for (auto Colony : ColonyList)
		{
			if (Colony->isComplete())
			{
				return new WraithIdleState();
			}
			else if (Colony->hp() > 150)
			{
				return new WraithIdleState();
			}
			else
			{
				targetUnit = Colony->unit()->getBuildUnit();
			}
		}
	}



	// Èç¹û¿ÉÒÔ´òËþµÄ»°
	if (targetUnit != nullptr)
	{
		CommandUtil::attackUnit(unit, targetUnit);
		return nullptr;
	}

	// ÕÒÕÒ¿´ÓÐÃ»ÓÐÓÐÇ¹±øµÄµï±¤
	uList enemyBuildingList = INFO.getBuildingsInRadius(E, unit->getPosition(), 10 * TILE_SIZE, true, false, true);
	bool isExistMarineInBunker = false;

	for (auto eb : enemyBuildingList)
	{
	      if (eb->type() == Terran_Bunker && eb->getMarinesInBunker())
		  isExistMarineInBunker = true;
		
	}

	if (isExistMarineInBunker)
	{
		//Èç¹ûÓÐµØ±¤µÄ»°
		if (unit->isCloaked())
		{
			if (unit->isUnderAttack())
				return new WraithIdleState();
		}
		else
		{
			if (unit->isUnderAttack())
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
	}

	// ¹¥»÷µØ±¤
	if (targetUnit != nullptr)
	{
		CommandUtil::attackUnit(unit, targetUnit);
		return nullptr;
	}

	//Ã»ÓÐÅ©Ãñ
	if (enemyWorkerList.empty())
		return new WraithIdleState();

	bool needCloak = false;

	//	if (unit->getTarget() == nullptr) //Å¸°Ù À¯´ÖÀÌ ¾ø´Â °æ¿ì
	{
		if (!enemyMedicList.empty()) //¸ÞµñÀÌ ÀÖÀ¸¸é ¸ÞµñºÎÅÍ Ã³¸®
		{
			for (auto medic : enemyMedicList) //ID°ªÀÌ °¡Àå ÀÛÀº À¯´ÖÀ» ¿ì¼±¼øÀ§·Î °ø°ÝÇÔ
			{
				if (medic->id() < targetId)
				{
					targetUnit = medic->unit();
					targetId = medic->id();
				}
			}
		}
		else
		{//ÓÐÅ©Ãñ
			for (auto worker : enemyWorkerList)
			{
				// Å¬·ÎÅ· ¾ÈµÇ¾î ÀÖÀ» ¶§ º¡Ä¿ °¡±îÀÌ ÀÖ´Â SCV ´Â Á¦¿Ü
				if (!unit->isCloaked() && closestDefense != nullptr && worker->unit()->getDistance(closestDefense->pos()) < 5 * TILE_SIZE)
				{
					continue;
				}

				if (worker->id() > targetId)
				{
					targetUnit = worker->unit();
					targetId = worker->id();
				}
			}

			// º¡Ä¿ ±ÙÃ³ scv ¹Û¿¡ ³²Áö ¾ÊÀº °æ¿ì Å¬·ÎÅ· ¿äÃ»
			if (targetUnit == nullptr)
			{
				for (auto worker : enemyWorkerList) //ID°ªÀÌ °¡Àå ÀÛÀº À¯´ÖÀ» ¿ì¼±¼øÀ§·Î °ø°ÝÇÔ
				{
					if (worker->id() < targetId)
					{
						targetUnit = worker->unit();
						targetId = worker->id();
					}
				}

				if (targetUnit != nullptr)
					needCloak = true;
			}
		}

		if (targetUnit != nullptr)
		{
			// Å¬·ÎÅ·ÀÌ ÇÊ¿ä
			if (needCloak)
			{
				// Å¬·ÎÅ· ¾ÈµÇ¾î ÀÖÀ¸¸é Å¬·ÎÅ·
				if (!unit->isCloaked())
				{
					if (unit->canCloak() && unit->getEnergy() > 40)
					{
						unit->cloak();
					}
				}
				else
				{
					// ¸¶³ª ¾øÀ¸¸é ¤¼¤¼
					//if (unit->getEnergy() < 10)
					//	return new WraithIdleState();

					if (unit->getEnergy() > 10)
					{
						// Å¬·ÎÅ· µÇ¾î ÀÖÀ¸¸é °ø°Ý
						CommandUtil::attackUnit(unit, targetUnit);

						if (unit->getDistance(targetUnit) > 4 * TILE_SIZE)
							CommandUtil::move(unit, (unit->getPosition() + targetUnit->getPosition()) / 2);
					}
				}
			}
			else
			{
				CommandUtil::attackUnit(unit, targetUnit);

				if (unit->getDistance(targetUnit) > 4 * TILE_SIZE)
					CommandUtil::move(unit, (unit->getPosition() + targetUnit->getPosition()) / 2);
			}
		}
	}

	return nullptr;
}

State *WraithFollowTankState::action()
{
	UnitInfo *w = INFO.getUnitInfo(unit, S);
	//Èç¹ûÊÕµ½¹¥»÷
	if (!INFO.getUnitInfo(unit, S)->getEnemiesTargetMe().empty())
	{
		//Èç¹ûÔÚÒþÐÎ×´Ì¬£¬³·ÍË
		if (!(unit->isCloaked()))
		{//ÇÐ»»µ½ÒþÐÎ×´Ì¬
			if (unit->canCloak() && unit->getEnergy() > 40 && unit->getHitPoints() > 50)
			{
				unit->cloak();
			}
			
		}
	}

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(w->pos(), &dangerPoint, true);

	UnitInfo *frontTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());
	if (frontTank != nullptr)
	{
		if (getGroundDistance(frontTank->pos(), w->pos()) > 15 * TILE_SIZE)

		{
			CommandUtil::attackMove(unit, frontTank->pos());
			return nullptr;
		}
	}
	if (dangerUnit == nullptr)
	{
		
		UnitInfo *closestAttack = INFO.getClosestUnit(E, unit->getPosition(), GroundCombatKind, 10 * TILE_SIZE, false, false, true);
		if (closestAttack != nullptr)
		{
			kiting(w, closestAttack, dangerPoint, 3 * TILE_SIZE);
			return nullptr;
		}

	}

	if (dangerPoint < 6 * TILE_SIZE)//Èç¹ûÓÐdangerÇÒÓÐÌ¹¿Ë£¬³¯×ÅÌ¹¿ËÒÆ¶¯
	{
		UnitInfo *closestAttack = INFO.getClosestUnit(E, unit->getPosition(), AirUnitKind, 10 * TILE_SIZE, false, false, true);
		if (closestAttack != nullptr)
		{
			kiting(w, closestAttack, dangerPoint, 3 * TILE_SIZE);
			return nullptr;
		}
		else
		{
			UnitInfo *closestground = INFO.getClosestUnit(E, unit->getPosition(), GroundCombatKind, 10 * TILE_SIZE, false, false, true);
			if (closestground != nullptr)
			{
				kiting(w, closestground, dangerPoint, 3 * TILE_SIZE);
				return nullptr;
			}
		}
	}
	else // DangerUnit > 6 * TILE_SIZEµÄÊ±ºò£¬ÎÒÃÇ¸ÉÂïÄØ
	{
		if (INFO.enemyRace == Races::Terran)
	 {
		if (w->unit()->exists() && w->unit()->getTarget() != nullptr)
		{
			if (w->unit()->getTarget()->getType() == Terran_Siege_Tank_Tank_Mode) return nullptr;
		}

		for (auto et : INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, w->pos(), 10 * TILE_SIZE, false))
		{
			w->unit()->attack(et->unit());
			break;
		}
	 }
		if (INFO.enemyRace == Races::Protoss)
		{
			if (w->unit()->exists() && w->unit()->getTarget() != nullptr)
			{
				if (w->unit()->getTarget()->getType() ==Protoss_Dragoon) return nullptr;
			}

			for (auto et : INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, w->pos(), 10 * TILE_SIZE, false))
			{
				w->unit()->attack(et->unit());
				break;
			}
		}
		UnitInfo *enemyBarrack = INFO.getClosestTypeUnit(E, w->pos(), Terran_Barracks);
		UnitInfo *Overlord = INFO.getClosestTypeUnit(E, w->pos(), Zerg_Overlord);
		// ¶°ÀÖÁö ¾ÊÀº°ÍÀº Å¸°ÙÆÃ ÇÏÁö ¾Ê´Â´Ù.
		if (enemyBarrack != nullptr && !enemyBarrack->getLift())
			enemyBarrack = nullptr;

		UnitInfo *enemyEngineering = INFO.getClosestTypeUnit(E, w->pos(), Terran_Engineering_Bay);

		// ¶°ÀÖÁö ¾ÊÀº°ÍÀº Å¸°ÙÆÃ ÇÏÁö ¾Ê´Â´Ù.
		if (enemyEngineering != nullptr && !enemyEngineering->getLift())
			enemyEngineering = nullptr;

		if (INFO.enemyRace == Races::Terran)
		{
			if (enemyBarrack == nullptr &&  enemyEngineering == nullptr)
			{
				if (frontTank != nullptr)
				{
					UnitInfo *closestTarget = INFO.getClosestUnit(E, frontTank->pos(), GroundUnitKind, 15 * TILE_SIZE, true, true);
					Position TargetPos = (closestTarget == nullptr) ? SM.getMainAttackPosition() : closestTarget->pos();

					CommandUtil::move(unit, getDirectionDistancePosition(frontTank->pos(), TargetPos, 4 * TILE_SIZE));
				}
				else
					CommandUtil::move(unit, MYBASE);
			}
			else if (enemyBarrack != nullptr && enemyEngineering != nullptr)
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
			if (Overlord == nullptr)
			{
				if (frontTank != nullptr)
				{
					UnitInfo *closestTarget = INFO.getClosestUnit(E, frontTank->pos(), GroundUnitKind, 15 * TILE_SIZE, true, true);
					Position TargetPos = (closestTarget == nullptr) ? SM.getMainAttackPosition() : closestTarget->pos();

					CommandUtil::move(unit, getDirectionDistancePosition(frontTank->pos(), TargetPos, 4 * TILE_SIZE));
				}
				else
					CommandUtil::move(unit, MYBASE);
			}
			else
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

State *WraithCloakState::action()
{
	
	if (unit->canCloak())
	{
		unit->cloak();
	}
	if (unit->isCloaked() && unit->getEnergy() >7)
	{
		uList turretList = INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, unit->getPosition(), TILE_SIZE * 10, true, true);
		uList CannonList = INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, unit->getPosition(), TILE_SIZE * 10, true, true);
		uList ColonyList = INFO.getTypeBuildingsInRadius(Zerg_Spore_Colony, E, unit->getPosition(), TILE_SIZE * 10, true, true);

		if (INFO.enemyRace == Races::Terran &&  turretList.size())
		{
			for (auto turret : turretList)
			{
				if (turret->isComplete())
				{
					return new WraithRetreatState();
				}
				else if (turret->hp() > 150)
				{
					return new WraithRetreatState();
				}
				else
				{
					CommandUtil::attackUnit(unit, turret->unit());
					return nullptr;
				}
			}
		}
		else if (INFO.enemyRace == Races::Protoss && CannonList.size())
		{
			for (auto Cannon : CannonList)
			{
				if (Cannon->isComplete())
				{
					return new WraithRetreatState();
				}
				else if (Cannon->hp() > 150)
				{
					return new WraithRetreatState();
				}
				else
				{
					CommandUtil::attackUnit(unit, Cannon->unit());
					return nullptr;
				}
			}
		}

		else if (INFO.enemyRace == Races::Zerg && ColonyList.size())
		{
			for (auto Colony : ColonyList)
			{
				if (Colony->isComplete())
				{
					return new WraithRetreatState();
				}
				else if (Colony->hp() > 150)
				{
					return new WraithRetreatState();
				}
				else
				{
					CommandUtil::attackUnit(unit, Colony->unit());
					return nullptr;
				}
			}
		}
		else
		{
			UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, unit->getPosition(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

			if (closestWorker != nullptr)
			{
				CommandUtil::attackUnit(unit, closestWorker->unit());
			}
			else
			{

				UnitInfo *closestAttack = INFO.getClosestUnit(E, unit->getPosition(), GroundCombatKind, 20 * TILE_SIZE, true, false, true);
				if (closestAttack == nullptr)
				{
					CommandUtil::attackMove(unit, SM.getMainAttackPosition());
					return nullptr;
				}
				else
				{
					CommandUtil::attackUnit(unit, closestAttack->unit());
					return nullptr;
				}
			}
		}
	}
	else if ((unit->isCloaked() && unit->getEnergy() <= 7))
	{
		return new WraithRetreatState();
	}
	
	return nullptr;
}

State *WraithRetreatState::action()
{
	if (unit->getDistance(MYBASE) < 90 * TILE_SIZE)
	{
		if (unit->isCloaked() && !unit->isUnderAttack())
		{
			unit->decloak();
		}
	}


	if (unit->getDistance(MYBASE) > 25 * TILE_SIZE)
	{
         CommandUtil::move(unit, MYBASE);
		 return nullptr;
	}
	else
	{
		return new WraithIdleState();
	}
	

	return nullptr;

}

State *WraithBattleAssistState::action()
{
	uList enemyWorkerList;
	if (INFO.enemyRace == Races::Terran)
		enemyWorkerList = INFO.getTypeUnitsInRadius(Terran_SCV, E, unit->getPosition(), TILE_SIZE * 8, false);


	if (INFO.enemyRace == Races::Protoss)
		enemyWorkerList = INFO.getTypeUnitsInRadius(Protoss_Probe, E, unit->getPosition(), TILE_SIZE * 10, false);

	if (INFO.enemyRace == Races::Zerg)
		enemyWorkerList = INFO.getTypeUnitsInRadius(Zerg_Drone, E, unit->getPosition(), TILE_SIZE * 10, false);

	uList enemyMedicList = INFO.getTypeUnitsInRadius(Terran_Medic, E, unit->getPosition(), TILE_SIZE * 10, false);
	uList turretList = INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, unit->getPosition(), TILE_SIZE * 12, true, true);
	uList CannonList = INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, unit->getPosition(), TILE_SIZE * 12, true, true);
	uList ColonyList = INFO.getTypeBuildingsInRadius(Zerg_Spore_Colony, E, unit->getPosition(), TILE_SIZE * 12, true, true);
	uList AroundVultureList = INFO.getTypeBuildingsInRadius(Terran_Vulture, S, unit->getPosition(), TILE_SIZE * 12, true, true);
	uList AroundMarineList = INFO.getTypeBuildingsInRadius(Terran_Marine, S, unit->getPosition(), TILE_SIZE * 12, true, true);
	Unit targetUnit = nullptr;
	UnitInfo *w = INFO.getUnitInfo(unit, S);//½ÓÊÜµ¥Î»ÐÅÏ¢
	
	if (INFO.enemyRace == Races::Terran)
	{
		for (auto turret : turretList)
		{
			if (turret->isComplete())
			{
				return new WraithRetreatState();
			}
			else if (turret->hp() > 150)
			{
				return new WraithRetreatState();
			}
			else
			{
				targetUnit = turret->unit()->getBuildUnit();
			}
		}
	}
	if (INFO.enemyRace == Races::Protoss)
	{
		for (auto Cannon : CannonList)
		{
			if (Cannon->isComplete())
			{
				return new WraithRetreatState();
			}
			else if (Cannon->hp() > 150)
			{
				return new WraithRetreatState();
			}
			else
			{
				targetUnit = Cannon->unit()->getBuildUnit();
			}
		}
	}

	if (INFO.enemyRace == Races::Zerg)
	{
		for (auto Colony : ColonyList)
		{
			if (Colony->isComplete())
			{
				return new WraithRetreatState();
			}
			else if (Colony->hp() > 150)
			{
				return new WraithRetreatState();
			}
			else
			{
				targetUnit = Colony->unit()->getBuildUnit();
			}
		}
	}
	if (targetUnit != nullptr)
	{
		CommandUtil::attackUnit(unit, targetUnit);
		return nullptr;
	}
	if (!enemyWorkerList.empty())
		return new WraithKillScvState();

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(w->pos(), &dangerPoint, true);
	UnitInfo *frontVulture = VultureManager::Instance().getFrontVultureFromPos(SM.getMainAttackPosition());
	if (frontVulture != nullptr)
	{
		if (getGroundDistance(frontVulture->pos(), w->pos()) > 15 * TILE_SIZE && (AroundVultureList.size() <= 1) && (AroundMarineList.size() <= 2))

		{
			CommandUtil::attackMove(unit, frontVulture->pos());
			return nullptr;
		}
	}

	if (dangerUnit == nullptr)
	{

		UnitInfo *closestAttack = INFO.getClosestUnit(E, unit->getPosition(), GroundCombatKind, 10 * TILE_SIZE, false, false, true);
		if (closestAttack != nullptr)
		{
			kiting(w, closestAttack, dangerPoint, 3 * TILE_SIZE);
			return nullptr;
		}

	}

	if (dangerPoint < 6 * TILE_SIZE)//Èç¹ûÓÐdangerÇÒÓÐÌ¹¿Ë£¬³¯×ÅÌ¹¿ËÒÆ¶¯
	{
		UnitInfo *closestAttack = INFO.getClosestUnit(E, unit->getPosition(), AirUnitKind, 10 * TILE_SIZE, false, false, true);
		if (closestAttack != nullptr)
		{
			kiting(w, closestAttack, dangerPoint, 3 * TILE_SIZE);
			return nullptr;
		}
		else
		{
			UnitInfo *closestground = INFO.getClosestUnit(E, unit->getPosition(), GroundCombatKind, 10 * TILE_SIZE, false, false, true);
			if (closestground != nullptr)
			{
				kiting(w, closestground, dangerPoint, 3 * TILE_SIZE);
				return nullptr;
			}
		}
	}
	else // DangerUnit > 6 * TILE_SIZEµÄÊ±ºò£¬ÎÒÃÇ¸ÉÂïÄØ
	{
		if (INFO.enemyRace == Races::Terran)
		{
			if (w->unit()->exists() && w->unit()->getTarget() != nullptr)
			{
				if (w->unit()->getTarget()->getType() == Terran_Siege_Tank_Tank_Mode) return nullptr;
			}

			for (auto et : INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, w->pos(), 10 * TILE_SIZE, false))
			{
				w->unit()->attack(et->unit());
				break;
			}
		}
		if (INFO.enemyRace == Races::Protoss)
		{
			if (w->unit()->exists() && w->unit()->getTarget() != nullptr)
			{
				if (w->unit()->getTarget()->getType() == Protoss_Dragoon) return nullptr;
			}

			for (auto et : INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, w->pos(), 10 * TILE_SIZE, false))
			{
				w->unit()->attack(et->unit());
				break;
			}
		}
		UnitInfo *enemyBarrack = INFO.getClosestTypeUnit(E, w->pos(), Terran_Barracks);
		UnitInfo *Overlord = INFO.getClosestTypeUnit(E, w->pos(), Zerg_Overlord);
		// ¶°ÀÖÁö ¾ÊÀº°ÍÀº Å¸°ÙÆÃ ÇÏÁö ¾Ê´Â´Ù.
		if (enemyBarrack != nullptr && !enemyBarrack->getLift())
			enemyBarrack = nullptr;

		UnitInfo *enemyEngineering = INFO.getClosestTypeUnit(E, w->pos(), Terran_Engineering_Bay);

		// ¶°ÀÖÁö ¾ÊÀº°ÍÀº Å¸°ÙÆÃ ÇÏÁö ¾Ê´Â´Ù.
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