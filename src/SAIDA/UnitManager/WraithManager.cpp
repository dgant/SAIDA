#include "WraithManager.h"
#include "VultureManager.h"
#include "../InformationManager.h"

using namespace MyBot;

WraithManager::WraithManager()
{
	kitingSet.initParam();
	kitingSecondSet.initParam();
	keepMultiSet.initParam();
}

WraithManager &WraithManager::Instance()
{
	static WraithManager instance;
	return instance;
}

void WraithManager::update()
{
	if (TIME < 300 || TIME % 2 != 0)
		return;

	if (canCloak == false)
	{
		if (S->hasResearched(TechTypes::Cloaking_Field))
			canCloak = true;
	}
	uList enemyAirArmyList = INFO.getUnits(AirUnitKind, E);
	uList enemyScoutList = INFO.getUnits(Protoss_Scout, E);
	uList enemyCorsairList = INFO.getUnits(Protoss_Corsair, E);
	uList enemyMutaList = INFO.getUnits(Zerg_Mutalisk, E);
	uList wraithList = INFO.getUnits(Terran_Wraith, S);
	uList enemyWraithList = INFO.getUnits(Terran_Wraith, E);
	uList enemyValkyrie = INFO.getUnits(Terran_Valkyrie, E);

	if (wraithList.empty())
		return;

	if (SM.getMainStrategy() == Eliminate)
		return;

	if (!enemyAirArmyList.empty() && (oneTargetUnit == nullptr || !oneTargetUnit->exists()))
	{
		int dist = INT_MAX;

		for (auto e : enemyAirArmyList)
		{
			int eDist = INFO.getMainBaseLocation(S)->getPosition().getApproxDistance(e->unit()->getPosition());

			if (eDist < dist && !e->isHide() && wraithList.size() >= INFO.getTypeUnitsInRadius(AirUnitKind, E, e->pos(), TILE_SIZE * 8).size()
					&& INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, e->pos(), TILE_SIZE * 5).empty())
			{
				if (!e->unit()->isCloaked())
				{
					oneTargetUnit = e->unit();
					dist = eDist;
				}
				else if (INFO.getAvailableScanCount() >= 1)
				{
					oneTargetUnit = e->unit();
					dist = eDist;
				}
			}
		}
	}

	Position killScvPosition = getKillScvTargetBase();

	//WraithList를 가져와서 State별로 Job을 할당한다
	for (auto w : wraithList)
	{
		string state = w->getState();

		if (state == "New" && w->isComplete())
		{
			if (killScvPosition != Positions::Unknown&&INFO.getCompletedCount(Terran_Wraith,S) ==1)
			{
				w->setState(new WraithKillScvState());
				w->action(killScvPosition);
				continue;
			}
			else
			{ 
			w->setState(new WraithIdleState());
			w->action();
			continue;
			}
		}

		if (state == "Idle")
		{
			if (w->hp() < 100 || isBeingRepaired(w->unit()) /* || w->unit()->getEnergy() < 40*/) //HP가 50보다 작거나 수리중인 경우 JOB을 할당하지 않는다..
			{
				w->action();
				continue;
			}

			//흔벎唐target
			if (oneTargetUnit != nullptr)
			{
				w->setState(new WraithAttackWraithState());
				w->action(oneTargetUnit);
				continue;
			}

		
			else
			{
				w->setState(new WraithBattleAssistState());
				w->action();
			}
				
		}
		else if (state == "AttackWraith")
		{
			bool targetExist = false;

			if (oneTargetUnit != nullptr)
			{
				for (auto AirEnemy : enemyAirArmyList)
				{
					if (AirEnemy->id() == oneTargetUnit->getID())
					{
						targetExist = true;

						if (AirEnemy->isHide())
						{
							oneTargetUnit = nullptr;
						}

						break;
					}
				}
			}

			if (!targetExist)
			{
				oneTargetUnit = nullptr;
			}

			if (oneTargetUnit == nullptr)
			{
				w->setState(new WraithIdleState());
				w->action();
				continue;
			}
			else if (!INFO.getTypeBuildingsInRadius(Terran_Missile_Turret, E, oneTargetUnit->getPosition(), TILE_SIZE * 10, false).empty())
			{
				oneTargetUnit = nullptr;
				w->setState(new WraithIdleState());
				w->action();
				continue;
			}

			w->action(oneTargetUnit);
		}
		else if (state == "BattleAssist")
		{
			if (w->hp() <= 60 && canCloak)
			{
				w->setState(new  WraithCloakState());//茶��
				w->action();
			}
			else if (w->hp() <= 60 && !canCloak)
			{
				w->setState(new  WraithRetreatState());//낮藁
				w->action();
			}
			
		}

		else if (state == "KillScv")
		{
			if (killScvPosition == Positions::Unknown )
			{
				w->setState(new WraithIdleState());
				w->action();
				continue;
			}
			if ( w->hp() < 30)
			{
				w->setState(new WraithRetreatState());
				w->action();
				continue;
			}
			////벙커가 있는데 클로킹 못 쓰는 경우는 그곳으로 가지 않음..
			//if (INFO.getNearestBaseLocation(killScvPosition)->GetEnemyBunkerCount() > 0
			//		&& !canCloak && w->unit()->getEnergy() < 40)
			//{
			//	w->setState(new WraithIdleState());
			//	w->action();
			//	continue;
			//}

			w->action(killScvPosition);
		}
		else if (state == "Retreat")
		{
			
			w->action();
		}
		else if (state == "Cloak")
		{
			if (w->hp() <= 30)
			{
				w->setState(new  WraithRetreatState());//낮藁
				w->action();
			}
			else
			{
				
				w->action();
			}
		}
		else
		{
			w->action();

		}
		kitingSet.setTarget(SM.getMainAttackPosition());
		if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion)
			kitingSet.action();
		

		if (SM.getNeedAttackMulti() == false)
		{
			kitingSecondSet.clearSet();
		}
		else
		{
			kitingSecondSet.setTarget(SM.getSecondAttackPosition());
			kitingSecondSet.action();
		}
	}

	if (SM.getMainStrategy() == AttackAll)
	{
		for (auto w : wraithList)
		{
			string state = w->getState();

			if (state == "BattleAssist")
			{
				// 할일 없으면 탱크 따라다니기(AttackAll모드)
				if (!INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S).empty())
				{
					w->setState(new WraithFollowTankState());
				}
			}

			if (state == "FollowTank")
			{
				kitingSet.del(w);
				w->action();
			}
		}
	}
	else
	{
		for (auto w : wraithList)
		{
			//string state = w->getState();

			//if (state == "FollowTank")
			//{
			//	w->setState(new WraithIdleState());
			//}
			w->action();
		}
	}
}

