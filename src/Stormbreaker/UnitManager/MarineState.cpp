#include "MarineState.h"
using namespace MyBot;

State *MarineIdleState::action()
{
	Unit b = MM.getBunker();

	
	if (b != nullptr && unit->isLoaded() && b->isCompleted() && unit->getTransport() != b)
	{
		unit->getTransport()->unload(unit);

		return nullptr;
	}

	if (b != nullptr && b->getLoadedUnits().size() != 4) {

		if (b->isCompleted())
			unit->rightClick(b);
		else
			CommandUtil::attackMove(unit, b->getPosition(), true);

		return nullptr;
	}


	if (INFO.enemyRace == Races::Zerg) {
		uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

		if (commandCenterList.empty())
			return nullptr;

		if (commandCenterList.size() == 1)
		{
			
			if (ESM.getEnemyInitialBuild() <= Zerg_12_Ap || ESM.getEnemyInitialBuild() == UnknownBuild)
			{
				Unit mineral = bw->getClosestUnit(commandCenterList[0]->pos(), Filter::IsMineralField);

				if (mineral != nullptr) {
					Position target = (commandCenterList[0]->pos() + mineral->getPosition()) / 2;
					unit->move(target);
					return nullptr;
				}
			}
		}
	}
	else if (ESM.getEnemyInitialBuild() == Toss_cannon_rush && INFO.getSecondChokePosition(S)) {
		if (INFO.getFirstChokePosition(S).getApproxDistance(unit->getPosition()) - INFO.getFirstChokePosition(S).getApproxDistance(INFO.getSecondChokePosition(S)) > 3 * TILE_SIZE)
			CommandUtil::move(unit, INFO.getSecondChokePosition(S), true);
		else {
			uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 7 * TILE_SIZE);

			if (eList.empty()) {
				uList eBuildingList = INFO.getBuildingsInRadius(E, unit->getPosition(), 7 * TILE_SIZE);

				if (eBuildingList.empty())
					CommandUtil::attackMove(unit, INFO.getSecondChokePosition(S));
				else
					CommandUtil::attackUnit(unit, eBuildingList.at(0)->unit());
			}
			else
				return new MarineAttackState(eList.at(0));
		}

		return nullptr;
	}

	if (unit->getDistance(MM.waitingNearCommand) > 50) {
		//unit->move(pos);
		CommandUtil::move(unit, MM.waitingNearCommand, true);
	}

	return nullptr;
}

