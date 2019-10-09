#include "InformationManager.h"

using namespace MyBot;

InformationManager::InformationManager()
{
	mapPlayerLimit = bw->getStartLocations().size();//地图游戏人数限制

	selfPlayer = S;
	enemyPlayer = E;

	selfRace = S->getRace();//己方种族
	enemyRace = E->getRace();//敌方种族

	_unitData[S] = UnitData();//存放敌我双方单位数据
	_unitData[E] = UnitData();

	updateStartAndBaseLocation(); //判断base是否在开局视野范围内，若是则将其加入到StartBaseLocations当中, 将Bases加入_allBaseLocations

	_mainBaseLocations[S] = INFO.getStartLocation(S);//我方基地位置
	_mainBaseLocationChanged[S] = true;
	_occupiedBaseLocations[S] = list<const Base *>();
	_occupiedBaseLocations[S].push_back(_mainBaseLocations[S]);//获取曾经占据的基地位置

	_mainBaseLocations[E] = getNextSearchBase();//敌方基地位置
	_mainBaseLocationChanged[E] = true;

	_occupiedBaseLocations[E] = list<const Base *>();

	_firstChokePoint[S] = nullptr;//敌我双方第一、二个要塞地点以及第一、二、三扩展Location
	_firstChokePoint[E] = nullptr;
	_secondChokePoint[S] = nullptr;
	_secondChokePoint[E] = nullptr;
	_firstExpansionLocation[S] = nullptr;
	_firstExpansionLocation[E] = nullptr;
	_secondExpansionLocation[S] = nullptr;
	_secondExpansionLocation[E] = nullptr;
	_thirdExpansionLocation[S] = nullptr;
	_thirdExpansionLocation[E] = nullptr;
	_islandExpansionLocation[S] = nullptr;
	_islandExpansionLocation[E] = nullptr;

	isEnemyScanResearched = false;
	availableScanCount = 0;

	
	updateBaseLocationInfo();

	updateChangedMainBaseInfo(S);
	updateChangedMainBaseInfo(E);

	
	_firstExpansionOfAllStartposition = map<Base *, Base *>();
	_mainBaseAreaPair = map<const Area *, const Area *>();
	updateAllFirstExpansions();

	activationMineralBaseCount = 0;
	activationGasBaseCount = 0;


	
	for (auto &area : theMap.Areas())
		area.calcBoundaryVertices();
}

InformationManager::~InformationManager()
{
}


void InformationManager::update()
{
	try {
		updateUnitsInfo();//跟新单位信息
	}
	catch (...) {
	}
	try {
		updateBaseInfo();
	}
	
	catch (...) {
	}

	try {
		updateChokePointInfo();
	}
	
	catch (...) {
	}


	try {

		setFirstWaitLinePosition();
	}
	
	catch (...) {
	}

	try {
		
		updateSecondExpansion();
	}
	catch (...) {
	}

	try {
		updateSecondExpansionForEnemy();
	}
	
	catch (...) {
	}


	try {
		
		updateThirdExpansion();
	}
	
	catch (...) {
	}

	try {
	
		updateIslandExpansion();
	}
	
	catch (...) {
	}

	try {
	
		checkActivationBaseCount();
	}
	
	catch (...) {
	}

	
	checkEnemyInMyArea();
	checkMoveInside();


}

void InformationManager::checkEnemyInMyArea()
{
	_enemyInMyArea.clear();
	_enemyInMyYard.clear();

	uMap allUnits = _unitData[E].getAllUnits();

	for (auto &eu : allUnits)
	{
		if (isInMyArea(eu.second))
		{
			_enemyInMyArea.push_back(eu.second);
			_enemyInMyYard.push_back(eu.second);
		}
		else
		{
			if (!eu.second->type().isFlyer() && eu.second->pos().getApproxDistance(INFO.getSecondAverageChokePosition(S)) < 300)
				_enemyInMyYard.push_back(eu.second);
		}
	}
}

void InformationManager::updateStartAndBaseLocation()
{
	for (auto &area : theMap.Areas())
	{
		for (auto &base : area.Bases())
		{
			

			if (base.Starting())//判断base是否在开局视野范围内，若是则将其加入到StartBaseLocations当中,将Bases加入_allBaseLocations
			{
				_startBaseLocations.push_back((Base *)&base);
			}

			_allBaseLocations.push_back((Base *)&base);
		}
	}


	if (_startBaseLocations.size() <= 2)		return;

	if (enemyRace == Races::Protoss) {
	
		sort(_startBaseLocations.begin(), _startBaseLocations.end(), [](Base * a, Base * b)
		{
			TilePosition mybase = INFO.getStartLocation(S)->Location();

			return mybase.getApproxDistance(a->Location()) < mybase.getApproxDistance(b->Location());
		});


	}
	else {
		
		sort(_startBaseLocations.begin(), _startBaseLocations.end(), [](Base * a, Base * b)
		{
			TilePosition C = TilePosition(theMap.Center());
			TilePosition A = a->Location() - C;
			TilePosition B = b->Location() - C;

			int d1 = C.getApproxDistance(A);
			int d2 = C.getApproxDistance(B);

			double ang1 = atan2(A.y, A.x);
			double ang2 = atan2(B.y, B.x);

			if (ang1 < 0) ang1 += 2 * 3.141592;

			if (ang2 < 0) ang2 += 2 * 3.141592;

			return ang1 < ang2 || (ang1 == ang2 && d1 < d2);
		});
	}
}

Base *InformationManager::getBaseLocation(TilePosition pos)
{
	for (auto base : _allBaseLocations)
		if (base->Location() == pos)
			return (Base *)base;

	return nullptr;
}

Base *InformationManager::getStartLocation(Player p)
{
	for (auto base : _startBaseLocations)
	{
		if (p->getStartLocation() == base->Location())
			return (Base *)base;
	}

	return nullptr;
}

Base *InformationManager::getNearestBaseLocation(Position pos, bool groundDist)
{
	Base *ret = nullptr;

	int dist = 100000;

	for (auto base : _allBaseLocations)
	{
		int tmp = groundDist ? getGroundDistance(pos, base->getPosition()) : pos.getApproxDistance(base->getPosition());

		if (tmp >= 0 && dist > tmp)
		{
			dist = tmp;
			ret = (Base *)base;
		}
	}

	return (Base *)ret;
}


set<ChokePoint *> InformationManager::getMineChokePoints()
{
	set<ChokePoint *> cps;

	int nextRank = 1;

	Base *eFirstExpansion = getFirstExpansionLocation(E);

	for (auto base : getBaseLocations())
	{
		if (base->GetExpectedEnemyMultiRank() == nextRank)
		{
			theMap.GetPath(eFirstExpansion->getPosition(), base->getPosition());
		}
	}



	return cps;
}



void InformationManager::initReservedMineCount()
{
	for (auto base : getBaseLocations())
		base->SetReservedMineCount(0);

	for (auto cp : theMap.GetAllChokePoints())
		cp->SetReservedMineCount(0);


}

void InformationManager::checkEnemyScan()
{
	if (!getUnits(Spell_Scanner_Sweep, E).empty())
	{
		isEnemyScanResearched = true;
	}
}

