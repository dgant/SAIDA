#include "TankState.h"
#include "../InformationManager.h"

using namespace MyBot;

State *TankIdleState::action()
{
	// 탱크 1기 있을때 적절한 idle pos
	if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) == 1) {
		if (INFO.getFirstExpansionLocation(S)
				&& (INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, INFO.getFirstExpansionLocation(S)->Center(), 5 * TILE_SIZE, true).size() > 0)) {

			Position p = (INFO.getFirstExpansionLocation(S)->Center() + INFO.getFirstChokePosition(S) + INFO.getSecondAverageChokePosition(S)) / 3;

			if (p.isValid() && bw->isWalkable((WalkPosition)p)) {
				idlePos = p;
			}
		}
	}

	if (idlePos == Positions::Unknown)
		return nullptr;

	if (idlePos == INFO.getWaitingPositionAtFirstChoke(5, 8)) {
		if (INFO.getFirstExpansionLocation(S)
				&& (INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, INFO.getFirstExpansionLocation(S)->Center(), 8 * TILE_SIZE, true).size() > 0)) {
			// 가스가 없으면 그냥 기존 로직
			for (auto g : theMap.GetArea((TilePosition)INFO.getFirstExpansionLocation(S)->Center())->Geysers()) {
				idlePos = g->Pos();
				break;
			}
		}
	}

	if (S->hasResearched(TechTypes::Tank_Siege_Mode)) {

		uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false);

		if (!unitsInMaxRange.empty()) {
			TM.siege(unit);
			return nullptr;
		}
		else // 주변에 적이 없는 경우
		{
			//탱크 idle인데 적 없는 경우 중립물 깨는 로직 추가
			if (INFO.getAllCount(Terran_Factory, S) > 3)
			{
				for (auto n : bw->getStaticNeutralUnits())
				{
					if (n->getType() == Zerg_Egg)// || n->getType() == Special_Power_Generator)
					{
						if (unit->isSieged())
						{
							if (n->getPosition().getDistance(unit->getPosition()) < WeaponTypes::Arclite_Shock_Cannon.maxRange() && INFO.getUnitsInRadius(S, n->getPosition(), 40).size() == 0)
							{
								CommandUtil::attackUnit(unit, n, true);
								return nullptr;
							}
						}
						else
						{
							if (n->getPosition().getDistance(unit->getPosition()) < 12 * TILE_SIZE)
							{
								CommandUtil::attackUnit(unit, n, true);
								return nullptr;
							}
						}
					}
				}
			}

		}

		//bw->drawTextMap(unit->getPosition() + Position(0, 20), "dist : %d, sieze : %s", unit->getPosition().getApproxDistance(idlePos), unit->canUnsiege() ? "true" : "false");

		if (unit->getPosition().getApproxDistance(idlePos) <= 16)
		{
			if (!unit->isSieged())
				TM.siege(unit);
		}
		else {
			if (unit->isSieged())
				TM.unsiege(unit);
			else {
				CommandUtil::move(unit, idlePos, true);
			}
		}
	}
	else {
		if (unit->getPosition().getApproxDistance(idlePos) <= 50)
		{
			CommandUtil::hold(unit);
		}
		else {
			if (unit->getGroundWeaponCooldown() == 0)
				CommandUtil::attackMove(unit, idlePos, true);
			else
				CommandUtil::move(unit, idlePos, true);
		}

	}

	return nullptr;
}

State *TankAttackMoveState::action()
{
	uList unitsInMaxRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false, false);
	uList buildings = INFO.getBuildingsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() - 2 * TILE_SIZE, true, false);
	unitsInMaxRange.insert(unitsInMaxRange.end(), buildings.begin(), buildings.end());
	uList unitsInMinRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 2 * TILE_SIZE, true, false);

	// 공격할 수 있는 적이 없는 경우
	if (unitsInMaxRange.size() == 0) {
		if (unit->isSieged())
			TM.unsiege(unit, false);
		else
			CommandUtil::attackMove(unit, SM.getMainAttackPosition());

		return nullptr;
	}

	if (unit->isSieged()) {
		// 시즈 상태인데, 공격 가능한 유닛이 없으면 Unsieged.
		if (unitsInMaxRange.size() == unitsInMinRange.size()) {
			TM.unsiege(unit, false);

			return nullptr;
		}

		TM.attackFirstTargetOfTank(unit, unitsInMaxRange);
		return nullptr;
	}
	else {

		// 모든 적이 너무 가까이 있을 때.
		if (unitsInMaxRange.size() == unitsInMinRange.size()) {

			// target이 유닛이 아니라 포지션으로 주어지면, 가장 가까운 유닛을 찾아야 한다.
			UnitInfo *closeUnitInfo = INFO.getClosestUnit(E, unit->getPosition(), TypeKind::GroundUnitKind, WeaponTypes::Arclite_Shock_Cannon.maxRange(), false);

			if (closeUnitInfo == nullptr)
				return nullptr;

			int distance = unit->getDistance(closeUnitInfo->unit());

			if (distance <= 2 * TILE_SIZE) {
				moveBackPostion(INFO.getUnitInfo(unit, S), closeUnitInfo->vPos(), 3 * TILE_SIZE);
			}
			else {
				if (unit->getGroundWeaponCooldown() == 0)
					CommandUtil::attackUnit(unit, closeUnitInfo->unit());
			}

		}
		else {
			TM.siege(unit);
		}
	}

	return nullptr;

}