State *MarineAttackState::action() {
	if (m_targetUnit != nullptr) {
		UnitInfo *eUnit = INFO.getUnitInfo(m_targetUnit, E);

		
		if (!eUnit) {
			
			uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 8 * TILE_SIZE);

			if (!eList.empty()) {
				m_targetUnit = eList.at(0)->unit();
				m_targetPos = m_targetUnit->getPosition();
			}
			
			else if (eList.empty() && (EIB == Toss_1g_forward || EIB == Toss_2g_forward)) {
				m_targetUnit = nullptr;
				m_targetPos = SM.getMainAttackPosition();
			}
			else
				return new MarineIdleState();

			return nullptr;
		}

	
		if (eUnit->pos() == Positions::Unknown) {
			if (unit->isMoving())
				return nullptr;

			return new MarineIdleState();
		}

		m_targetPos = eUnit->pos();
	}

	uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 10 * TILE_SIZE);

	
	if (eList.empty()) {
		if (unit->getPosition().getApproxDistance(m_targetPos) < 2 * TILE_SIZE && m_targetUnit == nullptr)
			return new MarineIdleState();

		if (unit->isLoaded()) {
			Unit bunker = MM.getBunker();
			bunker->unload(unit);

			return nullptr;
		}
	}

	int minHP = INT_MAX;
	int minDist = INT_MAX;
	Unit target = nullptr;
	Unit closest = nullptr;
	Unit lowestHP = nullptr;

	for (auto e : eList) {
		
		if (e->type().groundWeapon().targetsGround()) {
			int gap;
			int enemyRange = UnitUtil::GetAttackRange(e->unit(), unit);
			int selfRange = UnitUtil::GetAttackRange(unit, e->unit());
			int dist = getAttackDistance(e->unit(), unit);

			
			if (e->unit()->getOrderTarget() != nullptr && e->unit()->getOrderTarget() == unit
					|| e->unit()->getTarget() != nullptr && e->unit()->getTarget() == unit) {
				if (enemyRange >= selfRange)
					gap = -64;
				else
					gap = (int)(E->topSpeed(e->type()) * 24);
			}
			
			else {

			
				if (enemyRange + 64 < selfRange)
					gap = (int)(E->topSpeed(e->type()) * 14);
				
				else
					gap = -64;
			}

		
			if (enemyRange + gap >= dist) {
				CommandUtil::backMove(unit, e->unit());
				
				return nullptr;
			}

			if (dist < minDist) {
				minDist = dist;
				closest = e->unit();

				if (unit->isInWeaponRange(e->unit())) {
					if (minHP > e->hp()) {
						lowestHP = e->unit();
						minHP = e->hp();
					}
				}
			}
		}
	}

	target = lowestHP ? lowestHP : closest;

	if (!target) {
		uList eBuildings = INFO.getBuildingsInRadius(E, unit->getPosition(), 10 * TILE_SIZE);

		for (auto b : eBuildings) {
			if (b->type() == Protoss_Pylon) {
				target = b->unit();
				break;
			}
		}
	}

	if (target)
		CommandUtil::attackUnit(unit, target);
	else if (m_targetPos.isValid())
		CommandUtil::attackMove(unit, m_targetPos);

	return nullptr;
}


State *MarineKillScouterState::action(Position lastScouterPosition)
{
	if (!unit->isMoving() && !unit->isAttacking() && TIME % 24 == 0) {
		unit->stop();
		return nullptr;
	}

	
	if (!isSameArea(INFO.getMainBaseLocation(S)->getPosition(), unit->getPosition()) &&
			INFO.getFirstExpansionLocation(S) && !isSameArea(INFO.getFirstExpansionLocation(S)->getPosition(), unit->getPosition())) {

		uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

		if (commandCenterList.empty()) return nullptr;

		if (commandCenterList.size() == 1)
		{
			
			if (ESM.getEnemyInitialBuild() <= Zerg_9_Balup || ((INFO.enemyRace == Races::Zerg || INFO.enemyRace == Races::Unknown) && ESM.getEnemyInitialBuild() == UnknownBuild))
			{
				Unit mineral = bw->getClosestUnit(commandCenterList[0]->pos(), Filter::IsMineralField);
				Position target = (commandCenterList[0]->pos() + mineral->getPosition()) / 2;
				unit->move(target);

				return nullptr;
			}
		}

		Position pos = (INFO.getSecondChokePosition(S) + INFO.getFirstExpansionLocation(S)->Center()) / 2;
		CommandUtil::attackMove(unit, pos);
		return nullptr;

	}

	UnitInfo *uinfo = INFO.getUnitInfo(unit, S);

	if (uinfo->getVeryFrontEnemyUnit() != nullptr)
	{
		UnitInfo *enemUinfo = INFO.getUnitInfo(uinfo->getVeryFrontEnemyUnit(), E);

		if (!enemUinfo)
			return nullptr;

		attackFirstkiting(uinfo, enemUinfo, enemUinfo->pos().getApproxDistance(unit->getPosition()), 3 * TILE_SIZE);

	}
	else
	{
		UnitInfo *closestEnem = INFO.getClosestUnit(E, uinfo->pos(), MyBot::AllKind, 2000, true);

		if (closestEnem != nullptr)
		{
			
			CommandUtil::attackUnit(unit, closestEnem->unit());
		
		}
		else
			CommandUtil::attackMove(unit, lastScouterPosition);
	}


	return nullptr;
}

