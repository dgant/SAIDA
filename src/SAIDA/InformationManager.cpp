#include "InformationManager.h"

using namespace MyBot;

InformationManager::InformationManager()
{
	mapPlayerLimit = bw->getStartLocations().size();

	selfPlayer = S;
	enemyPlayer = E;

	selfRace = S->getRace();
	enemyRace = E->getRace();
	enemySelectRace = E->getRace();

	_unitData[S] = UnitData();
	_unitData[E] = UnitData();

	// 시작 및 모든 베이스 정보 구성
	updateStartAndBaseLocation();

	_mainBaseLocations[S] = INFO.getStartLocation(S);
	_mainBaseLocationChanged[S] = true;
	_occupiedBaseLocations[S] = list<const Base *>();
	_occupiedBaseLocations[S].push_back(_mainBaseLocations[S]);

	_mainBaseLocations[E] = getNextSearchBase();
	_mainBaseLocationChanged[E] = true;

	_occupiedBaseLocations[E] = list<const Base *>();

	_firstChokePoint[S] = nullptr;
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

	// BaseLocation 정보를 업데이트
	updateBaseLocationInfo();

	updateChangedMainBaseInfo(S);
	updateChangedMainBaseInfo(E);

	// 스타팅 포인트와 앞마당 매핑.
	_firstExpansionOfAllStartposition = map<Base *, Base *>();
	_mainBaseAreaPair = map<const Area *, const Area *>();
	updateAllFirstExpansions();

	activationMineralBaseCount = 0;
	activationGasBaseCount = 0;

	// Center Area  정보 업데이트
	//	updateCenterGridArea();

	// 모든 Area의 가장자리 정보 업데이트
	for (auto &area : theMap.Areas())
		area.calcBoundaryVertices();
}

InformationManager::~InformationManager()
{
}