State *TankDefenceState::action()
{
	uList enemy = INFO.getUnitsInRadius(E, unit->getPosition(), 1700, true, true, true);
	uList bList = INFO.getBuildingsInArea(E, MYBASE, true, false, true);
	enemy.insert(enemy.end(), bList.begin(), bList.end());

	UnitInfo *targetInfo = TM.getDefenceTargetUnit(enemy, unit, 1700);

	if (!targetInfo) {
		if (!unit->isSieged())
			CommandUtil::move(unit, TM.waitingPosition);

		return nullptr;
	}

	// 너무 밖으로 나간 탱크
	if (!isInMyArea(unit->getPosition()) && INFO.getBuildings(Terran_Command_Center, S).size() < 3)
	{
		if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 너무 밖으로 나간 탱크" << endl;

		if (unit->isSieged()) {
			uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 14 * TILE_SIZE, true, false, false);

			if (eList.empty())
				TM.unsiege(unit, false);

		}
		else {

			uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 8 * TILE_SIZE, true, false, false);

			if (eList.empty())
				CommandUtil::move(unit, TM.waitingPosition);
			else {

				if (unit->getGroundWeaponCooldown() == 0)
					CommandUtil::attackMove(unit, targetInfo->pos());
				else {
					CommandUtil::move(unit, MYBASE);
					//moveBackPostion(INFO.getUnitInfo(unit, S), targetInfo->vPos(), 3);
				}
			}

		}

		return nullptr;

	}

	// 시즈 모드 업그레이드 전 수비
	if (!S->hasResearched(TechTypes::Tank_Siege_Mode)) {

		if (targetInfo->type().isBuilding()) {
			if (!targetInfo->unit()->isCompleted()) {
				CommandUtil::attackUnit(unit, targetInfo->unit());
			}
			else {
				if (targetInfo->type().groundWeapon().maxRange() < 7 * TILE_SIZE) {
					CommandUtil::attackUnit(unit, targetInfo->unit());
				}
				else {
					int distFromT = targetInfo->pos().getApproxDistance(unit->getPosition());

					if (distFromT < 9 * TILE_SIZE) {
						CommandUtil::move(unit, MYBASE);
					}
					else if (distFromT > 10 * TILE_SIZE) {
						CommandUtil::attackMove(unit, targetInfo->pos());
					}
					else {
						CommandUtil::hold(unit);
					}

				}
			}

			return nullptr;
		}

		if (!targetInfo->unit()->isDetected()) {
			if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 시즈 모드 업그레이드 전 수비 - 보이지 않는 적" << endl;

			int eWeaponRange = targetInfo->type().groundWeapon().maxRange();

			if (unit->getDistance(targetInfo->unit()) - eWeaponRange > 2 * TILE_SIZE) {
				CommandUtil::attackMove(unit, targetInfo->pos());
			}
			else {
				moveBackPostion(INFO.getUnitInfo(unit, S), targetInfo->vPos(), 3 * TILE_SIZE);
			}
		}
		else {
			if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 시즈 모드 업그레이드 전 수비 - 보이는 적" << endl;

			uList dragoons = INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, unit->getPosition(), 12 * TILE_SIZE, true);
			uList tanks = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, unit->getPosition(), 10 * TILE_SIZE, false);

			for (auto dra : dragoons) {
				if (INFO.getAltitudeDifference((TilePosition)unit->getPosition(), (TilePosition)dra->pos()) > 0 ) {
					if (unit->getGroundWeaponCooldown() == 0) {
						CommandUtil::attackMove(unit, targetInfo->pos());
						return nullptr;
					}
				}
			}

			if (!isInMyArea(targetInfo) && !dragoons.empty() && dragoons.size() != tanks.size() && dragoons.size() * 2 >= (tanks.size() + 1)) {
				moveBackPostion(INFO.getUnitInfo(unit, S), targetInfo->vPos(16), 3 * TILE_SIZE);
			}
			else {

				if (unit->getGroundWeaponCooldown() == 0)
					CommandUtil::attackMove(unit, targetInfo->pos());
				else {
					int dangerPoint = 0;
					UnitInfo *dangerUnit = getDangerUnitNPoint(unit->getPosition(), &dangerPoint, false);

					if (dangerUnit && dangerUnit->type() == Zerg_Lurker) {
						CommandUtil::attackUnit(unit, dangerUnit->unit());
					} else if (dangerUnit != nullptr && dangerPoint <= 2 * TILE_SIZE)
						moveBackPostion(INFO.getUnitInfo(unit, S), targetInfo->vPos(), 3 * TILE_SIZE);
				}

			}

		}

		return nullptr;
	}

	// 질럿 올인일떄 탱크 수비
	if (TM.getZealotAllinRush()) {
		if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 질럿 올인일때 수비" << endl;

		if (unit->isSieged()) {
			TM.unsiege(unit);
		}
		else {
			// 너무 가까이 있으면 카이팅
			if (unit->getGroundWeaponCooldown() == 0)
				CommandUtil::attackUnit(unit, targetInfo->unit());
			else {
				if (unit->getDistance(targetInfo->unit()) < 4 * TILE_SIZE)
					moveBackPostion(INFO.getUnitInfo(unit, S), targetInfo->vPos(), 3 * TILE_SIZE);
				else
					CommandUtil::attackMove(unit, targetInfo->pos());
			}
		}

		return nullptr;

	}

	// 영역 밖에서 치는 적에 대한 수비
	if (!isInMyArea(targetInfo)) {
		if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 영역 밖에서 치는 적에 대한 수비" << endl;

		// 나와 멀리 떨어져 있음.
		if (unit->getPosition().getApproxDistance(targetInfo->pos()) > 12 * TILE_SIZE) {

			if (unit->isSieged()) {
				int attackRange = UnitUtil::GetAttackRange(targetInfo->type(), E, false);
				UnitInfo *closeBuildingFromE =
					INFO.getClosestUnit(S, targetInfo->pos(), TypeKind::BuildingKind, attackRange + 3 * TILE_SIZE);

				if (closeBuildingFromE) {
					if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 적과 가까운 우리 건물 있다." << endl;

					// 어택 레인지 안에 우리편 건물이 있으면
					//int attakRange = targetInfo->type().groundWeapon().maxRange();
					UnitInfo *closeTank = INFO.getClosestTypeUnit(S, closeBuildingFromE->pos(), Terran_Siege_Tank_Tank_Mode, 15 * TILE_SIZE);

					// 가장 가까운 탱크가 나라면 내가 수비한다.
					if (closeTank && closeTank->id() == unit->getID()) {
						if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 내가 가까우니까 내가 수비한다." << endl;

						if (unit->isSieged())
							TM.unsiege(unit);
						else {
							if (isInMyArea(unit->getPosition()))
								CommandUtil::attackUnit(unit, targetInfo->unit());
							else
								CommandUtil::move(unit, MYBASE);
						}

						return nullptr;
					}
					else {

						if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 난 머니까 스테이." << endl;

						if (unit->getDistance(INFO.getSecondChokePosition(S)) < 15 * TILE_SIZE) {
							if (unit->isSieged()) {
								// 길막일 때만 언씨즈
								if (checkZeroAltitueAroundMe(unit->getPosition(), 6))
									TM.unsiege(unit);
							}
							else {
								if (!checkZeroAltitueAroundMe(unit->getPosition(), 6))
									TM.siege(unit);
							}

							return nullptr;

						}
						else {
							// SecondChoke와 너무 멀다. 가까이 가자.
							if (unit->isSieged())
								TM.unsiege(unit);
							else
								CommandUtil::attackMove(unit, INFO.getSecondChokePosition(S));

							return nullptr;

						}

					}
				}
				else {
					if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 적과 가까운 우리 건물이 없다" << endl;

				}
			}
			else
				CommandUtil::attackMove(unit, targetInfo->pos());

			return nullptr;

		}
		else {

			if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 영역밖의 적과 내가 가까이 있다." << endl;

			// 나와 가까이 있음.
			if (unit->isSieged()) {
				// 일반 공격
				if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 일반 공격" << endl;

			}
			else {
				if (unit->getPosition().getApproxDistance(targetInfo->pos()) > 11 * TILE_SIZE
						&& !checkZeroAltitueAroundMe(unit->getPosition(), 6))
					TM.siege(unit);
				else
					moveBackPostion(INFO.getUnitInfo(unit, S), targetInfo->vPos(), 3 * TILE_SIZE);

				return nullptr;
			}
		}

		return nullptr;
	}

	// 보이지 않는 다크 같은 유닛
	if (targetInfo->type() == Protoss_Dark_Templar || targetInfo->type() == Zerg_Lurker) {
		if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 보이지 않는 유닛 수비" << endl;

		TM.defenceInvisibleUnit(unit, targetInfo);
		return nullptr;
	}

	// 나머지 경우
	if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 나머지 경우" << endl;

	TM.commonAttackActionByTank(unit, targetInfo);

	return nullptr;
}

