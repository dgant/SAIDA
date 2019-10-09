#include "HostileManager.h"

using namespace MyBot;

HostileManager &HostileManager::Instance()
{
	static HostileManager instance;
	return instance;
}

void HostileManager::update()
{
	if (TIME < 24 * 30 || TIME % 12 != 0)	return;

	if (TIME < 24 * 300)
		checkEnemyInitialBuild();

	
	if (INFO.isEnemyBaseFound == false && INFO.getAvailableScanCount() > 0)
		ComsatStationManager::Instance().useScan(INFO.getMainBaseLocation(E)->Center());


	if (TIME > 24 * 180)
		checkEnemyMainBuild();

	if (TIME < 24 * 60 * 10)
		checkDarkDropHadCome();
}

InitialBuildType HostileManager::getEnemyInitialBuild()
{
	return enemyInitialBuild;
}

MainBuildType HostileManager::getEnemyMainBuild()
{
	return enemyMainBuild;
}

void HostileManager::setEnemyMainBuild(MainBuildType st)
{
	MainBuildType currentStrategy = enemyMainBuild;

	if (enemyMainBuild == UnknownMainBuild)
		enemyMainBuild = st;
	else if (INFO.enemyRace == Races::Protoss)
	{
		if (st == Toss_dark_drop || st == Toss_reaver_drop || st == Toss_drop) {
			if (TIME < 24 * 60 * 10 && waitTimeForDrop == 0)
				waitTimeForDrop = TIME;
		}


		if (st == Toss_arbiter) {
			if (enemyMainBuild == Toss_fast_carrier)
				enemyMainBuild = Toss_arbiter_carrier;
		}
		else if (st == Toss_fast_carrier) {
			if (enemyMainBuild == Toss_arbiter)
				enemyMainBuild = Toss_arbiter_carrier;
		}
	}
	else if (INFO.enemyRace == Races::Terran)
	{
		if (enemyMainBuild != Terran_2fac)
			return;
	}

	if (enemyMainBuild < st)
		enemyMainBuild = st;

	if (enemyMainBuild != currentStrategy) {
		BasicBuildStrategy::Instance().onMainBuildChange(currentStrategy, enemyMainBuild);
		embHistory.emplace_back(enemyMainBuild, TIME);
		clearBuildConstructQueue();
	}
}

void HostileManager::setEnemyInitialBuild(InitialBuildType ib, bool force)
{
	InitialBuildType currentStrategy = enemyInitialBuild;

	if (force) {
		if (enemyInitialBuild != ib)
			enemyInitialBuild = ib;
	}
	else {
	
		if (enemyInitialBuild == UnknownBuild || enemyInitialBuild < ib)
			enemyInitialBuild = ib;
	}

	if (enemyInitialBuild != currentStrategy) {
		BasicBuildStrategy::Instance().onInitialBuildChange(currentStrategy, enemyInitialBuild);
		eibHistory.emplace_back(enemyInitialBuild, TIME);
		clearBuildConstructQueue();
	}
}


void HostileManager::clearBuildConstructQueue() {
	CM.clearNotUnderConstruction();
	BM.buildQueue.clearAll();
	BasicBuildStrategy::Instance().executeBasicBuildStrategy();
}

HostileManager::HostileManager()
{
	enemyInitialBuild = UnknownBuild;
	enemyMainBuild = UnknownMainBuild;
	enemyGasRushRefinery = nullptr;

	int distance = getGroundDistance(theMap.Center(), MYBASE);

	
	centerToBaseOfSCV = distance / Terran_SCV.topSpeed();
	timeToBuildBarrack = (int)centerToBaseOfSCV + Terran_Barracks.buildTime();

	centerToBaseOfProbe = distance / Protoss_Probe.topSpeed();
	timeToBuildGateWay = (int)centerToBaseOfProbe + Protoss_Gateway.buildTime() + Protoss_Pylon.buildTime();

	
	centerToBaseOfMarine = distance / Terran_Marine.topSpeed(); 
	centerToBaseOfZ = distance / Protoss_Zealot.topSpeed(); 
}

void HostileManager::checkEnemyInitialBuild()
{
	
	if (INFO.enemyRace == Races::Zerg && enemyInitialBuild != UnknownBuild) {

		if (enemyInitialBuild == Zerg_9_Drone || enemyInitialBuild == Zerg_12_Ap || enemyInitialBuild == Zerg_12_Hat)
			useFirstAttackUnit();

		return;
	}

	if (!INFO.getUnitInfo(enemyGasRushRefinery, E)) {

		UnitType refineryType = INFO.enemyRace == Races::Protoss ? Protoss_Assimilator : INFO.enemyRace == Races::Terran ? Terran_Refinery : Zerg_Extractor;

		for (auto refinery : INFO.getBuildings(refineryType, E)) {
			if (isSameArea(INFO.getMainBaseLocation(S)->Center(), refinery->pos())) {
				enemyGasRushRefinery = refinery->unit();
				break;
			}
		}
	}
	else {
		enemyGasRushRefinery = nullptr;
	}

	useFirstExpansion();
	useFirstTechBuilding();
	useFirstAttackUnit();
	useBuildingCount();
}

void HostileManager::checkEnemyMainBuild()
{
	checkEnemyBuilding();
	checkEnemyUnits();
	checkEnemyTechNUpgrade();
}

