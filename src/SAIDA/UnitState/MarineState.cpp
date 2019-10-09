#include "MarineState.h"
#include "../InformationManager.h"
#include "../StrategyManager.h"
#include "../EnemyStrategyManager.h"
using namespace MyBot;

State *MarineIdleState::action()
{
		CommandUtil::move(unit, MM.waitnearsecondchokepoint, true);
	return nullptr;
}

State *MarineAttackState::action() 
{
	if (m_targetUnit != nullptr) 
	{
		UnitInfo *eUnit = INFO.getUnitInfo(m_targetUnit, E);

		
		if (!eUnit) {
		
			uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 8 * TILE_SIZE);

			if (!eList.empty()) {
				m_targetUnit = eList.at(0)->unit();
				m_targetPos = m_targetUnit->getPosition();
			}
		
			else if (eList.empty() && (EIB == Toss_1g_forward || EIB == Toss_2g_forward))
			{
				m_targetUnit = nullptr;
				m_targetPos = SM.getMainAttackPosition();
			}
			else
				return new MarineIdleState();

			return nullptr;
		}

	
		if (eUnit->pos() == Positions::Unknown)
		{
			if (unit->isMoving())
				return nullptr;

			return new MarineIdleState();
		}

		m_targetPos = eUnit->pos();
	}

	uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 10 * TILE_SIZE);

	// �ֺ��� �ƹ� ���ֵ� ������ �ϴ� �������� ����.
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
		// ���� ������ �� �ִ� ����
		if (e->type().groundWeapon().targetsGround()) {
			int gap;
			int enemyRange = UnitUtil::GetAttackRange(e->unit(), unit);
			int selfRange = UnitUtil::GetAttackRange(unit, e->unit());
			int dist = getAttackDistance(e->unit(), unit);

			// ���� Ÿ���� �� ���� ���
			if (e->unit()->getOrderTarget() != nullptr && e->unit()->getOrderTarget() == unit
					|| e->unit()->getTarget() != nullptr && e->unit()->getTarget() == unit) {
				if (enemyRange >= selfRange)
					gap = -64;
				else
					gap = (int)(E->topSpeed(e->type()) * 24);
			}
			// ���� Ÿ���� ���� ���� ���� ���
			else {

				// �� ��Ÿ��� ���� �� ��� ��� �Ÿ��� �д�.
				if (enemyRange + 64 < selfRange)
					gap = (int)(E->topSpeed(e->type()) * 14);
				// ���̰� �� �ȳ��ų� ª�� ��� �����鼭 �ο��.
				else
					gap = -64;
			}

			// ���� ��Ÿ� + x �Ÿ��̸� ����
			if (enemyRange + gap >= dist) {
				CommandUtil::backMove(unit, e->unit());
				// moveBackPostion(INFO.getUnitInfo(unit, S), e->pos(), 2);
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

	// �ո���� ������ �ƴ� ���� ���� ���.
	if (!isSameArea(INFO.getMainBaseLocation(S)->getPosition(), unit->getPosition()) &&
			INFO.getFirstExpansionLocation(S) && !isSameArea(INFO.getFirstExpansionLocation(S)->getPosition(), unit->getPosition())) {

		uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

		if (commandCenterList.empty()) return nullptr;

		if (commandCenterList.size() == 1)
		{
			// 9��� ���� ������ �� ������ Ŀ�ǵ� �Ʒ��� ���
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
		return nullptr;
	}
	else
	{
		UnitInfo *closestEnem = INFO.getClosestUnit(E, uinfo->pos(), MyBot::AllKind, 2000, true);

		if (closestEnem != nullptr)
		{
			//if (unit->isInWeaponRange(closestEnem->unit()) && unit->getGroundWeaponCooldown() == 0)
			CommandUtil::attackUnit(unit, closestEnem->unit());
			return nullptr;
			//else
			//	CommandUtil::attackMove(unit, lastScouterPosition);
		}
		else
			CommandUtil::attackMove(unit, lastScouterPosition);
		return nullptr;
	}


	return nullptr;
}

State *MarineGoGoGoState::action()
{
	bool canStim = false;
	if (S->hasResearched(TechTypes::Stim_Packs))
		canStim = true;
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
			if (canStim && (!unit->isStimmed()))
			{
				unit->useTech(TechTypes::Stim_Packs);
			}
			CommandUtil::attackMove(unit, dangerUnit->pos(), true);
		}
		else if (unit->getPosition().getApproxDistance(closestTank->pos()) > 8 * TILE_SIZE)
		{
			CommandUtil::attackMove(unit, closestTank->pos());
		}

	}

	return nullptr;
}
State *MarineKillDState::action()
{
	bool canStim = false;
	if (S->hasResearched(TechTypes::Stim_Packs))
		canStim = true;
	if (unit->isLoaded()) return nullptr;

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(unit->getPosition(), &dangerPoint, false);
	UnitInfo *closestAttack = INFO.getClosestUnit(E, unit->getPosition(), GroundCombatKind, 10 * TILE_SIZE, false, false, true);
	UnitInfo *me = INFO.getUnitInfo(unit, S);
	if (getGroundDistance(MYBASE, unit->getPosition())>=4 * TILE_SIZE)
	{
		CommandUtil::attackMove(unit, MYBASE);
		return nullptr;
	}
	else if (closestAttack!=nullptr)
	{
		
		kiting(me, closestAttack, dangerPoint, 2 * TILE_SIZE);
		return nullptr;
	}

	return nullptr;
}
State *MarineDiveState::action()
{
	if (TIME - unit->getLastCommandFrame() < 24)
		return nullptr;

	UnitInfo *me = INFO.getUnitInfo(unit, S);

	Position target = INFO.getMainBaseLocation(E)->Center();
	Position targetsecond = INFO.getFirstExpansionLocation(E)->Center();
	if (!isSameArea(me->pos(), target))
	{
		unit->move(target);
		return nullptr;
	}

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(me->pos(), &dangerPoint, false);
	if (dangerUnit == nullptr)
	{
		UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, me->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

		if (closestWorker != nullptr && isSameArea(closestWorker->pos(), target))
		{
			kiting(me, closestWorker, dangerPoint, 2 * TILE_SIZE);
			return nullptr;
				//ԭ����kiting(me, closestWorker, dangerPoint, 2 * TILE_SIZE);
		}
		else if (me->pos().getApproxDistance(target) > 5 * TILE_SIZE)
		{
			CommandUtil::move(unit, target);
			return nullptr;
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
	else 
	{
		
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
			else 
			{
				kiting(me, closestAttack, dangerPoint, 3 * TILE_SIZE);
				return nullptr;
			}
		}
		else if (isNeedKitingUnitType(dangerUnit->type()))
		{
			if (closestAttack == nullptr)
			{
				if (me->pos().getApproxDistance(target) > 3 * TILE_SIZE)
					CommandUtil::move(unit, target);
				return nullptr;
			}
			else // closest Attacker ����
			{
			
					kiting(me, closestAttack, dangerPoint, 3 * TILE_SIZE);
					return nullptr;
			}
		}
		else
		{
			
			return new MarineKillWorkerFirstState();
		}
	}

	return nullptr;
}

State *MarineDive2State::action()
{
	if (TIME - unit->getLastCommandFrame() < 24)
		return nullptr;

	UnitInfo *me = INFO.getUnitInfo(unit, S);

	
	Position target = INFO.getFirstExpansionLocation(E)->Center();
	if (!isSameArea(me->pos(), target))
	{
		unit->move(target);
		return nullptr;
	}

	int dangerPoint = 0;
	UnitInfo *dangerUnit = getDangerUnitNPoint(me->pos(), &dangerPoint, false);
	if (dangerUnit == nullptr)
	{
		UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, me->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

		if (closestWorker != nullptr && isSameArea(closestWorker->pos(), target))
		{
			CommandUtil::attackUnit(me->unit(), closestWorker->unit());
			return nullptr;
			//ԭ����kiting(me, closestWorker, dangerPoint, 2 * TILE_SIZE);
		}
		else if (me->pos().getApproxDistance(target) > 5 * TILE_SIZE)
		{
			CommandUtil::move(unit, target);
			return nullptr;
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
	else
	{

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
			else
			{
				kiting(me, closestAttack, dangerPoint, 2 * TILE_SIZE);
				return nullptr;
			}
		}
		else if (isNeedKitingUnitType(dangerUnit->type()))
		{
			if (closestAttack == nullptr)
			{
				if (me->pos().getApproxDistance(target) > 3 * TILE_SIZE)
					CommandUtil::move(unit, target);
				return nullptr;
			}
			else 
			{

				kiting(me, closestAttack, dangerPoint, 2 * TILE_SIZE);
				return nullptr;
			}
		}
		else
		{

			return new MarineKillWorkerFirstState();
		}
	}

	return nullptr;
}

State *MarineKillWorkerFirstState::action()
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

	
		CommandUtil::attackUnit(unit, w->unit());
		return nullptr;
	}

	UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, me->pos(), INFO.getWorkerType(INFO.enemyRace), 0, true, true);

	if (closestWorker)
		CommandUtil::attackUnit(me->unit(), closestWorker->unit());
		//kiting(me, closestWorker, unit->getDistance(closestWorker->unit()), 2 * TILE_SIZE);
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