State *TankBaseDefenceState::action()
{
	uList eList = INFO.getUnitsInArea(E, MYBASE, true, true, true);
	uList bList = INFO.getBuildingsInArea(E, MYBASE, true, false, true);
	eList.insert(eList.end(), bList.begin(), bList.end());

	uList missiles = INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, S, MYBASE, 8 * TILE_SIZE);
	UnitInfo *turret = nullptr;

	if (!missiles.empty()) {
		turret = missiles.at(0);
	}

	UnitInfo *closeTarget = TM.getDefenceTargetUnit(eList, unit, 0);

	if (closeTarget == nullptr) {
		if (unit->isSieged())
			TM.unsiege(unit);
		else
			CommandUtil::move(unit, MYBASE + 2 * TILE_SIZE);

		return nullptr;
	}

	// 본진에 들어온 보이지 않는 다크 같은 유닛
	if (closeTarget->type() == Protoss_Dark_Templar || closeTarget->type() == Zerg_Lurker
			&& isSameArea(MYBASE, closeTarget->pos())) {
		if (unit->isSelected()) cout << unit->getID() << "[시즈 행동] 보이지 않는 유닛 수비" << endl;

		TM.defenceInvisibleUnit(unit, closeTarget);
		return nullptr;
	}

	if (INFO.enemyRace == Races::Protoss) {
		// 셔틀 리버 수비
		uList shuttles = INFO.getTypeUnitsInArea(Protoss_Shuttle, E, unit->getPosition(), true);
		uList reavers = INFO.getTypeUnitsInArea(Protoss_Reaver, E, unit->getPosition(), true);

		if (!shuttles.empty() || !reavers.empty()) {
			// 셔틀이나 리버가 왔다면,
			// 한마리는 본진 근처에서 시즈하고
			// 다른 한마리는 퉁퉁포로 싸운다.
			uList baseTanks = TM.getBaseDefenceTankSet().getUnits();

			// 2마리 이상이 수비할 경우, 본진 커맨드 근처에서 시즈모드로 수비
			if (baseTanks.size() > 1 && turret) {

				// 시즈해서 방어해야 하는 애
				if (unit->getID() == TM.getSiegeModeDefenceTank()) {
					if (unit->isSieged()) {
						uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), false, false);
						uList unitsInMinRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 2 * TILE_SIZE, false, false);

						// 적이 있는데 모든 적이 가까이에 있으면..?
						if (unitsInMaxRange.size() != 0 && unitsInMaxRange.size() == unitsInMinRange.size()) {
							TM.unsiege(unit);
						}
						// 리버나 보이는 적 공격
						else if (unitsInMaxRange.size() != unitsInMinRange.size()) {
							TM.attackFirstTargetOfTank(unit, unitsInMaxRange);
						}
						else {
							// 터렛과 거리가 멀면 가까이 가기 위해 Unsiege 한다.
							if (unit->getPosition().getApproxDistance(turret->pos()) > 2 * TILE_SIZE)
								TM.unsiege(unit);
						}

						return nullptr;

					}
					else {

						// Turret 에 가깝게 가서 시즈해야한다.
						// 한마리의 리버라도 시즈모드 해서 쏴죽일 수 있다면.. 시즈 해보자
						if (!reavers.empty()) {
							for (auto r : reavers) {
								if (r->pos().getApproxDistance(unit->getPosition()) > 10 * TILE_SIZE
										&& r->pos().getApproxDistance(unit->getPosition()) <= 12 * TILE_SIZE) {
									if (INFO.getAllInRadius(E, unit->getPosition(), 8 * TILE_SIZE, true, true).empty()) {
										TM.siege(unit);
										return nullptr;
									}
								}
							}
						}

						// 적과 멀면
						if (unit->getPosition().getApproxDistance(closeTarget->pos()) > 3 * TILE_SIZE) {
							// Turret 쪽으로 이동한 뒤,
							if (unit->getPosition().getApproxDistance(turret->pos()) > 2 * TILE_SIZE)
								CommandUtil::move(unit, turret->pos());
							else
								TM.siege(unit); //MYBASE와 적당히 가까워지면 시즈모드한다.

							return nullptr;

						}
						else {
							// 적과 너무 가까우면 카이팅
							if (unit->getGroundWeaponCooldown() == 0 && !closeTarget->unit()->isFlying())
								CommandUtil::attackMove(unit, closeTarget->pos());
							else
								moveBackPostion(INFO.getUnitInfo(unit, S), closeTarget->vPos(), 3 * TILE_SIZE);

							return nullptr;

						}
					}
				}
				else {
					// 퉁퉁포로 방어하는 애
					if (unit->isSieged()) {
						uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false);
						uList unitsInMinRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 3 * TILE_SIZE, true, false);

						if (unitsInMaxRange.size() != unitsInMinRange.size())
							TM.attackFirstTargetOfTank(unit, unitsInMaxRange);
						else
							TM.unsiege(unit);

						return nullptr;
					}
					else {
						if (unit->getGroundWeaponCooldown() == 0)
							CommandUtil::attackMove(unit, closeTarget->pos());
						else
							moveBackPostion(INFO.getUnitInfo(unit, S), closeTarget->vPos(), 3 * TILE_SIZE);

						return nullptr;

					}
				}
			}
			else {
				// 탱크가 한마리면 걍 퉁퉁포로 싸움
				if (unit->isSieged()) {

					uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false);
					uList unitsInMinRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 3 * TILE_SIZE, true, false);

					if (unitsInMaxRange.size() != unitsInMinRange.size())
						TM.attackFirstTargetOfTank(unit, unitsInMaxRange);
					else
						TM.unsiege(unit);

					return nullptr;

				}
				else {
					if (unit->getGroundWeaponCooldown() == 0)
						CommandUtil::attackMove(unit, closeTarget->pos());
					else
						moveBackPostion(INFO.getUnitInfo(unit, S), closeTarget->vPos(), 3 * TILE_SIZE);

					return nullptr;

				}
			}
		}

	}

	if (!S->hasResearched(TechTypes::Tank_Siege_Mode)) {

		if (closeTarget->type().isBuilding()) {
			if (!closeTarget->unit()->isCompleted()) {
				CommandUtil::attackUnit(unit, closeTarget->unit());
			}
			else {
				if (closeTarget->type().groundWeapon().maxRange() < 7 * TILE_SIZE) {
					CommandUtil::attackUnit(unit, closeTarget->unit());
				}
				else {
					int distFromT = closeTarget->pos().getApproxDistance(unit->getPosition());

					if (distFromT < 9 * TILE_SIZE) {
						CommandUtil::move(unit, MYBASE);
					}
					else if (distFromT > 10 * TILE_SIZE) {
						CommandUtil::attackMove(unit, closeTarget->pos());
					}
					else {
						CommandUtil::hold(unit);
					}

				}
			}

			return nullptr;
		}

		if (!closeTarget->unit()->isDetected()) {
			int eWeaponRange = closeTarget->type().groundWeapon().maxRange();

			if (unit->getDistance(closeTarget->unit()) - eWeaponRange > 2 * TILE_SIZE) {
				CommandUtil::attackMove(unit, closeTarget->pos());
			}
			else {
				moveBackPostion(INFO.getUnitInfo(unit, S), closeTarget->vPos(), 3 * TILE_SIZE);
			}
		}
		else {
			uList dragoons = INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, unit->getPosition(), 12 * TILE_SIZE, true);
			uList tanks = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, unit->getPosition(), 10 * TILE_SIZE, false);

			if (dragoons.size() != tanks.size() && dragoons.size() * 2 >= (tanks.size() + 1)) {
				moveBackPostion(INFO.getUnitInfo(unit, S), closeTarget->vPos(16), 3 * TILE_SIZE);
			}
			else {
				if (unit->getGroundWeaponCooldown() == 0)
					CommandUtil::attackMove(unit, closeTarget->pos());
				else {
					int dangerPoint = 0;
					UnitInfo *dangerUnit = getDangerUnitNPoint(unit->getPosition(), &dangerPoint, false);

					if (dangerUnit != nullptr && dangerPoint <= 2 * TILE_SIZE)
						moveBackPostion(INFO.getUnitInfo(unit, S), closeTarget->vPos(), 3 * TILE_SIZE);
				}
			}
		}

		return nullptr;
	}

	if (unit->isSieged()) {
		uList unitsInMaxRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false);
		uList unitsInMinRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 3 * TILE_SIZE, true, false);

		if (unitsInMaxRange.size() != unitsInMinRange.size()) {
			if (!TM.attackFirstTargetOfTank(unit, unitsInMaxRange))
				TM.unsiege(unit);
		} else
			TM.unsiege(unit);

		return nullptr;
	}
	else {

		int distance = unit->getDistance(closeTarget->pos());

		if (unit->getGroundWeaponCooldown() == 0) {
			CommandUtil::attackMove(unit, closeTarget->pos());
		}
		else {
			if (distance <= 2 * TILE_SIZE)
				moveBackPostion(INFO.getUnitInfo(unit, S), closeTarget->vPos(), 3 * TILE_SIZE);
			else
				CommandUtil::attackMove(unit, closeTarget->pos());
		}

		return nullptr;
	}

	return nullptr;
}

