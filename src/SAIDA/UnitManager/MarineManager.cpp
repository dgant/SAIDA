#include "MarineManager.h"
#include "TankManager.h"
#include "../InformationManager.h"
#include "../StrategyManager.h"
#include "../EnemyStrategyManager.h"

using namespace MyBot;

MarineManager::MarineManager()
{
	
	//==================
	kitingSet.initParam();
	kitingSecondSet.initParam();
	keepMultiSet.initParam();
	//==================
	getFirstDefensePos();
	firstBarrack = nullptr;     //µÚÒ»¸ö±øÓª
	nextBarrackSupply = nullptr;
	bunker = nullptr;             //µØ±¤
	zealotDefenceMarine = nullptr;//·ÀÊØzealotµÄÇ¹±ø
	rangeUnitNearBunker = false;
	//ÎÒÉè¶¨µÄÇ¹±øµÈ´ýµØÖ·
	waitnearsecondchokepoint = INFO.getSecondChokePosition(S, ChokePoint::end1);
	Position pos = (Position)INFO.getSecondAverageChokePosition(S);

	uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

	if (!commandCenterList.empty()) 
	{
		Unit mineral = bw->getClosestUnit(commandCenterList[0]->pos(), Filter::IsMineralField);

		if (mineral != nullptr)
			pos = (commandCenterList[0]->pos() + mineral->getPosition()) / 2;
	}

	waitingNearCommand = pos;//Ç¹±ø³õÊ¼µÈ´ýµØÖ·£¨¼ÐÔÚÅ©ÃñÖ®¼ä£©
}

MarineManager &MarineManager::Instance()
{
	static MarineManager instance;
	return instance;
}
//=======================================================================
void MarineManager::update()
{
	
	
	if (TIME % 2 != 0) return;

	uList marineList = INFO.getUnits(Terran_Marine, S);

	if (marineList.empty()) 
		return;

	setFirstBunker();


	int kitingnum = 0;
	int divenum = 0;
	int idlenum =0;
	//===========================¼ÆËã¼ÒÀïÊÇ·ñÓÐµÐÈË====================
	 enemyWorkersInMyYard = getEnemyInMyYard(1700, INFO.getWorkerType(INFO.enemyRace));
	enemyBuildingsInMyYard = getEnemyInMyYard(1700, UnitTypes::Buildings);
	 enemyInMyYard = getEnemyInMyYard(1700, Men, false);
	int totEnemNum = enemyInMyYard.size() + enemyBuildingsInMyYard.size();
	int totEnemBNum = enemyBuildingsInMyYard.size();
	//int zealotcnt = INFO.getTypeUnitsInArea(Protoss_Zealot, E, ENBASE).size();
	Unit bunker = MM.getBunker();
	int zealotcnt = INFO.getTypeUnitsInRadius(Protoss_Zealot, E, ENBASE, 30 * TILE_SIZE, true).size();
	int MarineArmycnt = INFO.getTypeUnitsInRadius(Terran_Marine, S, theMap.Center(), 2 * TILE_SIZE).size();
	//=============================·ÇÍêÈ«¹¥»÷×´Ì¬================================
	if (SM.getMainStrategy() != AttackAll)
	{

		if (SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller)
		{
			for (auto &bunker : INFO.getBuildings(Terran_Bunker, S))
			{ 
			if (INFO.getCompletedCount(Terran_Bunker, S) == 2)
			{
				if (isSameArea(bunker->pos(), INFO.getMainBaseLocation(S)->getPosition()))
				{
					if(!bunker->unit()->isUnderAttack() && bunker->unit()->getLoadedUnits().size() > 0)
						bunker->unit()->unloadAll();
				}
			}
			}
		}

		//====================Ç¹±ø×´Ì¬ÉèÖÃ¿ªÆô == == == == == == == ==
		for (auto v : marineList)
		{

			if (isStuckOrUnderSpell(v))
				continue;



			string state = v->getState();
			Position MarinePos = v->pos();

			if (state == "Kiting")
				kitingnum++;

			if (state == "New" && v->isComplete())
			{
				v->setState(new MarineIdleState());
			}

			if (state == "Idle")
			{


				if (bunker != NULL && (bunker->getLoadedUnits().size() != 4) && bunker->isCompleted())
				{
					v->setState(new MarineDefenceState());
					v->action();
				}
				
				else
				{
					v->setState(new MarineKitingState());
					kitingSet.add(v);
					v->action();

				}

			}

			else if (state == "Kiting")
			{

				kitingSet.add(v);
				if (INFO.enemyRace == Races::Terran)
				{
					if (bunker != NULL && (bunker->getLoadedUnits().size() < 4) && bunker->isCompleted())
					{
						if (INFO.getSecondChokePosition(S).getApproxDistance(v->pos()) < 10 * TILE_SIZE)
						{
							v->setState(new MarineDefenceState());
							kitingSet.del(v);
							v->action();
						}
					}
				}
				if (INFO.enemyRace == Races::Protoss && SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller)
				{
					if (bunker != NULL && (bunker->getLoadedUnits().size() < 4) && bunker->isCompleted())
					{
						if (INFO.getSecondChokePosition(E).getApproxDistance(v->pos()) < 10 * TILE_SIZE)
						{
							v->setState(new MarineDefenceState());
							kitingSet.del(v);
							v->action();
						}
					}
				}
				else if (INFO.enemyRace == Races::Protoss && 
					(SM.getMyBuild() == MyBuildTypes::Protoss_CarrierKiller || SM.getMyBuild() == MyBuildTypes::Protoss_DragoonKiller || SM.getMyBuild() == MyBuildTypes::Protoss_CannonKiller || SM.getMyBuild() == MyBuildTypes::Protoss_TemplarKiller))
				{
					if (bunker != NULL && (bunker->getLoadedUnits().size() < 4) && bunker->isCompleted())
					{
						
							v->setState(new MarineDefenceState());
							kitingSet.del(v);
							v->action();
						
					}
				}
				else if (INFO.enemyRace == Races::Protoss && SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller)
				{
					v->setState(new MarineDefenceState());
					kitingSet.del(v);
					v->action();
				}

				Base *firstExpension = INFO.getFirstExpansionLocation(E);
				if (firstExpension != nullptr && firstExpension->GetOccupiedInfo() == enemyBase &&
					isSameArea(v->pos(), INFO.getMainBaseLocation(E)->getPosition()))
				{
					v->setState(new MarineDiveState());
					kitingSet.del(v);
					v->action();
				}
			}
			else if (state == "Dive")
			{
				kitingSet.del(v);
				v->action();
			}
			else if (state == "KillD")
			{
				kitingSet.del(v);
				if (INFO.getDestroyedCount(Protoss_Dark_Templar,E)>=4)
					v->setState(new MarineKitingState());
				else
					v->action();

			}
			else if (state == "KillScouter")
			{
				kitingSet.del(v);
				if (enemyWorkersInMyYard.size() == 0)
					v->setState(new MarineIdleState());
				else
					v->action();

			}
			else if (state == "GoGoGo")
			{
				kitingSet.del(v);
				v->action();
			}
			else if (state == "Defence")
			{

				
				if ((bunker != NULL && (bunker->getLoadedUnits().size() == 4) && bunker->isCompleted()) && totEnemNum == 0 && SM.getMyBuild() != MyBuildTypes::Protoss_MineKiller&& SM.getMyBuild() != MyBuildTypes::Protoss_ZealotKiller)
				{
		
						v->setState(new MarineKitingState());
					
				}
				else if ((bunker != NULL && (bunker->getLoadedUnits().size() == 4) && bunker->isCompleted()) && SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller)
				{

					v->setState(new MarineKitingState());
				
				}

				else
				{
	             kitingSet.del(v);
				  v->action();
                 }
				

			}


			else v->action();
		}

	
	}
	if (SM.getMainStrategy() == AttackAll)
	{
		if (SM.getMyBuild() != MyBuildTypes::Protoss_MineKiller)
		{ 
		for (auto m : marineList)
		{
			if (m->getState() != "Kiting" )
			{
				m->setState(new MarineKitingState());
				kitingSet.add(m);
				m->action();
			}


		}
		
		for (auto &bunker : INFO.getBuildings(Terran_Bunker, S))
		{
			if (!bunker->unit()->isUnderAttack() && bunker->unit()->getLoadedUnits().size() > 0)
			{
				bunker->unit()->unloadAll();
			}
		}
		}
		if (SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller)
		{
			
			if (INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Siege_Mode, S, INFO.getFirstExpansionLocation(E)->Center(), 6 * TILE_SIZE).size() + INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, INFO.getFirstExpansionLocation(E)->Center(), 6 * TILE_SIZE).size() >= 2)
			{
				for (auto &bunker : INFO.getBuildings(Terran_Bunker, S))
				{
					if (!bunker->unit()->isUnderAttack() && bunker->unit()->getLoadedUnits().size() >= 1)
					{
						bunker->unit()->unloadAll();
					}
				}
			}

			for (auto m : marineList)
			{
				if (m->getState() != "Kiting"&& m->getState() != "Defence")
				{
					m->setState(new MarineKitingState());
					kitingSet.add(m);
					m->action();
				}
				else if (m->getState() == "Kiting")
				{

					kitingSet.add(m);
					m->action();
				}
				else if (m->getState() == "Defence")
				{
					if ( (INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Siege_Mode, S, INFO.getFirstExpansionLocation(E)->Center(), 6 * TILE_SIZE).size() + INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, INFO.getFirstExpansionLocation(E)->Center(), 6 * TILE_SIZE).size() >= 2))
					{
						m->setState(new MarineKitingState());
						kitingSet.add(m);
						m->action();
					}
					else
						m->action();
				}

			}
			
		
		}



	}
		
			kitingSet.setTarget(SM.getMainAttackPosition());

		


		if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion || SM.getMainStrategy() == AttackAll)
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

		if (needKeepMulti())
		{
			if (SM.getNeedKeepSecondExpansion(Terran_Vulture))
				keepMultiSet.setTarget(INFO.getSecondExpansionLocation(S)->Center());
			else
				keepMultiSet.setTarget(INFO.getThirdExpansionLocation(S)->Center());

			keepMultiSet.action();
		}
		else
		{
			keepMultiSet.clearSet();
		}

		return;
	

		





	
}
bool MarineManager::findFirstBarrackAndSupplyPosition()
{
	nextBarrackSupply = nullptr;

	for (auto b : INFO.getBuildings(Terran_Barracks, S)) {
		if (!b->unit()->isLifted()) {

			TilePosition barrackTile = b->unit()->getTilePosition();

			for (auto &u : Broodwar->getUnitsOnTile(TilePosition(barrackTile.x + 4, barrackTile.y + 1))) {
				if (u->getType() == Terran_Supply_Depot) {
					nextBarrackSupply = u;
					break;
				}
			}

		}

		firstBarrack = b->unit();
	}

	if (firstBarrack == nullptr || nextBarrackSupply == nullptr)
		return false;

	return true;
}