Position WraithManager::getKillScvTargetBase()
{
	list<const Base *> &enemyOccupiedBaseLocation = INFO.getOccupiedBaseLocations(E);
	Position myFirstExpantionPosition = INFO.getFirstExpansionLocation(S)->getPosition();
	Base *enemyFirstExpantionLocation = INFO.getFirstExpansionLocation(E);
	Position targetBasePosition = Positions::Unknown;
	int closestDistance = INT_MAX;
	int tempDistance = 0;
	//bool baseExists = false;

	if (enemyOccupiedBaseLocation.empty())
		return targetBasePosition;

	// 상대 본진밖에 없거나 앞마당 체크가 안된 경우 앞마당으로 이동
	/*if (enemyOccupiedBaseLocation.size() == 1 && enemyMainBase->Starting())
	{
		cout << "building:" << enemyMainBase->GetEnemyAirDefenseBuildingCount() << " / unit:" << enemyMainBase->GetEnemyAirDefenseUnitCount() << endl;

		if (enemyMainBase->GetEnemyAirDefenseBuildingCount() || enemyMainBase->GetEnemyAirDefenseUnitCount())
		{
			targetBasePosition = INFO.getFirstExpansionLocation(E)->getPosition();
		}
		else
		{
			targetBasePosition = enemyMainBase->getPosition();
		}
	}
	else
	{*/
	// 본진/앞마당 둘 다 터렛 있는 경우 레이스 Idle 로 변경되도록 return Positions::Unknown
	for (auto enemyBase : enemyOccupiedBaseLocation)
	{
		if (enemyBase->GetEnemyAirDefenseBuildingCount() > 0) //공중방어건물이 있는 곳은 가지 않음
		{
			continue;
		}

		if (enemyBase->GetEnemyAirDefenseUnitCount() > 0/* || enemyBase->GetEnemyBunkerCount() > 0)*/ && !canCloak) //공중방어유닛이 있고, 클로킹업 안된경우 가지 않음
		{
			continue;
		}

		if (enemyBase->GetWorkerCount() == 0) //Worker가 한 마리도 없는 경우 가지 않음
		{
			continue;
		}

		tempDistance = myFirstExpantionPosition.getApproxDistance(enemyBase->getPosition());

		if (tempDistance < closestDistance)
		{
			targetBasePosition = enemyBase->getPosition();
			closestDistance = tempDistance;
		}
	}

	return targetBasePosition;
}