State *TankBackMove::action()
{
	if (unit->isSieged()) {
		TM.unsiege(unit);
	}
	else {
		if (unit->getGroundWeaponCooldown() == 0)
			CommandUtil::attackMove(unit, targetPos);
		else
			CommandUtil::move(unit, targetPos);
	}

	return nullptr;
}

State *TankFightState::action()
{
	if (S->hasResearched(TechTypes::Tank_Siege_Mode))
		return new TankIdleState();

	vector<UnitType> types = { Terran_Siege_Tank_Tank_Mode, Terran_Goliath };
	UnitInfo *myFrontUnit = INFO.getClosestTypeUnit(S, SM.getMainAttackPosition(), types, 0, false, true);

	// 그냥 15 타일을 봐바
	UnitInfo *closestTank = INFO.getClosestTypeUnit(E, myFrontUnit->pos(), Terran_Siege_Tank_Tank_Mode, 15 * TILE_SIZE, true);

	if (closestTank && SM.getNeedDefenceWithScv())
	{
		if (unit->getGroundWeaponCooldown() == 0)
		{
			if (closestTank->isHide())
				CommandUtil::attackMove(unit, closestTank->pos());
			else
				CommandUtil::attackUnit(unit, closestTank->unit());
		}
		else
		{
			if (unit->getDistance(closestTank->pos()) > TILE_SIZE)
				CommandUtil::move(unit, closestTank->pos());
		}

		return nullptr;
	}

	UnitInfo *closestMarine = INFO.getClosestTypeUnit(E, myFrontUnit->pos(), Terran_Marine, 15 * TILE_SIZE, true);

	if (closestMarine)
	{
		UnitInfo *me = INFO.getUnitInfo(unit, S);
		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(me->pos(), &dangerPoint, false);

		UnitInfo *marine = INFO.getClosestTypeUnit(E, me->pos(), Terran_Marine, 0, true);
		attackFirstkiting(me, marine, dangerPoint, 3 * TILE_SIZE);

		return nullptr;
	}

	Position waitPosition = Positions::None;

	if (SM.getNeedAdvanceWait())
	{
		waitPosition = getDirectionDistancePosition(INFO.getSecondChokePosition(S), INFO.getFirstWaitLinePosition(), 2 * TILE_SIZE);

		if (!bw->isWalkable((WalkPosition)waitPosition))
		{
			waitPosition = getDirectionDistancePosition(INFO.getSecondChokePosition(S), INFO.getFirstWaitLinePosition(), 2 * TILE_SIZE);
		}

		if (!bw->isWalkable((WalkPosition)waitPosition))
		{
			waitPosition = INFO.getSecondChokePosition(S);
		}
	}
	else
		waitPosition = (INFO.getSecondChokePosition(S) + INFO.getFirstExpansionLocation(S)->getPosition()) / 2;

	if (unit->getGroundWeaponCooldown() == 0)
		CommandUtil::attackMove(unit, waitPosition);
	else
	{
		if (unit->getPosition().getApproxDistance(waitPosition) > 3 * TILE_SIZE)
			CommandUtil::move(unit, waitPosition, true);
	}

	return nullptr;
}