void MarineManager::onUnitDestroyed(Unit u)
{
	if (!u->isCompleted() || u == nullptr)
		return;

	UnitInfo *marine = INFO.getUnitInfo(u, S);

	if (marine == nullptr)
		return;

	if (marine->getState() == "ZealotDefence") {
		zealotDefenceMarine = nullptr;
	}


	kitingSet.del(u);
	kitingSecondSet.del(u);
	keepMultiSet.del(u);

}

void MarineManager::getFirstDefensePos()
{
	WalkPosition cpEnd1 = theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition())->ChokePoints().at(0)->Pos(BWEM::ChokePoint::end1);
	WalkPosition cpMiddle = theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition())->ChokePoints().at(0)->Pos(BWEM::ChokePoint::middle);
	Position mybase = INFO.getMainBaseLocation(S)->getPosition();

	Position cpEnd1Pos = Position(cpEnd1);
	Position cpMiddlePos = Position(cpMiddle);

	int x = cpEnd1Pos.x - cpMiddlePos.x;
	int y = cpEnd1Pos.y - cpMiddlePos.y;

	double m = (double)y / (double)x;
	double nm = -1 * (1 / m); // y = nm*x

	Position pos1(cpMiddlePos.x - 100, cpMiddlePos.y - (int)(nm * 100));
	Position pos2(cpMiddlePos.x + 100, cpMiddlePos.y + (int)(nm * 100));

	int defence_gap = 15;

	if (pos1.getDistance(mybase) < pos2.getDistance(mybase)) // pos 1 ÂÊÀÌ º»ÁøÀÌ¸é
		FirstDefensePos = Position(cpMiddlePos.x + defence_gap, cpMiddlePos.y + (int)(nm * defence_gap));
	else
		FirstDefensePos = Position(cpMiddlePos.x - defence_gap, cpMiddlePos.y - (int)(nm * defence_gap));
}

void MarineManager::checkRangeUnitNearBunker()
{
	rangeUnitNearBunker = false;

	if (bunker == nullptr) 
	{
		return;
	}

	uList units = INFO.getUnitsInRadius(E, bunker->getPosition(), 24 * TILE_SIZE);

	for (auto u : units) 
	{

		if (u->getLift()) 
		{
			rangeUnitNearBunker = true;
			return;
		}
		else if (Terran_Marine.groundWeapon().maxRange() <= u->type().groundWeapon().maxRange()
				 || u->type() == Zerg_Lurker) 
		{
			rangeUnitNearBunker = true;
			return;
		}
	}

	return;
}