void WraithManager::onUnitDestroyed(Unit u)
{
	if (!u->isCompleted() || u == nullptr)
		return;

	UnitInfo *Wraith = INFO.getUnitInfo(u, S);

	if (Wraith == nullptr)
		return;

	kitingSet.del(u);
	kitingSecondSet.del(u);
	keepMultiSet.del(u);

}

void WraithKiting::action()
{
	if (size() == 0)	return;
	
	//waitingTime놓迦뺏
	if (waitingTime != 0 && waitingTime + (24 * 10) < TIME)
	{
		waitingTime = 0;
		needWaiting = 0;
		needCheck = true;
	}

	Base *enFirstExp = INFO.getFirstExpansionLocation(E);

	if (enFirstExp && target == enFirstExp->Center())
		target = INFO.getMainBaseLocation(E)->Center();

	UnitInfo *frontUnit = getFrontUnitFromPosition(target);

	if (frontUnit == nullptr) return;

	Position backPos = (INFO.getSecondChokePosition(S) + INFO.getFirstExpansionLocation(S)->getPosition()) / 2;

	UnitInfo *frontTank = TM.getFrontTankFromPos(target);
//======================================================================
	if (needFight)
	{
		if (needBack(frontUnit->pos()))	needFight = false;
	}
	else
	{
		if (canFight(frontUnit->pos()))	needFight = true;
	}
//======================================================================
	for (auto v : getUnits())
	{
	
		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(v->pos(), &dangerPoint, true);//뚤黨령契膠角true

		if (dangerUnit == nullptr)//청唐濾뚤黨왕櫓데貫돨dangerunit珂（檢？
		{
			UnitInfo *buildingTower = INFO.getClosestTypeUnit(E, v->pos(), INFO.getAdvancedDefenseBuildingType(INFO.enemyRace), 10* TILE_SIZE);
			if (buildingTower != nullptr && enFirstExp && isSameArea(buildingTower->pos(), enFirstExp->Center())) 
			{
				CommandUtil::move(v->unit(), target);
				continue;
			}

			UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 15 * TILE_SIZE);

			if (closestWorker != nullptr)
				kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
			else//청唐dangerunit，청唐closesetWorker돨珂빅
			{

				if (INFO.enemyRace == Races::Zerg) 
				{
					UnitInfo *Overlord = INFO.getClosestTypeUnit(E, v->pos(), Zerg_Overlord, 8 * TILE_SIZE);

					if (Overlord)
					{
						CommandUtil::attackUnit(v->unit(), Overlord->unit());
						continue;
					}
				}
				if (INFO.enemyRace == Races::Protoss)
				{
					UnitInfo *zealot = INFO.getClosestTypeUnit(E, v->pos(), Protoss_Zealot, 8 * TILE_SIZE);

					if (zealot)
					{
						CommandUtil::attackUnit(v->unit(), zealot->unit());
						continue;
					}
				}
				if (INFO.enemyRace == Races::Terran)
				{
					UnitInfo *Tank = INFO.getClosestTypeUnit(E, v->pos(), Terran_Siege_Tank_Siege_Mode, 8 * TILE_SIZE);

					if (Tank)
					{
						CommandUtil::attackUnit(v->unit(), Tank->unit());
						continue;
					}
				}
				if (v->pos().getApproxDistance(target) > 12 * TILE_SIZE) // 꼇콘약잼커깃법陶
					CommandUtil::move(v->unit(), target);

				else if (TIME < (24 * 60 * 10) && (EIB == Toss_1g_forward || EIB == Toss_2g_forward)) // 본진 파일런이 아닌 경우에 파일런은 깬다.
				{
					UnitInfo *pylon = INFO.getClosestTypeUnit(E, target, Protoss_Pylon, 5 * TILE_SIZE);

					if (pylon) {
						CommandUtil::attackUnit(v->unit(), pylon->unit());
					}
				}
				else
				{
					if (INFO.enemyRace == Races::Zerg)
					{
						UnitInfo *larva = INFO.getClosestTypeUnit(E, v->pos(), Zerg_Larva, 8 * TILE_SIZE);

						if (larva) {
							CommandUtil::attackUnit(v->unit(), larva->unit());
							continue;
						}

						UnitInfo *egg = INFO.getClosestTypeUnit(E, v->pos(), Zerg_Egg, 8 * TILE_SIZE);

						if (egg) {
							CommandUtil::attackUnit(v->unit(), egg->unit());
							continue;
						}
					}
				}
			}
		} 
		else //唐dangerUnit돨헙워
		{

			if (isNeedKitingUnitTypeinAir(dangerUnit->type()))//흔벎角옵鹿kiting돨데貫
			{
				UnitInfo *closestAttack = INFO.getClosestUnit(E, v->pos(), AirUnitKind, 10 * TILE_SIZE, true, false, true);

				if (closestAttack == nullptr) // 보이질 않아
				{
					UnitInfo *closestGroundForce = INFO.getClosestUnit(E, v->pos(), GroundCombatKind, 10 * TILE_SIZE, true, false, true);
					if (closestAttack == nullptr) // 보이질 않아
					{
						CommandUtil::move(v->unit(), target);
						continue;
					}
					else
					kiting(v, closestGroundForce, dangerPoint, 3 * TILE_SIZE);
				}
				else // Kiting
				{
						kiting(v, closestAttack, dangerPoint, 3 * TILE_SIZE);
				}
			}
			else // 壇맡角檢돨헙워
			{
				// 싸워야 하는 경우 질저 아니면 그냥 일점사
				if (needFight)
				{
					UnitInfo *closestAttack = INFO.getClosestUnit(E, frontUnit->pos(), AirDefenseBuildingKind, 15 * TILE_SIZE, false, false, true);

					if (closestAttack == nullptr) // 보이질 않아
					{
						UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

						if (closestWorker != nullptr)
							kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
						else if (v->pos().getApproxDistance(target) > 5 * TILE_SIZE) // 목적지 이동
							CommandUtil::move(v->unit(), target);
					}
					else // Kiting
					{
						UnitInfo *weakUnit = getGroundWeakTargetInRange(v);

						if (weakUnit != nullptr) // 공격 대상 설정 변경
							closestAttack = weakUnit;

						////////////////////// Bunker는 수리 SCV 체크해야 함.
						if (closestAttack->type() == Terran_Bunker)
						{
							UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, closestAttack->pos(), INFO.getWorkerType(INFO.enemyRace), 2 * TILE_SIZE);

							if (closestWorker != nullptr)
								CommandUtil::attackUnit(v->unit(), closestWorker->unit());

							continue;
						}

						///////////////////////////////////////////
						CommandUtil::attackUnit(v->unit(), closestAttack->unit());

						if (v->posChange(closestAttack) == PosChange::Farther)
							v->unit()->move((v->pos() + closestAttack->vPos()) / 2);
					}

					continue;
				}// need Fight 종료
				else // 렷Attack All
				{
					if (needWaiting)
					{
						if (!dangerUnit->isHide())
							waitingTime = TIME;


						int backThreshold = 4;


						if (dangerUnit->type() ==INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
							backThreshold = 3;


						if (dangerUnit->type() == Zerg_Lurker)
							backThreshold = 2;


						if (dangerPoint > backThreshold * TILE_SIZE)
						{

							if (size() <= 1)
							{
								if (v == frontUnit)
									CommandUtil::hold(v->unit());
								else if (v->pos().getApproxDistance(frontUnit->pos()) < 3 * TILE_SIZE)
									CommandUtil::move(v->unit(), frontUnit->pos());
							}
						}
						else // 위험한 상태
						{
							// 러커는 안나오는데..
							UnitInfo *closestUnit = INFO.getClosestUnit(E, frontUnit->pos(), GroundCombatKind, 15 * TILE_SIZE, false, false, true);

							if (closestUnit == nullptr)
								continue;

							UnitInfo *closestTank = INFO.getClosestTypeUnit(S, v->pos(), Terran_Siege_Tank_Tank_Mode);

							if (closestTank == nullptr)
							{
								if (v->pos().getApproxDistance(backPos) < 2 * TILE_SIZE)
									v->unit()->holdPosition();
								else if (v->pos().getApproxDistance(backPos) > 4 * TILE_SIZE)
									CommandUtil::move(v->unit(), backPos);
							}
							else
							{
								if (closestTank->pos().getApproxDistance(v->pos()) < 2 * TILE_SIZE)
									CommandUtil::attackMove(v->unit(), target);
								else if (closestTank->pos().getApproxDistance(v->pos()) > 4 * TILE_SIZE)
									CommandUtil::move(v->unit(), closestTank->pos());
							}
						}
					} // 대기모드 종료
					else // Waiting 아님
					{
						// dangerUnit이 Tower일때
						if (dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
						{
							if (enFirstExp && isSameArea(dangerUnit->pos(), enFirstExp->Center()))
							{
								needWaiting = 1;
								waitingTime = TIME;
							}
							else //Tower
							{
								UnitInfo *closestAttack = INFO.getClosestUnit(E, v->pos(), AirDefenseBuildingKind, 10 * TILE_SIZE, false, false, true);

								if (closestAttack == nullptr)
								{
									UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

									if (closestWorker != nullptr)
									{
										kiting(v, closestWorker, dangerPoint, 3 * TILE_SIZE);
									}
									else
									{
										//int direct = v->getDirection();

										if (goWithoutDamage(v->unit(), target, direction, 4 * TILE_SIZE) == false)
										{
											if (direction == 1) direction *= -1;
											else
											{
												needWaiting = 1;
												waitingTime = TIME;
											}
										}
									}
								}
								else // closest Attacker 존재
								{
									if (isNeedKitingUnitType(closestAttack->type()))
									{
										kiting(v, closestAttack, dangerPoint, 3 * TILE_SIZE);
									}
								}
							}
						}
						else // 타워가 아닌경우
						{
							if (needCheck == true && dangerUnit->isHide())
							{
								CommandUtil::move(v->unit(), target);
								continue;
							}

							needWaiting = 2;
							waitingTime = TIME;
						}
					} //// Waiting 아님
				} //Attack All 아님
			}//질저 아님
		}// Danger 없음.
	}

	return;
}