State *TankSiegeLineState::action()
{
	uList unitsInMaxRange, buildings;

	// DrawLine일 때는 공격적으로 가야 하기 때문에 사거리 책정을 좀 줄인다.
	if (SM.getMainStrategy() == DrawLine || SM.getMainStrategy() == AttackAll)
	{
		unitsInMaxRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() - 3 * TILE_SIZE, true, false, false);
		buildings = INFO.getBuildingsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() - 3 * TILE_SIZE, true, false);
		unitsInMaxRange.insert(unitsInMaxRange.end(), buildings.begin(), buildings.end());
	}
	else
	{
		unitsInMaxRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() + TILE_SIZE, true, false, false);
		buildings = INFO.getBuildingsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() - 2 * TILE_SIZE, true, false);
		unitsInMaxRange.insert(unitsInMaxRange.end(), buildings.begin(), buildings.end());
	}

	uList unitsInMinRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange(), true, false, false);

	//	uList unitsInMinRange = INFO.getAllInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + TILE_SIZE, true, false);

	int dangerPoint = 0;
	UnitInfo *tankInfo = INFO.getUnitInfo(unit, S);
	UnitInfo *dangerUnit = getDangerUnitNPoint(tankInfo->pos(), &dangerPoint, false);

	Position targetPos = Positions::None;

	if (SM.getMainStrategy() == DrawLine)
		targetPos = SM.getDrawLinePosition();
	else if (SM.getMainStrategy() == WaitLine)
		targetPos = SM.getWaitLinePosition();
	else
		targetPos = SM.getMainAttackPosition();

	int myGDist, thresholdGDist, distToTarget;

	if (theMap.GetArea((WalkPosition)targetPos) == nullptr)
	{
		myGDist = tankInfo->pos().getApproxDistance(INFO.getSecondChokePosition(S));
		thresholdGDist = targetPos.getApproxDistance(INFO.getSecondChokePosition(S));
		distToTarget = tankInfo->pos().getApproxDistance(targetPos);
	}
	else
	{
		myGDist = getGroundDistance(MYBASE, tankInfo->pos());
		thresholdGDist = getGroundDistance(MYBASE, targetPos);
		distToTarget = tankInfo->pos().getApproxDistance(targetPos);
	}

	// 공격할 수 있는 적이 없는 경우
	if (unitsInMaxRange.size() == 0) {

		int Threshold = 3 * TILE_SIZE;

		if (SM.getMainStrategy() == DrawLine || SM.getMainStrategy() == AttackAll)
			Threshold = 1 * TILE_SIZE;

		if (dangerUnit == nullptr || dangerPoint > Threshold)
		{
			if (SM.getMainStrategy() == AttackAll || myGDist < thresholdGDist - 3 * TILE_SIZE || distToTarget > 15 * TILE_SIZE)
			{
				if (unit->isSieged())
					TM.unsiege(unit, false);
				else
				{
					CommandUtil::attackMove(unit, targetPos);
				}
			}
			else
			{
				if (!unit->isSieged())
					TM.siege(unit);
			}
		}
		else
		{
			if (!unit->isSieged())
				TM.siege(unit);
			else
			{
				unitsInMaxRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() + TILE_SIZE, true, false, false);
				TM.attackFirstTargetOfTank(unit, unitsInMaxRange);
			}
		}

		return nullptr;
	}

	if (unit->isSieged()) {
		// 시즈 상태인데, 공격 가능한 유닛이 없으면 Unsieged.
		if (unitsInMaxRange.size() == unitsInMinRange.size()) {
			TM.unsiege(unit);

			return nullptr;
		}

		TM.attackFirstTargetOfTank(unit, unitsInMaxRange);
		return nullptr;
	}
	else {
		// 모든 적이 너무 가까이 있을 때.
		if (unitsInMaxRange.size() == unitsInMinRange.size()) {

			// target이 유닛이 아니라 포지션으로 주어지면, 가장 가까운 유닛을 찾아야 한다.
			UnitInfo *closeUnitInfo = INFO.getClosestUnit(E, unit->getPosition(), TypeKind::GroundUnitKind, WeaponTypes::Arclite_Shock_Cannon.maxRange(), false);

			if (closeUnitInfo == nullptr)
				return nullptr;

			int distance = unit->getDistance(closeUnitInfo->unit());

			if (distance <= 2 * TILE_SIZE) {
				moveBackPostion(INFO.getUnitInfo(unit, S), closeUnitInfo->vPos(), 3 * TILE_SIZE);
			}
			else {
				if (unit->getGroundWeaponCooldown() == 0)
					CommandUtil::attackUnit(unit, closeUnitInfo->unit());
			}
		}
		else {
			TM.siege(unit);
		}
	}

	return nullptr;
}

