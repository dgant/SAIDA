#include "EnemyStrategyManager.h"

using namespace MyBot;

EnemyStrategyManager &EnemyStrategyManager::Instance()
{
	static EnemyStrategyManager instance;
	return instance;
}

void EnemyStrategyManager::update()
{
	if (TIME < 24 * 30 || TIME % 12 != 0)	return;

	if (TIME < 24 * 300)
		checkEnemyInitialBuild();

	// 적 본적 없으면 스캔 그냥 쓴다.
	if (INFO.isEnemyBaseFound == false && INFO.getAvailableScanCount() > 0)
		ComsatStationManager::Instance().useScan(INFO.getMainBaseLocation(E)->Center());

	//3분 30초에서 15분 사이
	if (TIME > 24 * 180)
		checkEnemyMainBuild();

	if (TIME < 24 * 60 * 10)
		checkDarkDropHadCome();
}

InitialBuildType EnemyStrategyManager::getEnemyInitialBuild()
{
	return enemyInitialBuild;
}

MainBuildType EnemyStrategyManager::getEnemyMainBuild()
{
	return enemyMainBuild;
}

void EnemyStrategyManager::setEnemyMainBuild(MainBuildType st)
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

		//if (st == Toss_Scout) {
		//	enemyMainBuild = Toss_Scout;
		//} else
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

void EnemyStrategyManager::setEnemyInitialBuild(InitialBuildType ib, bool force)
{
	InitialBuildType currentStrategy = enemyInitialBuild;

	if (force) {
		if (enemyInitialBuild != ib)
			enemyInitialBuild = ib;
	}
	else {
		// EnemyInitail Build 변경
		if (enemyInitialBuild == UnknownBuild || enemyInitialBuild < ib)
			enemyInitialBuild = ib;
	}

	if (enemyInitialBuild != currentStrategy) {
		BasicBuildStrategy::Instance().onInitialBuildChange(currentStrategy, enemyInitialBuild);
		eibHistory.emplace_back(enemyInitialBuild, TIME);
		clearBuildConstructQueue();
	}
}

// 적의 전략이 바꼈기 때문에 큐를 비워준다.
void EnemyStrategyManager::clearBuildConstructQueue() {
	CM.clearNotUnderConstruction();
	BM.buildQueue.clearAll();
	BasicBuildStrategy::Instance().executeBasicBuildStrategy();
}

EnemyStrategyManager::EnemyStrategyManager()
{
	enemyInitialBuild = UnknownBuild;
	enemyMainBuild = UnknownMainBuild;
	enemyGasRushRefinery = nullptr;

	int distance = getGroundDistance(theMap.Center(), MYBASE);

	// SCV가 맵의 중앙까지 가서 배럭을 짓는데 걸리는 시간
	centerToBaseOfSCV = distance / Terran_SCV.topSpeed();
	timeToBuildBarrack = (int)centerToBaseOfSCV + Terran_Barracks.buildTime();

	// 프로브가 맵의 중앙까지 가서 게이트 웨이를 짓는 시간
	centerToBaseOfProbe = distance / Protoss_Probe.topSpeed();
	timeToBuildGateWay = (int)centerToBaseOfProbe + Protoss_Gateway.buildTime() + Protoss_Pylon.buildTime();

	// 마린이 맵의 중앙에서 내 본진까지 오는데 걸리는 시간
	centerToBaseOfMarine = distance / Terran_Marine.topSpeed(); // 센터 어딘가에서 출발한 마린이 우리 본진까지 도착하는데 걸리는 시간 예측
	centerToBaseOfZ = distance / Protoss_Zealot.topSpeed(); // 센터 어딘가에서 출발한 질럿이 우리 본진까지 도착하는데 걸리는 시간 예측
}