void InformationManager::updateBaseInfo() {
	bool isChangedOccupyInfo = false;

	_occupiedBaseLocations[S].clear();
	_occupiedBaseLocations[E].clear();
	baseSafeMineralMap.clear();

	for (auto &area : theMap.Areas())
	{
		bool needCheckRange = false;

		if (area.Bases().size() > 1 || area.MiniTiles() > 1000 * 16 || area.MiniTiles() < 400 * 16)
			needCheckRange = true;

		for (auto &base : area.Bases())
		{


			uList sBuildings, eBuildings;
			uList sBuildings_, eBuildings_;

			if (!base.Starting() && needCheckRange) {
				sBuildings_ = INFO.getBuildingsInRadius(S, base.Center(), 12 * TILE_SIZE, true, false, true, true);
				eBuildings_ = INFO.getBuildingsInRadius(E, base.Center(), 12 * TILE_SIZE, true, false, true, true);

				for (auto b : sBuildings_)	{
					if (!isSameArea(b->pos(), MYBASE) && b->type() != Terran_Missile_Turret)
						sBuildings.push_back(b);
				}

				for (auto b : eBuildings_) {
					if (!isSameArea(b->pos(), INFO.getMainBaseLocation(E)->Center()) && b->type() != Terran_Missile_Turret)
						eBuildings.push_back(b);
				}
			}
			else {
				sBuildings_ = INFO.getBuildingsInArea(S, base.Center(), true, false, true);
				eBuildings_ = INFO.getBuildingsInArea(E, base.Center(), true, false, true);

				for (auto b : sBuildings_) {
					if (b->type() != Terran_Missile_Turret)
						sBuildings.push_back(b);
				}

				for (auto b : eBuildings_) {
					if (b->type() != Terran_Missile_Turret)
						eBuildings.push_back(b);
				}
			}


			if ( sBuildings.size() && eBuildings.size() )
			{
				if (base.GetOccupiedInfo() != shareBase)
				{
					base.SetOccupiedInfo(shareBase);
					isChangedOccupyInfo = true;
				}

				_occupiedBaseLocations[S].push_back(&base);
				_occupiedBaseLocations[E].push_back(&base);
			}
			else if ( sBuildings.size() ) 
			{
				if (base.GetOccupiedInfo() != myBase)
				{
					base.SetOccupiedInfo(myBase);
					isChangedOccupyInfo = true;
				}

				_occupiedBaseLocations[S].push_back(&base);
			}
			else if ( eBuildings.size() ) 
			{
				if (base.GetOccupiedInfo() != enemyBase)
				{
					base.SetOccupiedInfo(enemyBase);
					isChangedOccupyInfo = true;
				}

				_occupiedBaseLocations[E].push_back(&base);
			}
			else 
			{
				if (base.GetOccupiedInfo() != emptyBase)
				{
					base.SetOccupiedInfo(emptyBase);
					isChangedOccupyInfo = true;
				}
			}

			base.SetMineCount(INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, base.Center(), TILE_SIZE * 12, false).size());

			int workerCount = 0;
			int enemyAirDefenseUnitCount = 0;
			int enemyGroundDefenseUnitCount = 0;

			int airUnitInMyBase = 0; 
			int groundUnitInMyBase = 0; 

			uList sUnits, eUnits;

			if (needCheckRange)	{
				sUnits = INFO.getUnitsInRadius(S, base.Center(), 12 * TILE_SIZE, true, true, true, true);
				eUnits = INFO.getUnitsInRadius(E, base.Center(), 12 * TILE_SIZE, true, true, true, true);
			}
			else	{
				sUnits = INFO.getUnitsInArea(S, base.Center(), true, true, true, true);
				eUnits = INFO.getUnitsInArea(E, base.Center(), true, true, true, true);
			}

	
			for (auto u : eUnits)
			{
				if (u->type().isWorker() && base.GetOccupiedInfo() == enemyBase)
				{
					workerCount++;
					continue;
				}

				if (u->type().airWeapon().targetsAir())
				{
					enemyAirDefenseUnitCount++;
				}

				if (u->type().groundWeapon().targetsGround())
				{
					enemyGroundDefenseUnitCount++;
				}

				if (base.GetOccupiedInfo() == myBase)
				{
					if (u->type().isFlyer() && u->type().canAttack())
					{
						airUnitInMyBase++;
					}
					else if (!u->type().isFlyer() && u->type().canAttack() && !u->type().isWorker())
					{
						groundUnitInMyBase++;
					}
				}
			}

			int selfAirDefenseUnitCount = 0;
			int selfGroundDefenseUnitCount = 0;

		
			for (auto u : sUnits)
			{
				if (u->type().isWorker() && base.GetOccupiedInfo() == myBase)
				{
					workerCount++;
					continue;
				}

				if (u->type().airWeapon().targetsAir())
				{
					selfAirDefenseUnitCount++;
				}

				if (u->type().groundWeapon().targetsGround())
				{
					selfGroundDefenseUnitCount++;
				}
			}

			int enemyAirDefenseBuildingCount = 0;
			int enemyGroundDefenseBuildingCount = 0;
			int enemyBunkerCount = 0;

			for (auto u : eBuildings)
			{
				if (u->type().airWeapon().targetsAir() && u->isComplete())
				{
					enemyAirDefenseBuildingCount++;
				}

				if (u->type().groundWeapon().targetsGround() && u->isComplete())
				{
					enemyGroundDefenseBuildingCount++;
				}

				if (u->type() == Terran_Bunker && u->getMarinesInBunker() > 0)
				{
					enemyBunkerCount++;
				}
			}

			//}

			int selfAirDefenseBuildingCount = 0;
			int selfGroundDefenseBuildingCount = 0;


			for (auto u : sBuildings)
			{
				if (u->type().airWeapon().targetsAir())
				{
					selfAirDefenseBuildingCount++;
				}

				if (u->type().groundWeapon().targetsGround())
				{
					selfGroundDefenseBuildingCount++;
				}
			}

		

			
			base.SetEnemyAirDefenseUnitCount(enemyAirDefenseUnitCount);
	
			base.SetEnemyGroundDefenseUnitCount(enemyGroundDefenseUnitCount);
		
			base.SetSelfAirDefenseUnitCount(selfAirDefenseUnitCount);
			
			base.SetSelfGroundDefenseUnitCount(selfGroundDefenseUnitCount);
		
			base.SetEnemyAirDefenseBuildingCount(enemyAirDefenseBuildingCount);
			
			base.SetEnemyGroundDefenseBuildingCount(enemyGroundDefenseBuildingCount);
		
			base.SetSelfAirDefenseBuildingCount(selfAirDefenseBuildingCount);
			
			base.SetSelfGroundDefenseBuildingCount(selfGroundDefenseBuildingCount);
			
			base.SetWorkerCount(workerCount);
			
			base.SetEnemyBunkerCount(enemyBunkerCount);

			
			if (base.GetOccupiedInfo() == myBase)
			{
				bool isDangerousBaseForWorkers = false;

				for (auto u : INFO.getTypeUnitsInRadius(Terran_SCV, S, base.Center(), 8 * TILE_SIZE))
				{
					if (u->unit()->isUnderAttack() && (u->getState() == "Mineral" || u->getState() == "Gas" || u->getState() == "Idle"))
					{
						isDangerousBaseForWorkers = true;
						break;
					}
				}

				for (auto c : INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, base.Center(), 5 * TILE_SIZE, true))
				{
					if (c->unit()->isUnderAttack())
					{
						isDangerousBaseForWorkers = true;
						break;
					}
				}

				if (isDangerousBaseForWorkers) 
				{
					if (airUnitInMyBase && groundUnitInMyBase)
					{
						if (selfAirDefenseBuildingCount + selfAirDefenseUnitCount > 0 && selfGroundDefenseBuildingCount + selfGroundDefenseUnitCount > 0)
						{
							isDangerousBaseForWorkers = false;
						}
					}
					else if (airUnitInMyBase && !groundUnitInMyBase) 
					{
						if (selfAirDefenseBuildingCount + selfAirDefenseUnitCount > 0 || airUnitInMyBase < 3) 
						{
							isDangerousBaseForWorkers = false;
						}
					}
					else if (!airUnitInMyBase && groundUnitInMyBase) 
					{
						if (selfGroundDefenseBuildingCount + selfGroundDefenseUnitCount > 0)
						{
							isDangerousBaseForWorkers = false;
						}
					}
					else 
					{
						isDangerousBaseForWorkers = false;
					}
				}

				if (isDangerousBaseForWorkers)
					base.SetDangerousAreaForWorkers(isDangerousBaseForWorkers);

				bool baseInDanger = false;

				for (auto u : INFO.getUnitsInRadius(E, base.Center(), TILE_SIZE * 12, true, true, false))
				{
					if (u->type().canAttack())
					{
						baseInDanger = true;
						break;
					}
				}

				base.SetIsEnemyAround(baseInDanger);

				if (!baseInDanger)
					base.SetDangerousAreaForWorkers(false);
			}

			
			uList eList = INFO.getUnitsInRadius(E, base.getMineralBaseCenter(), 12 * TILE_SIZE, true, true, false);

			int damage = 0;
			Position avgPos = Position(0, 0);

			for (auto e : eList) {
				
				if (e->type() == Protoss_Reaver || e->type() == Protoss_Scarab || e->type() == Terran_Siege_Tank_Tank_Mode || e->type() == Protoss_High_Templar) {
					base.SetDangerousAreaForWorkers(true);
					damage = 0;
					break;
				}
				
				else if (e->type() == Zerg_Lurker && base.getMineralBaseCenter().getApproxDistance(e->pos()) < 7 * TILE_SIZE) {
					base.SetDangerousAreaForWorkers(true);
					damage = 0;
					break;
				}

				avgPos += e->pos();
				int d = getDamage(e->type(), Terran_SCV, E, S);
				damage += e->type() == Protoss_Zealot ? d * 2 : d;
			}

			

			if (!INFO.getTypeUnitsInRadius(Terran_Nuclear_Missile, E, base.getMineralBaseCenter(), 15 * TILE_SIZE).empty()) {
				base.SetDangerousAreaForWorkers(true);
				damage = 0;
			}

			if (!bw->getNukeDots().empty()) {
				for (auto p : bw->getNukeDots()) {
					if (isSameArea(p, base.Center()) || p.getApproxDistance(base.getMineralBaseCenter()) < 15 * TILE_SIZE) {
						base.SetDangerousAreaForWorkers(true);
						damage = 0;
						break;
					}
				}
			}

			if (damage >= 30) {
				avgPos /= eList.size();

				UnitInfo *c = getClosestTypeUnit(S, base.Center(), {Terran_Command_Center}, 7 * TILE_SIZE);

				if (c) {
					if (base.getEdgeMinerals().first != nullptr && base.getEdgeMinerals().second != nullptr)
						baseSafeMineralMap[base.getTilePosition()] = base.getEdgeMinerals().first->getPosition().getApproxDistance(avgPos)
																	 > base.getEdgeMinerals().second->getPosition().getApproxDistance(avgPos)
																	 ? base.getEdgeMinerals().first : base.getEdgeMinerals().second;
				}
			}

		
			if (INFO.enemyRace == Races::Zerg && base.GetLastVisitedTime() == 0 && !isEnemyBaseFound) {
				TilePosition upsize = base.getTilePosition() - TilePosition(1, 5);
				TilePosition leftsize = base.getTilePosition() - TilePosition(5, 1);
				TilePosition rightsize = base.getTilePosition() + TilePosition(8, -1);
				TilePosition downsize = base.getTilePosition() + TilePosition(-1, 7);

				
				for (int x = 0; x < 6; x++) {
					TilePosition u = TilePosition(upsize.x + x, upsize.y);
					TilePosition d = TilePosition(downsize.x + x, downsize.y);

					if ((bw->isBuildable(u) && bw->isVisible(u)) || (bw->isBuildable(d) && bw->isVisible(d))) {
						base.SetLastVisitedTime(TIME);
						base.SetHasCreepAroundBase(bw->hasCreep(u) || bw->hasCreep(d), TIME);
					}
				}

				if (base.GetLastVisitedTime() != TIME) {
					for (int y = 0; y < 5; y++) {
						TilePosition l = TilePosition(leftsize.x, leftsize.y + y);
						TilePosition r = TilePosition(rightsize.x, rightsize.y + y);

						if ((bw->isBuildable(l) && bw->isVisible(l)) || (bw->isBuildable(r) && bw->isVisible(r))) {
							base.SetLastVisitedTime(TIME);
							base.SetHasCreepAroundBase(bw->hasCreep(l) || bw->hasCreep(r), TIME);
						}
					}
				}
			}
			else {
				TilePosition rightBottomBase = base.getTilePosition() + Terran_Command_Center.tileSize() - 1;

				for (int x = base.getTilePosition().x; x <= rightBottomBase.x; x++) {
					for (int y = base.getTilePosition().y; y <= rightBottomBase.y; y++) {
					
						if (x == base.getTilePosition().x || x == rightBottomBase.x || y == base.getTilePosition().y || y == rightBottomBase.y) {
							if (bw->isVisible(x, y)) {
								base.SetLastVisitedTime(TIME);
								break;
							}
						}
					}

					if (base.GetLastVisitedTime() == TIME)
						break;
				}
			}
		}
	}



	try {
		updateBaseLocationInfo();

		if (isChangedOccupyInfo || TIME % 120 == 0) {
			updateExpectedMultiBases();
		}
	}
	
	catch (...) {
	}
}

Unit InformationManager::getSafestMineral(Unit scv) {
	if (theMap.GetArea((WalkPosition)scv->getPosition()) == nullptr)
		return nullptr;

	vector<Base> bases = theMap.GetArea((WalkPosition)scv->getPosition())->Bases();

	int minDist = INT_MAX;
	Base *closestBase = nullptr;

	for (auto &base : bases) {
		int dist = base.Center().getApproxDistance(scv->getPosition());

		if (dist < minDist) {
			minDist = dist;
			closestBase = &base;
		}
	}

	
	if (closestBase)
		if (baseSafeMineralMap.find(closestBase->getTilePosition()) != baseSafeMineralMap.end())
			return baseSafeMineralMap[closestBase->getTilePosition()];

	return nullptr;
}

void InformationManager::updateChokePointInfo()
{
	if (!isEnemyBaseFound)
		return;

	if (getFirstExpansionLocation(E) == nullptr || getFirstExpansionLocation(S) == nullptr)
		return;

	for (auto cps : theMap.GetPath(getFirstExpansionLocation(S)->getPosition(), getFirstExpansionLocation(E)->getPosition()))
	{




		cps->SetMineCount(getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, Position(cps->Center()), TILE_SIZE * 6).size());

		int enemyAirDefenseUnitCount = 0;
		int enemyGroundDefenseUnitCount = 0;

	
		for (auto u : INFO.getUnitsInRadius(E, Position(cps->Center()), TILE_SIZE * 6, true, true, true, true))
		{
			if (u->type().airWeapon().targetsAir())
			{
				enemyAirDefenseUnitCount++;
			}

			if (u->type().groundWeapon().targetsGround())
			{
				enemyGroundDefenseUnitCount++;
			}
		}

		int selfAirDefenseUnitCount = 0;
		int selfGroundDefenseUnitCount = 0;


		for (auto u : INFO.getUnitsInRadius(S, Position(cps->Center()), TILE_SIZE * 6, true, true, true, true))
		{
			if (u->type().airWeapon().targetsAir())
			{
				selfAirDefenseUnitCount++;
			}

			if (u->type().groundWeapon().targetsGround())
			{
				selfGroundDefenseUnitCount++;
			}
		}

	
		int enemyAirDefenseBuildingCount = 0;
		int enemyGroundDefenseBuildingCount = 0;

		int selfAirDefenseBuildingCount = 0;
		int selfGroundDefenseBuildingCount = 0;

	
		cps->SetEnemyAirDefenseUnitCount(enemyAirDefenseUnitCount);

		cps->SetEnemyGroundDefenseUnitCount(enemyGroundDefenseUnitCount);
	
		cps->SetSelfAirDefenseUnitCount(selfAirDefenseUnitCount);
	
		cps->SetSelfGroundDefenseUnitCount(selfGroundDefenseUnitCount);
		
		cps->SetEnemyAirDefenseBuildingCount(enemyAirDefenseBuildingCount);
		
		cps->SetEnemyGroundDefenseBuildingCount(enemyGroundDefenseBuildingCount);
	
		cps->SetSelfAirDefenseBuildingCount(selfAirDefenseBuildingCount);
		
		cps->SetSelfGroundDefenseBuildingCount(selfGroundDefenseBuildingCount);

		
		cps->SetIsEnemyAround(enemyAirDefenseUnitCount || enemyGroundDefenseUnitCount);


	}
}

void InformationManager::updateUnitsInfo()
{
	_unitData[selfPlayer].initializeAllInfo();
	_unitData[enemyPlayer].updateNcheckTypeAllInfo();
	_unitData[selfPlayer].updateAllInfo();
}

void InformationManager::onUnitCreate(Unit unit)
{
	if (unit->getType().isNeutral())
		return;


	if (unit->getType().isBuilding())
		_unitData[S].addUnitNBuilding(unit);

	_unitData[S].increaseCreateUnits(unit->getType());
}

void InformationManager::onUnitShow(Unit unit)
{
	if (unit->getType().isNeutral())
		return;

	_unitData[E].addUnitNBuilding(unit);
}

