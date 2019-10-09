#include "ComsatStationManager.h"
#include "../InformationManager.h"

using namespace MyBot;

ComsatStationManager::ComsatStationManager()
{
	availableScanCount = 0;
}

ComsatStationManager &ComsatStationManager::Instance()
{
	static ComsatStationManager instance;
	return instance;
}

void ComsatStationManager::update()
{
	if (TIME < 300 || TIME % 12 != 0)
		return;

	uList comsatStationList = INFO.getBuildings(Terran_Comsat_Station, S);

	if (comsatStationList.empty())
		return;

	int scanCount = 0;

	updateReserveScanList();

	for (auto c : comsatStationList)
	{
		string state = c->getState();

		if (state == "New" && c->isComplete()) {
			c->setState(new ComsatStationIdleState());
		}

		if (state == "Idle")
		{
			c->action();
		}

		if (state == "Activation")
		{
			auto rs = reserveScanList.begin();

			if (rs != reserveScanList.end())
			{
				c->setState(new ComsatStationScanState());
				c->action(*rs);
				reserveScanList.erase(rs);
			}

			scanCount += c->unit()->getEnergy() / TechTypes::Scanner_Sweep.energyCost();
		}
	}

	// 사용할 수 있는 전체 스캔 횟수
	availableScanCount = scanCount;
	INFO.setAvailableScanCount(scanCount);

	// 스캔 없을 경우 예약 리스트 초기화, 스캔 사용할 수 있을때까지 해당 유닛의 위치가 바뀌므로
	if (availableScanCount < 1)
	{
		reserveScanList.clear();
		return;
	}
}

bool ComsatStationManager::inDetectedArea(Unit targetUnit)
{
	return inDetectedArea(targetUnit->getType(), targetUnit->getPosition());
}

bool ComsatStationManager::inDetectedArea(UnitType targetType, Position targetPosition)
{
	bool isExist = false;

	uList missileTuretList = INFO.getBuildings(Terran_Missile_Turret, S);

	for (auto m : missileTuretList)
	{
		if (!m->isComplete())
			continue;

		// 터렛 범위내에 있는 유닛인지 확인 (a^2 + b^2 < c^2)
		int distance = getAttackDistance(m->unit(), targetType, targetPosition);

		if (distance < m->type().airWeapon().maxRange())
		{
			isExist = true;
			break;
		}
	}

	if (!isExist)
	{
		uList scienceVesselList = INFO.getUnits(Terran_Science_Vessel, S);

		for (auto s : scienceVesselList)
		{
			if (!s->isComplete())
				continue;

			// 베슬 범위내에 있는 유닛인지 확인 (a^2 + b^2 < c^2)
			if (inArea(s->pos(), targetPosition, s->type().sightRange()))
			{
				isExist = true;
				break;
			}
		}
	}

	return isExist;
}

bool ComsatStationManager::inTheScanArea(Position targetPosition)
{
	bool isExist = false;

	uList scanList = INFO.getUnits(Spell_Scanner_Sweep, S);

	for (auto s : scanList)
	{
		// 스캔 범위내에 있는 유닛인지 확인 (a^2 + b^2 < c^2)
		if (inArea(s->pos(), targetPosition, Spell_Scanner_Sweep.sightRange()))
		{
			isExist = true;
			break;
		}
	}

	return isExist;
}

bool ComsatStationManager::isReservedPosition(Position targetPosition)
{
	bool isExist = false;

	for (auto rs : reserveScanList)
	{
		// 예약된 스캔 위치에 있는 유닛인지 확인 (a^2 + b^2 < c^2)
		if (inArea(rs, targetPosition, Spell_Scanner_Sweep.sightRange()))
		{
			isExist = true;
			break;
		}
	}

	return isExist;
}