State *TankMultiBreakState::action()
{
	if (SM.getSecondAttackPosition() == Positions::Unknown)
		return nullptr;

	UnitInfo *me = INFO.getUnitInfo(unit, S);

	uList attackUnitsInMaxRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() + 2 * TILE_SIZE, true, false, false);
	uList attackUnitsInMinRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 4 * TILE_SIZE, true, false, false);
	uList buildings = INFO.getBuildingsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() - 2 * TILE_SIZE, true, false, true);
	UnitInfo *commandBuilding = nullptr;

	uList workersInMaxRange = INFO.getTypeUnitsInRadius(INFO.getWorkerType(INFO.enemyRace), E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), false);
	uList workersInMinRange = INFO.getTypeUnitsInRadius(INFO.getWorkerType(INFO.enemyRace), E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 2 * TILE_SIZE, false);

	uList neerVulture = INFO.getTypeUnitsInRadius(Terran_Vulture, S, unit->getPosition(), 6 * TILE_SIZE, false);
	uList neerGoliath = INFO.getTypeUnitsInRadius(Terran_Goliath, S, unit->getPosition(), 6 * TILE_SIZE, false);

	if (INFO.enemyRace == Races::Terran) // 테란전은 그냥 멀리서부터 시즈
	{
		attackUnitsInMaxRange.insert(attackUnitsInMaxRange.end(), buildings.begin(), buildings.end());
	}
	else
	{

		for (auto b : buildings)
		{
			if (b->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace)) {
				// 언덕위에 있는 캐논
				if (INFO.getAltitudeDifference((TilePosition)b->pos(), unit->getTilePosition()) > 0) {
					if (b->isHide()) {
						if (ComsatStationManager::Instance().getAvailableScanCount() > 3) {
							ComsatStationManager::Instance().useScan(b->pos());
						}
					}
				}

				attackUnitsInMaxRange.push_back(b);

			}
			else if (b->type() == INFO.getBasicResourceDepotBuildingType(INFO.enemyRace))
				commandBuilding = b;
		}
	}

	if (INFO.enemyRace == Races::Protoss) {
		// 아비터 만나면 째자.
		uList arbiters = INFO.getTypeUnitsInRadius(Protoss_Arbiter, E, unit->getPosition(), 15 * TILE_SIZE);
		uList vessels = INFO.getTypeUnitsInRadius(Terran_Science_Vessel, S, unit->getPosition(), 15 * TILE_SIZE);

		// 아비터가 있어서 빼야 하는경우
		if (!arbiters.empty() && vessels.empty() && !attackUnitsInMaxRange.empty() && ComsatStationManager::Instance().getAvailableScanCount() < 2) {
			if (unit->isSieged()) {
				if (!(unit->getGroundWeaponCooldown() == 0 && TM.attackFirstTargetOfTank(unit, attackUnitsInMaxRange))) {
					TM.unsiege(unit);
				}
			}
			else {
				if (unit->getGroundWeaponCooldown() == 0) {
					CommandUtil::attackMove(unit, MYBASE);
				}
				else {
					CommandUtil::move(unit, MYBASE);
				}
			}

			return nullptr;
		}

		//적 병력이 너무 많으면 째자
		uList eList = INFO.getUnitsInRadius(E, unit->getPosition(), 16 * TILE_SIZE, true, false, false);
		uList mList = INFO.getUnitsInRadius(S, unit->getPosition(), 16 * TILE_SIZE, true, false, false);

		if (eList.size() > mList.size() + 2) {

			if (unit->isSieged()) {
				if (!(unit->getGroundWeaponCooldown() == 0 && TM.attackFirstTargetOfTank(unit, eList)))
					TM.unsiege(unit);
			}
			else {
				if (unit->getGroundWeaponCooldown() == 0)
					CommandUtil::attackMove(unit, SM.getSecondAttackPosition());
				else
					CommandUtil::move(unit, INFO.getMainBaseLocation(S)->Center());
			}

			return nullptr;
		}
	}

	// 주변에 아예 적이 없는 경우 공격할 수 있는 적이 없는 경우
	if (attackUnitsInMaxRange.size() == 0) {

		if (unit->isSieged()) {

			// 매우 근접한 적 일꾼이 있으면 Unsiege 한다.
			if (!workersInMaxRange.empty() && (workersInMaxRange.size() == workersInMinRange.size())
					&& (neerVulture.empty() && neerGoliath.empty())) {
				TM.unsiege(unit);
				return nullptr;
			}

			// 적군 커맨드가 보이면 커맨드 공격
			if (commandBuilding)
				CommandUtil::attackUnit(unit, commandBuilding->unit());
			else
				TM.unsiege(unit);

			return nullptr;

		}
		else
		{

			// 근처에 일꾼이 있으면 카이팅해서 잡자.
			if (!workersInMaxRange.empty() && (workersInMaxRange.size() == workersInMinRange.size())
					&& (neerVulture.empty() && neerGoliath.empty())) {

				UnitInfo *closeUnitInfo = INFO.getClosestUnit(E, unit->getPosition(), TypeKind::GroundUnitKind, 5 * TILE_SIZE, false);

				if (closeUnitInfo == nullptr)
					return nullptr;

				attackFirstkiting(INFO.getUnitInfo(unit, S), closeUnitInfo, unit->getPosition().getApproxDistance(closeUnitInfo->pos()), 2 * TILE_SIZE);
				return nullptr;
			}
			else if (commandBuilding)
				TM.siege(unit);
			else {
				if (unit->getGroundWeaponCooldown() == 0)
					CommandUtil::attackMove(unit, SM.getSecondAttackPosition());
				else
					CommandUtil::move(unit, SM.getSecondAttackPosition());
			}
		}

		return nullptr;
	}

	if (unit->isSieged()) {

		// 시즈 상태인데, 모든 적이 근접한 경우.
		if (attackUnitsInMaxRange.size() == attackUnitsInMinRange.size())
			TM.unsiege(unit);
		else
			TM.attackFirstTargetOfTank(unit, attackUnitsInMaxRange);

		return nullptr;

	}
	else {
		// 모든 적이 너무 가까이 있을 때.
		if (attackUnitsInMaxRange.size() == attackUnitsInMinRange.size()) {

			// target이 유닛이 아니라 포지션으로 주어지면, 가장 가까운 유닛을 찾아야 한다.
			UnitInfo *closeUnitInfo = INFO.getClosestUnit(E, unit->getPosition(), TypeKind::GroundUnitKind, WeaponTypes::Arclite_Shock_Cannon.maxRange(), false);

			if (closeUnitInfo == nullptr)
				return nullptr;

			attackFirstkiting(INFO.getUnitInfo(unit, S), closeUnitInfo, unit->getPosition().getApproxDistance(closeUnitInfo->pos()), 5  * TILE_SIZE);
		}
		else {

			// Multi break는 소수 싸움이 대부분이라
			// Unsiege 이지만 달려들 수 있는 질럿이나 저글링이 있으면 시즈하지 않는다.
			if (INFO.getTypeUnitsInRadius(Protoss_Zealot, E, unit->getPosition(), 12 * TILE_SIZE).empty()
					&& INFO.getTypeUnitsInRadius(Zerg_Zergling, E, unit->getPosition(), 12 * TILE_SIZE).empty())
				TM.siege(unit);
			else {
				UnitInfo *closeUnitInfo = INFO.getClosestUnit(E, unit->getPosition(), TypeKind::GroundUnitKind, WeaponTypes::Arclite_Shock_Cannon.maxRange(), false);

				if (closeUnitInfo == nullptr)
					return nullptr;

				attackFirstkiting(INFO.getUnitInfo(unit, S), closeUnitInfo, unit->getPosition().getApproxDistance(closeUnitInfo->pos()), 5 * TILE_SIZE);

				//kiting(me, closeUnitInfo, me->pos().getApproxDistance(closeUnitInfo->pos()), 3 * TILE_SIZE);
			}
		}
	}

	return nullptr;
}

