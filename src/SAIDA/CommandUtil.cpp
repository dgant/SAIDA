#include "CommandUtil.h"
#include "InformationManager.h"

using namespace MyBot;

bool CommandUtil::attackUnit(Unit attacker, Unit target, bool repeat)
{
	if (!attacker || !target)
	{
		return false;
	}

	if (!target->exists()) {
		UnitInfo *u = INFO.getUnitInfo(target, E);

		if (u != nullptr && u->pos() != Positions::Unknown)
			return attackMove(attacker, u->pos(), repeat);

		return false;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (attacker->getLastCommandFrame() >= TIME)
	{
		return false;
	}

	int cooldown = target->isFlying() ? attacker->getAirWeaponCooldown() : attacker->getGroundWeaponCooldown();

	int maxCooldown = attacker->getPlayer()->weaponDamageCooldown(attacker->getType());

	if (cooldown > (int)(maxCooldown * 0.8))
		return false;

	// 명령 받은지 5초 이상 지났으면 동일 명령이라도 다시 내려준다.
	if (TIME - attacker->getLastCommandFrame() > 5 * 24)
		repeat = true;

	// get the unit's current command
	if (!repeat)
	{
		UnitCommand currentCommand(attacker->getLastCommand());

		// if we've already told this unit to attack this target, ignore this command
		if (currentCommand.getType() == UnitCommandTypes::Attack_Unit && currentCommand.getTarget() == target)
		{
			return false;
		}
	}

	// if nothing prevents it, attack the target
	return attacker->attack(target);
}

bool CommandUtil::attackMove(Unit attacker, const Position &targetPosition, bool repeat)
{
	if (!attacker || !targetPosition.isValid())
	{
		return false;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (attacker->getLastCommandFrame() >= TIME)
	{
		return false;
	}

	int cooldown = max(attacker->getAirWeaponCooldown(), attacker->getGroundWeaponCooldown());

	int maxCooldown = attacker->getPlayer()->weaponDamageCooldown(attacker->getType());

	if (cooldown > (int)(maxCooldown * 0.8))
		return false;

	// 명령 받은지 5초 이상 지났으면 동일 명령이라도 다시 내려준다.
	if (TIME - attacker->getLastCommandFrame() > 5 * 24)
		repeat = true;

	// get the unit's current command
	if (!repeat)
	{
		UnitCommand currentCommand(attacker->getLastCommand());

		// if we've already told this unit to attack this target, ignore this command
		if (currentCommand.getType() == UnitCommandTypes::Attack_Move && currentCommand.getTargetPosition() == targetPosition)
		{
			return false;
		}
	}

	// if nothing prevents it, attack the target
	return attacker->attack(targetPosition);
}

void CommandUtil::move(Unit attacker, const Position &targetPosition, bool repeat)
{
	if (!attacker || !targetPosition.isValid())
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (attacker->getLastCommandFrame() >= TIME)
	{
		return;
	}

	// 명령 받은지 5초 이상 지났으면 동일 명령이라도 다시 내려준다.
	if (TIME - attacker->getLastCommandFrame() > 5 * 24)
		repeat = true;

	// get the unit's current command
	if (!repeat)
	{
		UnitCommand currentCommand(attacker->getLastCommand());

		// if we've already told this unit to move to this position, ignore this command
		if ((currentCommand.getType() == UnitCommandTypes::Move) && (currentCommand.getTargetPosition() == targetPosition) && attacker->isMoving())
		{
			return;
		}
	}

	// if nothing prevents it, attack the target
	attacker->move(targetPosition);
}

void CommandUtil::rightClick(Unit unit, Unit target, bool repeat, bool rightClickOnly)
{
	if (!unit || !target)
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (unit->getLastCommandFrame() >= Broodwar->getFrameCount())
	{
		return;
	}

	// 명령 받은지 5초 이상 지났으면 동일 명령이라도 다시 내려준다.
	if (TIME - unit->getLastCommandFrame() > 5 * 24)
		repeat = true;

	CPPath path = theMap.GetPath(unit->getPosition(), target->getPosition());

	if (rightClickOnly || path.size() < 2) {
		// get the unit's current command
		if (!repeat)
		{
			UnitCommand currentCommand(unit->getLastCommand());

			// if we've already told this unit to move to this position, ignore this command
			if (currentCommand.getType() == UnitCommandTypes::Right_Click_Unit && (currentCommand.getTargetPosition() == target->getPosition() || currentCommand.getTarget() == target))
			{
				return;
			}
		}

		// if nothing prevents it, attack the target
		unit->rightClick(target);
	}
	else {
		// get the unit's current command
		if (!repeat)
		{
			UnitCommand currentCommand(unit->getLastCommand());

			// if we've already told this unit to move to this position, ignore this command
			if (currentCommand.getType() == UnitCommandTypes::Right_Click_Unit && currentCommand.getTargetPosition() == (Position)path.at(1)->Center())
			{
				return;
			}
		}

		unit->rightClick((Position)path.at(1)->Center());
	}
}

void CommandUtil::rightClick(Unit unit, Position target, bool repeat)
{
	if (!unit || !target.isValid())
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (unit->getLastCommandFrame() >= Broodwar->getFrameCount())
	{
		return;
	}

	CPPath path = theMap.GetPath(unit->getPosition(), target);

	if (path.size() < 2) {
		// get the unit's current command
		if (!repeat)
		{
			UnitCommand currentCommand(unit->getLastCommand());

			// if we've already told this unit to move to this position, ignore this command
			if (currentCommand.getType() == UnitCommandTypes::Right_Click_Unit && currentCommand.getTargetPosition() == target && (unit->isMoving() || unit->isConstructing() || unit->isAttacking() || unit->isGatheringGas() || unit->isGatheringMinerals()))
			{
				return;
			}
		}

		// if nothing prevents it, attack the target
		unit->rightClick(target);
	}
	else {
		// get the unit's current command
		if (!repeat)
		{
			UnitCommand currentCommand(unit->getLastCommand());

			// if we've already told this unit to move to this position, ignore this command
			if (currentCommand.getType() == UnitCommandTypes::Right_Click_Unit && currentCommand.getTargetPosition() == (Position)path.at(1)->Center() && unit->isMoving())
			{
				return;
			}
		}

		unit->rightClick((Position)path.at(1)->Center());
	}
}

void CommandUtil::repair(Unit unit, Unit target, bool repeat)
{
	if (!unit || !target)
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (unit->getLastCommandFrame() >= Broodwar->getFrameCount() || unit->isAttackFrame())
	{
		return;
	}

	// 명령 받은지 5초 이상 지났으면 동일 명령이라도 다시 내려준다.
	if (TIME - unit->getLastCommandFrame() > 5 * 24)
		repeat = true;

	// get the unit's current command
	if (!repeat)
	{
		UnitCommand currentCommand(unit->getLastCommand());

		// if we've already told this unit to move to this position, ignore this command
		if (currentCommand.getType() == UnitCommandTypes::Repair && currentCommand.getTarget() == target && unit->isRepairing())
		{
			return;
		}
	}

	// if nothing prevents it, attack the target
	unit->repair(target);
}

void CommandUtil::backMove(Unit attacker, Unit target, bool attackingMove, bool repeat)
{
	UnitInfo *enemy = INFO.getUnitInfo(target, E);
	int dist = 0;

	const CPPath &path = theMap.GetPath(attacker->getPosition(), INFO.getMainBaseLocation(S)->Center(), &dist);

	Position targetPos = Positions::None;

	if (!path.empty()) {
		int distance = attacker->getPosition().getApproxDistance((Position)path.at(0)->Center());

		if (distance >= 2 * TILE_SIZE && distance < 7 * TILE_SIZE) {
			targetPos = (Position)path.at(0)->Center();
		}
	}

	if (!targetPos.isValid()) {
		if (path.empty()) {
			uList bunkers = INFO.getUnits(Terran_Bunker, S);

			for (auto b : bunkers) {
				if (b->isComplete() && b->getSpaceRemaining()) {
					rightClick(attacker, b->unit(), repeat);
					return;
				}
			}

			targetPos = INFO.getMainBaseLocation(S)->Center();
		}
		else if (path.size() == 1)
			targetPos = INFO.getMainBaseLocation(S)->Center();
		else
			targetPos = (Position)path.at(1)->Center();

	}

	if (enemy == nullptr || enemy->isHide())
		CommandUtil::move(attacker, targetPos, repeat);
	else {
		bool canAttack = attackingMove && attacker->isInWeaponRange(enemy->unit()) && ((enemy->type().isFlyer() && attacker->getType().airWeapon().targetsAir() && !attacker->getAirWeaponCooldown())
																					   || (!enemy->type().isFlyer() && attacker->getType().groundWeapon().targetsGround() && !attacker->getGroundWeaponCooldown()));

		if (canAttack) {
			CommandUtil::attackUnit(attacker, enemy->unit(), repeat);
			return;
		}

		int eDist = 0;
		theMap.GetPath(enemy->pos(), INFO.getMainBaseLocation(S)->Center(), &eDist);

		// 적이 나보다 더 본진에 가깝게 있으면 위치를 바꿔준다.
		if (dist + TILE_SIZE > eDist || path.empty()) {
			Position back_pos = { 0, 0 };
			Position myPos = attacker->getPosition();

			double backDistaceNeed = 2 * TILE_SIZE;
			double distance = myPos.getDistance(enemy->pos());
			double alpha = backDistaceNeed / distance;

			back_pos.x = myPos.x + (int)((myPos.x - enemy->pos().x) * alpha);
			back_pos.y = myPos.y + (int)((myPos.y - enemy->pos().y) * alpha);

			int angle = 30;
			int total_angle = 0;

			int value = attacker->getType().isFlyer() ? getPathValueForAir(back_pos) : getPathValue(myPos, back_pos);
			Position bPos = back_pos;

			while (total_angle <= 360)
			{
				back_pos = getCirclePosFromPosByDegree(myPos, back_pos, angle);
				total_angle += angle;

				int tmp_val = attacker->getType().isFlyer() ? getPathValueForAir(back_pos) : getPathValue(myPos, back_pos);

				if (value < tmp_val)
				{
					value = tmp_val;
					bPos = back_pos;
				}
			}

			targetPos = bPos;
		}

		CommandUtil::move(attacker, targetPos, repeat);
	}
}

void CommandUtil::patrol(Unit patroller, const Position &targetPosition, bool repeat)
{
	if (!patroller || !targetPosition.isValid() || !patroller->canPatrol())
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (patroller->getLastCommandFrame() >= Broodwar->getFrameCount())
	{
		return;
	}

	if (!repeat)
	{
		UnitCommand currentCommand(patroller->getLastCommand());

		// if we've already told this unit to move to this position, ignore this command
		if ((currentCommand.getType() == UnitCommandTypes::Patrol) && (currentCommand.getTargetPosition() == targetPosition) && patroller->isPatrolling())
		{
			return;
		}
	}

	patroller->patrol(targetPosition);
}

void CommandUtil::goliathHold(Unit holder, BWAPI::Unit target, bool repeat) {
	// 골리앗 사업을 했으면 사거리 내의 유닛 공격.
	if (S->getUpgradeLevel(UpgradeTypes::Charon_Boosters)) {
		Unit enemy = nullptr;

		if (target != nullptr)
			enemy = target;
		else {
			UnitInfo *closest = INFO.getClosestUnit(E, holder->getPosition(), AllKind, 0, true);

			if (closest != nullptr)
				enemy = closest->unit();
		}

		// 적을 공격할수 있을때만 공격. 아니면 홀드.
		if (target != nullptr && holder->isInWeaponRange(target) && holder->getGroundWeaponCooldown() + holder->getAirWeaponCooldown() == 0) {
			holder->attack(enemy);
			return;
		}
	}

	// 골리앗 사업을 안했으면 기존처럼 홀드.
	hold(holder, repeat);
}

void CommandUtil::hold(Unit holder, bool repeat)
{
	if (!holder || !holder->canHoldPosition())
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (holder->getLastCommandFrame() >= Broodwar->getFrameCount())
	{
		return;
	}

	if (!repeat)
	{
		UnitCommand currentCommand(holder->getLastCommand());

		// if we've already told this unit to move to this position, ignore this command
		if ((currentCommand.getType() == UnitCommandTypes::Hold_Position) && (holder->isHoldingPosition() || holder->isAttacking()))
			return;
	}

	holder->holdPosition();
}

void CommandUtil::holdControll(Unit unit, Unit target, Position targetPosition, bool targetUnit) {
	if (unit == nullptr || target == nullptr)
		return;

	int myWeaponCooldown = target->isFlying() ? unit->getAirWeaponCooldown() : unit->getGroundWeaponCooldown();

	if (target->exists() && unit->isInWeaponRange(target) && !myWeaponCooldown) {
		if (targetUnit)
			attackUnit(unit, target);
		else {
			if (unit->isMoving())
				hold(unit);
			else
				attackUnit(unit, target);
		}
	}
	else
		move(unit, targetPosition);
}

bool CommandUtil::build(Unit builder, UnitType building, TilePosition buildPosition) {
	CPPath path = theMap.GetPath(builder->getPosition(), (Position)buildPosition);

	if (path.size() < 2) {
		UnitCommand currentCommand(builder->getLastCommand());

		if (currentCommand.getType() == UnitCommandTypes::Build && currentCommand.getTargetTilePosition() == buildPosition && (builder->isMoving() || builder->isConstructing())) {
			return false;
		}

		builder->move((Position)buildPosition + (Position)building.tileSize() / 2);
		builder->build(building, buildPosition);

		return true;
	}
	// 지으러 가는데 여러 area를 거쳐서 가야 하는 경우
	else {
		Position targetPosition = (Position)path.at(1)->Center();

		UnitCommand currentCommand(builder->getLastCommand());

		// if we've already told this unit to move to this position, ignore this command
		if (currentCommand.getType() == UnitCommandTypes::Move && currentCommand.getTargetPosition() == targetPosition && builder->isMoving()) {
			return false;
		}

		builder->move(targetPosition);

		return false;
	}
}

void CommandUtil::gather(Unit worker, Unit target) {
	CPPath path = theMap.GetPath(worker->getPosition(), target->getPosition());

	if (path.size() < 2) {
		worker->gather(target);
	}
	// 지으러 가는데 여러 area를 거쳐서 가야 하는 경우
	else {
		Position targetPosition = (Position)path.at(1)->Center();

		UnitCommand currentCommand(worker->getLastCommand());

		// if we've already told this unit to move to this position, ignore this command
		if (currentCommand.getType() == UnitCommandTypes::Move && currentCommand.getTargetPosition() == targetPosition && worker->isMoving()) {
			return ;
		}

		worker->move(targetPosition);
	}
}

bool UnitUtil::IsCombatUnit(Unit unit)
{
	if (!unit)
	{
		return false;
	}

	// no workers or buildings allowed
	if (unit && unit->getType().isWorker() || unit->getType().isBuilding())
	{
		return false;
	}

	// check for various types of combat units
	if (unit->getType().canAttack() ||
			unit->getType() == UnitTypes::Terran_Medic ||
			unit->getType() == UnitTypes::Protoss_High_Templar ||
			unit->getType() == UnitTypes::Protoss_Observer ||
			unit->isFlying() && unit->getType().spaceProvided() > 0)
	{
		return true;
	}

	return false;
}

bool UnitUtil::IsValidUnit(Unit unit)
{
	if (!unit)
	{
		return false;
	}

	if (unit->isCompleted()
			&& unit->getHitPoints() > 0
			&& unit->exists()
			&& unit->getType() != UnitTypes::Unknown
			&& unit->getPosition().x != Positions::Unknown.x
			&& unit->getPosition().y != Positions::Unknown.y)
	{
		return true;
	}
	else
	{
		return false;
	}
}

double UnitUtil::GetDistanceBetweenTwoRectangles(Rect &rect1, Rect &rect2)
{
	Rect &mostLeft = rect1.x < rect2.x ? rect1 : rect2;
	Rect &mostRight = rect2.x < rect1.x ? rect1 : rect2;
	Rect &upper = rect1.y < rect2.y ? rect1 : rect2;
	Rect &lower = rect2.y < rect1.y ? rect1 : rect2;

	int diffX = max(0, mostLeft.x == mostRight.x ? 0 : mostRight.x - (mostLeft.x + mostLeft.width));
	int diffY = max(0, upper.y == lower.y ? 0 : lower.y - (upper.y + upper.height));

	return sqrtf(static_cast<float>(diffX * diffX + diffY * diffY));
}

bool UnitUtil::CanAttack(Unit attacker, Unit target)
{
	return attacker->isCompleted() && GetWeapon(attacker, target) != WeaponTypes::None;
}

double UnitUtil::CalculateLTD(Unit attacker, Unit target)
{
	WeaponType weapon = GetWeapon(attacker, target);

	if (weapon == WeaponTypes::None)
	{
		return 0;
	}

	return static_cast<double>(weapon.damageAmount()) / weapon.damageCooldown();
}

WeaponType UnitUtil::GetWeapon(Unit attacker, Unit target)
{
	return GetWeapon(attacker, target->isFlying());
}

WeaponType UnitUtil::GetWeapon(Unit attacker, bool isTargetFlying)
{
	return GetWeapon(attacker->getType(), isTargetFlying);
}

WeaponType UnitUtil::GetWeapon(UnitType attackerType, bool isFlying)
{
	return isFlying ? attackerType.airWeapon() : attackerType.groundWeapon();
}

int UnitUtil::GetAttackRange(Unit attacker, Unit target)
{
	return attacker->getPlayer()->weaponMaxRange(GetWeapon(attacker, target));
}

int UnitUtil::GetAttackRange(Unit attacker, bool isTargetFlying)
{
	return attacker->getPlayer()->weaponMaxRange(GetWeapon(attacker, isTargetFlying));
}

int UnitUtil::GetAttackRange(UnitType attackerType, Player attackerPlayer, bool isFlying) {
	return attackerPlayer->weaponMaxRange(GetWeapon(attackerType, isFlying));
}

size_t UnitUtil::GetAllUnitCount(UnitType type)
{
	size_t count = 0;

	for (const auto &unit : Broodwar->self()->getUnits())
	{
		// trivial case: unit which exists matches the type
		if (unit->getType() == type)
		{
			count++;
		}

		// case where a zerg egg contains the unit type
		if (unit->getType() == UnitTypes::Zerg_Egg && unit->getBuildType() == type)
		{
			count += type.isTwoUnitsInOneEgg() ? 2 : 1;
		}

		// case where a building has started constructing a unit but it doesn't yet have a unit associated with it
		if (unit->getRemainingTrainTime() > 0)
		{
			UnitType trainType = unit->getLastCommand().getUnitType();

			if (trainType == type && unit->getRemainingTrainTime() == trainType.buildTime())
			{
				count++;
			}
		}
	}

	return count;
}

// 전체 순차탐색을 하기 때문에 느리다
Unit UnitUtil::GetClosestUnitTypeToTarget(UnitType type, Position target)
{
	Unit closestUnit = nullptr;
	double closestDist = 100000000;

	for (auto &unit : Broodwar->self()->getUnits())
	{
		if (unit->getType() == type)
		{
			double dist = unit->getDistance(target);

			if (!closestUnit || dist < closestDist)
			{
				closestUnit = unit;
				closestDist = dist;
			}
		}
	}

	return closestUnit;
}


// unit들의 평균 위치를 구한다.
Position UnitUtil::GetAveragePosition(vector<Unit>  units)
{
	Position pos = { 0, 0 };
	int cnt = 0;

	for (auto &eu : units)
	{
		pos += eu->getPosition();
		cnt++;
	}

	if (cnt)
	{
		pos = pos / cnt;
		return pos;
	}
	else
		return Positions::None;
}

// 넘겨받은 무리 중 myUnit과 가장 가까운 unit을 구한다.
Unit UnitUtil::GetClosestEnemyTargetingMe(Unit myUnit, vector<Unit>  units)
{
	int distance = 999999;
	Unit closestUnit = nullptr;

	for (auto &eu : units)
	{
		if (myUnit->getDistance(eu) < distance)
		{
			closestUnit = eu;
			distance = myUnit->getDistance(eu);
		}
	}

	return closestUnit;
}

BWAPI::Position UnitUtil::getPatrolPosition(BWAPI::Unit attacker, BWAPI::WeaponType weaponType, BWAPI::Position targetPos)
{
	/*
	int minDistance = attacker->getDistance(defenser) - weaponType.maxRange();
	*/
	int x1 = attacker->getPosition().x;
	int y1 = attacker->getPosition().y;
	int x2 = targetPos.x;
	int y2 = targetPos.y;

	Position attackPos = Positions::None;

	if (attacker->getDistance(targetPos) < weaponType.maxRange() - 30)
	{
		attackPos.x = attacker->getPosition().x + (int)((attacker->getPosition().x - targetPos.x) / 2);
		attackPos.y = attacker->getPosition().y + (int)((attacker->getPosition().y - targetPos.y) / 2);
	}
	else
	{
		if (x1 > x2) attackPos.x = x2 + (int)(weaponType.maxRange() - 10);
		else attackPos.x = x2 - (int)(weaponType.maxRange() - 10);

		if (y1 > y2) attackPos.y = y2 + (int)(weaponType.maxRange() - 10);
		else attackPos.x = y2 - (int)(weaponType.maxRange() - 10);
	}

	return attackPos.makeValid();
}