State *MarineKitingState::action()
{

	if (unit->isLoaded())
		return new MarineDefenceState();

	
	UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), AllUnitKind);

	if (closeUnit == nullptr || unit->getDistance(closeUnit->pos()) > 1600)
		return new MarineIdleState();

	int distanceFromMainBase = 0;

	theMap.GetPath(unit->getPosition(), MYBASE, &distanceFromMainBase);

	if (distanceFromMainBase >= 1600) {
		return new MarineDefenceState();
	}

	Unit bunker = MM.getBunker();


	if (bunker != nullptr && bunker->isCompleted()) {
		return new MarineDefenceState();
	}

	if (ESM.getEnemyInitialBuild() <= Zerg_12_Pool) 
	{
		if (INFO.getTypeUnitsInRadius(Zerg_Zergling, E, unit->getPosition(), 8 * TILE_SIZE).size()
				> INFO.getTypeUnitsInRadius(Terran_Marine, S, unit->getPosition(), 8 * TILE_SIZE).size())
		{
			Position myBase = INFO.getMainBaseLocation(S)->Center();

			TilePosition myBaseTile = INFO.getMainBaseLocation(S)->getTilePosition();

			if (!isSameArea(theMap.GetArea((WalkPosition)unit->getPosition()), theMap.GetArea(myBaseTile))
					|| INFO.getFirstChokePosition(S).getApproxDistance(unit->getPosition()) < 3 * TILE_SIZE)
			{
				unit->move(myBase);
				return nullptr;
			}
		}
	}

	if (!unit->isInWeaponRange(closeUnit->unit()))
	{
		CommandUtil::attackUnit(unit, closeUnit->unit());
	}
	// 사거리 안이면
	else {
		uList Scvs = INFO.getTypeUnitsInRadius(Terran_SCV, S, (unit->getPosition() + closeUnit->pos()) / 2, 50);

		if (unit->getGroundWeaponCooldown() == 0 && (Scvs.size() >= 3 || unit->getDistance(closeUnit->unit()) > 2 * TILE_SIZE))
		{
			CommandUtil::attackUnit(unit, closeUnit->unit());
		}
		else
		{
			if (closeUnit->type().groundWeapon().maxRange() < 4 * TILE_SIZE) {
				moveBackPostionMarine(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 1 * TILE_SIZE);
			}
		}

	}

	return nullptr;
}