State *TankKeepMultiState::action()
{
	if (INFO.enemyRace == Races::Terran) return nullptr;

	UnitInfo *me = INFO.getUnitInfo(unit, S);

	if (INFO.enemyRace != Races::Terran)
	{
		if (SM.getNeedKeepSecondExpansion(me->type()))
			m_targetPos = INFO.getSecondExpansionLocation(S)->Center();
		else if (SM.getNeedKeepThirdExpansion(me->type()))
			m_targetPos = INFO.getThirdExpansionLocation(S)->Center();
	}

	// 목적지에서 그냥 대기
	if (me->pos().getApproxDistance(m_targetPos) < 3 * TILE_SIZE)
	{
		if (!unit->isSieged())
			unit->siege();

		return nullptr;
	}

	uList unitsInMaxRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false, false);
	uList buildings = INFO.getBuildingsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() + TILE_SIZE, true, false);

	for (auto b : buildings)
	{
		if (b->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
			unitsInMaxRange.push_back(b);
	}

	uList unitsInMinRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.minRange() + 2 * TILE_SIZE, true, false, false);

	// 공격할 수 있는 적이 없는 경우
	if (unitsInMaxRange.size() == 0) {
		if (unit->isSieged())
			TM.unsiege(unit);
		else
		{
			if (me->pos().getApproxDistance(m_targetPos) >  3 * TILE_SIZE)
				CommandUtil::move(unit, m_targetPos);
		}

		return nullptr;
	}

	if (unit->isSieged()) {
		// 시즈 상태인데, 공격 가능한 유닛이 없으면 Unsieged.
		if (unitsInMaxRange.size() == unitsInMinRange.size()) {
			TM.unsiege(unit);

			return nullptr;
		}
		else
		{
			TM.attackFirstTargetOfTank(unit, unitsInMaxRange);
		}
	}
	else {
		// 모든 적이 너무 가까이 있을 때.
		if (unitsInMaxRange.size() == unitsInMinRange.size()) {

			// target이 유닛이 아니라 포지션으로 주어지면, 가장 가까운 유닛을 찾아야 한다.
			UnitInfo *closeUnitInfo = INFO.getClosestUnit(E, unit->getPosition(), TypeKind::GroundUnitKind, WeaponTypes::Arclite_Shock_Cannon.maxRange() + TILE_SIZE, false);

			if (closeUnitInfo == nullptr)
				return nullptr;

			kiting(me, closeUnitInfo, me->pos().getApproxDistance(closeUnitInfo->pos()), 3 * TILE_SIZE);
		}
		else {
			TM.siege(unit);
		}
	}

	return nullptr;
}