bool ComsatStationManager::checkScan(Unit targetUnit)
{
	// 해당 위치에 스캔이 이미 사용되었거나, 예약되어 있거나, 디텍트 지역이라면 스캔 필요 X
	if (inTheScanArea(targetUnit->getPosition()) || isReservedPosition(targetUnit->getPosition()) || inDetectedArea(targetUnit))
	{
		return false;
	}

	// targetUnit 공격 가능한 내 유닛
	uList myAttackableUnits = getMyAttackableUnitsForScan(targetUnit);

	if (myAttackableUnits.empty())
		return false;

	// 타겟 유닛이 지상
	if (!targetUnit->getType().isFlyer())
	{
		double scanValue = 0;

		for (auto u : myAttackableUnits)
		{
			scanValue += u->type().groundWeapon().damageAmount();
		}

		int eHP = targetUnit->getHitPoints() + targetUnit->getShields();
		eHP = eHP == 0 ? targetUnit->getType().maxHitPoints() + targetUnit->getType().maxShields() : eHP;

		if ((scanValue / (availableScanCount + 1) >= eHP) || myAttackableUnits.size() > 3)
		{
			return true;
		}
	}
	else
	{
		if (targetUnit->getType() == Protoss_Observer) {
			int baseScanCount = 1;
			int goliathCnt = 0;

			for (auto g : myAttackableUnits) {
				if (g->type() == Terran_Goliath) {
					goliathCnt++;
				}
			}

			if (EMB == Toss_arbiter || EMB == Toss_arbiter_carrier || EMB == Toss_dark ||
					EMB == Toss_dark_drop)
				baseScanCount = 3;
			else if (INFO.getAllCount(Protoss_Dark_Templar, E) + INFO.getAllCount(Protoss_Arbiter, E) > 0)
				baseScanCount = 3;

			int minGoliathCnt = S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) > 0 ? 1 : 2;

			if (availableScanCount > baseScanCount && (goliathCnt >= minGoliathCnt || myAttackableUnits.size() >= 3))
				return true;
		}

		// 클로킹 레이스
		else if (myAttackableUnits.size() >= 1)
			return true;
	}

	return false;
}

bool ComsatStationManager::inArea(Position targetPosition, Position centerPosition, int range)
{
	Position tempPosition = centerPosition - targetPosition;

	if ((tempPosition.x * tempPosition.x) + (tempPosition.y * tempPosition.y) < range * range)
		return true;

	return false;
}

bool ComsatStationManager::useScan(Position targetPosition)
{
	uList scanList = INFO.getUnits(Spell_Scanner_Sweep, S);
	bool isExist = false;

	if (!targetPosition.isValid())
		return false;

	// 해당 위치에 스캔이 이미 사용되었거나, 예약되어 있다면 스캔 필요 X
	if (inTheScanArea(targetPosition) || isReservedPosition(targetPosition))
	{
		return false;
	}

	// 사용된 곳이 아니라면 추가
	if (!isExist)
	{
		reserveScanList.push_back(targetPosition);
	}

	return isExist;
}

void ComsatStationManager::updateReserveScanList()
{
	uList scanList = INFO.getUnits(Spell_Scanner_Sweep, S);

	// 기존 스캔 사용된 지역 확인
	for (auto s : scanList)
	{
		auto existPosition = find_if(reserveScanList.begin(), reserveScanList.end(), [s](Position pos) {
			return pos == s->pos();
		});

		if (existPosition != reserveScanList.end())
		{
			BWEM::utils::fast_erase(reserveScanList, distance(reserveScanList.begin(), existPosition));
		}
	}

	// 적 공격 유닛 수 카운트 (인구 기준)
	uMap enemyUnits = INFO.getUnits(E);

	for (auto &eu : enemyUnits)
	{
		if (eu.second->isHide())
			continue;

		if (!eu.second->isComplete())
			continue;

		bool needScan = false;

		// 럴커가 땅파고 있거나 레이스/고스트가 투명 상태로 전환될 때 스캔 사용하도록 체크
		if (eu.second->unit()->isDetected())
		{
			if (eu.second->type() == Zerg_Lurker)
			{
				if (!eu.second->unit()->canBurrow(false) && !eu.second->unit()->canUnburrow(false))
					needScan = true;
			}
			else if (eu.second->type().isCloakable())
			{
				//if (!eu.second->unit()->canCloak(false) && !eu.second->unit()->canDecloak(false))
				//	needScan = true;
			}
		}
		else
		{
			needScan = true;
		}

		if (needScan)
		{
			// 스캔 뿌려져 있는지 확인 및 적 유닛 근처 내 유닛이 존재하는지 확인
			if (checkScan(eu.second->unit()))
			{
				reserveScanList.push_back(eu.second->pos());
			}
		}
	}

	if (INFO.nucLaunchedTime != 0) {
		if (!bw->getNukeDots().empty()) {
			for (auto p : bw->getNukeDots()) {

				uList ghost = INFO.getTypeUnitsInRadius(Terran_Ghost, E, p, 12 * TILE_SIZE, true);

				// 만약 NukeDot 주변에 고스트가 안보이면, dot의 좌우 조금 빗겨나간 타일에 스캔을 뿌린다.
				if (ghost.empty()) {
					Position left = Position(p.x - 32 * 5, p.y);
					Position right = Position(p.x + 32 * 5, p.y);

					if (!inTheScanArea(left) && !isReservedPosition(left))
					{
						reserveScanList.push_back(left);
					}

					if (!inTheScanArea(right) && !isReservedPosition(right))
					{
						reserveScanList.push_back(right);
					}
				}
			}
		}
	}
}