void EnemyStrategyManager::checkEnemyInitialBuild()
{
	// 프로토스 전략은 병력이나 건물 수를 계속 확인하면서 빌드를 갱신해야 한다.
	if (INFO.enemyRace == Races::Zerg && enemyInitialBuild != UnknownBuild) {
		// 9드론 판별된 경우에는 추가되는 저글링 수에 따라 4드론으로 바뀔수 있다.
		// 12앞마당 or 12해쳐리인 경우에 9해쳐리로 변경 가능하다.
		if (enemyInitialBuild == Zerg_9_Drone || enemyInitialBuild == Zerg_12_Ap || enemyInitialBuild == Zerg_12_Hat)
			useFirstAttackUnit();

		return;
	}

	if (!INFO.getUnitInfo(enemyGasRushRefinery, E)) {
		// 내 기지 안에 가스를 지은 경우
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

void EnemyStrategyManager::checkEnemyMainBuild()
{
	checkEnemyBuilding();
	checkEnemyUnits();
	checkEnemyTechNUpgrade();
}

void EnemyStrategyManager::useFirstExpansion()
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
			cout << "해쳐리 지은 시간 추정 : " << buildStartFrame / (24 * 60) << "분 " << (buildStartFrame / 24) % 60 << "초" << endl;

			if (buildStartFrame < 90 * 24)
				setEnemyInitialBuild(Zerg_9_Hat);
			else if (buildStartFrame < 120 * 24)
				setEnemyInitialBuild(Zerg_12_Ap);

			//else if (buildStartFrame < 130 * 24)
			//	setEnemyInitialBuild(Zerg_9_Drone);
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

		//앞마당 넥서스 확인
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
				// 드라군 사정거리 업그레이드 중이거나, 파일런 2개의 위치가 다 파악이 되면
				// 변칙 빌드라고 보기 어렵다.

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
			// 이미 완성되어 있으면 타이밍을 판단하기 어렵다.
			if (secondNexus->isComplete()) {
				// 1게이트 더블이라면, 보통 3분에 이후에 짓는다.
				// [3분 30초 + 걸리는 시간] 이전에 정찰이 갔는데 이미 완료되어 있으면 쌩더블
				if (currentFrameCnt <= (24 * 180 + Protoss_Nexus.buildTime()))
					setEnemyInitialBuild(Toss_pure_double);
				else
					setEnemyInitialBuild(Toss_1g_double);
			}
			// 만드는 중이면 HP를 보고 언제 지었는지 알 수 있다.
			else {
				// 현재 HP / 속도 = 걸린 시간
				// 현재 카운트 - 걸린시간 = 시작시간 유추 가능
				int elapsedFrameCount = (int)(secondNexus->hp() / nexusBuildSpd_per_frmCnt);
				int expectedFrameCount = currentFrameCnt - elapsedFrameCount;

				// Toss_pure_double, 쌩더블
				// [1:50 ~ 2:10]
				if (expectedFrameCount >= 24 * 110 && expectedFrameCount <= 24 * 130) {
					setEnemyInitialBuild(Toss_pure_double);
				}
				// [3:30 ~ 4:10]
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

		//앞마당 커맨드 확인
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

			// 이미 완성되어 있으면 타이밍을 판단하기 어렵다.
			if (secondCommandCenter->isComplete()) {
				// 1배럭 더블이라면, 보통 2분20~30에 이후에 짓는다.
				// [3분 30초 + 걸리는 시간] 이전에 정찰이 갔는데 이미 완료되어 있으면 쌩더블
				if (currentFrameCnt < (24 * 140 + Terran_Command_Center.buildTime()))
					setEnemyInitialBuild(Terran_pure_double);
				else
					setEnemyInitialBuild(Terran_1b_double);
			}
			// 만드는 중이면 HP를 보고 언제 지었는지 알 수 있다.
			else {
				// 현재 HP / 속도 = 걸린 시간
				// 현재 카운트 - 걸린시간 = 시작시간 유추 가능
				int elapsedFrameCount = (int)(secondCommandCenter->hp() / commandBuildSpd_per_frmCnt);
				int expectedFrameCount = currentFrameCnt - elapsedFrameCount;

				// 쌩더블
				// [2:00 ~ 2:10]
				if (expectedFrameCount >= 24 * 120 && expectedFrameCount <= 24 * 130) {
					setEnemyInitialBuild(Terran_pure_double);
				}
				// 1배럭 더블 [2:11 ~ 2:30]
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

void EnemyStrategyManager::useFirstTechBuilding()
{
	if (INFO.enemyRace == Races::Zerg)
	{
		uList hatcherys = INFO.getBuildings(Zerg_Hatchery, E);
		uList sunkenColonys = INFO.getBuildings(Zerg_Sunken_Colony, E);
		uList creepColonys = INFO.getBuildings(Zerg_Creep_Colony, E);

		// 해처리, 성큰, 크립이 우리편 앞마당 혹은 본진에 있는 경우 성큰러쉬 (TODO)
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

		// 스포닝 완성된 타이밍으로 계산
		if (isCompleteSpawningPool)
		{
			if (TIME < 24 * 110)
			{
				// [~ 01:50) 이미 건설 완료된 스포닝풀이 있을 경우 4~5드론
				setEnemyInitialBuild(Zerg_4_Drone);
			}
		}

		// 건설중인 스포닝 HP로 유추
		// 현재는 9풀인지만 확인, 혹시 정찰 빨리가게 될 경우 12풀까지는 확인로직 추가해야 할 듯
		if (isExistSpawningPool)
		{
			if (TIME >= 24 * 110 && TIME < 24 * 115)
			{
				// [1:50 ~ 01:55) 스포닝풀 HP 600 기준으로 9풀, 9오버풀 구분
				if (spawningPoolHp > 600)
				{
					// 가스가 있으면 발업링
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_Drone);
				}
				else if (spawningPoolHp <= 600)
				{
					// 가스가 있으면 발업링
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_OverPool);
				}
			}
			else if (TIME >= 24 * 115 && TIME < 24 * 120)
			{
				// [1:55 ~ 02:00) 스포닝풀 HP 650 기준으로 9풀, 9오버풀 구분
				if (spawningPoolHp > 650)
				{
					// 가스가 있으면 발업링
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_Drone);
				}
				else if (spawningPoolHp <= 650)
				{
					// 가스가 있으면 발업링
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_OverPool);
				}
			}
			else if (TIME >= 24 * 120 && TIME < 24 * 125)
			{
				// [2:00 ~ 02:05) 스포닝풀 HP 740 기준으로 9풀, 9오버풀 구분
				if (spawningPoolHp > 740)
				{
					// 가스가 있으면 발업링
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_Drone);
				}
				else if (spawningPoolHp <= 740)
				{
					// 가스가 있으면 발업링
					if (isExistExtractor)
						setEnemyInitialBuild(Zerg_9_Balup);
					else
						setEnemyInitialBuild(Zerg_9_OverPool);
				}
			}
			else if (TIME >= 24 * 125 && TIME < 24 * 140)
			{
				// [2:05 ~ 02:20) 이때부터는 9풀 확인 불가능, 12풀 or 12앞이냐 확인

			}
			else
			{
				// 이 후 시간대는 12풀 or 12햇 확인만 가능
			}
		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		// # 포지 더블 체크
		// 캐논 러쉬인지 체크 필요
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

				// 캐논이 우리편 앞마당 혹은 본진에 있는 경우
				if (INFO.getFirstExpansionLocation(S)->GetArea() == theMap.GetArea((TilePosition)c->pos()) ||
						INFO.getMainBaseLocation(S)->GetArea() == theMap.GetArea((TilePosition)c->pos())) {
					setEnemyInitialBuild(Toss_cannon_rush);
					return;
				}

				//캐논이 적 앞마당에 있는 경우
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

		// 배럭이나 벙커가 우리편 앞마당 혹은 본진에 있는 경우 벙커러시
		if (setStrategyByEnemyBuildingInMyBase(bunkers, Terran_bunker_rush)
				|| setStrategyByEnemyBuildingInMyBase(barracks, Terran_bunker_rush))
			return;
	}
	else
	{

	}
}