void HostileManager::useFirstExpansion()
{
	if (INFO.enemyRace == Races::Zerg)
	{
		uList enemyHatcheryList = INFO.getBuildings(Zerg_Hatchery, E);
		BWEM::Base *enemyFirstExpansionLocation = INFO.getFirstExpansionLocation(E);

		if (enemyHatcheryList.empty())
			return;

		if (enemyFirstExpansionLocation == nullptr)
			return;

		bool isExistEnemyFirstExpansion = false;
		int buildStartFrame = INT_MAX;

		for (auto h : enemyHatcheryList)
		{
			if (h->isHide())
				continue;

			int distance = h->pos().getApproxDistance(enemyFirstExpansionLocation->Center());

			if (distance < 6 * TILE_SIZE)
			{
				isExistEnemyFirstExpansion = true;
				buildStartFrame = min(buildStartFrame, getBuildStartFrame(h->unit()));
			}
		}

		if (isExistEnemyFirstExpansion)
		{
		

			if (buildStartFrame < 90 * 24)
				setEnemyInitialBuild(Zerg_9_Hat);
			else if (buildStartFrame < 120 * 24)
				setEnemyInitialBuild(Zerg_12_Ap);

		}
		else
		{

		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		const Base *enemyMainBase = INFO.getMainBaseLocation(E);

		if (enemyMainBase == nullptr)
			return;

	
		uList enemyNexusList = INFO.getBuildings(Protoss_Nexus, E);

		for (auto nexus : enemyNexusList)
		{
			if (!isSameArea(enemyMainBase->Center(), nexus->pos())) {
				secondNexus = nexus;
				break;
			}
		}

		int currentFrameCnt = TIME;

		if (secondNexus == nullptr) {
			uList cyCoreList = INFO.getBuildings(Protoss_Cybernetics_Core, E);
			uList pylonList = INFO.getBuildings(Protoss_Pylon, E);
			uList gateList = INFO.getBuildings(Protoss_Gateway, E);
			uList dra = INFO.getUnits(Protoss_Dragoon, E);

			if (!cyCoreList.empty() || !dra.empty()) {


				if ((!cyCoreList.empty() && cyCoreList.at(0)->unit()->isUpgrading()) ||  pylonList.size() > 2) {

					if (!cyCoreList.empty() && cyCoreList.at(0)->unit()->isUpgrading())
					{
						setEnemyMainBuild(Toss_range_up_dra_push);

						if (gateList.size() >= 2)
							setEnemyMainBuild(Toss_2gate_dra_push);
					}

					if (gateList.size() >= 2)
						setEnemyInitialBuild(Toss_2g_dragoon);
					else
						setEnemyInitialBuild(Toss_1g_dragoon);
				}
			}
		}
		else {
		
			if (secondNexus->isComplete()) {
			
				if (currentFrameCnt <= (24 * 180 + Protoss_Nexus.buildTime()))
					setEnemyInitialBuild(Toss_pure_double);
				else
					setEnemyInitialBuild(Toss_1g_double);
			}
			
			else {
			
				int elapsedFrameCount = (int)(secondNexus->hp() / nexusBuildSpd_per_frmCnt);
				int expectedFrameCount = currentFrameCnt - elapsedFrameCount;


				if (expectedFrameCount >= 24 * 110 && expectedFrameCount <= 24 * 130) {
					setEnemyInitialBuild(Toss_pure_double);
				}
	
				else if (expectedFrameCount >= 24 * 210 && expectedFrameCount <= 24 * 250) {
					uList dragoonList = INFO.getUnits(Protoss_Dragoon, E);
					uList gatewayList = INFO.getBuildings(Protoss_Gateway, E);

					if (gatewayList.size() == 1 || dragoonList.size() > 0)
						setEnemyInitialBuild(Toss_1g_dragoon);
				}
			}
		}
	}
	else if (INFO.enemyRace == Races::Terran)
	{
		const Base *enemyMainBase = INFO.getMainBaseLocation(E);

		if (enemyMainBase == nullptr)
			return;


		uList enemyCommandList = INFO.getBuildings(Terran_Command_Center, E);
		uList factories = INFO.getBuildings(Terran_Factory, E);
		uList barracks = INFO.getTypeBuildingsInRadius(Terran_Barracks, E, INFO.getMainBaseLocation(E)->Center(), 1100);

		for (auto command : enemyCommandList)
		{
			if (theMap.GetArea((TilePosition)enemyMainBase->Center()) != theMap.GetArea((TilePosition)command->pos())) {
				secondCommandCenter = command;
				break;
			}
		}

		int currentFrameCnt = TIME;

		if (secondCommandCenter == nullptr) {

			if (!factories.empty()) {
				setEnemyInitialBuild(Terran_1fac);
			}
			else if (barracks.size() > 1) {
				setEnemyInitialBuild(Terran_2b);
			}
		}
		else {

		
			if (secondCommandCenter->isComplete()) {
		
				if (currentFrameCnt < (24 * 140 + Terran_Command_Center.buildTime()))
					setEnemyInitialBuild(Terran_pure_double);
				else
					setEnemyInitialBuild(Terran_1b_double);
			}
		
			else {
	
				int elapsedFrameCount = (int)(secondCommandCenter->hp() / commandBuildSpd_per_frmCnt);
				int expectedFrameCount = currentFrameCnt - elapsedFrameCount;

	
				if (expectedFrameCount >= 24 * 120 && expectedFrameCount <= 24 * 130) {
					setEnemyInitialBuild(Terran_pure_double);
				}
		
				else if (expectedFrameCount > 24 * 130 && expectedFrameCount <= 24 * 150) {
					setEnemyInitialBuild(Terran_1b_double);
				}
			}
		}

	}
	else
	{

	}
}

void HostileManager::useFirstTechBuilding()
{
	if (INFO.enemyRace == Races::Zerg)
	{
		uList hatcherys = INFO.getBuildings(Zerg_Hatchery, E);
		uList sunkenColonys = INFO.getBuildings(Zerg_Sunken_Colony, E);
		uList creepColonys = INFO.getBuildings(Zerg_Creep_Colony, E);


		if (setStrategyByEnemyBuildingInMyBase(hatcherys, Zerg_sunken_rush)
				|| setStrategyByEnemyBuildingInMyBase(sunkenColonys, Zerg_sunken_rush)
				|| setStrategyByEnemyBuildingInMyBase(creepColonys, Zerg_sunken_rush))
			return;

		int isCompleteSpawningPool = INFO.getCompletedCount(Zerg_Spawning_Pool, E);
		int isExistExtractor = INFO.getAllCount(Zerg_Extractor, E);

		uList spawningPoolList = INFO.getBuildings(Zerg_Spawning_Pool, E);

		if (spawningPoolList.empty())
			return;

		bool isExistSpawningPool = false;
		int spawningPoolHp = 0;

		for (auto s : spawningPoolList)
		{
			spawningPoolHp = s->hp();
			isExistSpawningPool = true;
		}

	
		if (isCompleteSpawningPool)
		{
			if (TIME < 24 * 110)
			{
				
				setEnemyInitialBuild(Zerg_4_Drone);
			}
		}

	
		if (isExistSpawningPool)
		{
			if (TIME >= 24 * 110 && TIME < 24 * 115)
			{
			
				if (spawningPoolHp > 600)
				{
				
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_Drone);
				}
				else if (spawningPoolHp <= 600)
				{
					
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_OverPool);
				}
			}
			else if (TIME >= 24 * 115 && TIME < 24 * 120)
			{
			
				if (spawningPoolHp > 650)
				{
				
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_Drone);
				}
				else if (spawningPoolHp <= 650)
				{
				
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_OverPool);
				}
			}
			else if (TIME >= 24 * 120 && TIME < 24 * 125)
			{
			
				if (spawningPoolHp > 740)
				{
				
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_Drone);
				}
				else if (spawningPoolHp <= 740)
				{
			
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_OverPool);
				}
			}
			else if (TIME >= 24 * 125 && TIME < 24 * 140)
			{
			

			}
			else
			{
				
			}
		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{

		uList forgeList = INFO.getBuildings(Protoss_Forge, E);
		uList cannons = INFO.getBuildings(Protoss_Photon_Cannon, E);

		if (!forgeList.empty()) {

			if (INFO.getFirstExpansionLocation(E) != nullptr && INFO.getFirstExpansionLocation(E)->GetArea() == theMap.GetArea((TilePosition)forgeList.at(0)->pos())) {
				setEnemyInitialBuild(Toss_forge);
				return;
			}
		}

		if (!cannons.empty()) {
			for (auto c : cannons) {

			
				if (INFO.getFirstExpansionLocation(S)->GetArea() == theMap.GetArea((TilePosition)c->pos()) ||
						INFO.getMainBaseLocation(S)->GetArea() == theMap.GetArea((TilePosition)c->pos())) {
					setEnemyInitialBuild(Toss_cannon_rush);
					return;
				}

				if (INFO.getFirstExpansionLocation(E) != nullptr && INFO.getFirstExpansionLocation(E)->GetArea() == theMap.GetArea((TilePosition)c->pos())) {
					setEnemyInitialBuild(Toss_forge);
					return;
				}

			}
		}
	}
	else if (INFO.enemyRace == Races::Terran)
	{
		uList barracks = INFO.getBuildings(Terran_Barracks, E);
		uList bunkers = INFO.getBuildings(Terran_Bunker, E);

		if (setStrategyByEnemyBuildingInMyBase(bunkers, Terran_bunker_rush)
				|| setStrategyByEnemyBuildingInMyBase(barracks, Terran_bunker_rush))
			return;
	}
	else
	{

	}
}