void InformationManager::onUnitComplete(Unit unit)
{
	if (unit->getType().isNeutral())
		return;

	if (unit->getPlayer() == S)
	{
		if (_unitData[S].addUnitNBuilding(unit)) 
			_unitData[S].increaseCompleteUnits(unit->getType());
		else if (unit->getType().isBuilding()) { 
			_unitData[S].increaseCompleteUnits(unit->getType());
			
			ReserveBuilding::Instance().freeTiles(unit->getTilePosition(), unit->getType());
		}

		if (unit->getType() == Terran_Command_Center)
			addAdditionalExpansion(unit);
	}
	else if (unit->getPlayer() == E) 
	{
		_unitData[E].addUnitNBuilding(unit);

		if (enemyRace == Races::Unknown) {
			enemyRace = unit->getType().getRace();

			
			if (selfRace == Races::Terran && enemyRace != Races::Protoss) {
				SelfBuildingPlaceFinder::Instance().freeSecondSupplyDepot();
			}
		}
	}
}


void InformationManager::onUnitDestroy(Unit unit)
{
	if (unit->getType().isNeutral())
		return;

	if (unit->getType().isAddon()) 
	{
		_unitData[S].removeUnitNBuilding(unit);
		_unitData[E].removeUnitNBuilding(unit);
		return;
	}

	_unitData[unit->getPlayer()].removeUnitNBuilding(unit);

	if (unit->getPlayer() == S && unit->getType() == Terran_Command_Center)
		removeAdditionalExpansion(unit);
}

bool InformationManager::isCombatUnitType(UnitType type) const
{

	if (type.canAttack() ||
			type == Terran_Medic ||
			type == Protoss_Observer ||
			type == Terran_Bunker ||
			type == type == Zerg_Lurker)
	{
		return true;
	}

	return false;
}

bool InformationManager::isConnected(Position a, Position b)
{
	int distance;
	theMap.GetPath(a, b, &distance);

	if (distance < 0)	return false;

	return true;
}

InformationManager &InformationManager::Instance()
{
	static InformationManager instance;
	return instance;
}


void InformationManager::updateBaseLocationInfo() {

	updateSelfBaseLocationInfo();

	try {
	
		updateEnemyBaseLocationInfo();
	}
	catch (...) {
	}


	
	updateOccupiedAreas(enemyPlayer);
	updateOccupiedAreas(selfPlayer);
	updateChangedMainBaseInfo(selfPlayer);
	updateChangedMainBaseInfo(enemyPlayer);
}


void InformationManager::updateSelfBaseLocationInfo() {
	if (!existsPlayerBuildingInArea(theMap.GetArea(_mainBaseLocations[S]->Location()), S)) {
		for (list<const Base *>::const_iterator iterator = _occupiedBaseLocations[S].begin(), end = _occupiedBaseLocations[S].end(); iterator != end; ++iterator) {
			if (existsPlayerBuildingInArea(theMap.GetArea((*iterator)->Location()), S)) {
				_mainBaseLocations[S] = *iterator;
				_mainBaseLocationChanged[S] = true;
				break;
			}
		}
	}
}

void InformationManager::updateEnemyBaseLocationInfo() {
	
	if (isEnemyBaseFound) {
	
		if (_mainBaseLocations[E]->GetLastVisitedTime() > 0
				&& !existsPlayerBuildingInArea(_mainBaseLocations[E]->GetArea(), E)
				&& INFO.getAllCount(Spell_Scanner_Sweep, S) == 0
				&& _mainBaseLocations[E]->GetLastCreepFoundTime() + 240 < TIME) {

			const Base *tmpBase = nullptr;

			for (list<const Base *>::const_iterator iterator = _occupiedBaseLocations[E].begin(), end = _occupiedBaseLocations[E].end(); iterator != end; ++iterator) {
				if (existsPlayerBuildingInArea((*iterator)->GetArea(), E)) {
					if ((*iterator)->Starting()) {
						_mainBaseLocations[E] = *iterator;
						_mainBaseLocationChanged[E] = true;
						cout << "[" << TIME << "] _mainBaseLocations[enemyPlayer] changed by destruction as " << _mainBaseLocations[E]->Location() << endl;
						return;
					}

					if (!tmpBase)
						tmpBase = *iterator;
				}
			}

			if (tmpBase) {
				_mainBaseLocations[E] = tmpBase;
				_mainBaseLocationChanged[E] = true;
				cout << "[" << TIME << "] _mainBaseLocations[enemyPlayer] changed by destruction as " << _mainBaseLocations[E]->Location() << endl;
				return;
			}


		
			isEnemyBaseFound = false;
		}
	}

	if (!isEnemyBaseFound) {
	
		for (auto &b : getStartLocations()) {
			if (b == _mainBaseLocations[S])
				continue;

			if (existsPlayerBuildingInArea(b->GetArea(), E) || b->GetHasCreepAroundBase()) {
				if (getMainBaseLocation(E)->Center() != b->Center()) {
					cout << "update by another scouter " << b->getTilePosition() << endl;
					_mainBaseLocations[E] = b;
					_mainBaseLocationChanged[E] = true;
					_occupiedBaseLocations[E].push_back(b);
				}

				cout << "enemy base found" << endl;
				isEnemyBaseFound = true;
				
				b->ClearHasCreepAroundBase();
				return;
			}
		}

	
		int exploredStartLocations = 0;

		Base *unexplored = nullptr;

		uMap &eBuildings = getBuildings(E);
		int pathLength;

		for (Base *startLocation : INFO.getStartLocations())
		{
			if (startLocation == _mainBaseLocations[S])
				continue;

		
			for (auto &b : eBuildings) {
				if (b.second->pos() == Positions::Unknown)
					continue;

				theMap.GetPath(b.second->pos(), startLocation->Center(), &pathLength);

				if (isInFirstExpansionLocation(startLocation, (TilePosition)b.second->pos()) || (pathLength >= 0 && pathLength <= 1600)) {
					cout << "updateBaseLocationInfo" << startLocation->getTilePosition() << endl;
					_mainBaseLocations[E] = startLocation;
					_mainBaseLocationChanged[E] = true;
					_occupiedBaseLocations[E].push_back(startLocation);
					isEnemyBaseFound = true;
					return;
				}

			}

			if (startLocation->GetLastVisitedTime())
				exploredStartLocations++;
	
			else
				unexplored = startLocation;
		}

		if (exploredStartLocations == (mapPlayerLimit - 2) && unexplored)
		{
			cout << "updateBaseLocationInfo2" << unexplored->getTilePosition() << endl;
			_mainBaseLocations[E] = unexplored;
			_mainBaseLocationChanged[E] = true;
			_occupiedBaseLocations[E].push_back(unexplored);
			isEnemyBaseFound = true;
			return;
		}

	
		if (getMainBaseLocation(E)->GetLastVisitedTime() && getMainBaseLocation(E)->GetLastVisitedTime() + 20 > TIME) {
			_mainBaseLocations[E] = getNextSearchBase();
			_mainBaseLocationChanged[E] = true;
			cout << "update to NextSearchBase" << _mainBaseLocations[E]->getTilePosition() << endl;
		}
	}
}

const Base *InformationManager::getNextSearchBase(bool reverse) {

	if (!reverse) {
		auto iter = _startBaseLocations.begin();

		while (iter != _startBaseLocations.end() && (*iter)->Location() != _mainBaseLocations[S]->Location()) {
			iter++;
		}

		while (iter != _startBaseLocations.end())
		{
			if (!(*iter)->GetLastVisitedTime() && (*iter)->Location() != _mainBaseLocations[S]->Location())
				return *iter;

			iter++;
		}

		for (auto bl : _startBaseLocations)
		{
			if (bl->Location() == _mainBaseLocations[S]->Location())	break;

			if (!bl->GetLastVisitedTime())
				return bl;
		}
	}

	else {
		auto iter = _startBaseLocations.rbegin();

		while (iter != _startBaseLocations.rend() && (*iter)->Location() != _mainBaseLocations[S]->Location()) {
			iter++;
		}

		while (iter != _startBaseLocations.rend())
		{
			if (!(*iter)->GetLastVisitedTime() && (*iter)->Location() != _mainBaseLocations[S]->Location())
				return *iter;

			iter++;
		}

		iter = _startBaseLocations.rbegin();

		while (iter != _startBaseLocations.rend())
		{
			if ((*iter)->Location() == _mainBaseLocations[S]->Location())	break;

			if (!(*iter)->GetLastVisitedTime())
				return *iter;

			iter++;
		}
	}


	const Base *rBase = getMainBaseLocation(S);

	for (auto &base : _allBaseLocations) {
		if (base->isIsland())
			continue;

		if (!base->GetLastVisitedTime())
			return base;

		if (rBase->GetLastVisitedTime() > base->GetLastVisitedTime())
			rBase = base;
	}

	return rBase;
}

void InformationManager::updateChangedMainBaseInfo(Player player)
{
	if (_mainBaseLocationChanged[player] == true) {
		_mainBaseLocationChanged[player] = false;
		_firstChokePoint[player] = nullptr;
		_firstExpansionLocation[player] = nullptr;
		_secondChokePoint[player] = nullptr;

		if (_mainBaseLocations[player]) {
		
			const Area *baseArea = theMap.GetArea(_mainBaseLocations[player]->Location());

			try {
				updateChokePointAndExpansionLocation(*baseArea, player);
			}
			catch (...) {
			}
		}
	}
}

bool InformationManager::updateChokePointAndExpansionLocation(const Area &baseArea, Player player, vector<const Area *> passedAreas) {
	passedAreas.push_back(&baseArea);

	for (auto &cba : baseArea.ChokePointsByArea()) {
		if (checkPassedArea(*cba.first, passedAreas))
			continue;

	
		if (!cba.first->Bases().empty()) {
	
			for (auto &cp : *cba.second) {
				int dist = cp.Pos(ChokePoint::end1).getApproxDistance(cp.Pos(ChokePoint::middle));
				dist += cp.Pos(ChokePoint::end2).getApproxDistance(cp.Pos(ChokePoint::middle));

			
				if (dist < 39) {
					for (Base *targetBaseLocation : INFO.getBaseLocations()) {
						for (auto &base : cba.first->Bases()) {
							if (base.Location() == targetBaseLocation->Location()) {
							
								_firstExpansionLocation[player] = targetBaseLocation;
							
								_firstChokePoint[player] = &cp;

								word depth = 0;

								for (auto &expansionAreaCP : cba.first->ChokePoints()) {
								
									for (auto &baseAreaCP : *cba.second) {
										if (expansionAreaCP->Center() != baseAreaCP.Center()) {
										
											if (_secondChokePoint[player] == nullptr) {
												_secondChokePoint[player] = expansionAreaCP;

												depth = theMap.GetPath((Position)expansionAreaCP->Center(), theMap.Center(), nullptr, false).size();
											}
											else {
												word tmpDepth = theMap.GetPath((Position)expansionAreaCP->Center(), theMap.Center(), nullptr, false).size();

												if (tmpDepth <= depth) {
													depth = tmpDepth;

													if (_mainBaseLocations[player]->getPosition().getApproxDistance((Position)_secondChokePoint[player]->Center())
															> _mainBaseLocations[player]->getPosition().getApproxDistance((Position)expansionAreaCP->Center())) {
														_secondChokePoint[player] = expansionAreaCP;
													}
												}
											}
										}
									}
								}

								return true;
							}
						}
					}

					return true;
				}
			}
		}
	}


	for (auto &cba : baseArea.ChokePointsByArea()) {
		if (checkPassedArea(*cba.first, passedAreas))
			continue;

		if (updateChokePointAndExpansionLocation(*cba.first, player, passedAreas)) {
			return true;
		}
	}

	return false;
}


bool InformationManager::checkPassedArea(const Area &area, vector<const Area *> passedAreas) {
	for (auto passedArea : passedAreas) {
		if (passedArea->Id() == area.Id()) {
			return true;
		}
	}

	return false;
}

void InformationManager::updateOccupiedAreas(Player player)
{
	_occupiedAreas[player].clear();

	for (auto &b : getBuildings(player))
	{
		if (b.second->pos().isValid()) {
			Area *area = (Area *)theMap.GetArea(TilePosition(b.second->pos()));

			if (area)
			{
				_occupiedAreas[player].insert(area);
			}
		}
	}
}


bool InformationManager::hasBuildingAroundBaseLocation(Base *baseLocation, Player player, int radius)
{
	
	if (!baseLocation)
	{
		return false;
	}

	for (auto &b : getBuildings(player))
	{
		if (b.second->pos() == Positions::Unknown)		continue;

		TilePosition buildingPosition(b.second->pos());

		
		if (theMap.GetArea(buildingPosition) != baseLocation->GetArea())	continue;



		if (buildingPosition.x >= baseLocation->Location().x - radius && buildingPosition.x <= baseLocation->Location().x + radius
				&& buildingPosition.y >= baseLocation->Location().y - radius && buildingPosition.y <= baseLocation->Location().y + radius)
		{
			if (player == E)
			{
				isEnemyBaseFound = true;
			}

			return true;
		}
	}

	return false;
}