bool WraithKiting::canFight(Position frontPos)
{
	// 전진 게이트에 대한 특별 처리 - 마린이 있으면 그냥 싸우는 로직 필요

	int range = 6 * TILE_SIZE;

	if (size() > 5)
		range += 3 * TILE_SIZE;


	word WraithCnt = INFO.getTypeUnitsInRadius(Terran_Wraith, S, frontPos, range).size();
	if (WraithCnt < 2)
		return false;

	if (INFO.enemyRace == Races::Protoss)
	{
		word dragoon = INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, frontPos, 15 * TILE_SIZE, true).size();
		word zealot = INFO.getTypeUnitsInRadius(Protoss_Zealot, E, frontPos, 15 * TILE_SIZE, true).size();
		word photo = INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, frontPos, 15 * TILE_SIZE, true).size();

		word airCombat = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, frontPos, 15 * TILE_SIZE, true).size();
		airCombat += INFO.getTypeUnitsInRadius(Protoss_Scout, E, frontPos, 15 * TILE_SIZE, true).size();

		preAmountOfEnemy = dragoon*3;

		if ((dragoon )  <= WraithCnt)
			return true;
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		word hydra = INFO.getTypeUnitsInRadius(Zerg_Hydralisk, E, frontPos, 15 * TILE_SIZE, true).size();
		word zergling = INFO.getTypeUnitsInRadius(Zerg_Zergling, E, frontPos, 15 * TILE_SIZE, true).size();
		word sunken = INFO.getTypeBuildingsInRadius(Zerg_Sunken_Colony, E, frontPos, 15 * TILE_SIZE, true).size();
		word mutal = INFO.getTypeUnitsInRadius(Zerg_Mutalisk, E, frontPos, 15 * TILE_SIZE, true).size();
		uList luckers = INFO.getTypeUnitsInRadius(Zerg_Lurker, E, frontPos, 15 * TILE_SIZE, true);
		word lucker = 0;

		if (hydra <= WraithCnt )
			return true;
	}
	else
	{
		word vulture = INFO.getTypeUnitsInRadius(Terran_Vulture, E, frontPos, 15 * TILE_SIZE, true).size();
		word tank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, frontPos, 20 * TILE_SIZE, true).size();
		word goliath = INFO.getTypeUnitsInRadius(Terran_Goliath, E, frontPos, 15 * TILE_SIZE, true).size();
		word marine = INFO.getTypeUnitsInRadius(Terran_Marine, E, frontPos, 15 * TILE_SIZE, true).size();
		uList bunkers = INFO.getTypeBuildingsInRadius(Terran_Bunker, E, frontPos, 15 * TILE_SIZE, true);
		word bunker = 0;

		for (auto b : bunkers)
		if (b->getMarinesInBunker())
			bunker++;

		if ((goliath * 2) + (marine * 1) + (bunker * 4)<= WraithCnt)
			return true;
	}

	return false;
}