State *TankDropshipState::action()
{
	// 타고 있으면 스킵
	if (unit->isLoaded())
	{
		firstBoard = true;
		return nullptr;
	}

	if (firstBoard == false && (!m_targetUnit->exists() || m_targetUnit->getSpaceRemaining() < Terran_Siege_Tank_Tank_Mode.spaceRequired() || INFO.getUnitInfo(m_targetUnit, S)->getState() == "Go"))
	{
		return new TankIdleState();
	}

	// 한번도 탄적이 없어
	if (firstBoard == false)
	{
		if (unit->isSieged())
			unit->unsiege();

		// 드랍쉼 우클릭
		if (isInMyArea(unit->getPosition()))
			CommandUtil::rightClick(unit, m_targetUnit);
		else
			CommandUtil::move(unit, MYBASE);

		return nullptr;
	}

	// 여기부터 공격
	if (targetPosition == Positions::Unknown || INFO.getTypeBuildingsInRadius(Terran_Command_Center, E, targetPosition, 8 * TILE_SIZE).size() == 0)
	{
		if (timeToClear == 0)
			timeToClear = TIME;

		if (timeToClear + 24 * 60 < TIME)
		{
			timeToClear = 0;

			Base *nearestBase = nullptr;
			int dist = INT_MAX;

			for (auto base : INFO.getOccupiedBaseLocations(E))
			{
				if (INFO.getTypeBuildingsInRadius(Terran_Command_Center, E, base->Center(), 8 * TILE_SIZE).size() == 0)
					continue;

				int tmp = getGroundDistance(unit->getPosition(), base->Center());

				if (tmp >= 0 && dist > tmp)
				{
					dist = tmp;
					nearestBase = (Base *)base;
				}
			}

			if (nearestBase)
				targetPosition = nearestBase->Center();
			else
				targetPosition = SM.getMainAttackPosition();
		}
	}

	uList unitsInMaxRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false, true);

	uList buildings = INFO.getBuildingsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() + TILE_SIZE, true, false);
	unitsInMaxRange.insert(unitsInMaxRange.end(), buildings.begin(), buildings.end());

	if (unitsInMaxRange.size() == 0) // 공격할 대상이 없음.
	{
		if (unit->isSieged() && timeToClear + 24 * 60 < TIME)
			unit->unsiege();
		else
			CommandUtil::attackMove(unit, targetPosition);
	}
	else
	{
		if (!unit->isSieged())
			unit->siege();
		else
		{
			TM.attackFirstTargetOfTank(unit, unitsInMaxRange);
		}
	}

	return nullptr;
}

State *TankProtectAdditionalMultiState::action()
{
	Position targetPos = getDirectionDistancePosition(m_targetBase->Center(), theMap.Center(), 2 * TILE_SIZE);

	if (unit->getPosition().getApproxDistance(targetPos) < 3 * TILE_SIZE)
	{
		if (!unit->isSieged()) unit->siege();

		return nullptr;
	}

	uList unitsInMaxRange = INFO.getUnitsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange(), true, false, false);

	uList buildings = INFO.getBuildingsInRadius(E, unit->getPosition(), WeaponTypes::Arclite_Shock_Cannon.maxRange() + TILE_SIZE, true, false);
	unitsInMaxRange.insert(unitsInMaxRange.end(), buildings.begin(), buildings.end());

	if (unitsInMaxRange.size() == 0) // 공격할 대상이 없음.
	{
		if (unit->isSieged())
			unit->unsiege();
		else
			CommandUtil::attackMove(unit, targetPos);
	}
	else
	{
		if (!unit->isSieged())
			unit->siege();
		else
			TM.attackFirstTargetOfTank(unit, unitsInMaxRange);
	}

	return nullptr;
}