bool InformationManager::existsPlayerBuildingInArea(const Area *area, Player player)
{

	if (area == nullptr || player == nullptr)
	{
		return false;
	}

	for (auto &b : getBuildings(player))
	{
		if (b.second->pos() == Positions::Unknown)		continue;

		if (isSameArea(theMap.GetArea(TilePosition(b.second->pos())), area))
		{
			return true;
		}
	}

	return false;
}

bool InformationManager::isInFirstExpansionLocation(Base *startLocation, TilePosition pos)
{

	if (_firstExpansionOfAllStartposition[startLocation] == nullptr) {
		return false;
	}

	return isSameArea(_firstExpansionOfAllStartposition[startLocation]->getTilePosition(), pos);

}



set<Area *> &InformationManager::getOccupiedAreas(Player player)
{
	return _occupiedAreas[player];
}

list<const Base *> &InformationManager::getOccupiedBaseLocations(Player player)
{
	return _occupiedBaseLocations[player];
}

const Base *InformationManager::getMainBaseLocation(Player player)
{
	return _mainBaseLocations[player];
}

const ChokePoint *InformationManager::getFirstChokePoint(Player player)
{
	return _firstChokePoint[player];
}
Base *InformationManager::getFirstExpansionLocation(Player player)
{
	return _firstExpansionLocation[player];
}

Base *InformationManager::getSecondExpansionLocation(Player player)
{
	return _secondExpansionLocation[player];
}

Base *InformationManager::getThirdExpansionLocation(Player player)
{
	return _thirdExpansionLocation[player];
}

Base *InformationManager::getIslandExpansionLocation(Player player)
{
	return _islandExpansionLocation[player];
}

vector<Base *> InformationManager::getAdditionalExpansions()
{
	return _additionalExpansions;
}

void InformationManager::setAdditionalExpansions(Base *base)
{
	if (base == nullptr)
		return;

	Base *multiBase = nullptr;

	auto targetBase = find_if(_additionalExpansions.begin(), _additionalExpansions.end(), [multiBase](Base * base) {
		return multiBase == base;

	});

	if (targetBase == _additionalExpansions.end())
		_additionalExpansions.push_back(multiBase);
}

void InformationManager::removeAdditionalExpansionData(Unit depot)
{
	removeAdditionalExpansion(depot);
}

const ChokePoint *InformationManager::getSecondChokePoint(Player player)
{
	return _secondChokePoint[player];
}

UnitType InformationManager::getBasicCombatUnitType(Race race)
{
	if (race == Races::None) {
		race = selfRace;
	}

	if (race == Races::Protoss) {
		return Protoss_Zealot;
	}
	else if (race == Races::Terran) {
		return Terran_Marine;
	}
	else if (race == Races::Zerg) {
		return Zerg_Zergling;
	}
	else {
		return None;
	}
}

UnitType InformationManager::getAdvancedCombatUnitType(Race race)
{
	if (race == Races::None) {
		race = selfRace;
	}

	if (race == Races::Protoss) {
		return Protoss_Dragoon;
	}
	else if (race == Races::Terran) {
		return Terran_Goliath;
	}
	else if (race == Races::Zerg) {
		return Zerg_Hydralisk;
	}
	else {
		return None;
	}
}

UnitType InformationManager::getBasicCombatBuildingType(Race race)
{
	if (race == Races::None) {
		race = selfRace;
	}

	if (race == Races::Protoss) {
		return Protoss_Gateway;
	}
	else if (race == Races::Terran) {
		return Terran_Barracks;
	}
	else if (race == Races::Zerg) {
		return Zerg_Hatchery;
	}
	else {
		return None;
	}
}

UnitType InformationManager::getObserverUnitType(Race race)
{
	if (race == Races::None) {
		race = selfRace;
	}

	if (race == Races::Protoss) {
		return Protoss_Observer;
	}
	else if (race == Races::Terran) {
		return Terran_Science_Vessel;
	}
	else if (race == Races::Zerg) {
		return Zerg_Overlord;
	}
	else {
		return None;
	}
}

UnitType	InformationManager::getBasicResourceDepotBuildingType(Race race)
{
	if (race == Races::None) {
		race = selfRace;
	}

	if (race == Races::Protoss) {
		return Protoss_Nexus;
	}
	else if (race == Races::Terran) {
		return Terran_Command_Center;
	}
	else if (race == Races::Zerg) {
		return Zerg_Hatchery;
	}
	else {
		return None;
	}
}
UnitType InformationManager::getRefineryBuildingType(Race race)
{
	if (race == Races::None) {
		race = selfRace;
	}

	if (race == Races::Protoss) {
		return Protoss_Assimilator;
	}
	else if (race == Races::Terran) {
		return Terran_Refinery;
	}
	else if (race == Races::Zerg) {
		return Zerg_Extractor;
	}
	else {
		return None;
	}

}

UnitType	InformationManager::getWorkerType(Race race)
{
	if (race == Races::None) {
		race = selfRace;
	}

	if (race == Races::Protoss) {
		return Protoss_Probe;
	}
	else if (race == Races::Terran) {
		return Terran_SCV;
	}
	else if (race == Races::Zerg) {
		return Zerg_Drone;
	}
	else {
		return None;
	}
}

UnitType InformationManager::getBasicSupplyProviderUnitType(Race race)
{
	if (race == Races::None) {
		race = selfRace;
	}

	if (race == Races::Protoss) {
		return Protoss_Pylon;
	}
	else if (race == Races::Terran) {
		return Terran_Supply_Depot;
	}
	else if (race == Races::Zerg) {
		return Zerg_Overlord;
	}
	else {
		return None;
	}
}

UnitType InformationManager::getAdvancedDefenseBuildingType(Race race, bool isAirDefense)
{
	if (race == Races::None) {
		race = selfRace;
	}

	if (race == Races::Protoss) {
		return Protoss_Photon_Cannon;
	}
	else if (race == Races::Terran) {
		return isAirDefense ? Terran_Missile_Turret : Terran_Bunker;
	}
	else if (race == Races::Zerg) {
		return isAirDefense ? Zerg_Spore_Colony : Zerg_Sunken_Colony;
	}
	else {
		return None;
	}
}




UnitInfo *InformationManager::getUnitInfo(Unit unit, Player p)
{
	if (unit == nullptr)
		return nullptr;

	if (unit->getType() == UnitTypes::Unknown) {
		if (_unitData[p].getAllBuildings().find(unit) != _unitData[p].getAllBuildings().end())
			return _unitData[p].getAllBuildings()[unit];

		if (_unitData[p].getAllUnits().find(unit) != _unitData[p].getAllUnits().end())
			return _unitData[p].getAllUnits()[unit];

		return nullptr;
	}

	uMap &AllUnitMap = unit->getType().isBuilding() ? _unitData[p].getAllBuildings() : _unitData[p].getAllUnits();

	if (AllUnitMap.find(unit) != AllUnitMap.end())
		return AllUnitMap[unit];
	else
		return nullptr;
}


int InformationManager::getCompletedCount(UnitType t, Player p)
{
	if (p == S)
		return _unitData[S].getCompletedCount(t);
	else
	{
		int compleCnt = 0;

		if (t.isBuilding()) {
			for (auto u : _unitData[E].getBuildingVector(t)) {
				if (u->isComplete())
					compleCnt++;
			}
		}
		else {
			for (auto u : _unitData[E].getUnitVector(t)) {
				if (u->isComplete())
					compleCnt++;
			}
		}

		return compleCnt;
	}
}


int InformationManager::getDestroyedCount(UnitType t, Player p)
{
	return _unitData[p].getDestroyedCount(t);
}


map<UnitType, int> InformationManager::getDestroyedCountMap(Player p)
{
	return _unitData[p].getDestroyedCountMap();
}


int InformationManager::getAllCount(UnitType t, Player p)
{
	if (p == S)
		return _unitData[S].getAllCount(t);
	else
	{
		if (t.isBuilding())
			return _unitData[E].getBuildingVector(t).size();
		else
			return _unitData[E].getUnitVector(t).size();
	}
}


uList InformationManager::getUnitsInRadius(Player p, Position pos, int radius, bool ground, bool air, bool worker, bool hide, bool groundDistance)
{
	uList units;

	uMap allUnits = _unitData[p].getAllUnits();

	for (auto &u : allUnits)
	{
		if (u.second->type() != Zerg_Lurker && hide == false && u.second->isHide())
			continue;

	
		if (u.second->type() == Terran_Vulture_Spider_Mine)
			continue;

		if (air == false && u.second->getLift())
			continue;

		if (ground == false && !u.second->getLift())
			continue;

		if (worker == false && u.second->type().isWorker())
			continue;

		if (radius)
		{
			if (u.second->pos() == Positions::Unknown)
				continue;

			if (groundDistance)
			{
				int dist = 0;
				theMap.GetPath(pos, u.second->pos(), &dist);

				if (dist < 0 || dist > radius)
					continue;

				units.push_back(u.second);
			}
			else
			{
				Position newPos = pos - u.second->pos();

				if (abs(newPos.x) > radius || abs(newPos.y) > radius)
					continue;

				if (abs(newPos.x) + abs(newPos.y) <= radius)
					units.push_back(u.second);
				else
				{
					if ((newPos.x * newPos.x) + (newPos.y * newPos.y) <= radius * radius)
						units.push_back(u.second);
				}
			}
		}
		else {
			units.push_back(u.second);
		}

	}

	return units;
}

uList InformationManager::getUnitsInRectangle(Player p, Position leftTop, Position rightDown, bool ground, bool air, bool worker, bool hide)
{
	uList units;

	uMap allUnits = _unitData[p].getAllUnits();

	for (auto &u : allUnits)
	{
		if (u.second->type() != Zerg_Lurker && hide == false && u.second->isHide())
			continue;

		if (air == false && u.second->getLift())
			continue;

		if (ground == false && !u.second->getLift())
			continue;

		if (worker == false && u.second->type().isWorker())
			continue;

		if (u.second->pos() == Positions::Unknown)
			continue;

		int Threshold_L = leftTop.x;
		int Threshold_R = rightDown.x;
		int Threshold_T = leftTop.y;
		int Threshold_D = rightDown.y;

		if (u.second->pos().y > Threshold_D || u.second->pos().y < Threshold_T || u.second->pos().x > Threshold_R || u.second->pos().x  < Threshold_L)
			continue;

		units.push_back(u.second);
	}

	return units;
}

uList InformationManager::getBuildingsInRadius(Player p, Position pos, int radius, bool ground, bool air, bool hide, bool groundDistance)
{
	uList buildings;

	uMap allBuildings = _unitData[p].getAllBuildings();

	for (auto &u : allBuildings)
	{
		if (hide == false && u.second->isHide())
			continue;

		if (air == false && u.second->getLift())
			continue;

		if (ground == false && !u.second->getLift())
			continue;

		if (radius)
		{
			if (u.second->pos() == Positions::Unknown)
				continue;

			if (groundDistance)
			{
				int dist = 0;
				theMap.GetPath(pos, u.second->pos(), &dist);

				if (dist < 0 || dist > radius)
					continue;

				buildings.push_back(u.second);
			}
			else
			{
				Position newPos = pos - u.second->pos();

				if (abs(newPos.x) > radius || abs(newPos.y) > radius)
					continue;

				if (abs(newPos.x) + abs(newPos.y) <= radius)
					buildings.push_back(u.second);
				else
				{
					if ((newPos.x * newPos.x) + (newPos.y * newPos.y) <= radius * radius)
						buildings.push_back(u.second);
				}
			}
		}
		else
		{
			buildings.push_back(u.second);
		}
	}

	return buildings;
}