bool WraithKiting::needBack(Position frontPos)
{
	int range = 6 * TILE_SIZE;

	if (size() > 5)
		range += 3 * TILE_SIZE;


	word WraithCnt = INFO.getTypeUnitsInRadius(Terran_Wraith, S, frontPos, range).size();
	if (INFO.enemyRace == Races::Protoss)
	{
		word dragoon = INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, frontPos, 15 * TILE_SIZE, true).size();
		word zealot = INFO.getTypeUnitsInRadius(Protoss_Zealot, E, frontPos, 15 * TILE_SIZE, true).size();
		word photo = INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, frontPos, 15 * TILE_SIZE, true).size();

		word airCombat = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, frontPos, 15 * TILE_SIZE, true).size();
		airCombat += INFO.getTypeUnitsInRadius(Protoss_Scout, E, frontPos, 15 * TILE_SIZE, true).size();

		if ((dragoon )  > WraithCnt)
			return true;
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		word hydra = INFO.getTypeUnitsInRadius(Zerg_Hydralisk, E, frontPos, 15 * TILE_SIZE, true).size();
		word zergling = INFO.getTypeUnitsInRadius(Zerg_Zergling, E, frontPos, 15 * TILE_SIZE, true).size();
		word sunken = INFO.getTypeBuildingsInRadius(Zerg_Sunken_Colony, E, frontPos, 15 * TILE_SIZE, true).size();
		word mutal = INFO.getTypeUnitsInRadius(Zerg_Mutalisk, E, frontPos, 15 * TILE_SIZE, true).size();
		uList luckers = INFO.getTypeUnitsInRadius(Zerg_Lurker, E, frontPos, 15 * TILE_SIZE, true);
		word lucker = 0;



		if (hydra  > WraithCnt)
			return true;
	}
	else
	{
		word vulture = INFO.getTypeUnitsInRadius(Terran_Vulture, E, frontPos, 15 * TILE_SIZE, true).size();
		word tank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, frontPos, 20 * TILE_SIZE, true).size();
		word goliath = INFO.getTypeUnitsInRadius(Terran_Goliath, E, frontPos, 15 * TILE_SIZE, true).size();
		word marine = INFO.getTypeUnitsInRadius(Terran_Marine, E, frontPos, 15 * TILE_SIZE, true).size();
		uList bunkers = INFO.getTypeBuildingsInRadius(Terran_Bunker, E, frontPos, 15 * TILE_SIZE, true);
		word bunker = 0;

		for (auto b : bunkers)
		if (b->getMarinesInBunker())
			bunker++;

		if ((marine * 1 ) + (goliath * 2) +(bunker * 4) > WraithCnt)
			return true;
	}

	return false;
}

void WraithKiting::clearSet() {
	if (!size())	return;

	for (auto v : getUnits()) {
		v->setState(new WraithIdleState());
	}

	clear();
}