Position MarineManager::checkForwardPylon()
{
	// ÀüÁø °Ç¹° Ã£´Â µ¿ÀÛ ¸¶¹«¸® µÊ Done
	if (forwardBuildingPosition == Positions::None)
	{
		return ATTACKPOS;
	}

	// ÀüÁø ÆÄÀÏ·± Ã£¾Æ¼­ ºÎ½¤Áø °æ¿ì
	if (checkedForwardPylon)
	{
		// ÆÄÀÏ·±ÀÌ ¾ø´Â °æ¿ì
		if (INFO.getClosestTypeUnit(E, forwardBuildingPosition, Protoss_Pylon, 10 * TILE_SIZE, true) == nullptr)
		{
			// °ÔÀÌÆ®µµ ¾ø´Â °æ¿ì »óÈ²Á¾·á
			if (INFO.getClosestTypeUnit(E, forwardBuildingPosition, Protoss_Gateway, 10 * TILE_SIZE, true) == nullptr)
				forwardBuildingPosition = Positions::None;

			return ENBASE;
		}

		return forwardBuildingPosition;
	}

	if (forwardBuildingPosition == Positions::Unknown)// ¸ðµç °Ç¹°À» Ã£¾Æ¾ßÇÔ.
	{
		for (auto &g : INFO.getBuildings(E))
		{
			// ¸Ê Áß¾Óº¸´Ù °¡±î¿î °Ç¹° // ÆÄÀÏ·±À» ¸øº¸°í °ÔÀÌÆ®¸¸ ºÃÀ» ¶§µµ ÀÖÀ» ¼ö ÀÖÀ¸¹Ç·Î ±×¸®°í ¾îÂ÷ÇÇ °Å±â¿¡ ÆÄÀÏ·±ÀÌ ÀÖÀ» Å×´Ï
			if (INFO.getSecondChokePosition(S).getApproxDistance(g.second->pos()) < INFO.getSecondChokePosition(S).getApproxDistance(theMap.Center()) + 10 * TILE_SIZE)
			{
				forwardBuildingPosition = g.second->pos();
				return forwardBuildingPosition;
			}
			else // ±× ¹Û¿¡¼­ ºÃÀ¸¸é ±×³É »ý±î´Â Gate´Ù.
			{
				if (g.second->type() == Protoss_Gateway)
				{
					forwardBuildingPosition == Positions::None;
					return ENBASE;
				}
			}
		}

		Position basePos = Positions::Unknown;

		int rank = 5;

		for (Base *b : INFO.getBaseLocations())
		{
			if (b->GetOccupiedInfo() == myBase)
				continue;

			if (b->GetExpectedMyMultiRank() > rank)
				continue;

			if (b->GetLastVisitedTime() > 0)
				continue;

			if (INFO.getSecondChokePosition(S).getApproxDistance(b->Center()) > INFO.getSecondChokePosition(S).getApproxDistance(theMap.Center()) + 10 * TILE_SIZE)
				continue;

			rank = b->GetExpectedMyMultiRank();
			basePos = b->Center();
		}

		if (basePos == Positions::Unknown)
		{
			if (bw->isExplored((TilePosition)theMap.Center()))
				return ENBASE;
			else
				return theMap.Center();
		}
		else
			return basePos;
	}
	else
	{
		if (checkedForwardPylon == false) { // ÀüÁø °Ç¹°Àº Ã£À½
			UnitInfo *forwardPylon = INFO.getClosestTypeUnit(E, forwardBuildingPosition, Protoss_Pylon, 10 * TILE_SIZE, true);

			if (forwardPylon != nullptr) {
				forwardBuildingPosition = forwardPylon->pos();
				checkedForwardPylon = true;
			}
		}

		return forwardBuildingPosition;
	}
}

UnitInfo *MarineManager::getFrontMarineFromPos(Position pos)
{
	if (kitingSet.size() == 0)
		return nullptr;

	return kitingSet.getFrontUnitFromPosition(pos);
}

bool MarineManager::moveAsideForCarrierDefence(UnitInfo *v) {

	if (INFO.enemyRace == Races::Protoss) {
		
		if (isSameArea(v->pos(), MYBASE)) {
			
			if (!INFO.getTypeUnitsInArea(Protoss_Carrier, E, MYBASE).empty()) {
				
				int carrierDefenceCntNearFirstChoke = 0;

				for (auto g : INFO.getUnits(Terran_Goliath, S)) {
					if (g->getState() == "CarrierDefence")
					if (g->pos().getApproxDistance(INFO.getFirstChokePosition(S)) <= 10 * TILE_SIZE)
						carrierDefenceCntNearFirstChoke++;
				}

				// º»ÁøÀ¸·Î ¿òÁ÷¿© ÁØ´Ù.
				if (carrierDefenceCntNearFirstChoke >= 10) {
					CommandUtil::attackMove(v->unit(), MYBASE);
					return true;
				}
			}
		}
	}

	return false;
}

bool MarineManager::needKeepMulti()
{
	if (SM.getNeedKeepSecondExpansion(Terran_Marine))
	{
		if (!SM.getNeedKeepSecondExpansion(Terran_Siege_Tank_Tank_Mode))
		{
			uList vset = keepMultiSet.getUnits();

			for (auto v : vset)
			{

					v->setState(new MarineKitingState());
					kitingSet.add(v);
					keepMultiSet.del(v);
				
			}
		}

		return true;
	}

	if (SM.getNeedKeepThirdExpansion(Terran_Marine))
	{
		if (!SM.getNeedKeepThirdExpansion(Terran_Siege_Tank_Tank_Mode))
		{
			// ¸¶ÀÎ¸¸ ÇÊ¿äÇÑ »óÅÂ
			uList vset = keepMultiSet.getUnits();

			for (auto v : vset)
			{
			
					v->setState(new MarineKitingState());
					kitingSet.add(v);
					keepMultiSet.del(v);
				
			}
		}

		return true;
	}

	return false;
}

