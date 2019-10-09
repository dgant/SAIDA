#include "VessleManager.h"
#include "TankManager.h"
#include "VultureManager.h"
#include "GoliathManager.h"

using namespace MyBot;

VessleManager::VessleManager()
{
}

VessleManager &VessleManager::Instance()
{
	static VessleManager instance;
	return instance;
}

void VessleManager::update()
{
	if (TIME % 2 != 0)
		return;

	uList vessleList = INFO.getUnits(Terran_Science_Vessel, S);

	if (vessleList.empty())
		return;

	int num = 1;
	int defenseNum = 0;

	if (INFO.enemyRace == Races::Zerg || INFO.enemyRace == Races::Terran)
	{
		if (TIME % 24 == 0)
		{
			initTargetList();
		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		if (TIME % 480 == 0)
		{
			initTargetList();
		}
	}


	for (auto v : vessleList) {
		string state = v->getState();

		if (state == "New") {
			v->setState(new VessleBattleGuideState());
		}

		if (state == "Idle") {
			v->action();
		}

		if (state == "DefenseBase")
		{
			defenseNum++;

			if (vessleList.size() < 3)
			{
				v->setState(new VessleBattleGuideState());
				defenseNum--;
				continue;
			}

			v->action();
		}

		if (state == "BattleGuide") {
			if (v->unit()->isStasised()) continue;

			if (isBeingRepaired(v->unit())) continue;

			Unit targetUnit = choicePosition(num);
			Unit targetEnemy = choiceTarget(v);

			v->action(targetUnit, targetEnemy);

			num++;
		}
	}

	if (INFO.enemyRace == Races::Protoss && defenseNum == 0 && vessleList.size() >= 3)
	{
		UnitInfo *closestVessle = INFO.getClosestTypeUnit(S, MYBASE, Terran_Science_Vessel);

		if (closestVessle != nullptr)
		{
			closestVessle->setState(new VessleDefenseState());
			closestVessle->action();
		}
	}
}

void VessleManager::initTargetList()
{
	targetList.clear();
}

Unit VessleManager::choiceTarget(UnitInfo *v)
{
	UnitInfo *targetEnemy = nullptr;
	bool keepEnergyMode = false;

	
	if (INFO.enemyRace == Races::Zerg)
	{
		if (v->unit()->getEnergy() < 75) return nullptr;

		if (v->unit()->getSpellCooldown() == 0 && v->unit()->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Unit)
		{
			if (choiceUnit != nullptr && choiceUnit->exists() && choiceUnit->getDistance(v->unit()) < TILE_SIZE * 20)
			{
				
				return choiceUnit;
			}
		}

		if (!INFO.getUnits(Zerg_Defiler, E).empty() || !INFO.getUnits(Zerg_Queen, E).empty() || !INFO.getUnits(Zerg_Scourge, E).empty())
		{
			keepEnergyMode = true;
		}

		for (auto scourge : INFO.getTypeUnitsInRadius(Zerg_Scourge, E, v->pos(), 12 * TILE_SIZE))
		{
			if (scourge->unit()->isIrradiated()) continue;

			bool isAlreadyTarget = false;

			for (auto t : targetList.getUnits())
			{
				if (scourge == t)
				{
					isAlreadyTarget = true;
					break;
				}
			}

			if (isAlreadyTarget) continue;

			targetEnemy = scourge;
			break;
		}

		if (targetEnemy == nullptr)
		{
			for (auto defiler : INFO.getTypeUnitsInRadius(Zerg_Defiler, E, v->pos(), 30 * TILE_SIZE))
			{
				if (defiler->unit()->isIrradiated()) continue;

				bool isAlreadyTarget = false;

				for (auto t : targetList.getUnits())
				{
					if (defiler == t)
					{
						isAlreadyTarget = true;
						break;
					}
				}

				if (isAlreadyTarget) continue;

				targetEnemy = defiler;
				break;
			}
		}

		if (targetEnemy == nullptr)
		{
			for (auto queen : INFO.getTypeUnitsInRadius(Zerg_Queen, E, v->pos(), 30 * TILE_SIZE))
			{
				if (queen->unit()->isIrradiated()) continue;

				bool isAlreadyTarget = false;

				for (auto t : targetList.getUnits())
				{
					if (queen == t)
					{
						isAlreadyTarget = true;
						break;
					}
				}

				if (isAlreadyTarget) continue;

				targetEnemy = queen;
				break;
			}
		}

		if (targetEnemy == nullptr)
		{
			for (auto lurker : INFO.getTypeUnitsInRadius(Zerg_Lurker, E, v->pos(), 20 * TILE_SIZE))
			{
				if (lurker->unit()->isIrradiated() || lurker->unit()->getHitPoints() < 100) continue;

				bool isAlreadyTarget = false;

				for (auto t : targetList.getUnits())
				{
					if (lurker == t)
					{
						isAlreadyTarget = true;
						break;
					}
				}

				if (isAlreadyTarget) continue;

				if (INFO.getUnitsInRadius(E, lurker->pos(), 3 * TILE_SIZE).size() > 2 && (!keepEnergyMode || v->unit()->getEnergy() > 150))
					targetEnemy = lurker;

				break;
			}
		}

		if (targetEnemy == nullptr)
		{
			for (auto ultralisk : INFO.getTypeUnitsInRadius(Zerg_Ultralisk, E, v->pos(), 20 * TILE_SIZE))
			{
				if (ultralisk->unit()->isIrradiated()) continue;

				bool isAlreadyTarget = false;

				for (auto t : targetList.getUnits())
				{
					if (ultralisk == t)
					{
						isAlreadyTarget = true;
						break;
					}
				}

				if (isAlreadyTarget) continue;

				if (ultralisk->unit()->getHitPoints() > 100 && (!keepEnergyMode || v->unit()->getEnergy() > 150))
				{
					targetEnemy = ultralisk;
				}

				break;
			}
		}

		if (targetEnemy == nullptr)
		{
			for (auto guardian : INFO.getTypeUnitsInRadius(Zerg_Guardian, E, v->pos(), 20 * TILE_SIZE))
			{
				if (guardian->unit()->isIrradiated()) continue;

				bool isAlreadyTarget = false;

				for (auto t : targetList.getUnits())
				{
					if (guardian == t)
					{
						isAlreadyTarget = true;
						break;
					}
				}

				if (isAlreadyTarget) continue;

				if (guardian->unit()->getHitPoints() > 100 && INFO.getUnitsInRadius(E, guardian->pos(), 2 * TILE_SIZE).size() > 3 && (!keepEnergyMode || v->unit()->getEnergy() > 150))
				{
					targetEnemy = guardian;
				}
				else if (INFO.getTypeUnitsInRadius(Terran_Goliath, S, v->pos(), 12 * TILE_SIZE).empty()) 
				{
					targetEnemy = guardian;
				}

				break;
			}
		}

		if (targetEnemy == nullptr)
		{
			for (auto mutal : INFO.getTypeUnitsInRadius(Zerg_Mutalisk, E, v->pos(), 20 * TILE_SIZE))
			{
				if (mutal->unit()->isIrradiated()) continue;

				bool isAlreadyTarget = false;

				for (auto t : targetList.getUnits())
				{
					if (mutal == t)
					{
						isAlreadyTarget = true;
						break;
					}
				}

				if (isAlreadyTarget) continue;

				if (mutal->unit()->getHitPoints() > 100 && INFO.getUnitsInRadius(E, mutal->pos(), 2 * TILE_SIZE).size() > 3 && (!keepEnergyMode || v->unit()->getEnergy() > 150))
				{
					targetEnemy = mutal;
				}
				else if (INFO.getTypeUnitsInRadius(Terran_Goliath, S, v->pos(), 12 * TILE_SIZE).empty()) 
				{
					targetEnemy = mutal;
				}

				break;
			}
		}

		if (targetEnemy == nullptr)
		{
			for (auto hydra : INFO.getTypeUnitsInRadius(Zerg_Hydralisk, E, v->pos(), 20 * TILE_SIZE))
			{
				if (hydra->unit()->isIrradiated() || hydra->unit()->getHitPoints() < 70) continue;

				bool isAlreadyTarget = false;

				for (auto t : targetList.getUnits())
				{
					if (hydra == t)
					{
						isAlreadyTarget = true;
						break;
					}
				}

				if (isAlreadyTarget) continue;

				if (INFO.getUnitsInRadius(E, hydra->pos(), 2 * TILE_SIZE).size() > 4 && (!keepEnergyMode || v->unit()->getEnergy() > 150))
					targetEnemy = hydra;

				break;
			}
		}
	}
	
	else if (INFO.enemyRace == Races::Protoss)
	{
		if (v->unit()->getEnergy() < 100) return nullptr;

		if (v->unit()->getSpellCooldown() == 0 && v->unit()->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position)
		{
			if (choiceUnit != nullptr && choiceUnit->exists() && choiceUnit->getDistance(v->unit()) < TILE_SIZE * 20)
			{
				
				return choiceUnit;
			}
		}

		if (!INFO.getUnits(Protoss_Arbiter, E).empty())
		{
			keepEnergyMode = true;
		}

		int distance = INT_MAX;

		for (auto arbiter : INFO.getTypeUnitsInRadius(Protoss_Arbiter, E, v->pos(), 20 * TILE_SIZE))
		{
			bool isAlreadyTarget = false;

			if (arbiter->getEnergy() < 85) continue;

			for (auto t : targetList.getUnits())
			{
				if (arbiter == t)
				{
					isAlreadyTarget = true;
					break;
				}
			}

			if (isAlreadyTarget) continue;

			int temp = v->unit()->getDistance(arbiter->pos());

			if (temp < distance) {
				distance = temp;
				targetEnemy = arbiter;
			}
		}

		if (targetEnemy == nullptr)
		{
			for (auto highTempler : INFO.getTypeUnitsInRadius(Protoss_High_Templar, E, v->pos(), 20 * TILE_SIZE))
			{
				bool isAlreadyTarget = false;

				if (highTempler->getEnergy() < 70) continue;

				for (auto t : targetList.getUnits())
				{
					if (highTempler == t)
					{
						isAlreadyTarget = true;
						break;
					}
				}

				if (isAlreadyTarget) continue;

				if (!keepEnergyMode || v->unit()->getEnergy() > 160)
				{
					targetEnemy = highTempler;
					break;
				}
			}
		}

		if (targetEnemy == nullptr)
		{
			for (auto dragoon : INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, v->pos(), 20 * TILE_SIZE))
			{
				bool isAlreadyTarget = false;

				for (auto t : targetList.getUnits())
				{
					if (dragoon == t)
					{
						isAlreadyTarget = true;
						break;
					}
				}

				if (isAlreadyTarget) continue;

				if (INFO.getUnitsInRadius(E, dragoon->pos(), 3 * TILE_SIZE).size() > 4 && (!keepEnergyMode || v->unit()->getEnergy() > 170))
				{
					targetEnemy = dragoon;
					break;
				}
			}
		}
	}
	
	else 
	{
		if (v->unit()->getEnergy() < 100) return nullptr;

		if (v->unit()->getSpellCooldown() == 0 && v->unit()->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Unit)
		{
			if (choiceUnit != nullptr && choiceUnit->exists() && choiceUnit->getDistance(v->unit()) < TILE_SIZE * 12)
			{
				return choiceUnit;
			}
		}

		if (SM.getMainStrategy() == DrawLine || SM.getMainStrategy() == AttackAll)
		{
			Position attackPos = Positions::Unknown;

			if (SM.getMainStrategy() == DrawLine)
			{
				attackPos = SM.getDrawLinePosition();
			}
			else if (SM.getMainStrategy() == AttackAll)
			{
				attackPos = SM.getMainAttackPosition();
			}

			vector<UnitType> defensive;
			defensive.push_back(Terran_Vulture);
			defensive.push_back(Terran_Goliath);
			defensive.push_back(Terran_Siege_Tank_Tank_Mode);
			UnitInfo *closestUnit = INFO.getClosestTypeUnit(S, attackPos, defensive, 0, false, true);

			if (closestUnit != nullptr)
			{
				

				bool isAlreadyTarget = false;

				for (auto t : targetList.getUnits())
				{
					if (t == closestUnit)
					{
						isAlreadyTarget = true;
						break;
					}
				}

				if (!isAlreadyTarget && !closestUnit->unit()->isDefenseMatrixed() && closestUnit->unit()->isUnderAttack())
				{
					targetEnemy = closestUnit;
				}
			}
		}
		else if (SM.getMainStrategy() == WaitLine)
		{
			

			Position pos = SM.getWaitLinePosition();

			if (pos != Positions::Unknown)
			{
				for (auto tank : INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, pos, 15 * TILE_SIZE))
				{
					bool isAlreadyTarget = false;

					for (auto t : targetList.getUnits())
					{
						if (t == tank)
						{
							isAlreadyTarget = true;
							break;
						}
					}

					if (isAlreadyTarget) continue;

					if (tank->unit()->isUnderAttack())
					{
						targetEnemy = tank;

						break;
					}
				}
			}
		}
	}

	if (targetEnemy != nullptr)
	{
		targetList.add(targetEnemy);
		choiceUnit = targetEnemy->unit();
		return choiceUnit;
	}

	return nullptr;

}