uList InformationManager::getBuildingsInRectangle(Player p, Position leftTop, Position rightDown, bool ground, bool air, bool hide)
{
	uList buildings;

	uMap allBuildings = _unitData[p].getAllBuildings();

	for (auto &u : allBuildings)
	{
		if (u.second->pos() == Positions::Unknown)
			continue;

		if (hide == false && u.second->isHide())
			continue;

		if (air == false && u.second->getLift())
			continue;

		if (ground == false && !u.second->getLift())
			continue;

		int Threshold_L = leftTop.x;
		int Threshold_R = rightDown.x;
		int Threshold_T = leftTop.y;
		int Threshold_D = rightDown.y;

		if (u.second->unit()->getTop() >= Threshold_D || u.second->unit()->getBottom() <= Threshold_T || u.second->unit()->getLeft() >= Threshold_R || u.second->unit()->getRight() <= Threshold_L)
			continue;

		buildings.push_back(u.second);
	}

	return buildings;
}

uList InformationManager::getUnitsInArea(Player p, Position pos, bool ground, bool air, bool worker, bool hide)
{
	uList units;

	uMap allUnits = _unitData[p].getAllUnits();

	for (auto &u : allUnits)
	{
		if (hide == false && u.second->isHide())
			continue;

		if (air == false && u.second->getLift())
			continue;

		if (ground == false && !u.second->getLift())
			continue;

		if (worker == false && u.second->type().isWorker())
			continue;

		if (u.second->pos() == Positions::Unknown )
			continue;

		if (isSameArea(pos, u.second->pos()))
			units.push_back(u.second);
	}

	return units;
}

uList InformationManager::getBuildingsInArea(Player p, Position pos, bool ground, bool air, bool hide)
{
	uList buildings;

	for (auto &u : _unitData[p].getAllBuildings())
	{
		if (hide == false && u.second->isHide())
			continue;

		if (air == false && u.second->getLift())
			continue;

		if (ground == false && !u.second->getLift())
			continue;

		if (u.second->pos() == Positions::Unknown)
			continue;

		if (isSameArea(pos, u.second->pos()))
			buildings.push_back(u.second);
	}

	return buildings;
}

uList InformationManager::getAllInRadius(Player p, Position pos, int radius, bool ground, bool air, bool hide, bool groundDist)
{
	uList units = getUnitsInRadius(p, pos, radius, ground, air, true, hide, groundDist);
	uList buildings = getBuildingsInRadius(p, pos, radius, ground, air, hide, groundDist);
	units.insert(units.end(), buildings.begin(), buildings.end());
	return units;
}

uList InformationManager::getAllInRectangle(Player p, Position leftTop, Position rightDown, bool ground, bool air, bool hide)
{
	uList units = getUnitsInRectangle(p, leftTop, rightDown, ground, air, true, hide);
	uList buildings = getBuildingsInRectangle(p, leftTop, rightDown, ground, air, hide);
	units.insert(units.end(), buildings.begin(), buildings.end());
	return units;
}

uList InformationManager::getTypeUnitsInRadius(UnitType t, Player p, Position pos, int radius, bool hide)
{
	uList units;

	for (auto u : _unitData[p].getUnitVector(t))
	{
		if (hide == false && u->isHide())
			continue;

		if (radius)
		{
			if (u->pos() == Positions::Unknown)
				continue;

			Position newPos = pos - u->pos();

			if (abs(newPos.x) > radius || abs(newPos.y) > radius)
				continue;

			if (abs(newPos.x) + abs(newPos.y) <= radius)
				units.push_back(u);
			else
			{
				if ((newPos.x * newPos.x) + (newPos.y * newPos.y) <= radius * radius)
					units.push_back(u);
			}
		}
		else
		{
			units.push_back(u);
		}
	}

	return units;
}
uList InformationManager::getTypeBuildingsInRadius(UnitType t, Player p, Position pos, int radius, bool incomplete, bool hide)
{
	uList buildings;

	for (auto u : _unitData[p].getBuildingVector(t))
	{
		if (hide == false && u->isHide())
			continue;

		if (incomplete == false && (!u->isComplete() || u->isMorphing()))
			continue;

		if (radius)
		{
			if (u->pos() == Positions::Unknown)
				continue;

			Position newPos = pos - u->pos();

			if (abs(newPos.x) > radius || abs(newPos.y) > radius)
				continue;

			if (abs(newPos.x) + abs(newPos.y) <= radius)
				buildings.push_back(u);
			else
			{
				if ((newPos.x * newPos.x) + (newPos.y * newPos.y) <= radius * radius)
					buildings.push_back(u);
			}
		}
		else
		{
			buildings.push_back(u);
		}
	}

	return buildings;
}

uList InformationManager::getTypeUnitsInRectangle(UnitType t, Player p, Position leftTop, Position rightDown, bool hide)
{
	uList units;

	for (auto u : _unitData[p].getUnitVector(t))
	{
		if (u->pos() == Positions::Unknown)
			continue;

		if (hide == false && u->isHide())
			continue;

		int Threshold_L = leftTop.x;
		int Threshold_R = rightDown.x;
		int Threshold_T = leftTop.y;
		int Threshold_D = rightDown.y;

		if (u->pos().y > Threshold_D || u->pos().y < Threshold_T || u->pos().x > Threshold_R || u->pos().x  < Threshold_L)
			continue;

		units.push_back(u);
	}

	return units;
}

uList InformationManager::getTypeBuildingsInRectangle(UnitType t, Player p, Position leftTop, Position rightDown, bool incomplete, bool hide)
{
	uList buildings;

	for (auto u : _unitData[p].getBuildingVector(t))
	{
		if (u->pos() == Positions::Unknown)
			continue;

		if (hide == false && u->isHide())
			continue;

		if (incomplete == false && (!u->isComplete() || u->isMorphing()))
			continue;

		int Threshold_L = leftTop.x;
		int Threshold_R = rightDown.x;
		int Threshold_T = leftTop.y;
		int Threshold_D = rightDown.y;

		if (u->unit()->getTop() >= Threshold_D || u->unit()->getBottom() <= Threshold_T || u->unit()->getLeft() >= Threshold_R || u->unit()->getRight() <= Threshold_L)
			continue;

		buildings.push_back(u);
	}

	return buildings;
}

uList InformationManager::getTypeUnitsInArea(UnitType t, Player p, Position pos, bool hide)
{
	uList units;

	for (auto u : _unitData[p].getUnitVector(t))
	{
		if (hide == false && u->isHide())
			continue;

		if (u->pos() == Positions::Unknown)
			continue;

		if (isSameArea(u->pos(), pos))
			units.push_back(u);
	}

	return units;
}

uList InformationManager::getTypeBuildingsInArea(UnitType t, Player p, Position pos, bool incomplete, bool hide)
{
	uList buildings;

	for (auto u : _unitData[p].getBuildingVector(t))
	{
		if (hide == false && u->isHide())
			continue;

		if (incomplete == false && (!u->isComplete() || u->isMorphing()))
			continue;

		if (u->pos() == Positions::Unknown)
			continue;

		if (isSameArea(u->pos(), pos))
			buildings.push_back(u);
	}

	return buildings;
}

uList InformationManager::getDefenceBuildingsInRadius(Player p, Position pos, int radius, bool incomplete, bool hide)
{
	UnitType t = p == S ? getAdvancedDefenseBuildingType(INFO.selfRace) : getAdvancedDefenseBuildingType(INFO.enemyRace);
	return getTypeBuildingsInRadius(t, p, pos, radius, incomplete, hide);
}

UnitInfo *InformationManager::getClosestUnit(Player p, Position pos, TypeKind kind, int radius, bool worker, bool hide, bool groundDistance, bool detectedOnly)
{
	UnitInfo *closest = nullptr;
	int closestDist = INT_MAX;

	if (kind == TypeKind::AllUnitKind || kind == TypeKind::AirUnitKind || kind == TypeKind::GroundUnitKind || kind == TypeKind::GroundCombatKind || kind == TypeKind::AllKind)
	{
		uMap allUnits = _unitData[p].getAllUnits();

		for (auto &u : allUnits)
		{
			if (u.second->type().isFlyer() && (kind == GroundUnitKind || kind == GroundCombatKind))
				continue;

			if (!u.second->getLift() && kind == AirUnitKind)
				continue;

			if (u.second->type().isWorker() && worker == false)
				continue;

			if (hide == false && u.second->isHide())
				continue;

			if (u.second->pos() == Positions::Unknown)
				continue;

			if (detectedOnly && !u.second->isHide() && !u.second->unit()->isDetected())
				continue;

		
			if (u.second->type() == Zerg_Egg || u.second->type() == Zerg_Larva || u.second->type() == Protoss_Interceptor ||
					u.second->type() == Protoss_Scarab || u.second->type() == Terran_Vulture_Spider_Mine)
				continue;

			Position newPos = pos - u.second->pos();

			if (groundDistance)
			{
				int dist = 0;
				theMap.GetPath(pos, u.second->pos(), &dist);

				if (radius && dist > radius)
					continue;

				if (dist > 0 && dist < closestDist)
				{
					closest = u.second;
					closestDist = dist;
				}
			}
			else
			{
				if (radius)
				{
					if (abs(newPos.x) > radius || abs(newPos.y) > radius)
						continue;

					if ((newPos.x * newPos.x) + (newPos.y * newPos.y) > radius * radius)
						continue;
				}

				if ((newPos.x * newPos.x + newPos.y * newPos.y) < closestDist)
				{
					closest = u.second;
					closestDist = (newPos.x * newPos.x + newPos.y * newPos.y);
				}
			}
		}
	}

	if (kind == TypeKind::BuildingKind || kind == TypeKind::AllDefenseBuildingKind || kind == TypeKind::AirDefenseBuildingKind || kind == TypeKind::GroundDefenseBuildingKind || kind == TypeKind::AllKind || kind == GroundCombatKind)
	{
		uMap allBuildings = _unitData[p].getAllBuildings();

		for (auto &u : allBuildings)
		{
			if (hide == false && u.second->isHide())
				continue;

			if (u.second->pos() == Positions::Unknown)
				continue;

			if (u.second->isMorphing() || !u.second->isComplete())
				continue;

			if (!u.second->type().airWeapon().targetsAir() && u.second->type() != Terran_Bunker && (kind == AirDefenseBuildingKind || kind == AllDefenseBuildingKind))
				continue;

			if (!u.second->type().groundWeapon().targetsGround() && u.second->type() != Terran_Bunker && (kind == GroundDefenseBuildingKind || kind == AllDefenseBuildingKind || kind == GroundCombatKind))
				continue;

			Position newPos = pos - u.second->pos();

			if (groundDistance)
			{
				int dist = 0;
				theMap.GetPath(pos, u.second->pos(), &dist);

				if (radius && dist > radius)
					continue;

				if (dist > 0 && dist < closestDist)
				{
					closest = u.second;
					closestDist = dist;
				}
			}
			else
			{
				if (radius)
				{
					if (abs(newPos.x) > radius || abs(newPos.y) > radius)
						continue;

					if ((newPos.x * newPos.x) + (newPos.y * newPos.y) > radius * radius)
						continue;
				}

				if ((newPos.x * newPos.x + newPos.y * newPos.y) < closestDist)
				{
					closest = u.second;
					closestDist = (newPos.x * newPos.x + newPos.y * newPos.y);
				}
			}
		}
	}

	return closest;
}