void MarineManager::checkDiveMarine()
{
	
	UnitInfo *frontMarine = kitingSet.getFrontUnitFromPosition(INFO.getMainBaseLocation(E)->Center());

	if (frontMarine == nullptr)
		return;

	uList diveMarines = INFO.getTypeUnitsInRadius(Terran_Marine, S, frontMarine->pos(), 5 * TILE_SIZE);

	int MarineCnt = 0;

	for (auto v : diveMarines)
	{
		// hp 60% +
		if (v->hp() > (int)(v->type().maxHitPoints() * 0.6))
			MarineCnt++;
	}

	
	Base *firstExpension = INFO.getFirstExpansionLocation(E);

	if (firstExpension == nullptr || firstExpension->GetOccupiedInfo() != enemyBase)
		return;

	int firstExpensionTower = firstExpension->GetEnemyGroundDefenseBuildingCount();

	if (INFO.enemyRace == Races::Terran)
		firstExpensionTower = firstExpension->GetEnemyBunkerCount();

	
	if (firstExpensionTower == 0)
		return;

	bool needDive = false;
	int rangeUnits = 0;

	if (INFO.enemyRace == Races::Protoss)
		rangeUnits = INFO.getCompletedCount(Protoss_Dragoon, E);
	else if (INFO.enemyRace == Races::Zerg)
		rangeUnits = INFO.getCompletedCount(Zerg_Hydralisk, E);
	else
	{
		rangeUnits = INFO.getCompletedCount(Terran_Vulture, E);
		rangeUnits += INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, E);
	}

	if (firstExpensionTower * 2 <= MarineCnt && rangeUnits == 0)
	{
		for (auto v : diveMarines)
		{
			v->setState(new MarineDiveState());
			kitingSet.del(v);
		}
		diveDone = true;
	}
}
void MarineKiting::action()
{

	if (size() == 0)	return;
	bool canStim = false;

	if (S->hasResearched(TechTypes::Stim_Packs))
		canStim = true;


	if (waitingTime != 0 && waitingTime + (24 * 30) < TIME)
	{
		waitingTime = 0;
		needWaiting = 0;
		needCheck = true;
	}

	Base *enFirstExp = INFO.getFirstExpansionLocation(E);
	if (TIME <= 24 * 60 * 5 && INFO.enemyRace == Races::Terran)
	{
		if (EIB == Terran_1fac_1star ||
			EIB == Terran_1fac_2star ||
			EIB == Terran_2fac_1star ||
			EMB == Terran_1fac_1star ||
			EMB == Terran_1fac_2star ||
			EMB == Terran_2fac_1star ||
			EMB == Terran_2fac)
			target = INFO.getSecondChokePosition(S);
		else
			target = (INFO.getSecondChokePosition(E) + INFO.getFirstExpansionLocation(E)->Center()) / 2;
		//target = (INFO.getFirstExpansionLocation(E)->Center()) ;
	}
	else
	{
		if (enFirstExp && target == enFirstExp->Center())
			target = INFO.getMainBaseLocation(E)->Center();
	}

	UnitInfo *frontUnit = getFrontUnitFromPosition(target);

	if (frontUnit == nullptr) return;
	Position backPos1 = (INFO.getFirstChokePosition(S) + INFO.getMainBaseLocation(S)->getPosition()) / 2;
	Position backPos = (INFO.getSecondChokePosition(S) + INFO.getFirstExpansionLocation(S)->getPosition()) / 2;
	UnitInfo *frontTank = TM.getFrontTankFromPos(target);
	UnitInfo *frontMarine = MM.getFrontMarineFromPos(target);
	uList SCMarineArmy = INFO.getTypeUnitsInRadius(Terran_Marine, S, INFO.getSecondChokePosition(E), 6 * TILE_SIZE);
	//needfightµÄÊ±ºòneedback¾ÍÊÇfalse£¬·´Ö®ÒàÈ»====================
	if (needFight)
	{
		if (needBack(frontUnit->pos()))	needFight = false;
	}
	else
	{
		if (canFight(frontUnit->pos()))	needFight = true;
	}
	//============================================================
	for (auto v : getUnits())
	{
		if (isStuckOrUnderSpell(v))
			continue;
		Position pos = (INFO.getSecondChokePosition(S) + INFO.getFirstExpansionLocation(S)->Center()) / 2;
		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(v->pos(), &dangerPoint, false);
		UnitInfo *closestTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

		//=====================================================
		bool followtank = false;
		if (closestTank!=nullptr)
		{ 
		if (getGroundDistance(closestTank->pos(), INFO.getSecondChokePosition(E))<4 * TILE_SIZE)
			followtank = true;
		else if (getGroundDistance(closestTank->pos(), INFO.getFirstExpansionLocation(E)->Center())<10* TILE_SIZE)
			followtank = true;
		else if (getGroundDistance(closestTank->pos(), INFO.getMainBaseLocation(E)->Center())<20 * TILE_SIZE)
			followtank = true;
		else if (INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, closestTank->pos(),8*TILE_SIZE).size())
			followtank = true;
		}
		//=========================================================




		if ((SM.getMyBuild() == MyBuildTypes::Protoss_CarrierKiller || SM.getMyBuild() == MyBuildTypes::Protoss_DragoonKiller || SM.getMyBuild() == MyBuildTypes::Protoss_CarrierKiller || SM.getMyBuild() == MyBuildTypes::Protoss_CannonKiller) && EMB == Toss_dark_drop && INFO.getDestroyedCount(Protoss_Dark_Templar, E) < 4 && (TIME < 9.5 * 60 * 24))
		{
			if (getGroundDistance(MYBASE, v->pos()) > 4 * TILE_SIZE)
			{
				CommandUtil::move(v->unit(), MYBASE);
				continue;

			}
		}

		else if ((SM.getMyBuild() == MyBuildTypes::Protoss_DragoonKiller || SM.getMyBuild() == MyBuildTypes::Protoss_TemplarKiller || SM.getMyBuild() == MyBuildTypes::Protoss_CarrierKiller || SM.getMyBuild() == MyBuildTypes::Protoss_CannonKiller) && SM.getMainStrategy() != AttackAll)

		{

			if (getGroundDistance(INFO.getFirstExpansionLocation(S)->Center(), v->pos()) > 4 * TILE_SIZE)
			{
				CommandUtil::move(v->unit(), INFO.getFirstExpansionLocation(S)->Center());
				continue;

			}
		}

		else if ((SM.getMyBuild() == MyBuildTypes::Protoss_DragoonKiller || SM.getMyBuild() == MyBuildTypes::Protoss_TemplarKiller || SM.getMyBuild() == MyBuildTypes::Protoss_CarrierKiller || SM.getMyBuild() == MyBuildTypes::Protoss_CannonKiller) && SM.getMainStrategy() == AttackAll)
		{
			if (closestTank != nullptr && (!isSameArea(closestTank->pos(), INFO.getSecondChokePosition(S))))
			{
				if (getGroundDistance(closestTank->pos(), v->pos()) > 3 * TILE_SIZE)
				{
					CommandUtil::attackMove(v->unit(), closestTank->pos());
					continue;
				}
			}
			else if (closestTank != nullptr && (isSameArea(closestTank->pos(), INFO.getSecondChokePosition(S))))
			{
				if (getGroundDistance(closestTank->pos(), v->pos()) > 9 * TILE_SIZE)
				{
					CommandUtil::attackMove(v->unit(), closestTank->pos());
					continue;
				}
			}
		}

		else if (SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller && INFO.getAllCount(Terran_Command_Center, S) < 2 && INFO.getTypeBuildingsInRadius(Protoss_Zealot, E, MYBASE, 10 * TILE_SIZE).size() == 0)
		{

			if (getGroundDistance(MYBASE, v->pos()) > 5 * TILE_SIZE)
			{
				CommandUtil::move(v->unit(), MYBASE);
				continue;

			}
		}
		else if (SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller && !followtank)
		{
			Unit bunker = MM.getBunker();
			uList Dragoon = INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, bunker->getPosition(), 10 * TILE_SIZE);
			if (bunker && Dragoon.empty())
			{ 

				if (getGroundDistance(bunker->getPosition(), v->pos()) > 7 * TILE_SIZE)
				{
					CommandUtil::move(v->unit(), bunker->getPosition());
					continue;

				}
			}
			else if (bunker && !Dragoon.empty())
			{

				if (getGroundDistance(bunker->getPosition(), v->pos()) > 4 * TILE_SIZE)
				{
					CommandUtil::move(v->unit(), bunker->getPosition());
					continue;

				}
			}
			else
			{
				if (getGroundDistance(INFO.getSecondChokePosition(E), v->pos()) > 2 * TILE_SIZE)
				{
					CommandUtil::move(v->unit(), INFO.getSecondChokePosition(E));
					continue;

				}
			}
		}
		else if (SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller && followtank)
		{
		

			if (closestTank != nullptr && followtank)
			{
				if (getGroundDistance(closestTank->pos(), v->pos()) > 4 * TILE_SIZE)
				{
					CommandUtil::attackMove(v->unit(), closestTank->pos());
					continue;
				}
			}
		}
		else if (SM.getMainStrategy() != AttackAll && (EIB == Terran_1fac_1star || EIB == Terran_1fac_2star || EIB == Terran_2fac_1star || EMB == Terran_1fac_1star || EMB == Terran_1fac_2star || EMB == Terran_2fac_1star))
		{
			if (getGroundDistance(INFO.getFirstExpansionLocation(S)->Center(), v->pos()) > 3 * TILE_SIZE)
			{
				CommandUtil::move(v->unit(), INFO.getFirstExpansionLocation(S)->Center());
				return;

			}
		}


		if (dangerUnit == nullptr) //Ã»ÓÐ¶ÔÓÚÇ¹±øµÄÍþÐ²
		{
			UnitInfo *buildingTower = INFO.getClosestTypeUnit(E, v->pos(), INFO.getAdvancedDefenseBuildingType(INFO.enemyRace, false), 12 * TILE_SIZE);
			if (buildingTower != nullptr && enFirstExp && isSameArea(buildingTower->pos(), enFirstExp->Center()))
			{
				if (INFO.enemyRace == Races::Terran)
				{
					if (SCMarineArmy.size() >= 2)//Èç¹û2¿óÓÐÅÚÌ¨£¬Ç¹±ø´óÓÚËÄµÄ»°¾Í´òµôËû
					{
						CommandUtil::attackUnit(v->unit(), buildingTower->unit());
						continue;
					}
					else if (SCMarineArmy.size() > 5)//Èç¹ûÇ¹±ø´óÓÚ8¸ö£¬Ç±ÈëÖ÷»ùµØ
					{
						CommandUtil::move(v->unit(), target);
						continue;
					}
					else//ÆäÓàÇé¿ö£¬ÔÚscµÈºò
					{
						CommandUtil::attackMove(v->unit(), INFO.getFirstExpansionLocation(S)->Center());
						continue;
					}
				}
				else {
					if (SCMarineArmy.size() >= 4 && SCMarineArmy.size() <= 8)//Èç¹û2¿óÓÐÅÚÌ¨£¬Ç¹±ø´óÓÚËÄµÄ»°¾Í´òµôËû
					{
						CommandUtil::attackUnit(v->unit(), buildingTower->unit());
						continue;
					}
					else if (SCMarineArmy.size() > 8)//Èç¹ûÇ¹±ø´óÓÚ8¸ö£¬Ç±ÈëÖ÷»ùµØ
					{
						CommandUtil::move(v->unit(), target);
						continue;
					}
					else//ÆäÓàÇé¿ö£¬ÔÚscµÈºò
					{
						CommandUtil::attackMove(v->unit(), INFO.getSecondChokePosition(E));
						continue;
					}
				}

			}

			UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 15 * TILE_SIZE);
			uList Tower = INFO.getTypeBuildingsInRadius(INFO.getAdvancedDefenseBuildingType(INFO.enemyRace, false), E, INFO.getFirstExpansionLocation(E)->getPosition(), 12 * TILE_SIZE);
			if (closestWorker != nullptr)
			{
				if (INFO.enemyRace == Races::Terran)
				{
					if (TIME >= 24 * 60 * 3.5)
					{
						kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
						continue;
					}
					else
						CommandUtil::move(v->unit(), target);
				}
				else
				{
					kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
					continue;
				}
			}
			else
			{
				if (v->pos().getApproxDistance(target) > 5 * TILE_SIZE) // ¸ñÀûÁö ÀÌµ¿
					CommandUtil::move(v->unit(), target);
				else if (TIME < (24 * 60 * 10) && (EIB == Toss_1g_forward || EIB == Toss_2g_forward)) // º»Áø ÆÄÀÏ·±ÀÌ ¾Æ´Ñ °æ¿ì¿¡ ÆÄÀÏ·±Àº ±ü´Ù.
				{
					UnitInfo *pylon = INFO.getClosestTypeUnit(E, target, Protoss_Pylon, 5 * TILE_SIZE);

					if (pylon)
					{
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
		else //ÓÐdangerUnitµÄÇé¿ö
		{
			/*
			if (isNeedKitingUnitTypeforMarine(dangerUnit->type()))//¿ÉÒÔÖ±½ÓkitingµÄµ¥Î»£¬¾Í¼¸¸ö
			{
				UnitInfo *closestAttack = INFO.getClosestUnit(E, v->pos(), GroundCombatKind, 12 * TILE_SIZE, true, false, true);

				if (closestAttack == nullptr) //Ã»ÓÐµØÃæµ¥Î»Ê±
				{
					UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

					if (closestWorker == nullptr)//Ã»ÓÐÅ©ÃñÊ±
					{
						UnitInfo *closestAir = INFO.getClosestUnit(E, v->pos(), AirUnitKind, 12 * TILE_SIZE, true, false, true);
						if (closestAir != nullptr)//´ò»÷¿ÕÖÐµ¥Î»
						{
							kiting(v, closestAir, dangerPoint, 2 * TILE_SIZE);
						}
					}
					else if (closestWorker != nullptr)
					{
						kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
					}

				}
				else // ÓÐµØÃæÎ£ÏÕµ¥Î»Ê±
				{

					UnitInfo *closestTank = INFO.getClosestTypeUnit(S, v->pos(), Terran_Siege_Tank_Tank_Mode);
					UnitInfo *closestMarine = INFO.getClosestTypeUnit(S, v->pos(), Terran_Marine);
					UnitInfo *closestVulture = INFO.getClosestTypeUnit(S, v->pos(), Terran_Vulture);
					// ÁÖº¯¿¡ Tank°¡ ÀÖÀ¸¸é Kiting ÇÏÁö ¸»°í ±×³É °ø°ÝÇÏ´Â°Ô ³´´Ù.
					if (closestTank != nullptr && closestTank->pos().getApproxDistance(v->pos()) < 4 * TILE_SIZE)
						CommandUtil::attackUnit(v->unit(), closestAttack->unit());
					else
					{
						kiting(v, closestAttack, dangerPoint, 4 * TILE_SIZE);
					}

				}
			} */
			if (needFight)
			{
				UnitInfo *closestAttack = INFO.getClosestUnit(E, frontUnit->pos(), GroundCombatKind, 11 * TILE_SIZE, false, false, true);
				UnitInfo *closestAttacksoclose = INFO.getClosestUnit(E, frontUnit->pos(), GroundCombatKind, 6 * TILE_SIZE, false, false, true);
				if (canStim &&closestAttacksoclose != nullptr && (!v->unit()->isStimmed()))
				{
					v->unit()->useTech(TechTypes::Stim_Packs);
					continue;
				}
				if (closestAttack == nullptr) // º¸ÀÌÁú ¾Ê¾Æ
				{
					UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

					if (closestWorker != nullptr)
						kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
					else if (v->pos().getApproxDistance(target) > 7 * TILE_SIZE) // ¸ñÀûÁö ÀÌµ¿
						CommandUtil::move(v->unit(), target);
				}
				else
				{
					if (INFO.enemyRace == Races::Terran)
					{
						if (TIME <= 24 * 60 * 5)
						{
							UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, v->pos(), INFO.getWorkerType(INFO.enemyRace), 10 * TILE_SIZE);

							if (closestWorker != nullptr)
								kiting(v, closestWorker, dangerPoint, 2 * TILE_SIZE);
							else if (v->pos().getApproxDistance(target) > 7 * TILE_SIZE)
								CommandUtil::move(v->unit(), target);
						}
					}
					UnitInfo *weakUnit = getGroundWeakTargetInRange(v);
					if (weakUnit->type() != Terran_Vulture && weakUnit->type() != Terran_Marine)
					{
						if (weakUnit != nullptr)
							closestAttack = weakUnit;

						if (closestAttack->type() == Terran_Bunker)
						{
							UnitInfo *closestWorker = INFO.getClosestTypeUnit(E, closestAttack->pos(), INFO.getWorkerType(INFO.enemyRace), 2 * TILE_SIZE);

							if (closestWorker != nullptr)
								CommandUtil::attackUnit(v->unit(), closestWorker->unit());

							continue;
						}

						kiting(v, closestAttack, dangerPoint, 2 * TILE_SIZE);

						if (v->posChange(closestAttack) == PosChange::Farther)
							v->unit()->move((v->pos() + closestAttack->vPos()) / 2);
					}
				}

				continue;
			}
			else // ·ÇAttack All
			{
				if (needWaiting)
				{
					if (!dangerUnit->isHide())
						waitingTime = TIME;


					int backThreshold = 5;


					if (dangerUnit->type() == Terran_Siege_Tank_Siege_Mode || dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
						backThreshold = 8;

					if (dangerUnit->type() == Protoss_Dragoon)
						backThreshold = 8;

					if (dangerUnit->type() == Zerg_Lurker)
						backThreshold = 7;


					if (dangerPoint > backThreshold * TILE_SIZE)
					{

						if (size() <= 5)
						{
							if (v == frontUnit)
								CommandUtil::hold(v->unit());
							else if (v->pos().getApproxDistance(frontUnit->pos()) < 3 * TILE_SIZE)
								CommandUtil::move(v->unit(), frontUnit->pos());
						}
					}
					else // (dangerPoint <= backThreshold * TILE_SIZE)
					{
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
				}
				else
				{
					if (dangerUnit->type() == INFO.getAdvancedDefenseBuildingType(INFO.enemyRace))
					{
						if (enFirstExp && isSameArea(dangerUnit->pos(), enFirstExp->Center()))
						{
							needWaiting = 1;
							waitingTime = TIME;
						}
						else
						{
							UnitInfo *closestAttack = INFO.getClosestUnit(E, v->pos(), GroundUnitKind, 10 * TILE_SIZE, false, false, true);

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
							else // closest Attacker Á¸Àç
							{
								if (isNeedKitingUnitType(closestAttack->type()))
								{
									kiting(v, closestAttack, dangerPoint, 3 * TILE_SIZE);
								}
							}
						}
					}
					else // Å¸¿ö°¡ ¾Æ´Ñ°æ¿ì
					{
						if (needCheck == true && dangerUnit->isHide())
						{
							CommandUtil::move(v->unit(), target);
							continue;
						}

						needWaiting = 2;
						waitingTime = TIME;
					}
				}
			}
		}
	}


	return;
}


bool MarineKiting::canFight(Position frontPos)
{
	// ÀüÁø °ÔÀÌÆ®¿¡ ´ëÇÑ Æ¯º° Ã³¸® - ¸¶¸°ÀÌ ÀÖÀ¸¸é ±×³É ½Î¿ì´Â ·ÎÁ÷ ÇÊ¿ä

	int range = 5 * TILE_SIZE;

	if (size() > 5)
		range += 2 * TILE_SIZE;

	word VultureCnt = INFO.getTypeUnitsInRadius(Terran_Vulture, S, frontPos, range).size();
	word MarineCnt =INFO.getTypeUnitsInRadius(Terran_Marine, S, frontPos, range).size();
	word MedicCnt = INFO.getTypeUnitsInRadius(Terran_Medic, S, frontPos, range).size();
	word BunkerCnt = INFO.getTypeBuildingsInRadius(Terran_Bunker, S, frontPos, range).size();
	if (INFO.enemyRace == Races::Protoss)
	{
		word dragoon = INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, frontPos, 10 * TILE_SIZE, true).size();
		word zealot = INFO.getTypeUnitsInRadius(Protoss_Zealot, E, frontPos, 10 * TILE_SIZE, true).size();
		word Archon = INFO.getTypeUnitsInRadius(Protoss_Archon, E, frontPos, 10 * TILE_SIZE, true).size();
		word Carrier = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, frontPos, 10 * TILE_SIZE, true).size();
		word Scout = INFO.getTypeUnitsInRadius(Protoss_Scout, E, frontPos, 10 * TILE_SIZE, true).size();
		word Pho = INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, frontPos, 12 * TILE_SIZE, true).size();

		preAmountOfEnemy = dragoon + zealot + Archon ;

		if ((dragoon * 3) + (Archon * 5) + (zealot * 3) + (Scout * 3) + (Pho * 4) <= BunkerCnt*3+ MarineCnt + VultureCnt * 2 + MedicCnt)
			return true;
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		word hydra = INFO.getTypeUnitsInRadius(Zerg_Hydralisk, E, frontPos, 10 * TILE_SIZE, true).size();
		word zergling = INFO.getTypeUnitsInRadius(Zerg_Zergling, E, frontPos, 10 * TILE_SIZE, true).size();
		word mutal = INFO.getTypeUnitsInRadius(Zerg_Mutalisk, E, frontPos, 10 * TILE_SIZE, true).size();
		word guardian = INFO.getTypeUnitsInRadius(Zerg_Guardian, E, frontPos, 10 * TILE_SIZE, true).size();
		uList luckers = INFO.getTypeUnitsInRadius(Zerg_Lurker, E, frontPos, 10 * TILE_SIZE, true);
		word lucker = 0;

		for (auto l : luckers)
		if (!l->unit()->canBurrow(false))
			lucker++;

		preAmountOfEnemy = hydra + zergling  + lucker;

		if ((zergling)+(hydra * 2) + (lucker * 3) + (mutal * 2) + (guardian * 4) <= BunkerCnt * 3 + MarineCnt + VultureCnt * 2 + MedicCnt)
			return true;
	}
	else
	{
		word battlecruiser = INFO.getTypeUnitsInRadius(Terran_Battlecruiser, E, frontPos, 10 * TILE_SIZE, true).size();
		word vulture = INFO.getTypeUnitsInRadius(Terran_Vulture, E, frontPos, 10 * TILE_SIZE, true).size();
		word tank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, frontPos, 17 * TILE_SIZE, true).size();
		word goliath = INFO.getTypeUnitsInRadius(Terran_Goliath, E, frontPos, 10 * TILE_SIZE, true).size();
		word marine = INFO.getTypeUnitsInRadius(Terran_Marine, E, frontPos, 10 * TILE_SIZE, true).size();
		uList bunkers = INFO.getTypeBuildingsInRadius(Terran_Bunker, E, frontPos, 10 * TILE_SIZE, true);
		word bunker = 0;

		for (auto b : bunkers)
		if (b->getMarinesInBunker())
			bunker++;

		preAmountOfEnemy = vulture + tank + goliath + bunker + marine;
		if (TIME <= 24 * 60 * 4.5)
			return true;
		if ((marine)+(vulture * 2) + (goliath * 3) + (tank * 4) + (bunker * 6) <= BunkerCnt * 3 + MarineCnt + VultureCnt * 2 + MedicCnt)
			return true;
	}

	return false;
}