State *MarineZealotDefenceState::action()
{
	UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), AllUnitKind);

	if (closeUnit == nullptr) {

		Unit bunker = MM.getBunker();

		if (bunker && bunker->isCompleted()) {
			if (!unit->isLoaded()) {
				if (unit->isSelected()) cout << "벙커 들어가자 " << endl;

				unit->rightClick(bunker);
				return nullptr;
			}
		}

		if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) < 4) {

			if (unit->isSelected()) cout << "탱크 한테 붙자" << endl;

			UnitInfo *closeTank = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Siege_Tank_Tank_Mode, 0, false, true);

			if (closeTank) {
				if (unit->getDistance(closeTank->pos()) > 2 * TILE_SIZE) {
					CommandUtil::move(unit, closeTank->pos());
				}
			}
		}

		return nullptr;
	}

	Unit bunker = MM.getBunker();

	if (bunker != nullptr && bunker->isCompleted()) {

		uList vultures = INFO.getTypeUnitsInRadius(Terran_Vulture, S, MYBASE, 30 * TILE_SIZE);

		if (unit->isLoaded()) {
			if (vultures.size() != 0)
				return nullptr;

			if (!unit->isInWeaponRange(closeUnit->unit()))
			{
				bunker->unload(unit);
			}

			return nullptr;
		}
		
		else {

			if (vultures.size() != 0 && bunker->getLoadedUnits().size() != 4) {
				CommandUtil::rightClick(unit, bunker);
				return nullptr;
			}

			if (!unit->isInWeaponRange(closeUnit->unit()))
			{
				CommandUtil::attackUnit(unit, closeUnit->unit());
			}
		
			else {
			
				if (unit->getPosition().getDistance(closeUnit->pos()) > 4 * TILE_SIZE) {
					CommandUtil::attackUnit(unit, closeUnit->unit());
				}
				else {
					
					if (bunker->getLoadedUnits().size() != 4) {
						CommandUtil::rightClick(unit, bunker);
					}
					else {
						if (unit->getDistance(MYBASE) <= 1 * TILE_SIZE) {
							CommandUtil::attackUnit(unit, closeUnit->unit());
						}
						else {
							CommandUtil::move(unit, MYBASE);
						}
					}
				}
			}
		}
	}
	else {
	
		Unit barrack = MM.getFirstBarrack();
		Unit supply = MM.getNextBarrackSupply();

		if (barrack == nullptr || supply == nullptr)
			return nullptr;

		if (!unit->isInWeaponRange(closeUnit->unit()))
		{
			CommandUtil::attackUnit(unit, closeUnit->unit());
		}
		else {

		
			int top = supply->getTop();
			int bottom = barrack->getBottom();
			int newX = MM.getFirstBarrack()->getRight();

			int margin = TILE_SIZE; 
			Position posTop = Position(newX + 12, top - margin);
			Position posBottom = Position(newX, bottom + margin);
			Position target;

			if (unit->getPosition().getDistance(closeUnit->pos()) > 4 * TILE_SIZE) {
				
				CommandUtil::attackUnit(unit, closeUnit->unit());
			}
			else {

				if ((unit->getPosition().y <= top && closeUnit->pos().y >= bottom)
						|| (unit->getPosition().y >= bottom && closeUnit->pos().y <= top)) {
					CommandUtil::attackUnit(unit, closeUnit->unit());
				}
			
				else {

					
					if (unit->getPosition().y <= top) {
					
						if (abs(unit->getPosition().y - (top - margin)) <= margin && abs(unit->getPosition().x - newX) <= margin) {
							target = posBottom; 
						}
						else {
							target = posTop; 
						}
					}
					
					else if (unit->getPosition().y > bottom) {
					
						if (abs(unit->getPosition().y - (bottom + margin)) <= margin && abs(unit->getPosition().x - newX) <= margin) {
							target = posTop; 
						}
						else {
							target = posBottom; 
						}
					}
					
					else {
						
						if (unit->getPosition().y - closeUnit->pos().y > 0) {
							target = posBottom; 
						}
						else {
							target = posTop;
						}
					}

					uList eL = INFO.getUnitsInRadius(E, target, 3 * TILE_SIZE, true, false, false, false, true);

					if (eL.empty())
						CommandUtil::move(unit, target);
					else {

						
						int dist_m = 0;
						int dist_e = 0;
						theMap.GetPath(unit->getPosition(), target, &dist_m);

						for (auto e : eL) {
							theMap.GetPath(e->pos(), target, &dist_e);

							if (dist_e < dist_m) {
								UnitInfo *me = INFO.getUnitInfo(unit, S);

								
								if (me != nullptr) {
									moveBackPostion(me, closeUnit->vPos(), 2 * TILE_SIZE);
									return nullptr;
								}
							}
						}

						CommandUtil::move(unit, target);
					}
				}
			}
		}
	}

	return nullptr;
}

