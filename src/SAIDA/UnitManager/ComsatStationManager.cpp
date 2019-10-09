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

	
	availableScanCount = scanCount;
	INFO.setAvailableScanCount(scanCount);

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
	
	if (inTheScanArea(targetUnit->getPosition()) || isReservedPosition(targetUnit->getPosition()) || inDetectedArea(targetUnit))
	{
		return false;
	}

	
	uList myAttackableUnits = getMyAttackableUnitsForScan(targetUnit);

	if (myAttackableUnits.empty())
		return false;

	
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

	
	if (inTheScanArea(targetPosition) || isReservedPosition(targetPosition))
	{
		return false;
	}

	
	if (!isExist)
	{
		reserveScanList.push_back(targetPosition);
	}

	return isExist;
}

void ComsatStationManager::updateReserveScanList()
{
	uList scanList = INFO.getUnits(Spell_Scanner_Sweep, S);

	
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


	uMap enemyUnits = INFO.getUnits(E);

	for (auto &eu : enemyUnits)
	{
		if (eu.second->isHide())
			continue;

		if (!eu.second->isComplete())
			continue;

		bool needScan = false;

		
		if (eu.second->unit()->isDetected())
		{
			if (eu.second->type() == Zerg_Lurker)
			{
				if (!eu.second->unit()->canBurrow(false) && !eu.second->unit()->canUnburrow(false))
					needScan = true;
			}
			else if (eu.second->type().isCloakable())
			{
				
			}
		}
		else
		{
			needScan = true;
		}

		if (needScan)
		{
			
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