bool MarineKiting::needBack(Position frontPos)
{
	int range = 5 * TILE_SIZE;

	if (size() > 5)
		range += 2 * TILE_SIZE;

	word VultureCnt = INFO.getTypeUnitsInRadius(Terran_Vulture, S, frontPos, range).size();
	word MarineCnt =  INFO.getTypeUnitsInRadius(Terran_Marine, S, frontPos, range).size();
	word MedicCnt = INFO.getTypeUnitsInRadius(Terran_Marine, S, frontPos, range).size();
	word BunkerCnt = INFO.getTypeBuildingsInRadius(Terran_Bunker, S, frontPos, range).size();
	if (INFO.enemyRace == Races::Protoss)
	{
		word dragoon = INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, frontPos, 10 * TILE_SIZE, true).size();
		word zealot = INFO.getTypeUnitsInRadius(Protoss_Zealot, E, frontPos, 10 * TILE_SIZE, true).size();
		word Archon = INFO.getTypeUnitsInRadius(Protoss_Archon, E, frontPos, 10 * TILE_SIZE, true).size();
		word Carrier = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, frontPos, 10 * TILE_SIZE, true).size();
		word Scout = INFO.getTypeUnitsInRadius(Protoss_Scout, E, frontPos, 10 * TILE_SIZE, true).size();
		word Pho = INFO.getTypeBuildingsInRadius(Protoss_Photon_Cannon, E, frontPos, 12 * TILE_SIZE, true).size();
		if ((preAmountOfEnemy < dragoon + zealot + Archon) ||
			((dragoon * 3) + (Archon * 5) + (zealot * 3) + (Scout * 3) + +(Pho * 4)> MarineCnt + VultureCnt * 2 + MedicCnt + BunkerCnt*3))
			return true;
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		word hydra = INFO.getTypeUnitsInRadius(Zerg_Hydralisk, E, frontPos, 10 * TILE_SIZE, true).size();
		word zergling = INFO.getTypeUnitsInRadius(Zerg_Zergling, E, frontPos, 10 * TILE_SIZE, true).size();
		word mutal = INFO.getTypeUnitsInRadius(Zerg_Mutalisk, E, frontPos, 10 * TILE_SIZE, true).size();
		word guardian = INFO.getTypeUnitsInRadius(Zerg_Guardian, E, frontPos, 10 * TILE_SIZE, true).size();
		uList luckers = INFO.getTypeUnitsInRadius(Zerg_Lurker, E, frontPos, 10 * TILE_SIZE, true);
		word lucker = 0;

		for (auto l : luckers)
		if (!l->unit()->canBurrow(false))
			lucker++;

		if ((preAmountOfEnemy <  hydra + zergling + lucker) ||
			((zergling)+(hydra * 2) + (lucker * 3) + (mutal * 2) + (guardian * 4) > MarineCnt + VultureCnt * 2 + MedicCnt + BunkerCnt*3))
			return true;
	}
	else
	{
		word battlecruiser = INFO.getTypeUnitsInRadius(Terran_Battlecruiser, E, frontPos, 10 * TILE_SIZE, true).size();
		word vulture = INFO.getTypeUnitsInRadius(Terran_Vulture, E, frontPos, 10 * TILE_SIZE, true).size();
		word tank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, frontPos, 17 * TILE_SIZE, true).size();
		word goliath = INFO.getTypeUnitsInRadius(Terran_Goliath, E, frontPos, 10 * TILE_SIZE, true).size();
		word marine = INFO.getTypeUnitsInRadius(Terran_Marine, E, frontPos, 10 * TILE_SIZE, true).size();
		uList bunkers = INFO.getTypeBuildingsInRadius(Terran_Bunker, E, frontPos, 10 * TILE_SIZE, true);
		word bunker = 0;

		for (auto b : bunkers)
		if (b->getMarinesInBunker())
			bunker++;

		if (TIME <= 24 * 60 * 4.5)
			return false;
		if ((preAmountOfEnemy < vulture + tank + goliath + marine) ||
			((marine)+(vulture * 2) + (goliath * 3) + (tank * 4) + (bunker * 6)  > MarineCnt + VultureCnt * 2 + MedicCnt + BunkerCnt*3))
			return true;
	}

	return false;
}