State *MarineDefenceState::action()
{
	if (ESM.getEnemyInitialBuild() == Terran_bunker_rush) {


		bunkerRushDefence();
		return nullptr;
	}

	
	UnitInfo *closeUnit = getMarineDefenceTargetUnit(unit);

	Unit bunker = MM.getBunker();

	if (closeUnit == nullptr) {
	

		if (bunker && bunker->isCompleted()) {
			if (!unit->isLoaded()) {
				

				unit->rightClick(bunker);
				return nullptr;
			}
		}

		if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) < 4) {

			

			UnitInfo *closeTank = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Siege_Tank_Tank_Mode, 0, false, true);

			if (closeTank) {
				if (unit->getDistance(closeTank->pos()) > 2 * TILE_SIZE) {
					CommandUtil::move(unit, closeTank->pos());
				}
			}
		}

		return nullptr;
	}


	if (!isInMyArea(unit->getPosition())) {
		

	
		if (getGroundDistance(MYBASE, closeUnit->pos()) < getGroundDistance(MYBASE, unit->getPosition()))
			CommandUtil::attackUnit(unit, closeUnit->unit());
		else
			CommandUtil::move(unit, MYBASE, true);

		return nullptr;
	}

	if (INFO.enemyRace == Races::Protoss) {

	
		if (shuttleDefence(unit)) {
			

			return nullptr;
		}

	
		if (closeUnit->type() == Protoss_Dark_Templar)
			if (darkDefence(unit, closeUnit)) {
				if (unit->isSelected()) cout << "다크 수비 " << endl;

				return nullptr;
			}

	}

	uList vultures = INFO.getTypeUnitsInRadius(Terran_Vulture, S, MYBASE, 30 * TILE_SIZE);
	uList tanks = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, MYBASE, 30 * TILE_SIZE);
	uList marineList = INFO.getUnits(Terran_Marine, S);

	if (bunker && bunker->isCompleted()) {
		if (unit->isLoaded()) {
		
			if (!vultures.empty() || !tanks.empty())
				return nullptr;

			if (MM.isRangeUnitClose())
				return nullptr;

	
			bool isUnderAttack = false;
			uList baseUnits = INFO.getUnits(Terran_SCV, S);
			uList baseBuildings = INFO.getBuildingsInRadius(S, MYBASE, 15 * TILE_SIZE);

			for (auto u : baseUnits)
			{
				if (u->unit()->isUnderAttack() && u->getState() != "Scout")
				{
					isUnderAttack = true;
					break;
				}
			}

			if (isUnderAttack == false)
			{
				for (auto u : baseBuildings)
				{
					if (u->unit()->isUnderAttack())
					{
						isUnderAttack = true;
						break;
					}
				}
			}

			if (isUnderAttack && unit == marineList.at(0)->unit() && bunker->getDistance(closeUnit->unit()) > 5 * TILE_SIZE && bunker->canUnload(unit)) {
				bunker->unload(unit);
			}

			return nullptr;
		}
		else {

			if (escapeFromSpell(INFO.getUnitInfo(unit, S)))
				return nullptr;

			
			if ((!vultures.empty() || !tanks.empty()) && bunker->getLoadedUnits().size() != 4) {

				CommandUtil::rightClick(unit, bunker);

				return nullptr;
			}
			else {

				if (closeUnit->type().isWorker() &&
						INFO.getTypeUnitsInArea(INFO.getWorkerType(INFO.enemyRace), E, MYBASE).size()
						+ INFO.getTypeUnitsInArea(INFO.getWorkerType(INFO.enemyRace), E, INFO.getFirstExpansionLocation(S)->Center()).size() <= 1) {
				
					if (bunker->getPosition().getApproxDistance(unit->getPosition()) < 2 * TILE_SIZE) {
						CommandUtil::rightClick(unit, bunker);
					}
					else {
						CommandUtil::attackMove(unit, bunker->getPosition());
					}

					return nullptr;
				}

			

				if (INFO.getFirstExpansionLocation(S) &&
						isSameArea(INFO.getFirstExpansionLocation(S)->Center(), bunker->getPosition())
						&& isSameArea(closeUnit->pos(), MYBASE)) {
				
					if (unit->getGroundWeaponCooldown() == 0) {
						CommandUtil::attackUnit(unit, closeUnit->unit());
					}
					else {
						int range = closeUnit->type().groundWeapon().maxRange();

						if (unit->getDistance(closeUnit->unit()) > range + 2 * TILE_SIZE)
							CommandUtil::attackMove(unit, closeUnit->pos());
						else
							moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 3 * TILE_SIZE);
					}

					return nullptr;
				}
				else {

					int dist = unit->getPosition().getApproxDistance(closeUnit->pos());

					if (bunker->getLoadedUnits().size() != 4) {
					
						if (isNeedKitingUnitType(closeUnit->type()) && unit->getGroundWeaponCooldown() == 0 && dist > 4 * TILE_SIZE) {
							CommandUtil::attackUnit(unit, closeUnit->unit());
						}
						else {
							CommandUtil::rightClick(unit, bunker);
						}
					}
					else {
					
						if (unit->getGroundWeaponCooldown() == 0) {
							CommandUtil::attackUnit(unit, closeUnit->unit());
						}
						else {
							int range = closeUnit->type().groundWeapon().maxRange();

							if (unit->getDistance(closeUnit->unit()) > range + 2 * TILE_SIZE)
								CommandUtil::attackMove(unit, closeUnit->pos());
							else
								moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 3 * TILE_SIZE);
						}
					}
				}

				return nullptr;
			}

		}
	}
	else {

		if (escapeFromSpell(INFO.getUnitInfo(unit, S)))
			return nullptr;

		if (vultures.size() != 0) {

			if (INFO.getUnits(Protoss_Dragoon, E).empty()) {
				if (unit->getDistance(MM.waitingNearCommand) > 50)
					CommandUtil::attackMove(unit, MM.waitingNearCommand, true);

				return nullptr;
			}
			else {
				if (INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S).size() >= 2)
					return nullptr;
			}
		}

		if (INFO.enemyRace == Races::Zerg) {
			if (unit->getDistance(closeUnit->unit()) < 5 * TILE_SIZE) {
				MM.doKiting(unit);
				return nullptr;
			}

			bool isUnderAttack = false;

			uList baseUnits = INFO.getUnits(Terran_SCV, S);
			uList baseBuildings = INFO.getBuildingsInRadius(S, MYBASE, 15 * TILE_SIZE);

			for (auto u : baseUnits)
			{
				if (u->unit()->isUnderAttack() && u->getState() != "Scout")
				{
					isUnderAttack = true;
					break;
				}
			}

			if (isUnderAttack == false)
			{
				for (auto u : baseBuildings)
				{
					if (u->unit()->isUnderAttack())
					{
						isUnderAttack = true;
						break;
					}
				}
			}

			if (isUnderAttack) {
				MM.doKiting(unit);
				return nullptr;
			}
			else
			{
				uList centers = INFO.getBuildings(Terran_Command_Center, S);

				if (centers.size())
				{
					UnitInfo *center = centers[0];
					Unit mineral = bw->getClosestUnit(center->pos(), Filter::IsMineralField);

					if (mineral != nullptr) {
						Position target = (center->pos() + mineral->getPosition()) / 2;
						unit->move(target);
					}
				}
			}
		}
		else {
			MM.doKiting(unit);
		}

	}

	return nullptr;
}