State *MarineDefenceState::action()
{
	
	UnitInfo *closeUnit = getMarineDefenceTargetUnit(unit);

	Unit bunker = MM.getBunker();
	//�ƶ����ر���==================================
	if (closeUnit == nullptr) 
	{
		if (unit->isSelected()) cout << "Close unit ���� " << endl;

		if (bunker && bunker->isCompleted()) {
			if (!unit->isLoaded()) 
			{
				if (unit->isSelected()) cout << "��Ŀ ���� " << endl;

				unit->rightClick(bunker);
				return nullptr;
			}
		}
		//�ƶ��������̹���Ǳ�==================================
		if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) < 4) {

			if (unit->isSelected()) cout << "��ũ ���� ����" << endl;

			UnitInfo *closeTank = INFO.getClosestTypeUnit(S, unit->getPosition(), Terran_Siege_Tank_Tank_Mode, 0, false, true);

			if (closeTank) {
				if (unit->getDistance(closeTank->pos()) > 2 * TILE_SIZE) {
					CommandUtil::move(unit, closeTank->pos());
					return nullptr;
				}
			}
		}

		return nullptr;
	}

	//�ƶ�������==================================
	if (!isInMyArea(unit->getPosition())) {
		if (unit->isSelected()) cout << "���� ���� ������ ������" << endl;

		
		if (getGroundDistance(MYBASE, closeUnit->pos()) < getGroundDistance(MYBASE, unit->getPosition()))
		{
			CommandUtil::attackUnit(unit, closeUnit->unit());
			return nullptr;
		}
		else
			CommandUtil::move(unit, MYBASE, true);

		return nullptr;
	}

	
	//�������û��̹�˺��׳���ж�صر�==================================
	uList vultures = INFO.getTypeUnitsInRadius(Terran_Vulture, S, MYBASE, 30 * TILE_SIZE);
	uList tanks = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, MYBASE, 30 * TILE_SIZE);
	uList marineList = INFO.getUnits(Terran_Marine, S);

	if (bunker && bunker->isCompleted())
	{
		if (unit->isLoaded()) 
		{
			// �̹� ��ó�� ������ ��Ŀ���� ���.
			if (!vultures.empty() || !tanks.empty())
				return nullptr;

			if (MM.isRangeUnitClose())
				return nullptr;

			// ��Ŀ�� �� �ִ� ���, ��Ÿ����� �־����� ���� ġ�� �����̿��� ��.
			// ������ �����̿��� ����, �־����� ����.
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
		//ǹ�����˵ر�==================================
		else 
		{

			if (escapeFromSpell(INFO.getUnitInfo(unit, S)))
				return nullptr;

			// ������̹�˺��׳��������»صر�
			if ((!vultures.empty() || !tanks.empty()) && bunker->getLoadedUnits().size() != 4) 
			{

				CommandUtil::rightClick(unit, bunker);
				/*if (bunker->getPosition().getApproxDistance(unit->getPosition()) <= 1 * TILE_SIZE)
				else {
				CommandUtil::attackMove(unit, bunker->getPosition());
				}*/
				return nullptr;
			}
			else 
			{

				if (closeUnit->type().isWorker() &&
					INFO.getTypeUnitsInArea(INFO.getWorkerType(INFO.enemyRace), E, MYBASE).size()
					+ INFO.getTypeUnitsInArea(INFO.getWorkerType(INFO.enemyRace), E, INFO.getFirstExpansionLocation(S)->Center()).size() <= 1) {
					// ��Ŀ�� ����.
					if (bunker->getPosition().getApproxDistance(unit->getPosition()) < 2 * TILE_SIZE) {
						CommandUtil::rightClick(unit, bunker);
						return nullptr;
					}
					else {
						CommandUtil::attackMove(unit, bunker->getPosition());
					}

					return nullptr;
				}

			
				if (INFO.getFirstExpansionLocation(S) &&
					isSameArea(INFO.getFirstExpansionLocation(S)->Center(), bunker->getPosition())
					&& isSameArea(closeUnit->pos(), MYBASE)) {
					// ��� ���ͼ� �ο��.
					if (unit->getGroundWeaponCooldown() == 0) {
						CommandUtil::attackUnit(unit, closeUnit->unit());
						return nullptr;
					}
					else {
						int range = closeUnit->type().groundWeapon().maxRange();

						if (unit->getDistance(closeUnit->unit()) > range + 2 * TILE_SIZE){
							CommandUtil::attackMove(unit, closeUnit->pos());
							return nullptr;
						}
						else
						{
							moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 3 * TILE_SIZE);
							return nullptr;
						}
					}

					return nullptr;
				}
				else {

					int dist = unit->getPosition().getApproxDistance(closeUnit->pos());

					if (bunker->getLoadedUnits().size() != 4) {
						// ��Ŀ ����
						if (isNeedKitingUnitType(closeUnit->type()) && unit->getGroundWeaponCooldown() == 0 && dist > 4 * TILE_SIZE) {
							CommandUtil::attackUnit(unit, closeUnit->unit());
							return nullptr;
						}
						else {
							CommandUtil::rightClick(unit, bunker);
							return nullptr;
						}
					}
					else {
						// �ۿ��� ī����
						if (unit->getGroundWeaponCooldown() == 0) {
							CommandUtil::attackUnit(unit, closeUnit->unit());
							return nullptr;
						}
						else {
							int range = closeUnit->type().groundWeapon().maxRange();
							

							if (unit->getDistance(closeUnit->unit()) > range + 2 * TILE_SIZE)
							{
								CommandUtil::attackMove(unit, closeUnit->pos());
								return nullptr;
							}
							else
							{
								moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 3 * TILE_SIZE);
								return nullptr;
							}
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

		if (INFO.enemyRace == Races::Zerg) 
		{
			if (unit->getDistance(closeUnit->unit()) < 5 * TILE_SIZE) 
			{
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

			if (isUnderAttack) 
			{
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

State *MarineZealotDefenceState::action()
{
	UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), AllUnitKind);

	if (closeUnit == nullptr) {

		Unit bunker = MM.getBunker();

		if (bunker && bunker->isCompleted()) {
			if (!unit->isLoaded()) {
				if (unit->isSelected()) cout << "��Ŀ ���� " << endl;

				unit->rightClick(bunker);
				return nullptr;
			}
		}

		if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) < 4) {

			if (unit->isSelected()) cout << "��ũ ���� ����" << endl;

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

		// ��Ŀ �ȿ� �ִ� ����
		if (unit->isLoaded()) {
			// ��Ŀ�� �� �ִ� ���, ��Ÿ����� �־����� ���� ġ�� �����̿��� ��.
			// ������ �����̿��� ����, �־����� ����.

			//���İ� ������ ��Ŀ���� ���
			if (vultures.size() != 0)
				return nullptr;

			if (!unit->isInWeaponRange(closeUnit->unit()))
			{
				bunker->unload(unit);
			}

			return nullptr;
		}
		// ��Ŀ �ۿ� �ִ� ����
		else {

			if (vultures.size() != 0 && bunker->getLoadedUnits().size() != 4) {
				CommandUtil::rightClick(unit, bunker);
				return nullptr;
			}

			// ��Ÿ� ���̸� �����̰�
			if (!unit->isInWeaponRange(closeUnit->unit()))
			{
				CommandUtil::attackUnit(unit, closeUnit->unit());
			}
			// ��Ÿ� ���̸�
			else {
				// ������ ġ�ٰ� �ڷ� ����.
				if (unit->getPosition().getDistance(closeUnit->pos()) > 4 * TILE_SIZE) {
					CommandUtil::attackUnit(unit, closeUnit->unit());
				}
				else {
					// ��Ŀ�� ����
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
		// �跰�� ���� ���̷� �̵�.
		Unit barrack = MM.getFirstBarrack();
		Unit supply = MM.getNextBarrackSupply();

		if (barrack == nullptr || supply == nullptr)
			return nullptr;

		if (!unit->isInWeaponRange(closeUnit->unit()))
		{
			CommandUtil::attackUnit(unit, closeUnit->unit());
		}
		else {

			// ������ �̵��� ��ǥ��.
			int top = supply->getTop();
			int bottom = barrack->getBottom();
			int newX = MM.getFirstBarrack()->getRight();

			int margin = TILE_SIZE; // ������ ��ġ�� ã���� ��Ȯ�� �ȼ��� �ϸ� ������ �ʹ� ����.
			Position posTop = Position(newX + 12, top - margin);
			Position posBottom = Position(newX, bottom + margin);
			Position target;

			// ������ ġ�ٰ� �ڷ� ����.
			if (unit->getPosition().getDistance(closeUnit->pos()) > 4 * TILE_SIZE) {
				// 3Ÿ�� �̻��̸�, ����
				CommandUtil::attackUnit(unit, closeUnit->unit());
			}
			else {

				// �ʹ� ������� �������ߵǴµ�
				// 1. �跰�� �ݴ��� ���� ������ ����
				if ((unit->getPosition().y <= top && closeUnit->pos().y >= bottom)
					|| (unit->getPosition().y >= bottom && closeUnit->pos().y <= top)) {
					CommandUtil::attackUnit(unit, closeUnit->unit());
				}
				// 2. ��Ÿ� ���̸� ����
				else {

					// ������ �跰�̳� ���� ���� �ִ�. ->
					if (unit->getPosition().y <= top) {
						// ��ó���� ����.
						if (abs(unit->getPosition().y - (top - margin)) <= margin && abs(unit->getPosition().x - newX) <= margin) {
							target = posBottom; // �Ʒ��� �̵�
						}
						else {
							target = posTop; // ���� �̵�
						}
					}
					// ������ �跰�̳� ���� ���� �Ʒ��� �ִ�.
					else if (unit->getPosition().y > bottom) {
						// ��ó���� ����.
						if (abs(unit->getPosition().y - (bottom + margin)) <= margin && abs(unit->getPosition().x - newX) <= margin) {
							target = posTop; // ���� �̵�
						}
						else {
							target = posBottom; // �Ʒ��� �̵�
						}
					}
					// �߰��� ���ִ� ���.
					else {
						// ������ �������� ���� ������
						if (unit->getPosition().y - closeUnit->pos().y > 0) {
							target = posBottom; // �Ʒ��� �̵�
						}
						else {
							target = posTop;// ���� �̵�
						}
					}

					uList eL = INFO.getUnitsInRadius(E, target, 3 * TILE_SIZE, true, false, false, false, true);

					if (eL.empty())
						CommandUtil::move(unit, target);
					else {

						// ���� ������ target ��ġ�� ������ �������� �̵�
						int dist_m = 0;
						int dist_e = 0;
						theMap.GetPath(unit->getPosition(), target, &dist_m);

						for (auto e : eL) {
							theMap.GetPath(e->pos(), target, &dist_e);

							if (dist_e < dist_m) {
								UnitInfo *me = INFO.getUnitInfo(unit, S);

								//target ���� ���� ������ ����� ���� ������ �ڷ� ��������.
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

State *MarineDefenceState::bunkerRushDefence()
{
	// �� ������ �ִ� ���
	if (INFO.getCompletedCount(Terran_Marine, E)) {
		// �ϼ��� ��Ŀ�� �ִ� ���
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
	// �� ������ ���� ���
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

	// ��ó�� ������ ������ ������ �ϴ� ������ ��ó
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
				gap = 7 * TILE_SIZE; // ���� ���
			else
				gap = 3 * TILE_SIZE;

			int distFromDark = unit->getDistance(closeUnit->unit());
			int distFromBunker = unit->getDistance(bunker);

			// ���̸鼭 ������ �Ÿ��� �� ������ ������ ����
			if (closeUnit->unit()->isDetected() && (distFromDark > gap || unit->getGroundWeaponCooldown() == 0))
				CommandUtil::attackUnit(unit, closeUnit->unit());
			else {
				// �Ⱥ��̰ų� �Ÿ��� �ʹ� ����� ��, ��Ŀ�� ���°� ������?

				//��ũ���� ��Ŀ�� �� ������ ����.
				if (distFromDark > distFromBunker) {
					unit->rightClick(bunker);
					return true;
				}

				// ��ũ�� gap �� �����ϸ鼭 ������
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
			gap = 7 * TILE_SIZE; // ���� ���
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

State *MarineIronmanState::action()
{
	UnitInfo *me = INFO.getUnitInfo(unit, S);

	uList zealot = INFO.getTypeUnitsInRadius(Protoss_Zealot, E, me->pos(), 8* TILE_SIZE, true);

	for (auto z : zealot)
	{
		if (!unit->isInWeaponRange(z->unit()))
			continue;


		if (unit->getGroundWeaponCooldown() == 0)
		{
			z->setDamage(unit);
			break;
		}

		CommandUtil::attackUnit(unit, z->unit());
		return nullptr;
	}

	UnitInfo *closestzealot = INFO.getClosestTypeUnit(E, me->pos(), Protoss_Zealot, 0, true, true);

	if (closestzealot)
		CommandUtil::attackUnit(me->unit(), closestzealot->unit());
	
	return nullptr;
}

State *MarineGatherState::action()
{
	if (unit->isLoaded())
		return nullptr;

	waitnearsecondchokepoint = INFO.getSecondChokePosition(E, ChokePoint::end1);

	CommandUtil::attackMove(unit, theMap.Center());
	return nullptr;

	
}