bool HostileManager::setStrategyByEnemyBuildingInMyBase(uList eBuildings, InitialBuildType initialBuildType) {
	if (!eBuildings.empty()) {
		for (auto b : eBuildings) {
			if (isSameArea(INFO.getFirstExpansionLocation(S)->GetArea(), theMap.GetArea((TilePosition)b->pos())) ||
					isSameArea(INFO.getMainBaseLocation(S)->GetArea(), theMap.GetArea((TilePosition)b->pos()))) {
				setEnemyInitialBuild(initialBuildType);
				return true;
			}
		}
	}

	return false;
}

void HostileManager::useFirstAttackUnit()
{
	if (INFO.enemyRace == Races::Zerg)
	{
		uList drones = INFO.getTypeUnitsInRadius(Zerg_Drone, E, INFO.getMainBaseLocation(S)->getPosition(), 1000);

		if (drones.size() >= 4) {
			setEnemyInitialBuild(Zerg_4_Drone_Real);
			return;
		}

		uList zerglingList = INFO.getUnits(Zerg_Zergling, E);

		if (zerglingList.empty()) return;

		int reachTime = INT_MAX;
		int reachCount = 0;

		Base *nearestBase = nullptr;
		Base *farthestBase = nullptr;

		int nearestDist = INT_MAX;
		int farthestDist = INT_MIN;

		for (auto b : INFO.getStartLocations()) {
			if (MYBASE == b->Center())
				continue;

			int dist = MYBASE.getApproxDistance(b->Center());

			if (nearestDist > dist) {
				nearestDist = dist;
				nearestBase = b;
			}

			if (farthestDist < dist) {
				farthestDist = dist;
				farthestBase = b;
			}
		}

		int fastestTime = 24 * 110 + int(getGroundDistance(MYBASE, nearestBase->Center()) / Zerg_Zergling.topSpeed());
		int slowestTime = fastestTime + int(getGroundDistance(nearestBase->Center(), farthestBase->Center()) / Zerg_Zergling.topSpeed()) + 24 * 10;

		for (auto z : zerglingList) {
			if (z->pos() == Positions::Unknown)
				continue;

			int distance = getGroundDistance(z->pos(), INFO.getFirstChokePosition(S));

			if (distance < 0)
				continue;

			int tmpTime = z->getLastPositionTime() + int(distance / z->type().topSpeed());

			if (reachTime > tmpTime)
				reachTime = tmpTime;

			if (tmpTime < slowestTime)
				reachCount++;
		}


		bool canChange = INFO.getAllCount(Terran_Factory, S) == 0;

		if (!canChange) {
			uList factoryList = INFO.getBuildings(Terran_Factory, S);

			for (auto f : factoryList) {
				if (!f->isComplete() && f->hp() < 300) {
					canChange = true;
					break;
				}
			}
		}

		if (canChange && reachTime < fastestTime + 24 * 10)
			setEnemyInitialBuild(Zerg_4_Drone, true);
		else if (canChange && reachTime < slowestTime && (reachCount > 6 || reachCount >= 5 && INFO.getCompletedCount(Zerg_Zergling, E) > 6))
			setEnemyInitialBuild(Zerg_4_Drone, true);
		else if (INFO.getCompletedCount(Terran_Vulture, S) <= 1 && INFO.getDestroyedCount(Terran_Vulture, S) == 0 && INFO.getCompletedCount(Terran_Machine_Shop, S) == 0 && zerglingList.size() >= 8 && HasEnemyFirstExpansion()) {
			int completedHatcheryCnt = 0;

			for (auto h : INFO.getBuildings(Zerg_Hatchery, E)) {
				if (h->getCompleteTime() < 24 * 180)
					completedHatcheryCnt++;
			}

			if (completedHatcheryCnt >= 2)
				setEnemyInitialBuild(Zerg_9_Hat, true);
		}

		if (enemyInitialBuild != UnknownBuild)
			return;

		if (TIME >= 24 * 140 && TIME < 24 * 165)
		{
			if (isTheUnitNearMyBase(zerglingList))
			{
				setEnemyInitialBuild(Zerg_4_Drone);
			}
			else
			{
				setEnemyInitialBuild(Zerg_9_Drone);
			}
		}
		else if (TIME >= 24 * 165 && TIME < 24 * 190)
		{
			if (isTheUnitNearMyBase(zerglingList))
			{
				setEnemyInitialBuild(Zerg_9_Drone);
			}
			else
			{
				setEnemyInitialBuild(Zerg_12_Pool);
			}
		}
		else if (TIME >= 24 * 190 && TIME < 24 * 220)
		{
			if (isTheUnitNearMyBase(zerglingList))
			{
				setEnemyInitialBuild(Zerg_12_Pool);
			}
			else
			{
				setEnemyInitialBuild(Zerg_12_Hat);
			}
		}
		else
		{

		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		uList probes = INFO.getTypeUnitsInRadius(Protoss_Probe, E, INFO.getMainBaseLocation(S)->getPosition(), 1000);

		if (probes.size() >= 4) {
			if (INFO.getCompletedCount(Protoss_Zealot, E) == 0 && INFO.getCompletedCount(Protoss_Dragoon, E) == 0) {
				setEnemyInitialBuild(Toss_4probes);
				return;
			}
		}

		uList zealotList = INFO.getUnits(Protoss_Zealot, E);
		uList gateWay = INFO.getBuildings(Protoss_Gateway, E);
		const Base *enemyMainBase = INFO.getMainBaseLocation(E);
		const Base *enemyFirstExpansionLocation = INFO.getFirstExpansionLocation(E);

		if (gateWay.empty() && !zealotList.empty() && isTheUnitNearMyBase(zealotList)) {

			int distance;
			theMap.GetPath(enemyMainBase->getPosition(), MYBASE, &distance);
			double baseToBaseOfZ = distance / Protoss_Zealot.topSpeed(); 
			double errorRate = 1.05;

			
			if ((double)TIME * errorRate < Protoss_Probe.buildTime() * 3.5 + Protoss_Pylon.buildTime()
					+ Protoss_Gateway.buildTime() + zealotList.size() * Protoss_Zealot.buildTime() + baseToBaseOfZ) {
				setEnemyInitialBuild(Toss_1g_forward);
			}

			
			if (zealotList.size() >= 2) {

				if ((double)TIME * errorRate < Protoss_Probe.buildTime() +
						timeToBuildGateWay + zealotList.size() * Protoss_Zealot.buildTime() + centerToBaseOfZ) {
					setEnemyInitialBuild(Toss_2g_forward);
				}

			}

		}


		if (INFO.getCompletedCount(Protoss_Zealot, E) + INFO.getDestroyedCount(Protoss_Zealot, E) >= 2) {

			if (enemyFirstExpansionLocation != nullptr && enemyMainBase != nullptr) {

				const Area *enemyFirstExpansionArea = theMap.GetArea(enemyFirstExpansionLocation->Location());
				const Area *enemyMainBaseArea = theMap.GetArea(enemyMainBase->Location());
				int outSideZCount = 0;
				int inSideZCount = 0;

				for (auto zealot : zealotList) {
					if (zealot->pos() != Positions::Unknown) {
						const BWEM::Area *zealotArea = theMap.GetArea((WalkPosition)zealot->pos());

						
						if (zealotArea != enemyFirstExpansionArea && zealotArea != enemyMainBaseArea)
							outSideZCount++;
						else
							inSideZCount++;
					}
				}

				bool flag = false;
				int currentFrameCnt = TIME;

			
				if (inSideZCount + outSideZCount == 2) {
					
					if (currentFrameCnt < 24 * 165)
						flag = true;
				}
				else if (inSideZCount + outSideZCount >= 3)
				{
			
					if (currentFrameCnt < 24 * 210)
						flag = true;
				}
				else if (outSideZCount == 2) {
					
					if (currentFrameCnt < 24 * 170)
						flag = true;
				}
				else if (outSideZCount == 3) {
					
					if (currentFrameCnt < 24 * 180)
						flag = true;
				}

				if (inSideZCount + outSideZCount >= 4) {
					flag = true;
				}

				if (flag)
					setEnemyInitialBuild(Toss_2g_zealot);
			}
		}

		uList dragoonList = INFO.getUnits(Protoss_Dragoon, E);

		if (dragoonList.size() != 0) {
			setEnemyInitialBuild(Toss_1g_dragoon);
		}

		if (dragoonList.size() == 3) {
		
			if (TIME < 24 * 280)
				setEnemyInitialBuild(Toss_2g_dragoon);

		}
		else if (dragoonList.size() >= 4) {
			if (TIME < 24 * 310)
				setEnemyInitialBuild(Toss_2g_dragoon);
		}

	}
	else if (INFO.enemyRace == Races::Terran)
	{
		uList scvs = INFO.getTypeUnitsInRadius(Terran_SCV, E, INFO.getMainBaseLocation(S)->getPosition(), 1000);

		if (scvs.size() >= 4) {
			setEnemyInitialBuild(Terran_4scv);
			return;
		}

	
		uList marines = INFO.getUnits(Terran_Marine, E);

		if (!marines.empty() && isTheUnitNearMyBase(marines)) {
			const Base *enemyMainBase = INFO.getMainBaseLocation(E);
			int distance;
			
			theMap.GetPath(enemyMainBase->getPosition(), INFO.getMainBaseLocation(S)->getPosition(), &distance);
			baseToBaseOfMarine = distance / Terran_Marine.topSpeed(); 

	
			if (TIME < Terran_SCV.buildTime() * 6 + Terran_Barracks.buildTime() + marines.size() * Terran_Marine.buildTime() + baseToBaseOfMarine) {
				setEnemyInitialBuild(Terran_1b_forward);
			}

	
			int allMarineCount = INFO.getAllCount(Terran_Marine, E) + INFO.getDestroyedCount(Terran_Marine, E);

			if (allMarineCount > 2) {
			

				if (TIME < Terran_SCV.buildTime() * 4 + timeToBuildBarrack + allMarineCount * Terran_Marine.buildTime() + centerToBaseOfMarine) {
					setEnemyInitialBuild(Terran_2b_forward);
				}

			}

		}

	}

}

bool HostileManager::timeBetween(int from, int to) {
	return TIME >= from && TIME <= to;
}

void HostileManager::useBuildingCount()
{
	if (INFO.enemyRace == Races::Zerg)
	{
		uList spawningPool = INFO.getBuildings(Zerg_Spawning_Pool, E);
		uList hatchery = INFO.getBuildings(Zerg_Hatchery, E);
		uList lair = INFO.getBuildings(Zerg_Lair, E);
		bool isExistMorphLair = false; 

		for (auto h : hatchery)
		{
			if (h->isComplete() && h->unit()->isMorphing())
			{
				isExistMorphLair = true;
			}
		}


		if (!lair.empty() || isExistMorphLair)
		{
			return;
		}

		for (auto s : spawningPool)
		{
			if (s->unit()->isUpgrading())
			{
				
				setEnemyInitialBuild(Zerg_9_Balup);
			}
		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		
		uList gatewayList = INFO.getBuildings(Protoss_Gateway, E);

		if (gatewayList.empty())
			return;

		int forwardGate = 0;

		
		int forwardThresholdGroundDistance = getGroundDistance(MYBASE, INFO.getSecondChokePosition(S)) + 5 * TILE_SIZE;

		for (auto gateway : gatewayList) {
			bool outSide = true;

			for (auto startBase : INFO.getStartLocations()) {

				if (startBase->GetOccupiedInfo() == myBase)
					continue;

				
				if (getGroundDistance(startBase->Center(), gateway->pos()) < forwardThresholdGroundDistance)
				{
					outSide = false;
					break;
				}
			}

			if (outSide)
				forwardGate++;
		}

		if (forwardGate == 1)
			setEnemyInitialBuild(Toss_1g_forward);
		else if (forwardGate > 1)
			setEnemyInitialBuild(Toss_2g_forward);

	
		uList assimilatorList = INFO.getBuildings(Protoss_Assimilator, E);
		uList cyCore = INFO.getBuildings(Protoss_Cybernetics_Core, E);

		if (gatewayList.size() >= 2) {

			TilePosition topLeft = TilePositions::None;
			TilePosition bottomRight = TilePositions::None;
			bool gasCheckFlag = false;

			for (auto geysers : INFO.getMainBaseLocation(E)->Geysers()) {
				topLeft = geysers->TopLeft();
				bottomRight = geysers->BottomRight();

				if ((topLeft != TilePositions::None && bw->isExplored(topLeft)) ||
						(bottomRight != TilePositions::None && bw->isExplored(bottomRight))) {
					gasCheckFlag = true;
				}

				break;
			}

			if (gasCheckFlag)
			{
				if (assimilatorList.size() == 0 && cyCore.size() == 0) {
					setEnemyInitialBuild(Toss_2g_zealot);
				}
				else
					setEnemyInitialBuild(Toss_2g_dragoon);
			}

			return;
		}
		else if (gatewayList.size() == 1) {
			if (assimilatorList.size() != 0 || cyCore.size() != 0)
				setEnemyInitialBuild(Toss_1g_dragoon);
		}

	}
	else if (INFO.enemyRace == BWAPI::Races::Terran)
	{
		
		uList barracks = INFO.getBuildings(Terran_Barracks, E);

		if (barracks.empty())
			return;

		int forwardBarrackCnt = 0;
		bool isForwardBuilding = true;

		for (auto barrack : barracks) {

			for (auto startPosition : theMap.StartingLocations()) {
				
				if (theMap.GetArea(startPosition) == theMap.GetArea((TilePosition)barrack->pos())
						&& theMap.GetArea(startPosition) != theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition())) {
					isForwardBuilding = false;
					break;
				}
			}

			if (isForwardBuilding)
				forwardBarrackCnt++;

			isForwardBuilding = true;
		}

		if (forwardBarrackCnt == 1)
			setEnemyInitialBuild(Terran_1b_forward);
		else if (forwardBarrackCnt > 1)
			setEnemyInitialBuild(Terran_2b_forward);

	
		if (barracks.size() >= 2)
			setEnemyInitialBuild(Terran_2b);

	}
	else
	{

	}
}

void HostileManager::checkEnemyBuilding()
{
	if (INFO.enemyRace == Races::Zerg)
	{
		uList hatchery = INFO.getBuildings(Zerg_Hatchery, E);
		uList lair = INFO.getBuildings(Zerg_Lair, E);
		uList hydraliskDen = INFO.getBuildings(Zerg_Hydralisk_Den, E);
		uList spire = INFO.getBuildings(Zerg_Spire, E);
		bool isExistMorphLair = false; 

		for (auto h : hatchery)
		{
			if (h->isComplete() && h->unit()->isMorphing())
			{
				isExistMorphLair = true;
			}
		}

		if (!lair.empty() || isExistMorphLair)
		{
			setEnemyMainBuild(Zerg_main_maybe_mutal);
		}

		if (!hydraliskDen.empty())
		{
			
			if (!lair.empty() || isExistMorphLair)
			{
				setEnemyMainBuild(Zerg_main_lurker);
			}
		}


		if (!spire.empty())
		{
			setEnemyMainBuild(Zerg_main_mutal);

			if (TIME < 24 * 60 * 5)
				setEnemyMainBuild(Zerg_main_fast_mutal);
		}

		
		if (!hydraliskDen.empty() && !spire.empty())
		{
			setEnemyMainBuild(Zerg_main_hydra_mutal);
		}
	}
	else if (INFO.enemyRace == Races::Protoss) {

		
		uList stargate = INFO.getBuildings(Protoss_Stargate, E);
		uList fleetBeacon = INFO.getBuildings(Protoss_Fleet_Beacon, E);
		uList templarArchives = INFO.getBuildings(Protoss_Templar_Archives, E);
		uList adun = INFO.getBuildings(Protoss_Citadel_of_Adun, E);
		
		int templers = INFO.getUnits(Protoss_High_Templar, E).size() + INFO.getUnits(Protoss_Dark_Templar, E).size();
		uList scouts = INFO.getUnits(Protoss_Scout, E);

		uList gateWay = INFO.getBuildings(Protoss_Gateway, E);
		uList nexus = INFO.getBuildings(Protoss_Nexus, E);

		if (gateWay.size() >= 12 || nexus.size() >= 4) {
			setEnemyMainBuild(Toss_mass_production);
		}

		if (stargate.size() >= 1) {
			if ((!adun.empty() || !templarArchives.empty() || templers != 0) && fleetBeacon.empty()
					&& nexus.size() >= 2)
				setEnemyMainBuild(Toss_arbiter);
			else {

				if (!scouts.empty())
					setEnemyMainBuild(Toss_Scout);
				else
				{
					if (nexus.size() >= 2)
						setEnemyMainBuild(Toss_fast_carrier);
				}
			}
		}

		if (!fleetBeacon.empty()) {
			setEnemyMainBuild(Toss_fast_carrier);
		}

		uList arbiterTribunal = INFO.getBuildings(Protoss_Arbiter_Tribunal, E);

		if (!arbiterTribunal.empty()) {
			setEnemyMainBuild(Toss_arbiter);

			if (!fleetBeacon.empty())
				setEnemyMainBuild(Toss_arbiter_carrier);

			return;
		}

		uList robotics = INFO.getBuildings(Protoss_Robotics_Facility, E);

		if (!adun.empty() || !templarArchives.empty()) {
			if (!robotics.empty())
				setEnemyMainBuild(Toss_dark_drop);
			else
				setEnemyMainBuild(Toss_dark);

			return;
		}

		if (TIME < 24 * 420 && nexus.size() >= 3) {
			setEnemyMainBuild(Toss_pure_tripple);
			return;
		}

		// 8 ~ 10분
		if (TIME > 24 * 480 && TIME < 24 * 600) {
			if (nexus.size() < 3) {
				if (INFO.getSecondExpansionLocation(E)
						&& bw->isExplored((TilePosition)INFO.getSecondExpansionLocation(E)->Center())) {
					setEnemyMainBuild(Toss_2base_zealot_dra);
					return;
				}
			}
		}

		uList roboticsSupport = INFO.getBuildings(Protoss_Robotics_Support_Bay, E);
		uList observatory = INFO.getBuildings(Protoss_Observatory, E);

		if (!observatory.empty() || !robotics.empty() || !roboticsSupport.empty()) {
			if (!roboticsSupport.empty())
				setEnemyMainBuild(Toss_reaver_drop);
			else
				setEnemyMainBuild(Toss_drop);

			return;
		}

	}
	else {
		uList commandCenterList = INFO.getBuildings(Terran_Command_Center, E);
		uList factoryList = INFO.getBuildings(Terran_Factory, E);
		uList starportList = INFO.getBuildings(Terran_Starport, E);

		if (factoryList.size() == 1)
		{
			if (commandCenterList.size() > 1)
			{
				setEnemyMainBuild(Terran_1fac_double);
			}
		}
		else if (factoryList.size() == 2)
		{
			setEnemyMainBuild(Terran_2fac);

			if (commandCenterList.size() > 1)
			{
				setEnemyMainBuild(Terran_2fac_double);
			}

			if (starportList.size() >= 1)
			{
				setEnemyMainBuild(Terran_2fac_1star);
			}
		}
		else if (factoryList.size() == 3)
		{
			setEnemyMainBuild(Terran_3fac);
		}
		else
		{
		}

		if (starportList.size() == 1)
		{
			setEnemyMainBuild(Terran_1fac_1star);
		}
		else if (starportList.size() >= 2)
		{
			setEnemyMainBuild(Terran_1fac_2star);
		}
	}

}

void HostileManager::checkEnemyUnits()
{
	if (INFO.enemyRace == Races::Zerg)
	{
		int zerglingAllCount = INFO.getAllCount(Zerg_Zergling, E) + INFO.getDestroyedCount(Zerg_Zergling, E);
		int hydraAllCount = INFO.getAllCount(Zerg_Hydralisk, E) + INFO.getDestroyedCount(Zerg_Hydralisk, E);
		int lurkerAllCount = INFO.getAllCount(Zerg_Lurker, E) + INFO.getDestroyedCount(Zerg_Lurker, E);
		uList hydraliskList = INFO.getUnits(Zerg_Hydralisk, E);
		uList lurkerList = INFO.getUnits(Zerg_Lurker, E);
		uList lurkerEggList = INFO.getUnits(Zerg_Lurker_Egg, E);
		uList mutalList = INFO.getUnits(Zerg_Mutalisk, E);
		uList queenList = INFO.getUnits(Zerg_Queen, E);
		uList hatchery = INFO.getBuildings(Zerg_Hatchery, E);
		uList lair = INFO.getBuildings(Zerg_Lair, E);
		bool isExistMorphLair = false; 

		for (auto h : hatchery)
		{
			if (h->isComplete() && h->unit()->isMorphing())
			{
				isExistMorphLair = true;
			}
		}

		if (zerglingAllCount > 9)
		{
			setEnemyMainBuild(Zerg_main_zergling);
		}

		if (hydraAllCount)
		{
			if (hydraAllCount >= 6)
				setEnemyMainBuild(Zerg_main_hydra);

		
			if (!lair.empty() || isExistMorphLair)
			{
				setEnemyMainBuild(Zerg_main_lurker);
			}

		
			if ((enemyMainBuild == Zerg_main_zergling || E->getUpgradeLevel(UpgradeTypes::Metabolic_Boost))
					&& hatchery.size() <= 2 && TIME < 24 * 330)
			{
				setEnemyMainBuild(Zerg_main_lurker);
			}
		}

	
		if (!(hydraAllCount + lurkerAllCount))
		{
	
			if (enemyInitialBuild != Zerg_4_Drone && enemyMainBuild != Zerg_main_lurker
					&& (!INFO.isEnemyBaseFound || (hatchery.size() + lair.size()) <= 2) && TIME > 24 * 330 && TIME < 24 * 450)
			{
				setEnemyMainBuild(Zerg_main_fast_mutal);
			}
		}

		if (!lurkerList.empty() || !lurkerEggList.empty())
		{
			setEnemyMainBuild(Zerg_main_lurker);
		}

		
		if (enemyMainBuild == Zerg_main_lurker && lurkerList.empty())
		{
			
			if (hydraAllCount >= 6)
				setEnemyMainBuild(Zerg_main_hydra_mutal);
		}

		if (!mutalList.empty())
		{
			setEnemyMainBuild(Zerg_main_mutal);

			if (TIME < 24 * 60 * 6)
				setEnemyMainBuild(Zerg_main_fast_mutal);
		}

		if ((hydraAllCount + lurkerAllCount) >= 6
				&& (mutalList.size() >= 6 || enemyMainBuild == Zerg_main_mutal || enemyMainBuild == Zerg_main_fast_mutal))
		{
			setEnemyMainBuild(Zerg_main_hydra_mutal);
		}

		if (!hydraliskList.empty() && !queenList.empty())
		{
			setEnemyMainBuild(Zerg_main_queen_hydra);
		}
	}
	else if (INFO.enemyRace == Races::Protoss) {

		uList scouts = INFO.getUnits(Protoss_Scout, E);

		if (!scouts.empty()) {
			setEnemyMainBuild(Toss_Scout);
		}

		uList carrier = INFO.getUnits(Protoss_Carrier, E);

		if (!carrier.empty()) {
			setEnemyMainBuild(Toss_fast_carrier);
		}

		uList arbiter = INFO.getUnits(Protoss_Arbiter, E);

		if (!carrier.empty() && !arbiter.empty())
		{
			setEnemyMainBuild(Toss_arbiter_carrier);
			return;
		}

		if (!arbiter.empty()) {
			setEnemyMainBuild(Toss_arbiter);
			return;
		}


		if (TIME >= 5 * 24 * 60 && TIME <= 8 * 24 * 60) {

			int zealots = INFO.getUnits(Protoss_Zealot, E).size();
			int dragoon = INFO.getUnits(Protoss_Dragoon, E).size();

			if (HasEnemyFirstExpansion()) {

				if (zealots >= 10 || zealots + dragoon >= 20) {
					setEnemyMainBuild(Toss_2base_zealot_dra);
					return;
				}
			}
			else {

				if ((zealots >= 7 || (zealots > dragoon && zealots + dragoon >= 12))
						&& enemyInitialBuild != Toss_2g_forward) {
					setEnemyMainBuild(Toss_1base_fast_zealot);
					return;
				}
			}

		}

		uList darkTempler = INFO.getUnits(Protoss_Dark_Templar, E);
		uList shuttle = INFO.getUnits(Protoss_Shuttle, E);

		if (!darkTempler.empty()) {
			if (!shuttle.empty())
				setEnemyMainBuild(Toss_dark_drop);
			else
				setEnemyMainBuild(Toss_dark);

			return;
		}

		uList reaver = INFO.getUnits(Protoss_Reaver, E);

		if (!reaver.empty()) {
			setEnemyMainBuild(Toss_reaver_drop);
			return;
		}

		// 셔틀이 있다고 무조건 셔틀 드랍은 아니다.
		if (!shuttle.empty()) {
			setEnemyMainBuild(Toss_drop);
			return;
		}

	}
	else if (INFO.enemyRace == Races::Terran)
	{
		uList wraithList = INFO.getUnits(Terran_Wraith, E);
		uList dropshipList = INFO.getUnits(Terran_Dropship, E);
		uList factoryList = INFO.getBuildings(Terran_Factory, E);

		if (enemyMainBuild != Terran_1fac_double && (!wraithList.empty() || !dropshipList.empty()))
		{
			if (factoryList.size() == 1)
			{
				setEnemyMainBuild(Terran_1fac_1star);
			}
			else if (factoryList.size() > 1)
			{
				setEnemyMainBuild(Terran_2fac_1star);
			}
		}
	}
	else
	{
	}
}

void HostileManager::checkEnemyTechNUpgrade()
{
	if (INFO.enemyRace == Races::Zerg)
	{
		uList spawningPool = INFO.getBuildings(Zerg_Spawning_Pool, E);
		uList hydraliskDen = INFO.getBuildings(Zerg_Hydralisk_Den, E);

		for (auto s : spawningPool)
		{
			if (s->unit()->isUpgrading() || E->getUpgradeLevel(UpgradeTypes::Metabolic_Boost))
			{
				
				setEnemyMainBuild(Zerg_main_zergling);
			}
		}

		for (auto h : hydraliskDen)
		{
			
			if (h->unit()->isUpgrading()
					|| E->getUpgradeLevel(UpgradeTypes::Muscular_Augments)
					|| E->getUpgradeLevel(UpgradeTypes::Grooved_Spines))
			{
				setEnemyMainBuild(Zerg_main_hydra);
			}

		
			if (h->unit()->isResearching() || INFO.hasRearched(TechTypes::Lurker_Aspect))
			{
				setEnemyMainBuild(Zerg_main_lurker);
			}
		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		
		uList cyCoreList = INFO.getBuildings(Protoss_Cybernetics_Core, E);
		uList pylonList = INFO.getBuildings(Protoss_Pylon, E);
		uList gateList = INFO.getBuildings(Protoss_Gateway, E);
		uList dra = INFO.getUnits(Protoss_Dragoon, E);

		if (!cyCoreList.empty() || !dra.empty()) {
		

			if ((!cyCoreList.empty() && cyCoreList.at(0)->unit()->isUpgrading()) || pylonList.size() > 2) {

				if (!cyCoreList.empty() && cyCoreList.at(0)->unit()->isUpgrading())
				{
					setEnemyMainBuild(Toss_range_up_dra_push);

					if (gateList.size() >= 2)
						setEnemyMainBuild(Toss_2gate_dra_push);
				}
			}
		}

		uList dragoons = INFO.getTypeUnitsInRadius(Protoss_Dragoon, E, INFO.getFirstExpansionLocation(S)->getPosition(), 30 * TILE_SIZE);

		if (E->getUpgradeLevel(UpgradeTypes::Singularity_Charge) && !dragoons.empty()) {
			setEnemyMainBuild(Toss_range_up_dra_push);

			if (gateList.size() >= 2)
				setEnemyMainBuild(Toss_2gate_dra_push);

			return;
		}
	}
	else if (INFO.enemyRace == Races::Terran)
	{
	}
	else
	{
	}
}

bool HostileManager::isTheUnitNearMyBase(uList unitList)
{
	Position myMainBasePosition = INFO.getMainBaseLocation(S)->Center();
	Position myFirstExpansionPosition = INFO.getFirstExpansionLocation(S)->Center();
	int distanceFromMainBase = 0;
	int distanceFromFirstExpansion = 0;
	bool isExist = false;

	for (auto u : unitList)
	{
		distanceFromMainBase = myMainBasePosition.getApproxDistance(u->pos());
		distanceFromFirstExpansion = myFirstExpansionPosition.getApproxDistance(u->pos());

		
		if (distanceFromMainBase <= 25 * TILE_SIZE || distanceFromFirstExpansion <= 25 * TILE_SIZE)
		{
			isExist = true;
			break;
		}
	}

	return isExist;
}

void HostileManager::checkDarkDropHadCome()
{
	if (getWaitTimeForDrop() < 0)
		return;

	uList shuttles = INFO.getTypeUnitsInArea(Protoss_Shuttle, E, MYBASE);
	uList darks = INFO.getTypeUnitsInArea(Protoss_Dark_Templar, E, MYBASE);


	if (!shuttles.empty() && !darks.empty()) {
		setWaitTimeForDrop(-1);
		return;
	}

	if (SM.getMainStrategy() == AttackAll)
		setWaitTimeForDrop(-2);

	if (getWaitTimeForDrop() == -1 && shuttles.empty() && darks.empty())
		setWaitTimeForDrop(-2);

	return;
}