void MarineKiting::clearSet() {
	if (!size())	return;

	for (auto v : getUnits()) {
		v->setState(new MarineIdleState());
	}

	clear();
}

void MarineKiting::actionVsT()
{
	if (size() == 0)	return;

	if (changedTime != 0 && changedTime + (24 * 5) > TIME)
		return;

	if (preStage != DrawLine && SM.getMainStrategy() == DrawLine)
	{
		changedTime = TIME;
		preStage = SM.getMainStrategy();
		return;
	}

	preStage = SM.getMainStrategy();

	// Time To Checek
	if (waitingTime != 0 && waitingTime + (24 * 90) < TIME)
	{
		waitingTime = 0;
		needWaiting = false;
		needCheck = true;
	}

	Base *enemyFirstExpension = INFO.getFirstExpansionLocation(E);

	Position backPos = (INFO.getSecondChokePosition(S) + INFO.getFirstExpansionLocation(S)->getPosition()) / 2;

	Position VultureWaitPos;
	UnitInfo *closestEnemy = nullptr;
	int enSize = 0;

	if (SM.getMainStrategy() != AttackAll)
	{
		uList enemyToCut = getEnemyOutOfRange(SM.getWaitLinePosition(), true);

		closestEnemy = getClosestUnit(MYBASE, enemyToCut, true);

		if (closestEnemy)
			target = closestEnemy->pos();
		else
		{
			if (SM.getMainStrategy() == DrawLine)
				VultureWaitPos = SM.getDrawLinePosition();
			else
			{
				if (SM.getSecondAttackPosition() == Positions::Unknown)
					VultureWaitPos = getDirectionDistancePosition(SM.getWaitLinePosition(), INFO.getFirstWaitLinePosition(), 5 * TILE_SIZE);
				else {
					VultureWaitPos = SM.getSecondAttackPosition();
				}
			}
		}

		bw->drawCircleMap(target, 10, Colors::Cyan, true);
	}

	UnitInfo *frontUnit = getFrontUnitFromPosition(target);

	if (frontUnit == nullptr) return;

	if (needFight) {
		if (needBack(frontUnit->pos()))	needFight = false;
	}
	else {
		if (canFight(frontUnit->pos()))	needFight = true;
	}

	bool mineUp = S->hasResearched(TechTypes::Spider_Mines);

	for (auto v : getUnits())
	{
		if (isStuckOrUnderSpell(v))
			continue;

		if ((TIME - v->unit()->getLastCommandFrame() < 24) &&
			v->unit()->getLastCommand().getType() == UnitCommandTypes::Use_Tech_Position)
			continue;

		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(v->pos(), &dangerPoint, false);

		if (SM.getMainStrategy() == AttackAll)
		{
			// DrawLine -> °ø°Ý
			if (mineUp && v->unit()->getSpiderMineCount() > 0)
			{
				UnitInfo *closestTank = INFO.getClosestTypeUnit(E, v->pos(), Terran_Siege_Tank_Tank_Mode, 8 * TILE_SIZE, true, true);

				if (closestTank && INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, closestTank->pos(), 3 * TILE_SIZE).size() == 0)
				{
					if (v->pos().getApproxDistance(closestTank->pos()) < 2 * TILE_SIZE)
						v->unit()->useTech(TechTypes::Spider_Mines, v->pos());
					else
						CommandUtil::move(v->unit(), closestTank->pos());

					continue;
				}
			}

			CommandUtil::attackMove(v->unit(), target);
		}
		else if (SM.getMainStrategy() == WaitLine || SM.getMainStrategy() == DrawLine)
		{
			if (closestEnemy && needFight)
			{
				UnitInfo *closestTank = INFO.getClosestTypeUnit(E, v->pos(), Terran_Siege_Tank_Tank_Mode, 8 * TILE_SIZE, true, true);

				if (closestTank && INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, closestTank->pos(), 3 * TILE_SIZE).size() == 0)
				{
					if (mineUp && v->unit()->getSpiderMineCount() > 0) {
						if (v->pos().getApproxDistance(closestTank->pos()) < 2 * TILE_SIZE)
							v->unit()->useTech(TechTypes::Spider_Mines, v->pos());
						else
							CommandUtil::move(v->unit(), closestTank->pos());
					}

					continue;
				}

				UnitInfo *weakUnit = getGroundWeakTargetInRange(v, true);

				if (weakUnit != nullptr) // °ø°Ý ´ë»ó ¼³Á¤ º¯°æ
					closestEnemy = weakUnit;

				if (closestEnemy->isHide())
					CommandUtil::attackMove(v->unit(), closestEnemy->pos());
				else
				{
					CommandUtil::attackUnit(v->unit(), closestEnemy->unit());

					if (v->posChange(closestEnemy) == PosChange::Farther)
						v->unit()->move((v->pos() + closestEnemy->vPos()) / 2);
				}
			}
			else
			{
				if (SM.getMainStrategy() == WaitLine)
				{
					if (dangerUnit && dangerUnit->type() == Terran_Siege_Tank_Siege_Mode && dangerPoint < 4 * TILE_SIZE)
					{
						moveBackPostion(v, dangerUnit->pos(), 3 * TILE_SIZE);
						continue;
					}

					if (mineUp && v->unit()->getSpiderMineCount() > 1)
					{
						if (INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, v->pos(), 5 * TILE_SIZE).size() == 0)
						{
							v->unit()->useTech(TechTypes::Spider_Mines, v->pos());
							continue;
						}
					}

					if (target.getApproxDistance(v->pos()) > 4 * TILE_SIZE)
						CommandUtil::attackMove(v->unit(), VultureWaitPos, true);

				}
				else // Draw Line
				{
					CommandUtil::attackMove(v->unit(), VultureWaitPos);
				}
			}
		}
	}

	return;
}