void InformationManager::update()
{
	try {
		updateUnitsInfo();
	}
	catch (SAIDA_Exception e) {
		Logger::error("updateUnitsInfo Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("updateUnitsInfo Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("updateUnitsInfo Unknown Error.\n");
		throw;
	}

	//try {
	//	updateCenterGridArea();
	//}
	//catch (SAIDA_Exception e) {
	//	Logger::error("updateCenterGridArea Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
	//	throw e;
	//}
	//catch (const exception &e) {
	//	Logger::error("updateCenterGridArea Error. (Error : %s)\n", e.what());
	//	throw e;
	//}
	//catch (...) {
	//	Logger::error("updateCenterGridArea Unknown Error.\n");
	//	throw;
	//}

	try {
		updateBaseInfo();
	}
	catch (SAIDA_Exception e) {
		Logger::error("updateBaseInfo Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("updateBaseInfo Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("updateBaseInfo Unknown Error.\n");
		throw;
	}

	try {
		updateChokePointInfo();
	}
	catch (SAIDA_Exception e) {
		Logger::error("updateChokePointInfo Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("updateChokePointInfo Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("updateChokePointInfo Unknown Error.\n");
		throw;
	}

	//예약된 마인 수를 리셋해줍니다 (2분에 한 번)
	//if (TIME % (1 * 60 * 24) == 0)
	//	initReservedMineCount();

	try {
		// FirstWaitLinePosition 설정
		setFirstWaitLinePosition();
	}
	catch (SAIDA_Exception e) {
		Logger::error("setFirstWaitLinePosition Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("setFirstWaitLinePosition Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("setFirstWaitLinePosition Unknown Error.\n");
		throw;
	}

	try {
		// SecondExpansionLocation 설정
		updateSecondExpansion();
	}
	catch (SAIDA_Exception e) {
		Logger::error("updateSecondExpansion Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("updateSecondExpansion Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("updateSecondExpansion Unknown Error.\n");
		throw;
	}

	try {
		updateSecondExpansionForEnemy();
	}
	catch (SAIDA_Exception e) {
		Logger::error("updateSecondExpansionForEnemy Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("updateSecondExpansionForEnemy Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("updateSecondExpansionForEnemy Unknown Error.\n");
		throw;
	}


	try {
		// ThirdExpansionLocation 설정
		updateThirdExpansion();
	}
	catch (SAIDA_Exception e) {
		Logger::error("updateThirdExpansion Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("updateThirdExpansion Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("updateThirdExpansion Unknown Error.\n");
		throw;
	}

	try {
		// IslandExpansionLocation 설정
		updateIslandExpansion();
	}
	catch (SAIDA_Exception e) {
		Logger::error("updateIslandExpansion Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("updateIslandExpansion Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("updateIslandExpansion Unknown Error.\n");
		throw;
	}

	try {
		// 활성화된 멀티 개수 체크
		checkActivationBaseCount();
	}
	catch (SAIDA_Exception e) {
		Logger::error("checkActivationBaseCount Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("checkActivationBaseCount Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("checkActivationBaseCount Unknown Error.\n");
		throw;
	}

	// EnemyInMyArea 미리 계산
	checkEnemyInMyArea();
	checkMoveInside();

	/*if (!isEnemyScanResearched)
	{
	checkEnemyScan();
	}*/
	/*
	for (auto &area : theMap.Areas())
	{
		int idx = 1;

		for (auto p : area.getBoundaryVertices())
		{
			bw->drawTextMap(p, "%d", idx++);
		}
	}
	*/
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
			//printf("Base (%d, %d) center at (%d, %d) %s\n", base.Location().x, base.Location().y, base.Center().x, base.Center().y, base.Starting() ? "Starting" : "");

			if (base.Starting())
			{
				_startBaseLocations.push_back((Base *)&base);
			}

			_allBaseLocations.push_back((Base *)&base);
		}
	}

	// 2인용 맵은 정렬 필요 없음
	if (_startBaseLocations.size() <= 2)		return;

	if (enemyRace == Races::Protoss) {
		// 내 베이스 기준 거리 순으로 재 정렬
		sort(_startBaseLocations.begin(), _startBaseLocations.end(), [](Base * a, Base * b)
		{
			TilePosition mybase = INFO.getStartLocation(S)->Location();

			return mybase.getApproxDistance(a->Location()) < mybase.getApproxDistance(b->Location());
		});

		//printf("vs Protoss : sort by distance from mybase\n");

		//for (auto base : bases)
		//{
		//	printf("Base (%d, %d)\n", base->Location().x, base->Location().y);
		//}
	}
	else {
		// 시계방향(맵중앙 기준)으로 정렬
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

	for (auto base : _startBaseLocations)
	{
		printf("Base (%d, %d) center at (%d, %d) %s\n", base->Location().x, base->Location().y, base->Center().x, base->Center().y, base->Location() == S->getStartLocation() ? "<-- My Base" : "");
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

//나중에..
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

	//theMap.GetAllChokePoints()
	/*if (!isEnemyBaseFound) return nullptr;

	for (auto cps : theMap.GetPath(getFirstExpansionLocation(S)->getPosition(), getFirstExpansionLocation(E)->getPosition()))
	{
		if (cps->Center() == pos)
			return (ChokePoint *)cps;
	}*/

	return cps;
}

//
////예약된 마인 수를 리셋해줍니다 (2분에 한 번)
void InformationManager::initReservedMineCount()
{
	for (auto base : getBaseLocations())
		base->SetReservedMineCount(0);

	for (auto cp : theMap.GetAllChokePoints())
		cp->SetReservedMineCount(0);

	//	centerGridArea->initReservedMineCount();
}

void InformationManager::checkEnemyScan()
{
	if (!getUnits(Spell_Scanner_Sweep, E).empty())
	{
		isEnemyScanResearched = true;
	}
}
//
//void InformationManager::updateCenterGridArea()
//{
//	if (centerGridArea == nullptr)
//	{
//		const Area *wholeArea = theMap.GetNearestArea(TilePosition(theMap.Center()));
//		centerGridArea = new GridArea(wholeArea, 16);
//	}
//	else
//	{
//		centerGridArea->update();
//	}
//}

// base 의 정보를 업데이트 하고 _mainBaseLocations 를 수정한다.
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
			// 1. 해당 지역의 점령 상태 판별 (undefined, myBase, enemyBase)
			// [조건] INFO에서 빌딩을 가져와서 우리빌딩이 있으면 myBase, 적 빌딩 있으면 enemyBase, 둘다 있거나 없으면 undefined
			// Basic Resource Depot 있으면 무조건 점령지
			// 2. 예상 멀티 순위 판별(my, enemy) -> updateExpectedMultiBases()
			// [조건] MainBase를 찾은 후 / 점령지 상태가 바뀔때만 해줌
			// Turret만 있는 경우는 내 본진으로 보지않는다. 그냥 지은것들이기 때문에

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

			// 내건물과 상대 건물이 동시에 있는 경우 -> undefined
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
			else if ( sBuildings.size() ) //내 건물이 있는 경우
			{
				if (base.GetOccupiedInfo() != myBase)
				{
					base.SetOccupiedInfo(myBase);
					isChangedOccupyInfo = true;
				}

				_occupiedBaseLocations[S].push_back(&base);
			}
			else if ( eBuildings.size() ) //상대 건물이 있는 경우
			{
				if (base.GetOccupiedInfo() != enemyBase)
				{
					base.SetOccupiedInfo(enemyBase);
					isChangedOccupyInfo = true;
				}

				_occupiedBaseLocations[E].push_back(&base);
			}
			else //그 외의 경우
			{
				if (base.GetOccupiedInfo() != emptyBase)
				{
					base.SetOccupiedInfo(emptyBase);
					isChangedOccupyInfo = true;
				}
			}

			// 3. base의 내 mine 수
			base.SetMineCount(INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, base.Center(), TILE_SIZE * 12, false).size());

			int workerCount = 0;
			int enemyAirDefenseUnitCount = 0;
			int enemyGroundDefenseUnitCount = 0;

			int airUnitInMyBase = 0; //적 유닛이 내 베이스에 잇는지 체크
			int groundUnitInMyBase = 0; //적 유닛이 내 베이스에 있는지 체크

			uList sUnits, eUnits;

			if (needCheckRange)	{
				sUnits = INFO.getUnitsInRadius(S, base.Center(), 12 * TILE_SIZE, true, true, true, true);
				eUnits = INFO.getUnitsInRadius(E, base.Center(), 12 * TILE_SIZE, true, true, true, true);
			}
			else	{
				sUnits = INFO.getUnitsInArea(S, base.Center(), true, true, true, true);
				eUnits = INFO.getUnitsInArea(E, base.Center(), true, true, true, true);
			}

			//상대 유닛체크
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

			//내 유닛 체크
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

			/*if (base.GetOccupiedInfo() != myBase)
			{*/
			//상대 빌딩 체크
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

			/*if (base.GetOccupiedInfo() != enemyBase)
			{*/
			//내 빌딩 체크
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

			/*}*/

			// base에 상대 airAttack Unit 수
			base.SetEnemyAirDefenseUnitCount(enemyAirDefenseUnitCount);
			// base에 상대 groundAttack Unit 수
			base.SetEnemyGroundDefenseUnitCount(enemyGroundDefenseUnitCount);
			// base에 내 airAttack Unit 수
			base.SetSelfAirDefenseUnitCount(selfAirDefenseUnitCount);
			// base에 내 groundAttack Unit 수
			base.SetSelfGroundDefenseUnitCount(selfGroundDefenseUnitCount);
			// base에 상대 airAttack building 몇개?
			base.SetEnemyAirDefenseBuildingCount(enemyAirDefenseBuildingCount);
			// base에 상대 groundAttack building 몇개?
			base.SetEnemyGroundDefenseBuildingCount(enemyGroundDefenseBuildingCount);
			// base에 내 airAttack building 몇개?
			base.SetSelfAirDefenseBuildingCount(selfAirDefenseBuildingCount);
			// base에 내 groundAttack building 몇개?
			base.SetSelfGroundDefenseBuildingCount(selfGroundDefenseBuildingCount);
			// base에 Worker 몇 마리인가
			base.SetWorkerCount(workerCount);
			// base에 Bunker 몇 개?
			base.SetEnemyBunkerCount(enemyBunkerCount);

			// 9. 적군이 근처에 있거나, Worker에게 위험한 지역인지 판단
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

				if (isDangerousBaseForWorkers) //일꾼이나 커맨드가 공격받고 있다면
				{
					if (airUnitInMyBase && groundUnitInMyBase)//공중 지상 유닛 둘 다 있는 경우
					{
						if (selfAirDefenseBuildingCount + selfAirDefenseUnitCount > 0 && selfGroundDefenseBuildingCount + selfGroundDefenseUnitCount > 0)
						{
							isDangerousBaseForWorkers = false;
						}
					}
					else if (airUnitInMyBase && !groundUnitInMyBase) //공중 유닛만 있는 경우
					{
						if (selfAirDefenseBuildingCount + selfAirDefenseUnitCount > 0 || airUnitInMyBase < 3) // 공중유닛 2개는 그냥 무시하고 Defence 기다린다.(무탈, 레이쓰, 스카우트..)
						{
							isDangerousBaseForWorkers = false;
						}
					}
					else if (!airUnitInMyBase && groundUnitInMyBase) //지상 유닛만 있는 경우
					{
						if (selfGroundDefenseBuildingCount + selfGroundDefenseUnitCount > 0)
						{
							isDangerousBaseForWorkers = false;
						}
					}
					else //아무것도 없는 경우
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

			// 커맨드 근처에 적이 있는지 체크
			uList eList = INFO.getUnitsInRadius(E, base.getMineralBaseCenter(), 12 * TILE_SIZE, true, true, false);

			int damage = 0;
			Position avgPos = Position(0, 0);

			for (auto e : eList) {
				// 범위 공격이면서 사거리가 긴 유닛이 있는 경우 일꾼 전체가 대피해야한다.
				if (e->type() == Protoss_Reaver || e->type() == Protoss_Scarab || e->type() == Terran_Siege_Tank_Tank_Mode || e->type() == Protoss_High_Templar) {
					base.SetDangerousAreaForWorkers(true);
					damage = 0;
					break;
				}
				// 럴커가 7타일 이내에 있어도 대피.
				else if (e->type() == Zerg_Lurker && base.getMineralBaseCenter().getApproxDistance(e->pos()) < 7 * TILE_SIZE) {
					base.SetDangerousAreaForWorkers(true);
					damage = 0;
					break;
				}

				avgPos += e->pos();
				int d = getDamage(e->type(), Terran_SCV, E, S);
				damage += e->type() == Protoss_Zealot ? d * 2 : d;
			}

			//핵의 위협이 있는지 체크

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

			// base 방문 시점 선택
			if (INFO.enemyRace == Races::Zerg && base.GetLastVisitedTime() == 0 && !isEnemyBaseFound) {
				TilePosition upsize = base.getTilePosition() - TilePosition(1, 5);
				TilePosition leftsize = base.getTilePosition() - TilePosition(5, 1);
				TilePosition rightsize = base.getTilePosition() + TilePosition(8, -1);
				TilePosition downsize = base.getTilePosition() + TilePosition(-1, 7);

				// 최초 스타팅 위치에는 크립이 있기 때문에 5타일 전후만 봐도 TIME 을 업데이트 시켜준다.
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
						// 테두리만 확인한다.
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

	//base정보 테스트용
	//if (Broodwar->getFrameCount() % 360 == 0) {
	// base.toScreen();
	//}

	try {
		updateBaseLocationInfo();

		if (isChangedOccupyInfo || TIME % 120 == 0) {
			updateExpectedMultiBases();
		}
	}
	catch (SAIDA_Exception e) {
		Logger::error("updateBaseLocationInfo Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("updateBaseLocationInfo Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("updateBaseLocationInfo Unknown Error.\n");
		throw;
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

	// nullptr 인 경우 없어야 함.
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

		// 1. 해당 지역의 점령 상태 판별 (undefined, myBase, enemyBase)
		// [조건] INFO에서 빌딩을 가져와서 우리빌딩이 있으면 myBase, 적 빌딩 있으면 enemyBase, 둘다 있거나 없으면 undefined
		// Basic Resource Depot 있으면 무조건 점령지
		// 2. 예상 멀티 순위 판별(my, enemy) -> updateExpectedMultiBases()
		// [조건] MainBase를 찾은 후 / 점령지 상태가 바뀔때만 해줌


		// 3. base의 내 mine 수
		cps->SetMineCount(getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, Position(cps->Center()), TILE_SIZE * 6).size());

		int enemyAirDefenseUnitCount = 0;
		int enemyGroundDefenseUnitCount = 0;

		//상대 유닛체크
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

		//내 유닛 체크
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

		// 건물 일단 Skip...나중에 업데이트
		int enemyAirDefenseBuildingCount = 0;
		int enemyGroundDefenseBuildingCount = 0;

		int selfAirDefenseBuildingCount = 0;
		int selfGroundDefenseBuildingCount = 0;

		// base에 상대 airAttack Unit 수
		cps->SetEnemyAirDefenseUnitCount(enemyAirDefenseUnitCount);
		// base에 상대 groundAttack Unit 수
		cps->SetEnemyGroundDefenseUnitCount(enemyGroundDefenseUnitCount);
		// base에 내 airAttack Unit 수
		cps->SetSelfAirDefenseUnitCount(selfAirDefenseUnitCount);
		// base에 내 groundAttack Unit 수
		cps->SetSelfGroundDefenseUnitCount(selfGroundDefenseUnitCount);
		// base에 상대 airAttack building 몇개?
		cps->SetEnemyAirDefenseBuildingCount(enemyAirDefenseBuildingCount);
		// base에 상대 groundAttack building 몇개?
		cps->SetEnemyGroundDefenseBuildingCount(enemyGroundDefenseBuildingCount);
		// base에 내 airAttack building 몇개?
		cps->SetSelfAirDefenseBuildingCount(selfAirDefenseBuildingCount);
		// base에 내 groundAttack building 몇개?
		cps->SetSelfGroundDefenseBuildingCount(selfGroundDefenseBuildingCount);

		// 9. 적군이 근처에 있거나, Worker에게 위험한 지역인지 판단
		cps->SetIsEnemyAround(enemyAirDefenseUnitCount || enemyGroundDefenseUnitCount);

		/*if (Broodwar->getFrameCount() % 120 == 0) {
			cps->toScreen();
		}*/
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

	// 건물은 Create 시점부터 DB에 저장
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
		if (_unitData[S].addUnitNBuilding(unit)) // 새로 추가되는 경우만
			_unitData[S].increaseCompleteUnits(unit->getType());
		else if (unit->getType().isBuilding()) { // 건물은 Create 후 추가되므로 1번만 호출됨
			_unitData[S].increaseCompleteUnits(unit->getType());
			// 건물이 완성되는 경우 reserve map 에서 해제해준다.
			ReserveBuilding::Instance().freeTiles(unit->getTilePosition(), unit->getType());
		}

		if (unit->getType() == Terran_Command_Center)
			addAdditionalExpansion(unit);
	}
	else if (unit->getPlayer() == E) // 적군의 경우 Show에서 추가 되었는지 안된지를 알수 없음.
	{
		_unitData[E].addUnitNBuilding(unit);

		if (enemyRace == Races::Unknown) {
			enemyRace = unit->getType().getRace();

			// 내 종족이 테란이고 상대 종족이 프로토스가 아닌 경우 서플라이 디팟 예약 해제
			if (selfRace == Races::Terran && enemyRace != Races::Protoss) {
				TerranConstructionPlaceFinder::Instance().freeSecondSupplyDepot();
			}
		}
	}
}

// 유닛이 파괴/사망한 경우, 해당 유닛 정보를 삭제한다
void InformationManager::onUnitDestroy(Unit unit)
{
	if (unit->getType().isNeutral())
		return;

	if (unit->getType().isAddon()) // Add On은 중립건물 처리됨.
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
	// check for various types of combat units
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
	// 나의 BaseLocation 정보를 업데이트 한다.
	updateSelfBaseLocationInfo();

	try {
		// 적의 BaseLocation 정보를 업데이트 한다.
		updateEnemyBaseLocationInfo();
	}
	catch (SAIDA_Exception e) {
		Logger::error("updateEnemyBaseLocationInfo Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("updateEnemyBaseLocationInfo Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("updateEnemyBaseLocationInfo Unknown Error.\n");
		throw;
	}


	// for each enemy building unit we know about
	updateOccupiedAreas(enemyPlayer);
	updateOccupiedAreas(selfPlayer);
	updateChangedMainBaseInfo(selfPlayer);
	updateChangedMainBaseInfo(enemyPlayer);
}

// 나의 mainBaseLocations에 대해, 그곳에 있는 건물이 모두 파괴된 경우 _occupiedBaseLocations 중에서 _mainBaseLocations 를 선정한다
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
	// 적의 기지를 아는 경우 적진 건물이 모두 부서지면 새로운 베이스를 선택한다.
	if (isEnemyBaseFound) {
		// 적군의 빠른 앞마당 건물 건설 + 아군의 가장 마지막 정찰 방문의 경우,
		// enemy의 mainBaseLocations를 방문안한 상태에서는 건물이 하나도 없다고 판단하여 mainBaseLocation 을 변경하는 현상이 발생해서
		// enemy의 mainBaseLocations을 실제 방문했었던 적이 한번은 있어야 한다라는 조건 추가.
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


			// 적이 점령한 base area 가 없는 경우 다른 base 를 탐색. (종족별 탐방 순서 이후, 탐방 오래된 순서)
			isEnemyBaseFound = false;
		}
	}

	// enemy 의 startLocation을 아직 모르는 경우
	if (!isEnemyBaseFound) {
		// 적 베이스로 지정된 곳에 => 저그의 경우 정찰일꾼이 두마리 출발하기 때문에 (혹은 이후에도 다른 정찰로 MainBaseLocation 이외의 곳에서 기지를 발견할 수 있음) 추가 체크를 해준다.
		// 적 건물이 보이면 찾은것으로 간주. 저그인 경우 크립이 있으면 찾은것으로 간주
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
				// creep 존재 여부를 clear. 이후 해당 base 에서는 creep 관련 체크는 하지 않는다.
				b->ClearHasCreepAroundBase();
				return;
			}
		}

		// how many start locations have we explored
		int exploredStartLocations = 0;

		// an unexplored base location holder
		Base *unexplored = nullptr;

		uMap &eBuildings = getBuildings(E);
		int pathLength;

		for (Base *startLocation : INFO.getStartLocations())
		{
			if (startLocation == _mainBaseLocations[S])
				continue;

			// 적의 건물이 특정 스타트 포지션의 앞마당에 있거나 GroundDistance 1600 이내라면
			// 해당 스타트 포지션을 적의 베이스로 간주한다.
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

			// if it's explored, increment
			if (startLocation->GetLastVisitedTime())
				exploredStartLocations++;
			// otherwise set it as unexplored base
			else
				unexplored = startLocation;
		}

		// if we've explored every start location except one, it's the enemy
		if (exploredStartLocations == (mapPlayerLimit - 2) && unexplored)
		{
			cout << "updateBaseLocationInfo2" << unexplored->getTilePosition() << endl;
			_mainBaseLocations[E] = unexplored;
			_mainBaseLocationChanged[E] = true;
			_occupiedBaseLocations[E].push_back(unexplored);
			isEnemyBaseFound = true;
			return;
		}

		// 현재 가고있는 base 의 lastVisitTime 이 20 프레임 이내인 경우 다음 base 선택.
		if (getMainBaseLocation(E)->GetLastVisitedTime() && getMainBaseLocation(E)->GetLastVisitedTime() + 20 > TIME) {
			_mainBaseLocations[E] = getNextSearchBase();
			_mainBaseLocationChanged[E] = true;
			cout << "update to NextSearchBase" << _mainBaseLocations[E]->getTilePosition() << endl;
		}
	}
}

const Base *InformationManager::getNextSearchBase(bool reverse) {
	// 정방향
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
	// 역방향
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

	// 모든 base 를 탐색 한적이 있다면 안간지 오래된 base 부터 탐색. (추후 보완 필요하려나?)
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
			// 현재 위치에서 모든 인접한 area 를 가져온다.
			const Area *baseArea = theMap.GetArea(_mainBaseLocations[player]->Location());

			try {
				updateChokePointAndExpansionLocation(*baseArea, player);
			}
			catch (SAIDA_Exception e) {
				Logger::error("updateChokePointAndExpansionLocation Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
				throw e;
			}
			catch (const exception &e) {
				Logger::error("updateChokePointAndExpansionLocation Error. (Error : %s)\n", e.what());
				throw e;
			}
			catch (...) {
				Logger::error("updateChokePointAndExpansionLocation Unknown Error.\n");
				throw;
			}
		}
	}
}

bool InformationManager::updateChokePointAndExpansionLocation(const Area &baseArea, Player player, vector<const Area *> passedAreas) {
	passedAreas.push_back(&baseArea);

	for (auto &cba : baseArea.ChokePointsByArea()) {
		if (checkPassedArea(*cba.first, passedAreas))
			continue;

		// 주변에 base 가 있는지?
		if (!cba.first->Bases().empty()) {
			// 이어진 chokepoint 의 길이가 특정 이하인지?
			for (auto &cp : *cba.second) {
				int dist = cp.Pos(ChokePoint::end1).getApproxDistance(cp.Pos(ChokePoint::middle));
				dist += cp.Pos(ChokePoint::end2).getApproxDistance(cp.Pos(ChokePoint::middle));

				// 초크포인트 크기가 작은 경우
				if (dist < 39) {
					for (Base *targetBaseLocation : INFO.getBaseLocations()) {
						for (auto &base : cba.first->Bases()) {
							if (base.Location() == targetBaseLocation->Location()) {
								// 첫번째 확장기지 세팅
								_firstExpansionLocation[player] = targetBaseLocation;
								// 첫번째 초크포인트 세팅
								_firstChokePoint[player] = &cp;

								word depth = 0;

								for (auto &expansionAreaCP : cba.first->ChokePoints()) {
									// 데스티네이션이 secondChokePoint 가 두개이기 때문에 본진에 가까운 것을 secondChokePoint 로 지정한다.
									// 단장의능선은 secondChokePoint 가 상대 본진 반대방향이 있기 때문에 CP depth 도 함께 체크 한다.
									for (auto &baseAreaCP : *cba.second) {
										if (expansionAreaCP->Center() != baseAreaCP.Center()) {
											// 두번째 초크포인트 세팅
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

	// 못찾은 경우 인접한 area 기준으로 다시 검색하기
	for (auto &cba : baseArea.ChokePointsByArea()) {
		if (checkPassedArea(*cba.first, passedAreas))
			continue;

		if (updateChokePointAndExpansionLocation(*cba.first, player, passedAreas)) {
			return true;
		}
	}

	return false;
}

// 기존에 지난 area 인지 체크
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

// BaseLocation 주위 원 안에 player의 건물이 있으면 true 를 반환한다
bool InformationManager::hasBuildingAroundBaseLocation(Base *baseLocation, Player player, int radius)
{
	// invalid areas aren't considered the same, but they will both be null
	if (!baseLocation)
	{
		return false;
	}

	for (auto &b : getBuildings(player))
	{
		if (b.second->pos() == Positions::Unknown)		continue;

		TilePosition buildingPosition(b.second->pos());

		// 직선거리는 가깝지만 다른 Area 에 있는 건물인 경우 무시한다
		if (theMap.GetArea(buildingPosition) != baseLocation->GetArea())	continue;

		// BasicBot 1.2 Patch End //////////////////////////////////////////////////

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
	// invalid areas aren't considered the same, but they will both be null
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

// BasicBot 1.1 Patch End //////////////////////////////////////////////////

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

///////////////////////////////////////////
// 공통 Function 구현
///////////////////////////////////////////

// UnitData에서 UnitInfo를 찾아 Return함.
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

// getUnitsInRadius
// pos에서 radius(pixel) 만큼 안에 있는 Unit을 가져온다.
// 필수 input : Player
// 옵션 input :
// pos, radius( 입력하지 않는 경우 모든 유닛을 가져온다. )
// ground (지상 유닛, 일꾼 포함) , air ( 공중 유닛 ), worker (일꾼 포함 여부, 참고로 일꾼만은 가져올 수 없음)
// hide( 현재 맵에서 없어진 유닛까지 가져올때 )
uList InformationManager::getUnitsInRadius(Player p, Position pos, int radius, bool ground, bool air, bool worker, bool hide, bool groundDistance)
{
	uList units;

	uMap allUnits = _unitData[p].getAllUnits();

	for (auto &u : allUnits)
	{
		if (u.second->type() != Zerg_Lurker && hide == false && u.second->isHide())
			continue;

		// 일단 Mine은 Skip
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

		//		if (u.second->unit()->getTop() > Threshold_D || u.second->unit()->getBottom() < Threshold_T || u.second->unit()->getLeft() > Threshold_R || u.second->unit()->getRight() < Threshold_L)
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

		//		if (u.second->unit()->getTop() > Threshold_D || u.second->unit()->getBottom() < Threshold_T || u.second->unit()->getLeft() > Threshold_R || u.second->unit()->getRight() < Threshold_L)
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

			// Closest 유닛에서 제외하는 종류
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

			// Closest 유닛에서 제외하는 종류
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

// 예상 멀티 순위 판별(my, enemy)
// [호출조건] 점령지(occupied) 상태가 바뀔때만 해줌
void InformationManager::updateExpectedMultiBases()
{
	// MainBase를 찾은 후 처리
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

		if (base->isIsland()) continue; //섬 지역 제외

		int path = 0;
		theMap.GetPath(base->Center(), theMap.Center(), &path, false);

		if (path < 0) continue; // Fortress Ground Distance 제외

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
	/// FirstExpansion, SecondChokePoint 를 이용해 처음으로 대기해야 할 장소를 구한다.
	Position pos = Positions::Unknown;

	// 한번 설정되면 더이상 구하지 않는다.
	if (_firstWaitLinePosition != Positions::Unknown)
		return;

	if (_firstExpansionLocation[S] == nullptr || _secondChokePoint[S] == nullptr)
		return;

	if (INFO.getOccupiedBaseLocations(S).size() < 2 || INFO.getCompletedCount(Terran_Command_Center, S) < 2)
		return;

	// FirstExpansion 에서 SecondChokePoint 방향으로, SecondChokePoint 기준 10타일 위치
	pos = getDirectionDistancePosition(_firstExpansionLocation[S]->getPosition(), (Position)_secondChokePoint[S]->Center(),
									   _firstExpansionLocation[S]->getPosition().getApproxDistance((Position)_secondChokePoint[S]->Center()) + 10 * TILE_SIZE);

	// 위에서 구한 위치에서 2번째 멀티 위치쪽으로 보정
	if (INFO.getSecondExpansionLocation(S) != nullptr)
		pos = getDirectionDistancePosition(pos, INFO.getSecondExpansionLocation(S)->getPosition(), 5 * TILE_SIZE);

	// 맵 센터쪽으로도 보정
	pos = getDirectionDistancePosition(pos, (Position)theMap.Center(), 3 * TILE_SIZE);

	// 적 방향 쪽으로도 보정
	pos = getDirectionDistancePosition(pos, INFO.getSecondChokePosition(E), 3 * TILE_SIZE);

	// 구한 pos 이 걸어서 갈 수 없는 곳이면 내 앞마당쪽으로 보정
	if (!bw->isWalkable((WalkPosition)pos))
	{
		for (int i = 0; i < 3; i++)
		{
			pos = getDirectionDistancePosition(pos, (Position)_secondChokePoint[S]->Center(), 3 * TILE_SIZE);

			if (bw->isWalkable((WalkPosition)pos))
				break;
		}
	}

	// 구한 pos 이 벽이랑 너무 가까우면 센터쪽으로 보정
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
	// 실제 미네랄 채취중인 멀티 개수 (구현해야함.. 어떻게 처리할 지 고민중)
	uList commandCenter = getBuildings(Terran_Command_Center, S);
	int enoughMineralScv = 0;

	for (auto c : commandCenter)
	{
		if (!c->isComplete())
			continue;

		// 미네랄 숫자보다 미네랄 채취중인 scv 가 더 많을 경우 활성화된 멀티라고 판단
		if (ScvManager::Instance().getDepotMineralSize(c->unit()) > 0 && getAverageMineral(c->pos()) > 100
				&& ScvManager::Instance().getAssignedScvCount(c->unit()) >= ScvManager::Instance().getDepotMineralSize(c->unit()))
		{
			enoughMineralScv++;
		}
	}

	activationMineralBaseCount = enoughMineralScv;

	// 실제 가스 채취중인 멀티 개수
	int gasSvcCount = 0;

	for (auto iter = ScvManager::Instance().getRefineryScvCountMap().begin(); iter != ScvManager::Instance().getRefineryScvCountMap().end(); iter++)
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
	// 해당 위치의 Base 에 내 커맨드센터가 있고 남아있는 평균 미네랄 값, 커맨드 없으면 INT_MAX
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
		return averageMineral; // return INT_MAX

	averageMineral = ScvManager::Instance().getRemainingAverageMineral(depot);

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
			//if (_firstWaitLinePosition != Positions::Unknown && _firstWaitLinePosition.isValid())
			{
				// firstWaitLine 보다 센터랑 거리가 가까우면 Skip
				//if (theMap.Center().getApproxDistance((*targetBase)->getPosition()) < theMap.Center().getApproxDistance(_firstWaitLinePosition))
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

		// 해당 베이스에 남은 미네랄이 너무 적으면 안먹을래
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

		// 본진에서 확장까지 초크포인트를 2개 지나면 본진과 확장 사이에 있는 area를 본진과 동일하게 취급해준다.
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

		// 내 _secondExpansionLocation 이 적이 점령한 상태라면 재계산 필요
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

	// 상대가 테란인 경우 무조건 가스 멀티로 먼저 확인, 센터 아닌곳으로
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

		// 내 _thirdExpansionLocation 이 적이 점령한 상태라면 재계산 필요
		if (enemyBase != _occupiedBaseLocations[E].end())
		{
			cout << "=== 상대 점령지라 리셋할래" << endl;
			reset = true;
		}
		else
		{
			auto myBase = find_if(_occupiedBaseLocations[S].begin(), _occupiedBaseLocations[S].end(), [pos](const Base * base) {
				return pos == base->Center();
			});

			// 나/상대 베이스 둘 다 아닐때 해당 위치에 상대가 많으면 위치 변경할래
			if (myBase == _occupiedBaseLocations[S].end())
			{
				// ThirdExpansionLocation 주변에 적 유닛이 너무 많으면 위치 변경 필요
				int myUnitCountInThirdExpansion = INFO.getUnitsInRadius(S, INFO.getThirdExpansionLocation(S)->getPosition(), 20 * TILE_SIZE, true, true, false, true).size();
				int enemyUnitCountInThirdExpansion = INFO.getUnitsInRadius(E, INFO.getThirdExpansionLocation(S)->getPosition(), 20 * TILE_SIZE, true, true, false, true).size();

				// ThirdExpansionLocation 주변에 적 유닛들이 나보다 많으면 안먹어
				if (enemyUnitCountInThirdExpansion > 3 && myUnitCountInThirdExpansion <= enemyUnitCountInThirdExpansion)
				{
					cout << "=== ThirdExpansion 주변에 적이 많아서 위치 리셋할래 " << _thirdExpansionLocation[S]->getTilePosition() << endl;
					notCenter = true;
					//_thirdExpansionLocation[S]->SetExpectedMyMultiRank(INT_MAX);
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

	// 상대가 테란인 경우 무조건 가스 멀티로 먼저 확인, 일단 센터 멀티는 제외
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
		// 갱신하는 부분 개발 필요
		bw->drawTextMap(_islandExpansionLocation[S]->getPosition(), "IslandExpansionLocation");
	}

	if (_islandExpansionLocation[E] != nullptr)
	{
		// 갱신하는 부분 개발 필요
	}

	for (Base *baseLocation : _allBaseLocations)
	{
		if (!baseLocation->isIsland())
			continue;

		if (baseLocation->Minerals().size() < 3)
			continue;

		// 1. 해당 지역의 점령 상태 판별 (undefined, myBase, enemyBase)
		// [조건] INFO에서 빌딩을 가져와서 적 빌딩 있으면 enemyBase, 없으면 myBase 로 지정

		uList sBuildings = INFO.getBuildingsInArea(S, baseLocation->getPosition(), true, false, true);
		uList eBuildings = INFO.getBuildingsInArea(E, baseLocation->getPosition(), true, false, true);

		if (!eBuildings.empty()) //상대 건물이 있는 경우
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

	// 해당 베이스 주위 1000 반경안의 적들을 가져온다
	vector<UnitInfo *> enem = INFO.getUnitsInRadius(E, base->getPosition(), 1000, true, true, true);

	if (enem.empty())
		return true;


	for (auto ui : enem)
	{
		// 지상을 공격할 수 있는 유닛 중
		if (ui->unit()->getType().groundWeapon().targetsGround())
		{
			// 클로킹 유닛이 있고 내가 볼 수가 없다면 안전하지 않다
			if (!ui->unit()->isDetected())
				return false;

			if (ui->unit()->isFlying())
				airEnem++;
			else
				groundEnem++;
		}
	}

	// 해당 베이스 주위 1000 반경 안의 아군들을 가져온다
	vector<UnitInfo *> my = INFO.getUnitsInRadius(S, base->getPosition(), 1000, true, true, true, true);

	if (my.empty())
		return false;

	int groundMy = 0;
	int airMy = 0;

	// 적군과 싸울 수 있는 유닛
	for (auto ui : my)
	{
		if (ui->unit()->getType().groundWeapon().targetsGround())
			groundMy++;
		else if (ui->unit()->getType().airWeapon().targetsAir())
			airMy++;
	}

	// 공중적군은 있는데 공중공격가능 아군이 없다면
	if (airEnem > 0 && airMy == 0)
		return false;

	// 지상적군은 있는데 지상공격가능 아군이 없다면
	if (groundEnem > 0 && groundMy == 0)
		return false;

	// 아군 공격가능 유닛이 3마리도 안되고 적군과 아군(scv 포함)의 차이가 10마리 이상 날 경우
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

// 첫번째 초크에 있는 center end1 end2 세개의 점을 이용하여 직선의 방정식을 구한뒤,
// 수선의 발을 내린 후,
// 최소 min 타일부터 최대 max 타일 내에 있는 하나의 점을 찾는다.
Position InformationManager::getWaitingPositionAtFirstChoke(int min, int max) {

	Position waitingPositionAtFirstChoke;

	Position chokeE1;
	Position chokeE2;
	Position chokeCenter;
	Position waitingPoint; // 앞마당에서 기다릴 것인가, 본진언덕에서 기다릴 것인가

	// 역언덕 or 평지 체크
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

	// 기울기가 1보다 크면, Y 좌표 +,- 를 찾음.
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
		// 기울기가 1보다 작으면, X 좌표 +,- 를 찾음.

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

	// 앞마당에서 언덕을 올라오려는 적이 있는가?
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

	// 앞마당에 이미 랜딩한 커맨드가 있는가?
	if (INFO.getFirstExpansionLocation(S)) {
		uList command = INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, INFO.getFirstExpansionLocation(S)->Center(), 3 * TILE_SIZE, true);

		for (auto b : command) {
			if (!b->unit()->isLifted()) {
				needCnt = 0;
			}
		}
	}

	// 전략 판단을 늦게해서 이미 커맨드를 본진 안쪽에 짓고 있는 경우
	uList commandList = INFO.getTypeBuildingsInArea(Terran_Command_Center, S, MYBASE, false);

	if (commandList.size() > 1)
		needCnt = needCnt == 0 ? 1 : needCnt;

	if (needCnt != 0 && INFO.getFirstChokePosition(S) != Positions::None) {

		uList eList = INFO.getUnitsInRadius(E, INFO.getFirstChokePosition(S), 10 * TILE_SIZE, true, false, false);

		for (auto e : eList) {
			// 적이 한명이라도 들어왔다면?
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

	// 상대 초크 10 TILE 안의 지상 유닛을 본다.
	uList aroundChoke = getUnitsInRadius(S, getFirstChokePosition(E), 10 * TILE_SIZE, true, false, false);
	uList aroundEnChoke = getUnitsInRadius(E, getFirstChokePosition(E), 15 * TILE_SIZE, true, false, false); // 탱크 때문에 15
	// 초크포인트를 지나서 자리 잡을 공간도 필요하기 때문에 더 많은 범위를 체크한다.
	int aroundDefChoke = getDefenceBuildingsInRadius(E, getFirstChokePosition(E), 15 * TILE_SIZE, false).size();

	// 저그는 성큰이 세기 때문에 가중치를 더 준다.
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

	// 첫번째는 본진에..
	if (INFO.getMainBaseLocation(E))
		scanPositionOfMainBase.push_back((TilePosition)INFO.getMainBaseLocation(E)->Center());

	// 본진과 상,하,좌,우 를 살펴보면서 가장 공간이 넓은 순서로 스캔 뿌린다.

	TilePosition nPosition = (TilePosition)INFO.getMainBaseLocation(E)->Center();
	int index = 1;
	vector<pair<int, int>> sortArray;
	vector<pair<int, int>>::iterator iter;

	// 상
	while (nPosition.y - index > 0
			&& isSameArea((Position)TilePosition(nPosition.x, nPosition.y - index), (Position)nPosition))
		index++;

	if (index > 11)
		sortArray.push_back(pair<int, int>(0, index));

	index = 1;

	// 하
	while (nPosition.y + index < 128
			&& isSameArea((Position)TilePosition(nPosition.x, nPosition.y + index), (Position)nPosition))
		index++;

	if (index > 11)
		sortArray.push_back(pair<int, int>(1, index));

	index = 1;

	// 좌
	while (nPosition.x - index > 0
			&& isSameArea((Position)TilePosition(nPosition.x - index, nPosition.y), (Position)nPosition))
		index++;

	if (index > 11)
		sortArray.push_back(pair<int, int>(2, index));

	index = 1;

	// 우
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
		sortArray.erase(sortArray.end() - 1); // 마지막 방향은 삭제

	for (iter = sortArray.begin(); iter != sortArray.end(); iter++) {

		// 상
		if (iter->first == 0)
			scanPositionOfMainBase.push_back(TilePosition(nPosition.x, nPosition.y - 8));
		// 하
		else if (iter->first == 1)
			scanPositionOfMainBase.push_back(TilePosition(nPosition.x, nPosition.y + 8));
		// 좌
		else if (iter->first == 2)
			scanPositionOfMainBase.push_back(TilePosition(nPosition.x - 8, nPosition.y));
		// 우
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