State *MarineDefenceState::bunkerRushDefence()
{

	if (INFO.getCompletedCount(Terran_Marine, E)) {
	
		if (INFO.getCompletedCount(Terran_Bunker, E)) {
			if (INFO.getCompletedCount(Terran_Marine, S) < 3)
				return nullptr;
			else {
				uList eSCV = INFO.getTypeUnitsInRadius(Terran_SCV, E);

				for (auto scv : eSCV) {
					if (scv->unit()->isRepairing()) {
						CommandUtil::attackUnit(unit, scv->unit());
						return nullptr;
					}
				}

				uList eBunker = INFO.getTypeBuildingsInRadius(Terran_Bunker, E);

				for (auto bunker : eBunker) {
					if (bunker->isHide()) {
						CommandUtil::attackMove(unit, bunker->pos());
						break;
					}
					else {
						CommandUtil::attackUnit(unit, bunker->unit());
						break;
					}
				}
			}
		}
		else {
			uList eSCV = INFO.getTypeUnitsInRadius(Terran_SCV, E);

			for (auto scv : eSCV) {
				if (scv->unit()->isConstructing()) {
					CommandUtil::attackUnit(unit, scv->unit());
					return nullptr;
				}
			}

			UnitInfo *e = INFO.getClosestUnit(E, unit->getPosition(), AllKind, 0, true, false, true);

			if (e)
				CommandUtil::attackUnit(unit, e->unit());
			else
				CommandUtil::attackMove(unit, INFO.getSecondChokePosition(S));
		}
	}

	else {
		uList eSCV = INFO.getTypeUnitsInRadius(Terran_SCV, E);

		for (auto scv : eSCV) {
			if (scv->unit()->isConstructing() || scv->unit()->isRepairing()) {
				CommandUtil::attackUnit(unit, scv->unit());
				return nullptr;
			}
		}

		UnitInfo *e = INFO.getClosestUnit(E, unit->getPosition(), AllKind, 0, true, false, true);

		if (e)
			CommandUtil::attackUnit(unit, e->unit());
		else
			CommandUtil::attackMove(unit, INFO.getSecondChokePosition(S));
	}

	return nullptr;
}