Unit VessleManager::choicePosition(int num)
{
	UnitInfo *closestTank = TankManager::Instance().getFrontTankFromPos(SM.getMainAttackPosition());
	UnitInfo *closestGol = GoliathManager::Instance().getFrontGoliathFromPos(SM.getMainAttackPosition());
	UnitInfo *closestVul = VultureManager::Instance().getFrontVultureFromPos(SM.getMainAttackPosition());

	
	if (closestVul != nullptr)
	{
		if (INFO.getTypeUnitsInRadius(Terran_Vulture, S, closestVul->pos(), 10 * TILE_SIZE).size() < 3)
		{
			closestVul = nullptr;
		}
	}

	if (INFO.enemyRace == Races::Zerg)
	{
		if (num == 1 || num == 5)
		{
			if (closestGol != nullptr)
				return closestGol->unit();
			else if (closestVul != nullptr)
				return closestVul->unit();
			else if (closestTank != nullptr)
				return closestTank->unit();
		}
		else if (num == 2 || num == 4)
		{
			if (closestVul != nullptr)
				return closestVul->unit();
			else if (closestTank != nullptr)
				return closestTank->unit();
			else if (closestGol != nullptr)
				return closestGol->unit();
		}
		else if (num == 3)
		{
			if (closestTank != nullptr)
				return closestTank->unit();
			else if (closestGol != nullptr)
				return closestGol->unit();
			else if (closestVul != nullptr)
				return closestVul->unit();
		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		if (num == 1 || num == 3)
		{
			if (closestVul != nullptr)
				return closestVul->unit();
			else if (closestGol != nullptr)
				return closestGol->unit();
			else if (closestTank != nullptr)
				return closestTank->unit();

		}
		else if (num == 2 || num == 4)
		{
			if (closestGol != nullptr)
				return closestGol->unit();
			else if (closestTank != nullptr)
				return closestTank->unit();
			else if (closestVul != nullptr)
				return closestVul->unit();
		}
		else if (num == 5)
		{
			if (closestTank != nullptr)
				return closestTank->unit();
			else if (closestVul != nullptr)
				return closestVul->unit();
			else if (closestGol != nullptr)
				return closestGol->unit();
		}
	}
	else 
	{
		if (num == 1 || num == 3)
		{
			if (closestTank != nullptr)
				return closestTank->unit();
			else if (closestGol != nullptr)
				return closestGol->unit();
			else if (closestVul != nullptr)
				return closestVul->unit();

		}
		else if (num == 2 || num == 4)
		{
			if (closestGol != nullptr)
				return closestGol->unit();
			else if (closestTank != nullptr)
				return closestTank->unit();
			else if (closestVul != nullptr)
				return closestVul->unit();
		}
		else if (num == 5)
		{
			if (closestTank != nullptr)
				return closestTank->unit();
			else if (closestVul != nullptr)
				return closestVul->unit();
			else if (closestGol != nullptr)
				return closestGol->unit();
		}
	}

	return nullptr;
}