void MarineManager::setFirstBunker()
{
	uList bunkers = INFO.getBuildings(Terran_Bunker, S);

	if (bunkers.size() == 0) 
	{
		bunker = nullptr;
	}
	else {
		if (bunkers.size() == 1)
		{
			bunker = bunkers.at(0)->unit();
			checkRangeUnitNearBunker();
		}
		else 
		{
			for (auto b : bunkers) 
			{
				if (isSameArea(b->pos(), INFO.getFirstExpansionLocation(S)->Center()))
				{
					bunker = b->unit();
					checkRangeUnitNearBunker();
					break;
				}
			}
		}
	}

}

void MarineManager::setDefenceMarine(uList &marineList)
{
	if (ESM.getEnemyInitialBuild() == Toss_1g_forward || ESM.getEnemyInitialBuild() == Toss_2g_forward)
	{
		if (INFO.getCompletedCount(Protoss_Zealot, E) <= 1 && INFO.getDestroyedCount(Protoss_Zealot, E) > 0
			&& INFO.getCompletedCount(Protoss_Dragoon, E) == 0) {
			UnitInfo *gateway = INFO.getClosestTypeUnit(E, MYBASE, Protoss_Gateway, 0, true, true);

			if (gateway) {
				if (INFO.getSecondChokePosition(S).getApproxDistance(theMap.Center()) + 10 * TILE_SIZE
						> INFO.getSecondChokePosition(S).getApproxDistance(gateway->pos())) {
					for (auto m : marineList)
					{
						m->setState(new MarineAttackState(gateway));
					}

					return;
				}
			}
		}
	}

	 enemyWorkersInMyYard = getEnemyInMyYard(1700, INFO.getWorkerType(INFO.enemyRace));
	 enemyBuildingsInMyYard = getEnemyInMyYard(1700, UnitTypes::Buildings);
	 enemyInMyYard = getEnemyInMyYard(1700, Men, false);

	int totEnemNum = enemyInMyYard.size() + enemyBuildingsInMyYard.size();

	
	if (totEnemNum == 0 && (TIME - lastSawScouterTime > 24 * 5)) {
		for (auto m : marineList)
		{
			if (m->getState() == "Defence" || m->getState() == "KillScouter")
				m->setState(new MarineIdleState());
		}
	}
	
	else {
		string state;

		int enemBuildingNum = enemyBuildingsInMyYard.size();
		int enemWorkerNum = enemyWorkersInMyYard.size();

		if (enemWorkerNum > 0 || enemBuildingNum > 0)
		{
			lastSawScouterTime = TIME;

			if (enemWorkerNum > 0)
			if (enemyWorkersInMyYard.front()->vPos() != Positions::None)
				lastScouterPosition = enemyWorkersInMyYard.front()->vPos();
			else
				lastScouterPosition = enemyWorkersInMyYard.front()->pos();
			else
				lastScouterPosition = enemyBuildingsInMyYard.front()->pos();
		}

		bool goKillScouter = true;

		if (totEnemNum > 0 && totEnemNum - enemWorkerNum - enemBuildingNum == 0 && ESM.getEnemyInitialBuild() != Terran_bunker_rush) {
			for (auto eBuilding : enemyBuildingsInMyYard) {
				if (eBuilding->type().groundWeapon().targetsGround()) {
					goKillScouter = false;
					break;
				}
			}
		}
		else {
			goKillScouter = false;
		}
		for (auto m : marineList)
		{
			state = m->getState();

			if (state == "New" || state == "Idle" || state == "KillScouter" || state == "Defence") 
			{
				if (ESM.getEnemyInitialBuild() == Toss_cannon_rush)
					m->setState(new MarineAttackState(INFO.getSecondChokePosition(S)));
				else if (goKillScouter) {
					Unit b = getBunker();

					if ((b && m == marineList.at(0)) || !b)
						m->setState(new MarineKillScouterState());
				}
				else
					m->setState(new MarineDefenceState());
			}
		}
		
	}

	return;
}