bool EnemyStrategyManager::setStrategyByEnemyBuildingInMyBase(uList eBuildings, InitialBuildType initialBuildType) {
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

void EnemyStrategyManager::useFirstAttackUnit()
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

		// 바로 오는데 걸리는 시간
		int fastestTime = 24 * 110 + int(getGroundDistance(MYBASE, nearestBase->Center()) / Zerg_Zergling.topSpeed());
		// 돌아오는데 걸리는 시간
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

		//cout << "예상 도착 시간 : " << reachTime << " 최단 도착 시간 : " << fastestTime << " 최장 도착 시간 : " << slowestTime << " 최장도착시간 내 도달 가능 저글링 수 : " << reachCount << endl;

		// 팩토리가 이미 있는 경우 체력이 300 미만일 경우만 4드론으로 변경시킨다.
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

		// 저글링이 앞마당에 최단시간 + 10초 내에 도착하면 4드론
		if (canChange && reachTime < fastestTime + 24 * 10)
			setEnemyInitialBuild(Zerg_4_Drone, true);
		else if (canChange && reachTime < slowestTime && (reachCount > 6 || reachCount >= 5 && INFO.getCompletedCount(Zerg_Zergling, E) > 6))
			setEnemyInitialBuild(Zerg_4_Drone, true);
		else if (INFO.getCompletedCount(Terran_Vulture, S) <= 1 && INFO.getDestroyedCount(Terran_Vulture, S) == 0 && INFO.getCompletedCount(Terran_Machine_Shop, S) == 0 && zerglingList.size() >= 8 && HasEnemyFirstExpansion()) {
			int completedHatcheryCnt = 0;

			for (auto h : INFO.getBuildings(Zerg_Hatchery, E)) {
				// 첫번째 서치가 안될 수 있으므로..
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
			// [2:20 ~ 02:45) 저글링 위치에 따라 내 본진, 앞마당 근처인 경우 4~5드론, 아니면 9드론 구분
			if (isTheUnitNearMyBase(zerglingList))
			{
				// 내 본진과 앞마당 기준 25 TILE 내에 저글링이 보이면 4~5드론
				setEnemyInitialBuild(Zerg_4_Drone);
			}
			else
			{
				// 내 본진 근처에서 발견되지 않았으면 9드론
				setEnemyInitialBuild(Zerg_9_Drone);
			}
		}
		else if (TIME >= 24 * 165 && TIME < 24 * 190)
		{
			// [2:45 ~ 03:10) 저글링 위치에 따라 내 본진, 앞마당 근처인 경우 9드론, 아니면 12드론 구분
			if (isTheUnitNearMyBase(zerglingList))
			{
				// 내 본진과 앞마당 기준 25 TILE 내에 저글링이 보이면 9드론
				setEnemyInitialBuild(Zerg_9_Drone);
			}
			else
			{
				// 내 본진 근처에서 발견되지 않았으면 12드론
				setEnemyInitialBuild(Zerg_12_Pool);
			}
		}
		else if (TIME >= 24 * 190 && TIME < 24 * 220)
		{
			// [3:10 ~ 03:40) 12풀 or 12앞.
			if (isTheUnitNearMyBase(zerglingList))
			{
				// 내 본진과 앞마당 기준 25 TILE 내에 저글링이 보이면 12드론
				setEnemyInitialBuild(Zerg_12_Pool);
			}
			else
			{
				// 내 본진 근처에서 발견되지 않았으면 12앞
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

		// 일단은 본진 근처에서 적 유닛을 봤을때 기준으로 판단..
		// 적 게이트를 못봣을 때만
		if (gateWay.empty() && !zealotList.empty() && isTheUnitNearMyBase(zealotList)) {

			int distance;
			// 적 본진에서 내 본진까지 질럿이 오는 시간
			theMap.GetPath(enemyMainBase->getPosition(), MYBASE, &distance);
			double baseToBaseOfZ = distance / Protoss_Zealot.topSpeed(); // 센터 어딘가에서 출발한 질럿이 우리 본진까지 도착하는데 걸리는 시간 예측
			double errorRate = 1.05;

			// [전진 1게이트]
			// 정석 보다 빠르게 도착하면
			if ((double)TIME * errorRate < Protoss_Probe.buildTime() * 3.5 + Protoss_Pylon.buildTime()
					+ Protoss_Gateway.buildTime() + zealotList.size() * Protoss_Zealot.buildTime() + baseToBaseOfZ) {
				setEnemyInitialBuild(Toss_1g_forward);
			}

			// [전진 2게이트]
			// 전진 1게이트보다 질럿이 빠르면
			if (zealotList.size() >= 2) {

				if ((double)TIME * errorRate < Protoss_Probe.buildTime() +
						timeToBuildGateWay + zealotList.size() * Protoss_Zealot.buildTime() + centerToBaseOfZ) {
					setEnemyInitialBuild(Toss_2g_forward);
				}

			}

		}

		// [2게이트 질럿 판단]
		// 질럿의 숫자를 보고 전략판단.
		if (INFO.getCompletedCount(Protoss_Zealot, E) + INFO.getDestroyedCount(Protoss_Zealot, E) >= 2) {

			if (enemyFirstExpansionLocation != nullptr && enemyMainBase != nullptr) {

				const Area *enemyFirstExpansionArea = theMap.GetArea(enemyFirstExpansionLocation->Location());
				const Area *enemyMainBaseArea = theMap.GetArea(enemyMainBase->Location());
				int outSideZCount = 0;
				int inSideZCount = 0;

				for (auto zealot : zealotList) {
					if (zealot->pos() != Positions::Unknown) {
						const BWEM::Area *zealotArea = theMap.GetArea((WalkPosition)zealot->pos());

						// 질럿이 적의 앞마당이나 본진이 아닌 곳에서 발견된 경우,
						if (zealotArea != enemyFirstExpansionArea && zealotArea != enemyMainBaseArea)
							outSideZCount++;
						else
							inSideZCount++;
					}
				}

				bool flag = false;
				int currentFrameCnt = TIME;

				// [1게이트 질럿 생산 타이밍]
				// 2:20, 2:45, 3:10
				// [2게이트 질럿 생산 타이밍]
				// 2:20, 2:35, 2:45, 3:00, 3:10
				if (inSideZCount + outSideZCount == 2) {
					// 투게이트가 아니면 이시간전에 절대 질럿 2마리가 나올 수 없다.
					if (currentFrameCnt < 24 * 165)
						flag = true;
				}
				else if (inSideZCount + outSideZCount >= 3)
				{
					// 투게이트가 아니면 이시간전에 절대 질럿 3마리 이상 나올 수 없다.
					if (currentFrameCnt < 24 * 210)
						flag = true;
				}
				else if (outSideZCount == 2) {
					// 투게이트가 아니면 이 시간전에 절대 질럿 2를 밖에서 볼 수가 없다.
					if (currentFrameCnt < 24 * 170)
						flag = true;
				}
				else if (outSideZCount == 3) {
					// 투게이트가 아니면 이 시간전에 절대 질럿 3기 이상을 밖에서 볼 수가 없다.
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
			// 4분 40초 이전에 3마리 보면..
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

		// 마린의 숫자를 보고 전진 배럭 전략판단.
		uList marines = INFO.getUnits(Terran_Marine, E);

		// 일단은 본진 근처에서 적 유닛을 봤을때 기준으로 판단..
		if (!marines.empty() && isTheUnitNearMyBase(marines)) {
			const Base *enemyMainBase = INFO.getMainBaseLocation(E);
			int distance;
			// 적 본진에서 내 본진까지 마린이 오는 시간
			theMap.GetPath(enemyMainBase->getPosition(), INFO.getMainBaseLocation(S)->getPosition(), &distance);
			baseToBaseOfMarine = distance / Terran_Marine.topSpeed(); // 센터 어딘가에서 출발한 마린이 우리 본진까지 도착하는데 걸리는 시간 예측

			// [전진 1배럭]
			// 정석 보다 빠르게 도착하면
			if (TIME < Terran_SCV.buildTime() * 6 + Terran_Barracks.buildTime() + marines.size() * Terran_Marine.buildTime() + baseToBaseOfMarine) {
				setEnemyInitialBuild(Terran_1b_forward);
			}

			// [전진 2배럭]
			int allMarineCount = INFO.getAllCount(Terran_Marine, E) + INFO.getDestroyedCount(Terran_Marine, E);

			if (allMarineCount > 2) {
				//cout << "Frame = " << Terran_SCV.buildTime() * 3 + timeToBuildBarrack + marines.size() * Terran_Marine.buildTime() + centerToBaseOfMarine << endl;
				//cout << "Sec = " << (Terran_SCV.buildTime() * 3 + timeToBuildBarrack + marines.size() * Terran_Marine.buildTime() + centerToBaseOfMarine) / 24 << endl;

				if (TIME < Terran_SCV.buildTime() * 4 + timeToBuildBarrack + allMarineCount * Terran_Marine.buildTime() + centerToBaseOfMarine) {
					setEnemyInitialBuild(Terran_2b_forward);
				}

			}

		}

	}

}

bool EnemyStrategyManager::timeBetween(int from, int to) {
	return TIME >= from && TIME <= to;
}

void EnemyStrategyManager::useBuildingCount()
{
	if (INFO.enemyRace == Races::Zerg)
	{
		uList spawningPool = INFO.getBuildings(Zerg_Spawning_Pool, E);
		uList hatchery = INFO.getBuildings(Zerg_Hatchery, E);
		uList lair = INFO.getBuildings(Zerg_Lair, E);
		bool isExistMorphLair = false; // 레어로 변태중인 해처리가 있는가

		for (auto h : hatchery)
		{
			if (h->isComplete() && h->unit()->isMorphing())
			{
				isExistMorphLair = true;
			}
		}

		// 레어 올라가고 있거나 있으면 발업 저글링 체크 안함
		if (!lair.empty() || isExistMorphLair)
		{
			return;
		}

		for (auto s : spawningPool)
		{
			if (s->unit()->isUpgrading())
			{
				// 스포닝풀이 업그레이드 중이면 저글링 발업
				setEnemyInitialBuild(Zerg_9_Balup);
			}
		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		// # 전진게이트 체크
		uList gatewayList = INFO.getBuildings(Protoss_Gateway, E);

		if (gatewayList.empty())
			return;

		int forwardGate = 0;

		// 기준은 내 본진에서 SecondChoke + 5 TILE
		int forwardThresholdGroundDistance = getGroundDistance(MYBASE, INFO.getSecondChokePosition(S)) + 5 * TILE_SIZE;

		for (auto gateway : gatewayList) {
			bool outSide = true;

			for (auto startBase : INFO.getStartLocations()) {

				if (startBase->GetOccupiedInfo() == myBase)
					continue;

				// Gate가 너무 멀리 있으면 forward gate로 판단.
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

		// # 2 게이트 빌드 체크
		// 초반 정찰 타이밍에 게이트웨이가 두개, 가스가 늦으면 2게이트 질럿 푸쉬 올 확률이 높다.
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
		// # 전진배럭 체크
		uList barracks = INFO.getBuildings(Terran_Barracks, E);

		if (barracks.empty())
			return;

		int forwardBarrackCnt = 0;
		bool isForwardBuilding = true;

		for (auto barrack : barracks) {

			for (auto startPosition : theMap.StartingLocations()) {
				// 어떤 스타트 포지션에 있다면 전진 전략이 아님(단 내 본진에 지었다면 전진 전략)
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

		// # 본진 2배럭 빌드 체크
		// 앞에서 전진으로 체크되면, 빌드 Inum 변수 순서 때문에 배럭 사이즈가 2개 이상이어도 세팅 되지 않는다.
		if (barracks.size() >= 2)
			setEnemyInitialBuild(Terran_2b);

	}
	else
	{

	}
}

void EnemyStrategyManager::checkEnemyBuilding()
{
	if (INFO.enemyRace == Races::Zerg)
	{
		uList hatchery = INFO.getBuildings(Zerg_Hatchery, E);
		uList lair = INFO.getBuildings(Zerg_Lair, E);
		uList hydraliskDen = INFO.getBuildings(Zerg_Hydralisk_Den, E);
		uList spire = INFO.getBuildings(Zerg_Spire, E);
		bool isExistMorphLair = false; // 레어로 변태중인 해처리가 있는가

		for (auto h : hatchery)
		{
			if (h->isComplete() && h->unit()->isMorphing())
			{
				isExistMorphLair = true;
			}
		}

		// 레어 있거나 레어 올라가고 있으면 뮤탈로 예측
		if (!lair.empty() || isExistMorphLair)
		{
			setEnemyMainBuild(Zerg_main_maybe_mutal);
		}

		// 히드라덴 있고
		if (!hydraliskDen.empty())
		{
			// 레어 있거나 레어 올라가고 있으면
			if (!lair.empty() || isExistMorphLair)
			{
				setEnemyMainBuild(Zerg_main_lurker);
			}
		}

		// 스파이어 있으면
		if (!spire.empty())
		{
			setEnemyMainBuild(Zerg_main_mutal);

			if (TIME < 24 * 60 * 5)
				setEnemyMainBuild(Zerg_main_fast_mutal);
		}

		// 히드라, 뮤탈 둘 다 사용할 경우
		if (!hydraliskDen.empty() && !spire.empty())
		{
			setEnemyMainBuild(Zerg_main_hydra_mutal);
		}
	}
	else if (INFO.enemyRace == Races::Protoss) {

		// ### 확인이 쉬운 순서대로 코딩이 되있으므로, 순서 변경 금지 ###
		uList stargate = INFO.getBuildings(Protoss_Stargate, E);
		uList fleetBeacon = INFO.getBuildings(Protoss_Fleet_Beacon, E);
		uList templarArchives = INFO.getBuildings(Protoss_Templar_Archives, E);
		uList adun = INFO.getBuildings(Protoss_Citadel_of_Adun, E);
		// 죽은 유닛도 생각 해야되는데..
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
			// 멀티가 1개 밖에 없으면 타이밍
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

		// 로보틱스가 있으면 리버 드랍일 확률이 있다.
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

void EnemyStrategyManager::checkEnemyUnits()
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
		bool isExistMorphLair = false; // 레어로 변태중인 해처리가 있는가

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

			// 레어 있거나 레어 올라가고 있는데 히드라가 있으면
			if (!lair.empty() || isExistMorphLair)
			{
				setEnemyMainBuild(Zerg_main_lurker);
			}

			// 저글링 발업 되어 있고, 2햇 이하, 5분 30초전에 히드라가 보이면 럴커일껄?
			if ((enemyMainBuild == Zerg_main_zergling || E->getUpgradeLevel(UpgradeTypes::Metabolic_Boost))
					&& hatchery.size() <= 2 && TIME < 24 * 330)
			{
				setEnemyMainBuild(Zerg_main_lurker);
			}
		}

		// 히드라나 럴커를 하나도 못봤을 때
		if (!(hydraAllCount + lurkerAllCount))
		{
			// 이니셜 빌드가 4드론이 아니고 4분 30초 ~ 5분 30초 전까지 히드라 없고 2햇 이하면 패스트 뮤탈일 가능성
			// -> 5분30초 ~ 7분30초 까지로 변경
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

		// 건물이나 다른걸로 럴커 분류가 됐지만 실제 럴커가 없이 히드라인 경우
		if (enemyMainBuild == Zerg_main_lurker && lurkerList.empty())
		{
			// 히드라가 많이 보이면 히드라 뮤탈로 분류 (어차피 레어 있어서 럴커 분류 됐을거니까)
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

		// 5 ~ 8 분사이에 앞마당이 없고 적 유닛이 비정상적으로 많으면
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

void EnemyStrategyManager::checkEnemyTechNUpgrade()
{
	if (INFO.enemyRace == Races::Zerg)
	{
		uList spawningPool = INFO.getBuildings(Zerg_Spawning_Pool, E);
		uList hydraliskDen = INFO.getBuildings(Zerg_Hydralisk_Den, E);

		for (auto s : spawningPool)
		{
			if (s->unit()->isUpgrading() || E->getUpgradeLevel(UpgradeTypes::Metabolic_Boost))
			{
				// 스포닝풀이 업그레이드 중이면 저글링 발업
				setEnemyMainBuild(Zerg_main_zergling);
			}
		}

		for (auto h : hydraliskDen)
		{
			// 히드라 속업 or 사업 된 경우
			if (h->unit()->isUpgrading()
					|| E->getUpgradeLevel(UpgradeTypes::Muscular_Augments)
					|| E->getUpgradeLevel(UpgradeTypes::Grooved_Spines))
			{
				setEnemyMainBuild(Zerg_main_hydra);
			}

			// 러커업 중이거나 러커업 된 경우
			if (h->unit()->isResearching() || INFO.hasRearched(TechTypes::Lurker_Aspect))
			{
				setEnemyMainBuild(Zerg_main_lurker);
			}
		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		// 드라군 푸쉬 판단
		uList cyCoreList = INFO.getBuildings(Protoss_Cybernetics_Core, E);
		uList pylonList = INFO.getBuildings(Protoss_Pylon, E);
		uList gateList = INFO.getBuildings(Protoss_Gateway, E);
		uList dra = INFO.getUnits(Protoss_Dragoon, E);

		if (!cyCoreList.empty() || !dra.empty()) {
			// 드라군 사정거리 업그레이드 중이거나, 파일런 2개의 위치가 다 파악이 되면
			// 변칙 빌드라고 보기 어렵다.

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

bool EnemyStrategyManager::isTheUnitNearMyBase(uList unitList)
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

		// 내 본진과 앞마당 기준 25 TILE 내에 저글링이 보이면 4~5드론
		if (distanceFromMainBase <= 25 * TILE_SIZE || distanceFromFirstExpansion <= 25 * TILE_SIZE)
		{
			isExist = true;
			break;
		}
	}

	return isExist;
}

void EnemyStrategyManager::checkDarkDropHadCome()
{
	if (getWaitTimeForDrop() < 0)
		return;

	uList shuttles = INFO.getTypeUnitsInArea(Protoss_Shuttle, E, MYBASE);
	uList darks = INFO.getTypeUnitsInArea(Protoss_Dark_Templar, E, MYBASE);

	// 다크를 내리기 전에 죽는다면?
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