UnitInfo *InformationManager::getFarthestUnit(Player p, Position pos, TypeKind kind, int radius, bool worker, bool hide, bool groundDistance, bool detectedOnly)
{
	UnitInfo *closest = nullptr;
	int closestDist = -1;

	if (kind == TypeKind::AllUnitKind || kind == TypeKind::AirUnitKind || kind == TypeKind::GroundUnitKind || kind == TypeKind::GroundCombatKind || kind == TypeKind::AllKind)
	{
		uMap allUnits = _unitData[p].getAllUnits();

		for (auto &u : allUnits)
		{
			if (u.second->type().isFlyer() && (kind == GroundUnitKind || kind == GroundCombatKind))
				continue;

			if (!u.second->unit()->isFlying() && kind == AirUnitKind)
				continue;

			if (u.second->type().isWorker() && worker == false)
				continue;

			if (hide == false && u.second->isHide())
				continue;

			if (u.second->pos() == Positions::Unknown)
				continue;

			if (detectedOnly && !u.second->isHide() && !u.second->unit()->isDetected())
				continue;

			
			if (u.second->type() == Zerg_Egg || u.second->type() == Zerg_Larva || u.second->type() == Protoss_Interceptor ||
					u.second->type() == Protoss_Scarab || u.second->type() == Terran_Vulture_Spider_Mine)
				continue;

			Position newPos = pos - u.second->pos();

			if (groundDistance)
			{
				int dist = 0;
				theMap.GetPath(pos, u.second->pos(), &dist);

				if (radius && dist > radius)
					continue;

				if (dist > 0 && dist > closestDist)
				{
					closest = u.second;
					closestDist = dist;
				}
			}
			else
			{
				if (radius)
				{
					if (abs(newPos.x) > radius || abs(newPos.y) > radius)
						continue;

					if ((newPos.x * newPos.x) + (newPos.y * newPos.y) > radius * radius)
						continue;
				}

				if ((newPos.x * newPos.x + newPos.y * newPos.y) > closestDist)
				{
					closest = u.second;
					closestDist = (newPos.x * newPos.x + newPos.y * newPos.y);
				}
			}
		}
	}

	if (kind == TypeKind::BuildingKind || kind == TypeKind::AllDefenseBuildingKind || kind == TypeKind::AirDefenseBuildingKind || kind == TypeKind::GroundDefenseBuildingKind || kind == TypeKind::AllKind || kind == GroundCombatKind)
	{
		for (auto &u : _unitData[p].getAllBuildings())
		{
			if (hide == false && u.second->isHide())
				continue;

			if (u.second->pos() == Positions::Unknown)
				continue;

			if (!u.second->unit()->getType().airWeapon().targetsAir() && (kind == AirDefenseBuildingKind || kind == AllDefenseBuildingKind))
				continue;

			if (!u.second->unit()->getType().groundWeapon().targetsGround() && (kind == GroundDefenseBuildingKind || kind == AllDefenseBuildingKind || kind == GroundCombatKind))
				continue;

			Position newPos = pos - u.second->pos();

			if (groundDistance)
			{
				int dist = 0;
				theMap.GetPath(pos, u.second->pos(), &dist);

				if (radius && dist > radius)
					continue;

				if (dist > 0 && dist > closestDist)
				{
					closest = u.second;
					closestDist = dist;
				}
			}
			else
			{
				if (radius)
				{
					if (abs(newPos.x) > radius || abs(newPos.y) > radius)
						continue;

					if ((newPos.x * newPos.x) + (newPos.y * newPos.y) > radius * radius)
						continue;
				}

				if ((newPos.x * newPos.x + newPos.y * newPos.y) > closestDist)
				{
					closest = u.second;
					closestDist = (newPos.x * newPos.x + newPos.y * newPos.y);
				}
			}
		}
	}

	return closest;
}

UnitInfo *InformationManager::getClosestTypeUnit(Player p, Position pos, UnitType type, int radius, bool hide, bool groundDistance, bool detectedOnly)
{
	if (!pos.isValid())
		return nullptr;

	UnitInfo *closest = nullptr;
	int closestDist = INT_MAX;

	uList &unitVector = type.isBuilding() ? _unitData[p].getBuildingVector(type) : _unitData[p].getUnitVector(type);

	for (auto &u : unitVector)
	{
		if (hide == false && u->isHide())
			continue;

		if (u->pos() == Positions::Unknown)
			continue;

		if (detectedOnly && !u->isHide() && !u->unit()->isDetected())
			continue;

		Position newPos = pos - u->pos();

		if (groundDistance)
		{
			int dist = 0;
			theMap.GetPath(pos, u->pos(), &dist);

			if (radius && dist > radius)
				continue;

			if (dist >= 0 && dist < closestDist)
			{
				closest = u;
				closestDist = dist;
			}
		}
		else
		{
			if (radius)
			{
				if (abs(newPos.x) > radius || abs(newPos.y) > radius)
					continue;

				if ((newPos.x * newPos.x) + (newPos.y * newPos.y) > radius * radius)
					continue;
			}

			if ((newPos.x * newPos.x + newPos.y * newPos.y) < closestDist)
			{
				closest = u;
				closestDist = (newPos.x * newPos.x + newPos.y * newPos.y);
			}
		}
	}

	return closest;
}

UnitInfo *InformationManager::getClosestTypeUnit(Player p, Unit my, UnitType type, int radius, bool hide, bool groundDist, bool detectedOnly)
{
	if (my == nullptr || !my->exists())
		return nullptr;

	if (!my->getPosition().isValid())
		return nullptr;

	UnitInfo *closest = nullptr;
	int closestDist = INT_MAX;

	uList &unitVector = type.isBuilding() ? _unitData[p].getBuildingVector(type) : _unitData[p].getUnitVector(type);

	for (auto &u : unitVector)
	{
		if (hide == false && u->isHide())
			continue;

		if (u->pos() == Positions::Unknown)
			continue;

		if (detectedOnly && !u->isHide() && !u->unit()->isDetected())
			continue;

		if (u->id() == my->getID())
			continue;

		Position newPos = my->getPosition() - u->pos();

		if (groundDist)
		{
			int dist = 0;
			theMap.GetPath(my->getPosition(), u->pos(), &dist);

			if (radius && dist > radius)
				continue;

			if (dist >= 0 && dist < closestDist)
			{
				closest = u;
				closestDist = dist;
			}
		}
		else
		{
			if (radius)
			{
				if (abs(newPos.x) > radius || abs(newPos.y) > radius)
					continue;

				if ((newPos.x * newPos.x) + (newPos.y * newPos.y) > radius * radius)
					continue;
			}

			if ((newPos.x * newPos.x + newPos.y * newPos.y) < closestDist)
			{
				closest = u;
				closestDist = (newPos.x * newPos.x + newPos.y * newPos.y);
			}
		}
	}

	return closest;
}

UnitInfo *InformationManager::getClosestTypeUnit(Player p, Position pos, vector<UnitType> &types, int radius, bool hide, bool groundDistance, bool detectedOnly)
{
	if (!pos.isValid())
		return nullptr;

	UnitInfo *closest = nullptr;
	int closestDist = INT_MAX;

	for (auto type : types)
	{
		uList &unitVector = type.isBuilding() ? _unitData[p].getBuildingVector(type) : _unitData[p].getUnitVector(type);

		for (auto u : unitVector)
		{
			if (hide == false && u->isHide())
				continue;

			if (u->pos() == Positions::Unknown)
				continue;

			if (detectedOnly && !u->isHide() && !u->unit()->isDetected())
				continue;

			Position newPos = pos - u->pos();

			if (groundDistance)
			{
				int dist = 0;
				theMap.GetPath(pos, u->pos(), &dist);

				if (radius && dist > radius)
					continue;

				if (dist >= 0 && dist < closestDist)
				{
					closest = u;
					closestDist = dist;
				}
			}
			else
			{
				if (radius)
				{
					if (abs(newPos.x) > radius || abs(newPos.y) > radius)
						continue;

					if ((newPos.x * newPos.x) + (newPos.y * newPos.y) > radius * radius)
						continue;
				}

				if ((newPos.x * newPos.x + newPos.y * newPos.y) < closestDist)
				{
					closest = u;
					closestDist = (newPos.x * newPos.x + newPos.y * newPos.y);
				}
			}
		}
	}

	return closest;
}

UnitInfo *InformationManager::getFarthestTypeUnit(Player p, Position pos, UnitType type, int radius, bool hide, bool groundDistance, bool detectedOnly)
{
	UnitInfo *farthest = nullptr;
	int farthestDist = 0;

	uList &unitVector = type.isBuilding() ? _unitData[p].getBuildingVector(type) : _unitData[p].getUnitVector(type);

	for (auto u : unitVector)
	{
		if (hide == false && u->isHide())
			continue;

		if (u->pos() == Positions::Unknown)
			continue;

		if (detectedOnly && !u->isHide() && !u->unit()->isDetected())
			continue;

		Position newPos = pos - u->pos();

		if (groundDistance)
		{
			int dist = 0;
			theMap.GetPath(pos, u->pos(), &dist);

			if (radius && dist > radius)
				continue;

			if (dist > farthestDist)
			{
				farthest = u;
				farthestDist = dist;
			}
		}
		else
		{
			if (radius)
			{
				if (abs(newPos.x) > radius || abs(newPos.y) > radius)
					continue;

				if ((newPos.x * newPos.x) + (newPos.y * newPos.y) > radius * radius)
					continue;
			}

			if ((newPos.x * newPos.x + newPos.y * newPos.y) > farthestDist)
			{
				farthest = u;
				farthestDist = (newPos.x * newPos.x + newPos.y * newPos.y);
			}
		}
	}

	return farthest;
}


void InformationManager::updateExpectedMultiBases()
{
	
	if (!isEnemyBaseFound) return;

	if (getOccupiedBaseLocations(E).empty() || getOccupiedBaseLocations(S).empty()) return;

	map<int, Base *> enemySortMap;
	map<int, Base *> mySortMap;

	for (auto base : getBaseLocations())
	{
		if (base->GetOccupiedInfo() != emptyBase )
		{
			base->SetExpectedEnemyMultiRank(INT_MAX);
			base->SetExpectedMyMultiRank(INT_MAX);
			continue;
		}

		if (base->isIsland()) continue; 

		int path = 0;
		theMap.GetPath(base->Center(), theMap.Center(), &path, false);

		if (path < 0) continue; 

		int enemyDist = 0;

		for (auto enemyBase : getOccupiedBaseLocations(E))
		{
			int tmp = 0;
			theMap.GetPath(base->Center(), enemyBase->Center(), &tmp);

			if (tmp > 0)
				enemyDist += tmp;
		}

		enemyDist = (int)(enemyDist / getOccupiedBaseLocations(E).size());

		int myDist = 0;

		for (auto myBase : getOccupiedBaseLocations(S))
		{
			int tmp = 0;
			theMap.GetPath(base->Center(), myBase->Center(), &tmp);

			if (tmp > 0)
				myDist += tmp;
		}

		myDist = (int)(myDist / getOccupiedBaseLocations(S).size());

		enemySortMap[enemyDist - myDist] = base;
		mySortMap[myDist - enemyDist] = base;
	}

	int index = 1;

	for (auto &eBase : enemySortMap)
	{
		eBase.second->SetExpectedEnemyMultiRank(index);
		index++;
	}

	index = 1;

	for (auto &mBase : mySortMap)
	{
		mBase.second->SetExpectedMyMultiRank(index);
		index++;
	}
}

void InformationManager::setFirstWaitLinePosition()
{
	Position pos = Positions::Unknown;

	if (_firstWaitLinePosition != Positions::Unknown)
		return;

	if (_firstExpansionLocation[S] == nullptr || _secondChokePoint[S] == nullptr)
		return;

	if (INFO.getOccupiedBaseLocations(S).size() < 2 || INFO.getCompletedCount(Terran_Command_Center, S) < 2)
		return;

	pos = getDirectionDistancePosition(_firstExpansionLocation[S]->getPosition(), (Position)_secondChokePoint[S]->Center(),
									   _firstExpansionLocation[S]->getPosition().getApproxDistance((Position)_secondChokePoint[S]->Center()) + 10 * TILE_SIZE);

	if (INFO.getSecondExpansionLocation(S) != nullptr)
		pos = getDirectionDistancePosition(pos, INFO.getSecondExpansionLocation(S)->getPosition(), 5 * TILE_SIZE);

	pos = getDirectionDistancePosition(pos, (Position)theMap.Center(), 3 * TILE_SIZE);

	pos = getDirectionDistancePosition(pos, INFO.getSecondChokePosition(E), 3 * TILE_SIZE);

	if (!bw->isWalkable((WalkPosition)pos))
	{
		for (int i = 0; i < 3; i++)
		{
			pos = getDirectionDistancePosition(pos, (Position)_secondChokePoint[S]->Center(), 3 * TILE_SIZE);

			if (bw->isWalkable((WalkPosition)pos))
				break;
		}
	}

	if (theMap.GetMiniTile((WalkPosition)pos).Altitude() < 150)
	{
		for (int i = 0; i < 5; i++)
		{
			pos = getDirectionDistancePosition(pos, (Position)theMap.Center(), 3 * TILE_SIZE);

			if (bw->isWalkable((WalkPosition)pos) && theMap.GetMiniTile((WalkPosition)pos).Altitude() > 150)
				break;
		}
	}

	if (pos.isValid())
		_firstWaitLinePosition = pos;
}

void InformationManager::checkActivationBaseCount()
{
	uList commandCenter = getBuildings(Terran_Command_Center, S);
	int enoughMineralScv = 0;

	for (auto c : commandCenter)
	{
		if (!c->isComplete())
			continue;

		if (SoldierManager::Instance().getDepotMineralSize(c->unit()) > 0 && getAverageMineral(c->pos()) > 100
				&& SoldierManager::Instance().getAssignedScvCount(c->unit()) >= SoldierManager::Instance().getDepotMineralSize(c->unit()))
		{
			enoughMineralScv++;
		}
	}

	activationMineralBaseCount = enoughMineralScv;

	int gasSvcCount = 0;

	for (auto iter = SoldierManager::Instance().getRefineryScvCountMap().begin(); iter != SoldierManager::Instance().getRefineryScvCountMap().end(); iter++)
	{
		gasSvcCount += iter->second;
	}

	activationGasBaseCount = (int)(gasSvcCount / 3);
}