UnitInfo *MarineDefenceState::getMarineDefenceTargetUnit(const Unit &unit) {
	UnitInfo *closeUnit = nullptr;
	uList eList = getEnemyInMyYard(1700, Men, false);
	uList eList2;

	if (INFO.getFirstExpansionLocation(S))
		eList2 = INFO.getUnitsInArea(E, INFO.getFirstExpansionLocation(S)->Center(), true, true, true, false);

	eList.insert(eList.end(), eList2.begin(), eList2.end());

	int dist = MAXINT;
	int temp = 0;

	for (auto e : eList) {
		theMap.GetPath(e->pos(), unit->getPosition(), &temp);

		if (e->type().isWorker()) {
			if (!closeUnit || (closeUnit->type().isWorker() && (temp >= 0 && dist > temp))) {
				dist = temp;
				closeUnit = e;
			}
		}
		else {
			if (temp >= 0 && dist > temp) {
				dist = temp;
				closeUnit = e;
			}
		}
	}

	if (!closeUnit) {
		uList enemyBuildingsInMyYard = getEnemyInMyYard(1700, UnitTypes::Buildings);
		dist = MAXINT;
		temp = 0;

		for (auto e : enemyBuildingsInMyYard) {
			theMap.GetPath(e->pos(), unit->getPosition(), &temp);

			if (temp >= 0 && dist > temp) {
				dist = temp;
				closeUnit = e;
			}
		}
	}

	return closeUnit;
}


bool MarineDefenceState::shuttleDefence(const Unit &unit) {

	uList shuttles = INFO.getTypeUnitsInArea(Protoss_Shuttle, E, MYBASE, false);
	uList shuttles2;

	if (INFO.getFirstExpansionLocation(S) != nullptr)
		shuttles2 = INFO.getTypeUnitsInArea(Protoss_Shuttle, E, INFO.getFirstExpansionLocation(S)->getPosition(), false);

	uList goliaths = INFO.getTypeUnitsInArea(Terran_Goliath, S, MYBASE, false);

	if ((shuttles.empty() && shuttles2.empty()) || !goliaths.empty())
		return false;

	Unit bunker = MM.getBunker();

	if (bunker && bunker->isCompleted()) {

		if (unit->isLoaded()) {
			if (INFO.getUnitsInRadius(E, bunker->getPosition(), 7 * TILE_SIZE, true, true).empty() && bunker->canUnload(unit)) {
				bunker->unload(unit);
				return true;
			}
		}
	}

	
	UnitInfo *closeDangerUnit = INFO.getClosestUnit(E, unit->getPosition(), GroundCombatKind, 4 * TILE_SIZE, false);

	if (closeDangerUnit) {
		if (unit->getGroundWeaponCooldown() == 0)
			CommandUtil::attackUnit(unit, closeDangerUnit->unit());
		else
			moveBackPostionMarine(INFO.getUnitInfo(unit, S), closeDangerUnit->vPos(), 2 * TILE_SIZE);

		return true;
	}

	UnitInfo *closeShuttle = INFO.getClosestTypeUnit(E, unit->getPosition(), Protoss_Shuttle, 30 * TILE_SIZE, false);

	if (closeShuttle && isInMyArea(closeShuttle)) {
		CommandUtil::attackUnit(unit, closeShuttle->unit());
		return true;
	}

	return false;

}