void MarineManager::doKiting(Unit unit) {

	// »ó´ë°¡ ¿¡¾î À¯´ÖÀÌ°Å³ª ³ªº¸´Ù »çÁ¤°Å¸®°¡ ±æ°Å³ª °°À¸¸é º¡Ä¿¿¡¼­ ´ë±âÇÏ°Å³ª ±×³É °ø°ÝÇÑ´Ù.
	UnitInfo *closeUnit = INFO.getClosestUnit(E, unit->getPosition(), AllUnitKind);

	if (closeUnit == nullptr || unit->getDistance(closeUnit->pos()) > 1600)
		return;

	Unit bunker = MarineManager::Instance().getBunker();

	// º¡Ä¿°¡ ¿Ï¼ºµÇ¾î ÀÖ°Å³ª ±ÙÃ³ À¯´ÖÀÌ ¾øÀ¸¸é Defence Mode·Î º¯È¯
	if (bunker != nullptr && bunker->isCompleted() && bunker->getLoadedUnits().size() != 4) {
		CommandUtil::rightClick(unit, bunker);
		return;
	}

	if (ESM.getEnemyInitialBuild() <= Zerg_12_Pool) // 9¹ß¾÷ ÀÌÇÏ¿¡¼­´Â ¸¶¸°ÀÌ ¹ÛÀ¸·Î ³ª°¡Áö ¾Ê´Â´Ù.
	{
		if (INFO.getTypeUnitsInRadius(Zerg_Zergling, E, unit->getPosition(), 8 * TILE_SIZE).size()
				> INFO.getTypeUnitsInRadius(Terran_Marine, S, unit->getPosition(), 8 * TILE_SIZE).size())
		{
			Position myBase = INFO.getMainBaseLocation(S)->Center();

			TilePosition myBaseTile = INFO.getMainBaseLocation(S)->getTilePosition();

			if (!isSameArea(theMap.GetArea((WalkPosition)unit->getPosition()), theMap.GetArea(myBaseTile))
				|| INFO.getFirstChokePosition(S).getApproxDistance(unit->getPosition()) < 3 * TILE_SIZE)
			{
				CommandUtil::move(unit, myBase);
				return;
			}
		}
	}

	if (!unit->isInWeaponRange(closeUnit->unit()))
	{
		CommandUtil::attackUnit(unit, closeUnit->unit());
	}
	// »ç°Å¸® ¾ÈÀÌ¸é
	else {
		UnitInfo *me = INFO.getUnitInfo(unit, S);

		int dangerPoint = 0;
		UnitInfo *dangerUnit = getDangerUnitNPoint(me->pos(), &dangerPoint, false);

		if (INFO.enemyRace == Races::Zerg)
			attackFirstkiting(me, closeUnit, dangerPoint, 4 * TILE_SIZE);
		else
			kiting(me, closeUnit, dangerPoint, 4 * TILE_SIZE);

		//uList Scvs = INFO.getTypeUnitsInRadius(Terran_SCV, S, (unit->getPosition() + closeUnit->pos()) / 2, 50);

		//if (unit->getGroundWeaponCooldown() == 0 && (Scvs.size() >= 3 || unit->getDistance(closeUnit->unit()) > 2 * TILE_SIZE))
		//{
		//	CommandUtil::attackUnit(unit, closeUnit->unit());
		//}
		//else
		//{
		//	if (closeUnit->type().groundWeapon().maxRange() < 4 * TILE_SIZE) {
		//		moveBackPostion(INFO.getUnitInfo(unit, S), closeUnit->vPos(), 2 * TILE_SIZE);
		//	}
		//}

	}

	return;
}