void InformationManager::addAdditionalExpansion(Unit depot)
{
	if (INFO.getOccupiedBaseLocations(S).size() < 5)
		return;

	Base *multiBase = nullptr;

	for (Base *baseLocation : _allBaseLocations)
	{
		if (baseLocation->getTilePosition() == depot->getTilePosition())
		{
			multiBase = baseLocation;
			break;
		}
	}

	if (multiBase != nullptr)
	{
		if (INFO.getMainBaseLocation(S) != nullptr && multiBase == INFO.getMainBaseLocation(S))
			return;

		if (INFO.getFirstExpansionLocation(S) != nullptr && multiBase == INFO.getFirstExpansionLocation(S))
			return;

		if (INFO.getSecondExpansionLocation(S) != nullptr && multiBase == INFO.getSecondExpansionLocation(S))
			return;

		if (INFO.getThirdExpansionLocation(S) != nullptr && multiBase == INFO.getThirdExpansionLocation(S))
			return;

		auto targetBase = find_if(_additionalExpansions.begin(), _additionalExpansions.end(), [multiBase](Base * base) {
			return multiBase == base;

		});

		if (targetBase == _additionalExpansions.end())
			_additionalExpansions.push_back(multiBase);
	}
}

void InformationManager::removeAdditionalExpansion(Unit depot)
{
	if (_additionalExpansions.empty())
		return;

	Base *multiBase = nullptr;

	for (Base *baseLocation : _allBaseLocations)
	{
		if (baseLocation->getTilePosition() == depot->getTilePosition())
		{
			multiBase = baseLocation;
			break;
		}
	}

	if (multiBase != nullptr)
	{
		auto targetBase = find_if(_additionalExpansions.begin(), _additionalExpansions.end(), [multiBase](Base * base) {
			return multiBase == base;

		});

		if (targetBase != _additionalExpansions.end())
			_additionalExpansions.erase(targetBase);
	}
}

int InformationManager::getActivationMineralBaseCount()
{
	return activationMineralBaseCount;
}

int InformationManager::getActivationGasBaseCount()
{
	return activationGasBaseCount;
}

int InformationManager::getAverageMineral(Position basePosition)
{
	uList commandCenter = INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, basePosition, 5 * TILE_SIZE, true);
	Unit depot = nullptr;
	int averageMineral = INT_MAX;

	for (auto c : commandCenter)
	{
		if (!c->getLift())
		{
			depot = c->unit();
			break;
		}
	}

	if (depot == nullptr)
		return averageMineral; 

	averageMineral = SoldierManager::Instance().getRemainingAverageMineral(depot);

	return averageMineral;
}

Base *InformationManager::getFirstMulti(Player p, bool existGas, bool notCenter, int multiRank)
{
	Base *base = nullptr;
	int rank = multiRank;

	if (_allBaseLocations.empty())
		return base;

	for (word i = 0; i < _allBaseLocations.size(); i++)
	{
		auto targetBase = find_if(_allBaseLocations.begin(), _allBaseLocations.end(), [rank, p](Base * base) {
			if (p == S)
				return rank == base->GetExpectedMyMultiRank();
			else
				return rank == base->GetExpectedEnemyMultiRank();

		});

		if (targetBase == _allBaseLocations.end())
			break;

		if (notCenter)
		{
			{
				if (isSameArea(theMap.Center(), (*targetBase)->getPosition()))
				{
					rank++;
					continue;
				}
			}
		}

		if (existGas)
		{
			if ((*targetBase)->Geysers().empty())
			{
				rank++;
				continue;
			}
		}

		if (getAverageMineral((*targetBase)->getPosition()) < 200)
		{
			rank++;
			continue;
		}

		base = *targetBase;
		break;
	}

	return base;
}

Position InformationManager::getFirstWaitLinePosition()
{
	return _firstWaitLinePosition;
}

void InformationManager::updateAllFirstExpansions()
{
	_firstExpansionOfAllStartposition.clear();
	_mainBaseAreaPair.clear();
	Base *expectedLocation;

	int temp = 0;
	CPPath path;

	for (Base *startLocation : INFO.getStartLocations())
	{
		int distance = 10000000;

		for (Base *baseLocation : INFO.getBaseLocations()) {

			if (baseLocation == startLocation)
				continue;

			CPPath tmpPath = theMap.GetPath(startLocation->Center(), baseLocation->Center(), &temp);

			if (temp > 0 && distance > temp && baseLocation->Geysers().size() != 0) {
				distance = temp;
				path = tmpPath;
				expectedLocation = baseLocation;
			}
		}

		if (path.size() > 1) {
			for (auto &area : theMap.Areas()) {
				int sameCp = 0;

				for (auto cp : area.ChokePoints()) {
					for (auto pCp : path) {
						if (cp == pCp) {
							sameCp++;
							break;
						}
					}
				}

				if (sameCp == path.size()) {
					_mainBaseAreaPair[startLocation->GetArea()] = &area;
					_mainBaseAreaPair[&area] = startLocation->GetArea();
					break;
				}
			}
		}

		_firstExpansionOfAllStartposition[startLocation] = expectedLocation;
	}

}

void InformationManager::updateSecondExpansion()
{
	if (_secondExpansionLocation[S] != nullptr)
	{
		Position pos = _secondExpansionLocation[S]->Center();

		auto targetBase = find_if(_occupiedBaseLocations[E].begin(), _occupiedBaseLocations[E].end(), [pos](const Base * base) {
			return pos == base->Center();
		});

		if (targetBase != _occupiedBaseLocations[E].end())
		{
			_secondExpansionLocation[S] = nullptr;
		}
		else
		{
			bw->drawTextMap(_secondExpansionLocation[S]->Center(), "SecondExpansionLocation");
			return;
		}
	}

	if (_occupiedBaseLocations[S].size() < 2)
		return;

	Base *multi = nullptr;

	if (INFO.enemyRace == Races::Terran)
	{
		multi = INFO.getFirstMulti(S, true, true);

		if (multi != nullptr && multi != INFO.getThirdExpansionLocation(S))
		{
			_secondExpansionLocation[S] = multi;
		}
		else
		{
			multi = INFO.getFirstMulti(S);

			if (multi != nullptr)
			{
				_secondExpansionLocation[S] = multi;
			}
		}
	}
	else
	{
		multi = multi = INFO.getFirstMulti(S);

		if (multi != nullptr && multi != INFO.getThirdExpansionLocation(S))
		{
			_secondExpansionLocation[S] = multi;
		}
		else
		{
			multi = INFO.getFirstMulti(S, false, false, 2);

			if (multi != nullptr)
			{
				_secondExpansionLocation[S] = multi;
			}
		}
	}
}

void InformationManager::updateSecondExpansionForEnemy()
{
	if (_secondExpansionLocation[E] != nullptr)
		return;

	if (_occupiedBaseLocations[E].size() < 2)
		return;

	Base *multi = nullptr;

	if (INFO.enemyRace != Races::Protoss)
	{
		multi = INFO.getFirstMulti(E, true);

		if (multi != nullptr)
		{
			_secondExpansionLocation[E] = multi;
		}
		else
		{
			multi = INFO.getFirstMulti(E);

			if (multi != nullptr)
			{
				_secondExpansionLocation[E] = multi;
			}
		}
	}
	else
	{
		multi = INFO.getFirstMulti(E);

		if (multi != nullptr)
		{
			_secondExpansionLocation[E] = multi;
		}
	}

}

void InformationManager::updateThirdExpansion()
{
	if (_occupiedBaseLocations[S].size() < 3)
		return;

	bool notCenter = false;
	bool reset = false;

	if (_thirdExpansionLocation[S] != nullptr)
	{
		Position pos = _thirdExpansionLocation[S]->Center();

		auto enemyBase = find_if(_occupiedBaseLocations[E].begin(), _occupiedBaseLocations[E].end(), [pos](const Base * base) {
			return pos == base->Center();
		});

		if (enemyBase != _occupiedBaseLocations[E].end())
		{
			reset = true;
		}
		else
		{
			auto myBase = find_if(_occupiedBaseLocations[S].begin(), _occupiedBaseLocations[S].end(), [pos](const Base * base) {
				return pos == base->Center();
			});

			if (myBase == _occupiedBaseLocations[S].end())
			{
				int myUnitCountInThirdExpansion = INFO.getUnitsInRadius(S, INFO.getThirdExpansionLocation(S)->getPosition(), 20 * TILE_SIZE, true, true, false, true).size();
				int enemyUnitCountInThirdExpansion = INFO.getUnitsInRadius(E, INFO.getThirdExpansionLocation(S)->getPosition(), 20 * TILE_SIZE, true, true, false, true).size();

				if (enemyUnitCountInThirdExpansion > 3 && myUnitCountInThirdExpansion <= enemyUnitCountInThirdExpansion)
				{
					notCenter = true;
					reset = true;
				}
				else
				{
					bw->drawTextMap(_thirdExpansionLocation[S]->Center(), "ThirdExpansionLocation");
					return;
				}
			}
			else
			{
				bw->drawTextMap(_thirdExpansionLocation[S]->Center(), "ThirdExpansionLocation");
				return;
			}
		}
	}

	if (reset)
		_thirdExpansionLocation[S] = nullptr;

	Base *multi = nullptr;

	if (INFO.enemyRace == Races::Terran)
	{
		if (notCenter)
			multi = INFO.getFirstMulti(S, true, true);
		else
			multi = INFO.getFirstMulti(S, true);

		if (multi != nullptr)
		{
			_thirdExpansionLocation[S] = multi;
		}
		else
		{
			multi = INFO.getFirstMulti(S);

			if (multi != nullptr)
			{
				_thirdExpansionLocation[S] = multi;
			}
		}
	}
	else
	{
		multi = INFO.getFirstMulti(S);

		if (multi != nullptr)
		{
			_thirdExpansionLocation[S] = multi;
		}
	}
}

void InformationManager::updateIslandExpansion()
{
	int closestDistance = INT_MAX;
	Base *islandBase = nullptr;

	if (_mainBaseLocations[S] == nullptr)
		return;

	if (_islandExpansionLocation[S] != nullptr)
	{
		bw->drawTextMap(_islandExpansionLocation[S]->getPosition(), "IslandExpansionLocation");
	}

	if (_islandExpansionLocation[E] != nullptr)
	{
	}

	for (Base *baseLocation : _allBaseLocations)
	{
		if (!baseLocation->isIsland())
			continue;

		if (baseLocation->Minerals().size() < 3)
			continue;


		uList sBuildings = INFO.getBuildingsInArea(S, baseLocation->getPosition(), true, false, true);
		uList eBuildings = INFO.getBuildingsInArea(E, baseLocation->getPosition(), true, false, true);

		if (!eBuildings.empty()) 
		{
			if (_islandExpansionLocation[E] == nullptr)
				_islandExpansionLocation[E] = baseLocation;

			continue;
		}

		int tempDistance = _mainBaseLocations[S]->getPosition().getApproxDistance(baseLocation->getPosition());

		if (tempDistance < closestDistance)
		{
			islandBase = baseLocation;
			closestDistance = tempDistance;
		}
	}

	if (_islandExpansionLocation[S] == nullptr && islandBase != nullptr)
	{
		_islandExpansionLocation[S] = islandBase;
	}
}


bool InformationManager::isMainBasePairArea(const Area *a1, const Area *a2) {
	return a1 != nullptr && a2 != nullptr && _mainBaseAreaPair[a1] != nullptr && _mainBaseAreaPair[a1] == a2;
}

bool InformationManager::isTimeToMoveCommandCenterToFirstExpansion()
{
	if (!INFO.getFirstExpansionLocation(S))
		return false;

	if (enemyRace == Races::Zerg)
	{
		if (enemyInMyArea().empty() || getEnemyInMyYard(1000, Zerg_Zergling).empty())
			return true;
	}
	else if (enemyRace == Races::Protoss)
	{
		if (INFO.getCompletedCount(Terran_Command_Center, S) < 2)
			return false;

		if (S->hasResearched(TechTypes::Tank_Siege_Mode) && INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 2)
			return true;

		if (ESM.getEnemyInitialBuild() == Toss_2g_zealot || ESM.getEnemyInitialBuild() == Toss_1g_forward || ESM.getEnemyInitialBuild() == Toss_2g_forward)
			return true;

		if (INFO.getCompletedCount(Terran_Vulture, S) >= 4 || enemyInMyArea().empty())
			return true;
	}

	return false;
}