bool MarineDefenceState::darkDefence(const Unit &unit, const UnitInfo *closeUnit) {

	Unit bunker = MM.getBunker();

	if (bunker && bunker->isCompleted()) {

		if (!unit->isLoaded()) {

			int gap = 0;

			if (!closeUnit->unit()->isDetected())
				gap = 7 * TILE_SIZE; 
			else
				gap = 3 * TILE_SIZE;

			int distFromDark = unit->getDistance(closeUnit->unit());
			int distFromBunker = unit->getDistance(bunker);

		
			if (closeUnit->unit()->isDetected() && (distFromDark > gap || unit->getGroundWeaponCooldown() == 0))
				CommandUtil::attackUnit(unit, closeUnit->unit());
			else {
			

			
				if (distFromDark > distFromBunker) {
					unit->rightClick(bunker);
					return true;
				}

				
				if (unit->getDistance(closeUnit->unit()) < gap) {
					moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 3 * TILE_SIZE);
				}
				else {
					CommandUtil::attackMove(unit, closeUnit->vPos());
				}

			}
		}

		return true;
	}
	else {
		int gap = 0;

		if (!closeUnit->unit()->isDetected())
			gap = 7 * TILE_SIZE; // 도망 모드
		else
			gap = 3 * TILE_SIZE;

		if (closeUnit->unit()->isDetected() && (unit->getDistance(closeUnit->unit()) > gap || unit->getGroundWeaponCooldown() == 0))
			CommandUtil::attackUnit(unit, closeUnit->unit());
		else
			moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 3 * TILE_SIZE);

		return true;
	}

	return false;

}

MyBot::MarineFirstChokeDefenceState::MarineFirstChokeDefenceState(Position p)
{
	defencePosition = p;
}


State *MarineFirstChokeDefenceState::action()
{
	if (INFO.isTimeToMoveCommandCenterToFirstExpansion()) {

		UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), TypeKind::GroundCombatKind, 20 * TILE_SIZE, false);

		UnitInfo *closeTank = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Siege_Tank_Tank_Mode, 0, false, true);

		if (closeUnit && closeTank) {
		

			if (unit->getDistance(closeTank->pos()) <= 2 * TILE_SIZE) {
				CommandUtil::attackMove(unit, closeUnit->pos());
			}
			else {
				CommandUtil::attackMove(unit, closeTank->pos());
			}

			return nullptr;
		}
		else {

			if (INFO.getFirstExpansionLocation(S)) {
				uList tList = INFO.getTypeBuildingsInArea(Terran_Missile_Turret, S, INFO.getFirstExpansionLocation(S)->Center());

				if (!tList.empty()) {
					CommandUtil::attackMove(unit, tList.at(0)->pos());
				}
				else {
					if (closeTank)
						if (closeTank->pos().getApproxDistance(unit->getPosition()) > 3 * TILE_SIZE)
							CommandUtil::attackMove(unit, closeTank->pos());
						else
							CommandUtil::attackMove(unit, INFO.getSecondAverageChokePosition(S));
					else
						CommandUtil::attackMove(unit, INFO.getSecondAverageChokePosition(S));
				}
			}
		}

		return nullptr;
	}

	if (unit->getDistance(defencePosition) > 16 ) {
		CommandUtil::move(unit, defencePosition, true);
	}
	else {
		CommandUtil::hold(unit, false);
	}

	return nullptr;
}

State *MarineGoGoGoState::action()
{
	if (unit->isLoaded()) return nullptr;

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(unit->getPosition(), &dangerPoint, false);
	UnitInfo *closestTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

	if (dangerUnit == nullptr)
	{
		CommandUtil::attackMove(unit, SM.getMainAttackPosition());
	}
	else if (dangerUnit != nullptr && closestTank != nullptr)
	{
		if (unit->getPosition().getApproxDistance(closestTank->pos()) < 5 * TILE_SIZE)
		{
			CommandUtil::attackMove(unit, dangerUnit->pos(), true);
		}
		else if (unit->getPosition().getApproxDistance(closestTank->pos()) > 8 * TILE_SIZE)
		{
			CommandUtil::attackMove(unit, closestTank->pos());
		}

	}

	return nullptr;
}