bool InformationManager::isBaseSafe(const Base *base)
{
	if (base == nullptr)
		return false;

	int groundEnem = 0;
	int airEnem = 0;

	vector<UnitInfo *> enem = INFO.getUnitsInRadius(E, base->getPosition(), 1000, true, true, true);

	if (enem.empty())
		return true;


	for (auto ui : enem)
	{
		if (ui->unit()->getType().groundWeapon().targetsGround())
		{
			if (!ui->unit()->isDetected())
				return false;

			if (ui->unit()->isFlying())
				airEnem++;
			else
				groundEnem++;
		}
	}

	vector<UnitInfo *> my = INFO.getUnitsInRadius(S, base->getPosition(), 1000, true, true, true, true);

	if (my.empty())
		return false;

	int groundMy = 0;
	int airMy = 0;

	for (auto ui : my)
	{
		if (ui->unit()->getType().groundWeapon().targetsGround())
			groundMy++;
		else if (ui->unit()->getType().airWeapon().targetsAir())
			airMy++;
	}

	if (airEnem > 0 && airMy == 0)
		return false;

	if (groundEnem > 0 && groundMy == 0)
		return false;

	if ((airMy + groundMy) < 3 && enem.size() - my.size() > 10)
		return false;

	return true;
}

bool InformationManager::isBaseHasResourses(const Base *base, int mineral, int gas)
{
	if (base == nullptr)
		return false;

	int totMin = 0;
	int totGas = 0;

	for (auto m : base->Minerals())
		totMin += m->Amount();

	for (auto g : base->Geysers())
		totGas += g->Amount();

	if (totMin > mineral)
		return true;

	if (totGas > gas)
		return true;

	return false;
}

Base *InformationManager::getClosestSafeEmptyBase(UnitInfo *c)
{
	int dist = INT_MAX;
	Base *targetBase = nullptr;


	dist = INT_MAX;

	for (Base *b : INFO.getBaseLocations())
	{
		if (b->GetOccupiedInfo() == emptyBase && isBaseHasResourses(b, 1000, 1000) && isBaseSafe(b))
		{
			int tempDist = c->pos().getApproxDistance(b->getPosition());

			if (tempDist < dist)
			{
				dist = tempDist;
				targetBase = b;
			}
		}
	}

	return targetBase;
}

int InformationManager::getAltitudeDifference(TilePosition p1, TilePosition p2) {
	if (p2 == TilePositions::None) {
		if (INFO.getFirstExpansionLocation(S) == nullptr)
			return -1;

		p2 = INFO.getFirstExpansionLocation(S)->getTilePosition();
	}

	if (!p1.isValid() || !p2.isValid())
		return -1;

	int gh1 = bw->getGroundHeight(p1);
	int gh2 = bw->getGroundHeight(p2);

	if (gh1 == gh2)
		return 0;

	return gh1 > gh2 ? 1 : -1;
}

Position InformationManager::getWaitingPositionAtFirstChoke(int min, int max) {

	Position waitingPositionAtFirstChoke;

	Position chokeE1;
	Position chokeE2;
	Position chokeCenter;
	Position waitingPoint; 

	if (INFO.getAltitudeDifference() <= 0) {
		if (!INFO.getSecondChokePoint(S))
			return Positions::None;

		chokeE1 = INFO.getSecondChokePosition(S, ChokePoint::end1);
		chokeE2 = INFO.getSecondChokePosition(S, ChokePoint::end2);
		chokeCenter = (chokeE1 + chokeE2) / 2;
		waitingPoint = INFO.getFirstExpansionLocation(S)->Center();
	}
	else {
		if (!INFO.getFirstChokePoint(S))
			return Positions::None;

		chokeE1 = INFO.getFirstChokePosition(S, ChokePoint::end1);
		chokeE2 = INFO.getFirstChokePosition(S, ChokePoint::end2);
		chokeCenter = (chokeE1 + chokeE2) / 2;
		waitingPoint = INFO.getMainBaseLocation(S)->Center();
	}

	double gradient;

	if ((chokeE2.y - chokeE1.y) == 0) {
		gradient = 0.5;
	}
	else if ((chokeE2.x - chokeE1.x) == 0) {
		gradient = 1.5;
	}
	else {
		gradient = (chokeE2.y - chokeE1.y) / (double)(chokeE2.x - chokeE1.x);
	}

	gradient *= -1.0;
	double b = chokeCenter.y - gradient * chokeCenter.x;

	
	if (abs(gradient) > 1) {

		for (int i = min; i <= max; i++) {

			int newY = chokeCenter.y + i * 32;
			int newX = (int)((newY - b) / gradient);

			Position newPos(newX, newY);

			if (isSameArea(newPos, waitingPoint)) {
				waitingPositionAtFirstChoke = newPos;
				break;
			}
			else {
				newY = chokeCenter.y - i * 32;
				newX = (int)((newY - b) / gradient);
				newPos = Position(newX, newY);

				if (isSameArea(newPos, waitingPoint)) {
					waitingPositionAtFirstChoke = newPos;
					break;
				}
			}

		}

	}
	else {

		for (int i = min; i <= max; i++) {

			int newX = chokeCenter.x + i * 32;
			int newY = (int)(newX * gradient + b);
			Position newPos(newX, newY);

			if (isSameArea(newPos, waitingPoint)) {
				waitingPositionAtFirstChoke = newPos;
				break;
			}
			else {
				newX = chokeCenter.x - i * 32;
				newY = (int)(newX * gradient + b);
				newPos = Position(newX, newY);

				if (isSameArea(newPos, waitingPoint)) {
					waitingPositionAtFirstChoke = newPos;
					break;
				}
			}
		}
	}

	if (waitingPositionAtFirstChoke == Positions::None) {
		waitingPositionAtFirstChoke = (chokeCenter + waitingPoint) / 2;
	}

	return waitingPositionAtFirstChoke;
}

int InformationManager::getFirstChokeDefenceCnt()
{
	if (INFO.enemyRace == Races::Terran || TIME < 3 * 24 * 60 || INFO.getAltitudeDifference() <= 0)
		return 0;

	int needCnt = 0;

	if (INFO.enemyRace == Races::Protoss) {

		if (ESM.getEnemyInitialBuild() == Toss_1g_dragoon) {
			needCnt = 1;

			if (INFO.getCompletedCount(Protoss_Zealot, E) > 0)
				needCnt = 2;

		}
		else if (ESM.getEnemyInitialBuild() == Toss_2g_dragoon) {
			needCnt = 2;
		}
	}
	else if (INFO.enemyRace == Races::Zerg) {

	}

	if (INFO.getFirstExpansionLocation(S)) {
		uList command = INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, INFO.getFirstExpansionLocation(S)->Center(), 3 * TILE_SIZE, true);

		for (auto b : command) {
			if (!b->unit()->isLifted()) {
				needCnt = 0;
			}
		}
	}

	uList commandList = INFO.getTypeBuildingsInArea(Terran_Command_Center, S, MYBASE, false);

	if (commandList.size() > 1)
		needCnt = needCnt == 0 ? 1 : needCnt;

	if (needCnt != 0 && INFO.getFirstChokePosition(S) != Positions::None) {

		uList eList = INFO.getUnitsInRadius(E, INFO.getFirstChokePosition(S), 10 * TILE_SIZE, true, false, false);

		for (auto e : eList) {
			if (isSameArea(e->pos(), MYBASE) && getGroundDistance(e->pos(), INFO.getFirstChokePosition(S)) > 64) {
				needCnt = 0;
				break;
			}
		}
	}

	return needCnt;
}

bool InformationManager::hasRearched(TechType tech) {
	return researchedSet.find(tech) != researchedSet.end();
}

void InformationManager::setRearched(UnitType unitType) {
	if (unitType == Terran_Siege_Tank_Siege_Mode)
		researchedSet.insert(TechTypes::Tank_Siege_Mode);
	else if (unitType == Zerg_Lurker || unitType == Zerg_Lurker_Egg)
		researchedSet.insert(TechTypes::Lurker_Aspect);
	else if (unitType == Terran_Vulture_Spider_Mine)
		researchedSet.insert(TechTypes::Spider_Mines);
}

void InformationManager::checkMoveInside()
{
	if (getFirstChokePosition(E) == Positions::None || INFO.enemyRace == Races::Terran)
	{
		_needMoveInside = false;
		return;
	}

	uList aroundChoke = getUnitsInRadius(S, getFirstChokePosition(E), 10 * TILE_SIZE, true, false, false);
	uList aroundEnChoke = getUnitsInRadius(E, getFirstChokePosition(E), 15 * TILE_SIZE, true, false, false); // 탱크 때문에 15
	int aroundDefChoke = getDefenceBuildingsInRadius(E, getFirstChokePosition(E), 15 * TILE_SIZE, false).size();

	if (INFO.enemyRace == Races::Zerg)
		aroundDefChoke *= 3;

	if (aroundChoke.size() < 20 || aroundChoke.size() < (aroundEnChoke.size() + aroundDefChoke) + 12)
	{
		_needMoveInside = false;
		return;
	}

	Position avgPos = getAvgPosition(aroundChoke);

	int areaCnt = 0;

	for (auto u : aroundChoke)
		if (isSameArea(u->pos(), getMainBaseLocation(E)->Center()))
			areaCnt++;

	if (areaCnt > 7 || areaCnt > 0.6 * aroundChoke.size())
		_needMoveInside = false;
	else
		_needMoveInside = true;
}

void InformationManager::setNextScanPointOfMainBase()
{
	if (!scanPositionOfMainBase.empty())
		return;

	if (INFO.getMainBaseLocation(E))
		scanPositionOfMainBase.push_back((TilePosition)INFO.getMainBaseLocation(E)->Center());


	TilePosition nPosition = (TilePosition)INFO.getMainBaseLocation(E)->Center();
	int index = 1;
	vector<pair<int, int>> sortArray;
	vector<pair<int, int>>::iterator iter;

	while (nPosition.y - index > 0
			&& isSameArea((Position)TilePosition(nPosition.x, nPosition.y - index), (Position)nPosition))
		index++;

	if (index > 11)
		sortArray.push_back(pair<int, int>(0, index));

	index = 1;

	while (nPosition.y + index < 128
			&& isSameArea((Position)TilePosition(nPosition.x, nPosition.y + index), (Position)nPosition))
		index++;

	if (index > 11)
		sortArray.push_back(pair<int, int>(1, index));

	index = 1;

	while (nPosition.x - index > 0
			&& isSameArea((Position)TilePosition(nPosition.x - index, nPosition.y), (Position)nPosition))
		index++;

	if (index > 11)
		sortArray.push_back(pair<int, int>(2, index));

	index = 1;

	while (nPosition.x + index < 128
			&& isSameArea((Position)TilePosition(nPosition.x + index, nPosition.y), (Position)nPosition))
		index++;

	if (index > 11)
		sortArray.push_back(pair<int, int>(3, index));

	sort(sortArray.begin(), sortArray.end(), [](pair<int, int> a, pair<int, int> b)
	{
		return a.second > b.second;
	});

	if (sortArray.size() > 0)
		sortArray.erase(sortArray.end() - 1); 

	for (iter = sortArray.begin(); iter != sortArray.end(); iter++) {

		if (iter->first == 0)
			scanPositionOfMainBase.push_back(TilePosition(nPosition.x, nPosition.y - 8));
		else if (iter->first == 1)
			scanPositionOfMainBase.push_back(TilePosition(nPosition.x, nPosition.y + 8));
		else if (iter->first == 2)
			scanPositionOfMainBase.push_back(TilePosition(nPosition.x - 8, nPosition.y));
		else if (iter->first == 3)
			scanPositionOfMainBase.push_back(TilePosition(nPosition.x + 8, nPosition.y));
	}
}

TilePosition MyBot::InformationManager::getScanPositionOfMainBase()
{
	if (scanIndex >= scanPositionOfMainBase.size()) scanIndex = 0;

	TilePosition ret = scanPositionOfMainBase.at(scanIndex);
	scanIndex++;
	return ret;
}

