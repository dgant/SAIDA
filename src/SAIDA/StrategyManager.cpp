#include "StrategyManager.h"
#include "UnitManager\TankManager.h"
#include "UnitManager\GoliathManager.h"
#include "TrainManager.h"
#include "DeepLearning\Supervised.h"
#include "DeepLearning\SharedMemoryManager.h"

#define PERIOD 24 * 5                // 周期
#define BASE_WINNING_RATE 65.00      // （基准胜率）

using namespace MyBot;

StrategyManager &StrategyManager::Instance()
{
	static StrategyManager instance;
	return instance;
}

StrategyManager::StrategyManager()
{

	string mapName = bw->mapFileName();

	if (mapName.find("CircuitBreaker") != string::npos) {
		isCircuitBreakers = true;
	}

	scanForcheckMulti = false;
	mainStrategy = WaitToBase;//基本策略首先设为WaitToBase
	myBuild = MyBuildTypes::Basic;

	searchPoint = 0;

	int find_size_x = (theMap.Size().x * 32) / 20;
	int find_size_y = (theMap.Size().y * 32) / 20;

	for (int i = 0; i < 20; i++)
	{
		for (int j = 0; j < 20; j++)
		{
			map400[searchPoint].x = find_size_x / 2 + (i * find_size_x);
			map400[searchPoint].y = find_size_y / 2 + (j * find_size_y);
			searchPoint++;
		}
	}

	random_shuffle(&map400[0], &map400[399]);

	searchPoint = 0;

	mainAttackPosition = INFO.getMainBaseLocation(E)->Center();

	lastUsingScanTime = 0;
	needTank = false;
	scanForAttackAll = false;
}

void StrategyManager::onStart()
{
}

void StrategyManager::onEnd(bool isWinner)
{
}
//============================================================================
void StrategyManager::update()
{
	try {
		executeSupplyManagement();
	}
	catch (SAIDA_Exception e) {
		Logger::error("executeSupplyManagement Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("executeSupplyManagement Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("executeSupplyManagement Unknown Error.\n");
		throw;
	}

	try {
		BasicBuildStrategy::Instance().executeBasicBuildStrategy();
	}
	catch (SAIDA_Exception e) {
		Logger::error("BasicBuildStrategy::executeBasicBuildStrategy Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("executeBasicBuildStrategy Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("executeBasicBuildStrategy Unknown Error.\n");
		throw;
	}

	try {
		makeMainStrategy();
	}
	catch (SAIDA_Exception e) {
		Logger::error("makeMainStrategy Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("makeMainStrategy Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("makeMainStrategy Unknown Error.\n");
		throw;
	}

	try {
		if (mainStrategy == Eliminate)
			searchForEliminate();
	}
	catch (SAIDA_Exception e) {
		Logger::error("searchForEliminate Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("searchForEliminate Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("searchForEliminate Unknown Error.\n");
		throw;
	}

	try {
		checkUpgrade();
	}
	catch (SAIDA_Exception e) {
		Logger::error("checkUpgrade Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("checkUpgrade Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("checkUpgrade Unknown Error.\n");
		throw;
	}

	try {
		checkUsingScan();
	}
	catch (SAIDA_Exception e) {
		Logger::error("checkUsingScan Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("checkUsingScan Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("checkUsingScan Unknown Error.\n");
		throw;
	}

	try {
		setAttackPosition();
	}
	catch (SAIDA_Exception e) {
		Logger::error("setAttackPosition Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("setAttackPosition Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("setAttackPosition Unknown Error.\n");
		throw;
	}

	setMyBuild();

	checkNeedAttackMulti();

	decideDropship();

	checkSecondExpansion();

	checkThirdExpansion();

	checkKeepExpansion();

	checkAdvanceWait();

	checkNeedDefenceWithScv();

	if (!isExistEnemyMine)
	{
		if (!INFO.getUnits(Terran_Vulture_Spider_Mine, E).empty())
			isExistEnemyMine = true;
	}

	/*
	Position e = INFO.getSecondChokePosition(E);

	for (auto cp : theMap.GetPath(INFO.getSecondChokePosition(S), INFO.getFirstExpansionLocation(E)->Center()))
	{
	e = (Position)cp->Center();

	vector<Position> roundPos = getRoundPositions(e, 12 * TILE_SIZE, 10, INFO.getSecondChokePosition(S));

	int i = 0;

	for (auto p : roundPos)
	{
	if (i >= 8)
	break;
	else
	i++;

	bw->drawCircleMap(p, 4, Colors::Blue, true);
	}

	if (roundPos.size())
	{
	if (roundPos.size() > 7)
	bw->drawLineMap(roundPos[0], roundPos[7], Colors::Cyan);
	else
	bw->drawLineMap(roundPos[0], roundPos[roundPos.size() - 1], Colors::Cyan);
	}
	}
	*/
	/*
	//////////////////////////////////////////
	Position c = INFO.getFirstExpansionLocation(E)->Center();
	Position e = INFO.getSecondChokePosition(E);

	Position pre = c;

	for (auto cp : theMap.GetPath(c, theMap.Center()))
	{
	c = pre;
	e = (Position)cp->Center();
	pre = e;
	}

	bw->drawCircleMap(c, 5, Colors::Red, true);

	bw->drawCircleMap(e, 5, Colors::Red, true);

	bw->drawLineMap(c, e, Colors::Red);

	double m = (double)(c.y - e.y) / (double)(c.x - e.x);
	double m2 = (1 / m) * -1;

	Position newPos1 = getDirectionDistancePosition(c, e, c.getApproxDistance(e) + (10 * TILE_SIZE));
	bw->drawCircleMap(newPos1, 5, Colors::Blue, true);
	Position newPos2 = getDirectionDistancePosition(c, e, c.getApproxDistance(e) + (15 * TILE_SIZE));
	bw->drawCircleMap(newPos2, 5, Colors::Blue, true);
	Position newPos3 = getDirectionDistancePosition(c, e, c.getApproxDistance(e) + (20 * TILE_SIZE));
	bw->drawCircleMap(newPos3, 5, Colors::Blue, true);

	Position tmpPos = newPos1;

	int nx1 = tmpPos.x - 400;
	int nx2 = tmpPos.x + 400;
	Position pos1 = Position(nx1, (int)(m2 * (nx1 - tmpPos.x)) + tmpPos.y);
	Position pos2 = Position(nx2, (int)(m2 * (nx2 - tmpPos.x)) + tmpPos.y);

	bw->drawLineMap(pos1, pos2, Colors::Blue);

	tmpPos = newPos2;

	nx1 = tmpPos.x - 400;
	nx2 = tmpPos.x + 400;
	pos1 = Position(nx1, (int)(m2 * (nx1 - tmpPos.x)) + tmpPos.y);
	pos2 = Position(nx2, (int)(m2 * (nx2 - tmpPos.x)) + tmpPos.y);

	bw->drawLineMap(pos1, pos2, Colors::Blue);

	tmpPos = newPos3;

	nx1 = tmpPos.x - 400;
	nx2 = tmpPos.x + 400;
	pos1 = Position(nx1, (int)(m2 * (nx1 - tmpPos.x)) + tmpPos.y);
	pos2 = Position(nx2, (int)(m2 * (nx2 - tmpPos.x)) + tmpPos.y);

	bw->drawLineMap(pos1, pos2, Colors::Blue);
	*/
}
//检查敌人的分基地==============================================================
void StrategyManager::checkEnemyMultiBase()
{
	int scanThreshold = 4;

	if (INFO.enemyRace == Races::Protoss) 
	{
		scanThreshold = 2;

		if (EMB == Toss_arbiter || EMB == Toss_arbiter_carrier || EMB == Toss_fast_carrier
				|| EMB == Toss_dark || EMB == Toss_dark_drop) {
			scanThreshold = 4;
		}
	}

	if (INFO.getAvailableScanCount() >= scanThreshold)
	{
		int rank = 6;
		Base *bestBase = nullptr;

		//for每一个可能的基地位置
		for (auto base : INFO.getBaseLocations())
		{//如果基地被占据，跳出
			if (base->GetOccupiedInfo() != emptyBase)
				continue;

			// 两分钟之后再次查看
			if (base->GetLastVisitedTime() + (24 * 60 * 2) > TIME) continue;

			if (base->GetExpectedEnemyMultiRank() <= 6 && base->GetExpectedEnemyMultiRank() < rank)
			{
				rank = base->GetExpectedEnemyMultiRank();
				bestBase = base;
			}
		}

		if (bestBase != nullptr)//如果bestbase能找到，就用雷达去扫
			ComsatStationManager::Instance().useScan(bestBase->Center());
	}
}
//检查敌人的分基地是否需要进攻==============================================================
void StrategyManager::checkNeedAttackMulti()
{
	if (multiBreakDeathPosition != Positions::Unknown)
	{
		//		cout << "multi break Death " << (TilePosition)multiBreakDeathPosition << endl;
		//		bw->drawCircleMap(multiBreakDeathPosition, 50, Colors::Blue, true);
		//		bw->drawCircleMap(multiBreakDeathPosition, 10 * TILE_SIZE, Colors::Blue, false);

		if (INFO.enemyRace == Races::Terran)
		{
			int eVulture = INFO.getTypeUnitsInRadius(Terran_Vulture, E, multiBreakDeathPosition, 10 * TILE_SIZE, true).size();
			int eTank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, multiBreakDeathPosition, 15 * TILE_SIZE, true).size();
			int eWraith = INFO.getTypeUnitsInRadius(Terran_Wraith, E, multiBreakDeathPosition, 10 * TILE_SIZE, true).size();
			int eGoliath = INFO.getTypeUnitsInRadius(Terran_Goliath, E, multiBreakDeathPosition, 10 * TILE_SIZE, true).size();

			int enemyPoint = eTank * 10 + eVulture * 3 + eGoliath * 5 + eWraith * 3;

			//			cout << "enemyPoint : " << enemyPoint << endl;

			if (enemyPoint >= 30) {
				needAttackMulti = false;
				return;
			}

			multiBreakDeathPosition = Positions::Unknown;
		}
		else if (INFO.enemyRace == Races::Protoss)
		{

		}
		else
		{

		}
	}

	if (INFO.enemyRace == Races::Terran)
		return;

	// 每秒一次运行一次 
	if (TIME % 24 != 0) {
		return;
	}

	if (INFO.enemyRace == Races::Zerg) 
	{
		// MainAttack Position에 도달했을 때
		if (secondAttackPosition == Positions::Unknown || !secondAttackPosition.isValid())
		{
			needAttackMulti = false;
			return;
		}

		UnitInfo *firstTank = TM.getFrontTankFromPos(secondAttackPosition);
		UnitInfo *firstGol = GM.getFrontGoliathFromPos(secondAttackPosition);

		if (firstTank != nullptr)
		{
			if (isSameArea(firstTank->pos(), secondAttackPosition)) {
				needAttackMulti = true;
				return;
			}
		}

		if (firstGol != nullptr)
		{
			if (isSameArea(firstGol->pos(), secondAttackPosition)) {
				needAttackMulti = true;
				return;
			}
		}

		needAttackMulti = false;
	}
	else if (INFO.enemyRace == Races::Protoss) 
	{
		// 상대 멀티가 없을 때
		if (secondAttackPosition == Positions::Unknown || !secondAttackPosition.isValid())
		{
			needAttackMulti = false;
			return;
		}

		// 적 앞마당의 넥서스를 파괴
		if (!INFO.getFirstExpansionLocation(E)) 
		{
			needAttackMulti = false;
			return;
		}

		Position firstExpansionE = INFO.getFirstExpansionLocation(E)->Center();
		UnitInfo *firstTank = TM.getFrontTankFromPos(firstExpansionE);

		// 탱크가 적 앞마당에 도착했고, 적 앞마당 넥서스 날렸을 때
		if (firstTank && isSameArea(firstTank->pos(), firstExpansionE)
				&& INFO.getTypeBuildingsInArea(Protoss_Nexus, E, firstExpansionE, true).empty()) {
			// 우리 병력이 최소기준을 유지했을 때
			word tankThreshold = (EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier) ? 2 : 4;

			if (INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, firstTank->pos(), 15 * TILE_SIZE).size() > tankThreshold) {
				needAttackMulti = true;
			}
		}
	}

	return;
}
//制定主要策略==============================================================
void StrategyManager::makeMainStrategy()
{
	// 1초에 한번만 실행
	if (TIME % 24 != 0)
	{
		return;
	}

	// Eliminate는 일단 내비둔다. 나중에 수정 필요
	if (mainStrategy == Eliminate && INFO.getOccupiedBaseLocations(E).empty())
	{
		return;
	}
	int MarineCount = INFO.getCompletedCount(Terran_Marine, S);
	int vultureCount = INFO.getCompletedCount(Terran_Vulture, S);
	int tankCount = INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S);
	int goliathCount = INFO.getCompletedCount(Terran_Goliath, S);
	int enemyVultureCount = INFO.getCompletedCount(Terran_Vulture, E);
	int enemyTankCount = INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, E);
	int enemyGoliathCount = INFO.getCompletedCount(Terran_Goliath, E);

	//==========================
	mainStrategy = WaitToBase;
	//==========================
	// vsT, 벌쳐 사용하다가 나도 탱크를 빠르게 준비해야 하는지 판단
	if (needTank == false)
	{
		if (ESM.getEnemyMainBuild() == Terran_3fac)
		{
			needTank = true;
		}

		if (TIME < 24 * 60 * 6
			&& (enemyTankCount > 0 || enemyGoliathCount > 1))
		{
			needTank = true;
		}
		else if (TIME >= 24 * 60 * 6 && TIME < 24 * 60 * 8
			&& ((enemyTankCount > 3 && INFO.hasRearched(TechTypes::Tank_Siege_Mode)) || enemyGoliathCount > 4))
		{
			needTank = true;
		}
		else if (TIME > 24 * 60 * 8)
		{
			needTank = true;
		}
	}

	// 내 공격 유닛 수 카운트 (인구 기준, 마린은 제외)//计算我方攻击单位的人口
	uMap myUnits = INFO.getUnits(S);
	int myAttackUnitSupply = 0;

	for (auto &myUnit : myUnits)
	{
		if (!myUnit.second->isComplete())
			continue;

		if (myUnit.second->type().isWorker())
			continue;

		if (myUnit.second->type() == Terran_Marine)
			continue;

		myAttackUnitSupply += myUnit.second->type().supplyRequired();
	}

	// 적 공격 유닛 수 카운트 (인구 기준)//计算敌方攻击单位的人口
	uMap enemyUnits = INFO.getUnits(E);
	int enemyAttackUnitSupply = 0;

	for (auto &enemyUnit : enemyUnits)
	{
		if (!enemyUnit.second->isComplete())
			continue;

		if (enemyUnit.second->type().isWorker())
			continue;

		// 캐리어의 경우 전체 공격 판단에서 제외, 일단 지상 병력 기준으로만 판단
		if (enemyUnit.second->type() == Protoss_Carrier)
			continue;

		// 퀸은 즉시 전력이 아니므로 인구에서 제외, 추후 상황에 따라 가중치 부여해야 할 듯
		if (enemyUnit.second->type() == Zerg_Queen)
			continue;

		enemyAttackUnitSupply += enemyUnit.second->type().supplyRequired();
	}

	if (mainStrategy == AttackAll)     //Attack all 到 Back all 的情况
	{
		if (INFO.enemyRace == Races::Zerg)
		{
			//if (myAttackUnitSupply < enemyAttackUnitSupply * 1.0 + 4)
			if (myAttackUnitSupply < enemyAttackUnitSupply * 0.8)
			{
				mainStrategy = BackAll;
				scanForAttackAll = false;
				printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
				return;
			}
		}
		else if (INFO.enemyRace == Races::Terran)
		{
			if (myBuild == MyBuildTypes::Terran_TankGoliath)
			{
				waitLinePosition = Positions::Unknown;
				drawLinePosition = Positions::Unknown;

				if (S->supplyUsed() < 320 || (tankCount < enemyTankCount * 1.2 && S->supplyUsed() < 380))
					//if (S->supplyUsed() < 150 || (tankCount < enemyTankCount * 1.0 && S->supplyUsed() < 160))
				{
					mainStrategy = WaitLine;
					scanForAttackAll = false;
					printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
					return;
				}
			}
			else if (myBuild == MyBuildTypes::Terran_VultureTankWraith)
			{
				if (myAttackUnitSupply < enemyAttackUnitSupply)
				{
					mainStrategy = BackAll;
					scanForAttackAll = false;
					printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
					return;
				}

				if (needTank)
				{
					if (enemyTankCount > 2 && !S->hasResearched(TechTypes::Tank_Siege_Mode) && tankCount == 0)
					{
						mainStrategy = BackAll;
						scanForAttackAll = false;
						printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
						return;
					}
				}
			}
		}
		else
		{
			if (ESM.getEnemyMainBuild() == Toss_fast_carrier || ESM.getEnemyMainBuild() == Toss_arbiter_carrier)
			{
				if (finalAttack)
				{
					if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) < 2)
						finalAttack = false;
					else
						return;
				}

				if (SM.getMainAttackPosition() != Positions::Unknown)
				{
					UnitInfo *frontTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

					if (frontTank && frontTank->pos().getApproxDistance(SM.getMainAttackPosition()) < 30 * TILE_SIZE)
					{
						uList interceptor = INFO.getUnits(Protoss_Interceptor, E);
						word interceptorCntInAttackArea = 0;

						for (auto i : interceptor)
						{
							if (i->getLastSeenPosition().getApproxDistance(SM.getMainAttackPosition()) <= 50 * TILE_SIZE)
							{
								interceptorCntInAttackArea++;
							}
						}

						uList goliath = INFO.getTypeUnitsInRadius(Terran_Goliath, S, frontTank->pos(), 20 * TILE_SIZE);

						if (interceptorCntInAttackArea > goliath.size() * 4)
						{
							mainStrategy = BackAll;
							scanForAttackAll = false;
							printf("[BackAll] near Protoss_Interceptor.size() = %d, near goliath.size()  = %d\n", interceptorCntInAttackArea, goliath.size());
							return;
						}
					}
				}

				if (myAttackUnitSupply < enemyAttackUnitSupply*0.9 && myAttackUnitSupply < 30 && tankCount < 4)
				{
					mainStrategy = BackAll;
					scanForAttackAll = false;
					printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
					return;
				}

			}
		else {
				// 상대 아비터가 보유되어있으면 베슬이 있거나, 없어도 스캔 여유가 되면 나간다.
				if (!INFO.getUnits(Protoss_Arbiter, E).empty()) {
					if (INFO.getCompletedCount(Terran_Science_Vessel, S) == 0 && INFO.getAvailableScanCount() < 1) {
						mainStrategy = BackAll;
						scanForAttackAll = false;
						printf("[BackAll] 디텍트 못해서 ㅌㅌ \n", myAttackUnitSupply, enemyAttackUnitSupply);
						return;
					}
				}

				if (myAttackUnitSupply < enemyAttackUnitSupply && myAttackUnitSupply < 45 && tankCount < 1)
				{
					mainStrategy = BackAll;
					scanForAttackAll = false;
					printf("[BackAll] my = %d, enemy = %d\n", myAttackUnitSupply, enemyAttackUnitSupply);
					return;
				}

			}

		}
	}
	else if (mainStrategy == DrawLine)//Draw line 到Attack all、WaitLine的情况
	{
	if (INFO.enemyRace != Races::Terran)
	{
		if (S->supplyUsed() >= 380 && (INFO.getOccupiedBaseLocations(E).size() < 2 || INFO.getAllCount(Terran_Command_Center, E) < 2))
			//if (S->supplyUsed() >= 240 && (INFO.getOccupiedBaseLocations(E).size() <= 3 || INFO.getAllCount(Terran_Command_Center, E) < 2))
		{
			mainStrategy = AttackAll;
			return;
		}
	}
	else
	{
		if (myBuild == MyBuildTypes::Terran_TankGoliath)
		{
			if ((S->supplyUsed() >= 380 && (INFO.getOccupiedBaseLocations(E).size() <= 2 || INFO.getAllCount(Terran_Command_Center, E) <= 2)) ||
				(myAttackUnitSupply >= (int)(enemyAttackUnitSupply * 1.4) && S->supplyUsed() >= 120))
			{
				mainStrategy = AttackAll;
				return;
			}
		}
		else if (myBuild == MyBuildTypes::Terran_VultureTankWraith)
		{
			if (S->supplyUsed() >= 240 && (INFO.getOccupiedBaseLocations(E).size() <= 2 || INFO.getAllCount(Terran_Command_Center, E) <= 3))
				//if (TIME >= 24 * 60 * 5 && myAttackUnitSupply >= (int)(enemyAttackUnitSupply * 1.5) && tankCount >= 4)
			{
				mainStrategy = AttackAll;
				return;
			}
		}
	}

	if (INFO.enemyRace == Races::Terran)
	{
		UnitInfo *firstTank = TM.getFrontTankFromPos(mainAttackPosition);

		if (firstTank == nullptr)
		{
			// 탱크 다 죽었으면 Wait 는 SecondChokePoint 로 지정
			// 다시 나갈때 Draw 가 FirstWaitLine 으로 설정되게 초기화
			waitLinePosition = INFO.getSecondChokePosition(S);

			cout << "[" << TIME << "]" << "=== 내 탱크 다 죽어서 Wait - " << endl;
			drawLinePosition = Positions::Unknown;
			mainStrategy = WaitLine;

			return;
		}

		// 멀티 3개 이하일 때 공 2업 전까지 firstWaitLine 에서 계속 대기할 것
		// 상대 마인이 없으면 볼때까지는 진출
		//if (!isExistEnemyMine
		//		&& INFO.getOccupiedBaseLocations(S).size() <= 3 && S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 2
		//		&& (INFO.getSecondChokePosition(S).getApproxDistance(INFO.getFirstWaitLinePosition())
		//			< INFO.getSecondChokePosition(S).getApproxDistance(firstTank->pos()) + 3 * TILE_SIZE))
		//{
		//	drawLinePosition == Positions::Unknown;
		//	mainStrategy = WaitLine;

		//	return;
		//}

		if (needWait(firstTank))
		{
			cout << "@@ Draw에서 WaitLine 으로 대기해야겠어 ***" << endl;
			mainStrategy = WaitLine;
			drawLinePosition = Positions::Unknown;
			return;
		}

		// 스캔 너무 적으면 Wait (스캔 체크는 적 마인이나 레이스 있을 경우에만)
		if (INFO.getAvailableScanCount() < 1 && INFO.getUnits(Spell_Scanner_Sweep, S).empty()
			&& (isExistEnemyMine || (INFO.getAllCount(Terran_Wraith, E) + INFO.getDestroyedCount(Terran_Wraith, E))))
		{
			cout << "[" << TIME << "]" << "=== 스캔 적어서 Wait - " << INFO.getAvailableScanCount() << endl;
			drawLinePosition = Positions::Unknown;
			mainStrategy = WaitLine;

			return;
		}

		// 다음 타겟 위치 설정设置下一个目标位置
		if (TIME > nextScanTime || drawLinePosition.getApproxDistance(firstTank->pos()) <= 3 * TILE_SIZE)
		{
			setDrawLinePosition();

			// 다음 가야할 위치 모르면 대기상태로位置未知
			if (drawLinePosition == Positions::Unknown || !drawLinePosition.isValid())
			{
				cout << "[" << TIME << "]" << "=== 어디로 가야할 지 몰라 Wait" << endl;
				drawLinePosition = Positions::Unknown;
				mainStrategy = WaitLine;

				return;
			}

			// 스캔 사용
			if (drawLinePosition.getApproxDistance(firstTank->pos()) > 9 * TILE_SIZE || !bw->isVisible((TilePosition)drawLinePosition))
			{
				int myUnitCountInDrawLine = INFO.getUnitsInRadius(S, drawLinePosition, 10 * TILE_SIZE, true).size();
				int enemyUnitCountInDrawLine = INFO.getUnitsInRadius(E, drawLinePosition, 10 * TILE_SIZE, true).size();

				if (myUnitCountInDrawLine <= enemyUnitCountInDrawLine)
				{
					cout << "[DrawLine] [" << TIME << "]" << "--- 스캔 사용" << (TilePosition)drawLinePosition << endl;
					ComsatStationManager::Instance().useScan(drawLinePosition);
					nextScanTime = TIME + 25 * 24;
				}
			}
		}

		// DrawLine 에서 첫번째 탱크 근처로 hide 중인 마인이 있을 경우 스캔 사용
		uList enemyMine = INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, E, firstTank->pos(), Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange(), true);

		if (!enemyMine.empty())
		{
			Position scanPosition = getAvgPosition(enemyMine);

			if (scanPosition.isValid())
			{
				ComsatStationManager::Instance().useScan(drawLinePosition);
				cout << "@@@@ 첫번째 탱크 주변에 마인 있어!! 스캔 뿌릴게" << getAvgPosition(enemyMine) / TILE_SIZE << endl;
			}
		}

		if (!moreUnits(firstTank))
		{
			drawLinePosition = Positions::Unknown;
			mainStrategy = WaitLine;
		}

		return;
	}
	}
	else if (mainStrategy == WaitLine)  //WaitLine到AttackAll、
	{
	if (INFO.enemyRace != Races::Terran)
	{
		if (S->supplyUsed() >= 320 && (INFO.getOccupiedBaseLocations(E).size() < 2 || INFO.getAllCount(Terran_Command_Center, E) < 2))
			//if (S->supplyUsed() >= 240 && (INFO.getOccupiedBaseLocations(E).size() <= 3 || INFO.getAllCount(Terran_Command_Center, E) < 2))
		{
			mainStrategy = AttackAll;
			return;
		}
	}
	else
	{
		if (myBuild == MyBuildTypes::Terran_TankGoliath)
		{
			if ((S->supplyUsed() >= 380 && (INFO.getOccupiedBaseLocations(E).size() <= 2 || INFO.getAllCount(Terran_Command_Center, E) <= 2)) ||
				(myAttackUnitSupply >= (int)(enemyAttackUnitSupply * 1.4) && S->supplyUsed() >= 120))
			{
				mainStrategy = AttackAll;
				return;
			}
		}
		else if (myBuild == MyBuildTypes::Terran_VultureTankWraith)
		{
			if (S->supplyUsed() >= 240 && (INFO.getOccupiedBaseLocations(E).size() <= 2 || INFO.getAllCount(Terran_Command_Center, E) <= 2))
				//if (TIME>=24*60*5&&myAttackUnitSupply >= (int)(enemyAttackUnitSupply * 1.5) && tankCount >= 4)
			{
				mainStrategy = AttackAll;
				return;
			}
		}
	}

	if (INFO.enemyRace == Races::Terran)
	{
		UnitInfo *firstTank = TM.getFrontTankFromPos(mainAttackPosition);

		if (firstTank != nullptr)
		{
			int distanceToFirstTank = 0;
			int distanceToSecondChokePoint = 0;

			theMap.GetPath(MYBASE, firstTank->pos(), &distanceToFirstTank);
			theMap.GetPath(MYBASE, INFO.getSecondChokePosition(S), &distanceToSecondChokePoint);

			if (distanceToFirstTank < distanceToSecondChokePoint)
				waitLinePosition = INFO.getSecondChokePosition(S);
			else
				waitLinePosition = firstTank->pos();
		}
		else
		{
			// 탱크 다 죽었으면 Wait 는 SecondChokePoint 로 지정
			// 다시 나갈때 Draw 가 FirstWaitLine 으로 설정되게 초기화
			waitLinePosition = INFO.getSecondChokePosition(S);

			return;
		}

		// 다시 DrawLine 으로 변경하기까지 20초 후에 판단
		if (nextDrawTime == 0)
		{
			nextDrawTime = TIME + 20 * 24;
			return;
		}

		if (nextDrawTime > 0 && nextDrawTime > TIME)
		{
			return;
		}

		// 20초 지난 후 Draw 로 변경되지 않았을 경우 다시 20초 후 확인되도록 리셋
		nextDrawTime = 0;
		drawLinePosition = Positions::Unknown;

		if (needWait(firstTank))
		{
			cout << "@@ Wait 계속 유지하자아" << endl;
			waitLinePosition = firstTank->pos();
			return;
		}

		// 탱크나 골리앗 수가 적거나 스캔 체크(적 마인이 있거나 레이스가 있을 경우)
		if (tankCount < 3 || goliathCount < 2
			|| (INFO.getAvailableScanCount() < 2
				&& (isExistEnemyMine || (INFO.getAllCount(Terran_Wraith, E) + INFO.getDestroyedCount(Terran_Wraith, E)))))
		{
			return;
		}

		// 다음 타겟 위치 설정
		setDrawLinePosition();

		// 다음 가야할 위치 모르면 대기상태로
		if (drawLinePosition == Positions::Unknown || !drawLinePosition.isValid())
		{
			cout << "[" << TIME << "]" << "=== 어디로 가야할 지 몰라 Wait" << endl;
			drawLinePosition = Positions::Unknown;

			return;
		}

		// 스캔 사용
		if (drawLinePosition.getApproxDistance(firstTank->pos()) > 9 * TILE_SIZE || !bw->isVisible((TilePosition)drawLinePosition))
		{
			int myUnitCountInDrawLine = INFO.getUnitsInRadius(S, drawLinePosition, 10 * TILE_SIZE, true).size();
			int enemyUnitCountInDrawLine = INFO.getUnitsInRadius(E, drawLinePosition, 10 * TILE_SIZE, true).size();

			if (myUnitCountInDrawLine <= enemyUnitCountInDrawLine)
			{
				cout << "[WaitLine] [" << TIME << "]" << "--- 스캔 사용" << (TilePosition)drawLinePosition << endl;
				ComsatStationManager::Instance().useScan(drawLinePosition);
			}
		}

		if (moreUnits(firstTank))
		{
			mainStrategy = DrawLine;
			return;
		}

		// 여기까지 왔다면 계속 wait
		drawLinePosition = Positions::Unknown;
	}
		return;
	}
	else      //主要策略等于其他
	{
		//if (S->supplyUsed() >= 380 && (INFO.getOccupiedBaseLocations(E).size() < 2 || INFO.getAllCount(Terran_Command_Center, E) < 2))
		if (SM.getMyBuild() != MyBuildTypes::Protoss_DragoonKiller&&SM.getMyBuild() != MyBuildTypes::Protoss_CannonKiller
			&&SM.getMyBuild() != MyBuildTypes::Protoss_CarrierKiller&&SM.getMyBuild() != MyBuildTypes::Protoss_ZealotKiller)
		{
			if (INFO.enemyRace != Races::Terran)
			{
				if (S->supplyUsed() >= 320 && (INFO.getOccupiedBaseLocations(E).size() < 2 || INFO.getAllCount(Terran_Command_Center, E) < 2))
					//if (S->supplyUsed() >= 240 && (INFO.getOccupiedBaseLocations(E).size() <= 3 || INFO.getAllCount(Terran_Command_Center, E) < 2))
				{
					mainStrategy = AttackAll;
					return;
				}
			}
			else
			{
				if (myBuild == MyBuildTypes::Terran_TankGoliath)
				{
					if ((S->supplyUsed() >= 380 && (INFO.getOccupiedBaseLocations(E).size() <= 2 || INFO.getAllCount(Terran_Command_Center, E) <= 2)) ||
						(myAttackUnitSupply >= (int)(enemyAttackUnitSupply * 1.4) && S->supplyUsed() >= 120))
					{
						mainStrategy = AttackAll;
						return;
					}
				}
				else if (myBuild == MyBuildTypes::Terran_VultureTankWraith)
				{
					if (S->supplyUsed() >= 240 && (INFO.getOccupiedBaseLocations(E).size() <= 2 || INFO.getAllCount(Terran_Command_Center, E) <= 3))
						//if (TIME >= 24 * 60 * 5 && myAttackUnitSupply >= (int)(enemyAttackUnitSupply * 1.5)&& tankCount >= 4)
					{
						mainStrategy = AttackAll;
						return;
					}
				}
			}
        }
		if (INFO.enemyRace == Races::Zerg)
		{
			if (myAttackUnitSupply >= 12 * 2 && S->hasResearched(TechTypes::Tank_Siege_Mode) && tankCount > 2 &&
				myAttackUnitSupply >= enemyAttackUnitSupply * 1.5 + 8)
			{
				UnitInfo *armory = INFO.getClosestTypeUnit(S, MYBASE, Terran_Armory);

				if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Plating) || S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons)
					|| (armory != nullptr && armory->unit()->getRemainingUpgradeTime() > 0 && armory->unit()->getRemainingUpgradeTime() < 24 * 30))
				{
					mainStrategy = AttackAll;

					if (scanForAttackAll == false && INFO.getAvailableScanCount() > 0)
					{
						ComsatStationManager::Instance().useScan(INFO.getSecondChokePosition(E));
						scanForAttackAll = true;
					}

					return;
				}
			}
		}
		else if (INFO.enemyRace == Races::Terran)
		{
			if (myBuild == MyBuildTypes::Terran_TankGoliath)
			{

				if (S->hasResearched(TechTypes::Tank_Siege_Mode))
				{
					UnitInfo *closestTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

					if (closestTank != nullptr)
					{
						waitLinePosition = closestTank->pos();
						drawLinePosition = Positions::Unknown;
						mainStrategy = WaitLine;

						return;
					}

				}

			}
			else if (myBuild == MyBuildTypes::Terran_VultureTankWraith)
			{
				if (myAttackUnitSupply < 3 * 2)
					return;

				if (needTank)
				{
					if (!S->hasResearched(TechTypes::Tank_Siege_Mode) && tankCount < enemyTankCount)
					{
						return;
					}
				}

				if (myAttackUnitSupply >= (int)(enemyAttackUnitSupply * 1.4) && tankCount >= 3)
				{
					mainStrategy = AttackAll;

					if (scanForAttackAll == false && INFO.getAvailableScanCount() > 0)
					{
						ComsatStationManager::Instance().useScan(INFO.getSecondChokePosition(E));
						scanForAttackAll = true;
					}

					return;
				}
			}
		}
		else if (INFO.enemyRace == Races::Protoss)
		{
		
		/*	if (ESM.getEnemyMainBuild() == Toss_fast_carrier || ESM.getEnemyMainBuild() == Toss_arbiter_carrier)
			{
				//3기 이하일때
				int threshold = 3;

				if (INFO.getCompletedCount(Protoss_Carrier, E) <= 3)
					threshold = 2;

				int sizeDiff = INFO.getCompletedCount(Terran_Goliath, S) - INFO.getCompletedCount(Protoss_Carrier, E) * threshold;

				if (S->hasResearched(TechTypes::Tank_Siege_Mode) && tankCount >= 3)
				{
					bool canAttack = false;

					if (SM.getMainAttackPosition() != Positions::Unknown) {

						uList carrier = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, SM.getMainAttackPosition(), 30 * TILE_SIZE);

						// 공격가려는 위치에 캐리어가 존재하고, 탱크가 거의 가까이 근접했다면
						if (carrier.size() > 4) {

							UnitInfo *frontTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

							if (frontTank) {

								if (frontTank->pos().getApproxDistance(SM.getMainAttackPosition()) < 20 * TILE_SIZE) {
									uList goliath = INFO.getTypeUnitsInRadius(Protoss_Carrier, E, frontTank->pos(), 20 * TILE_SIZE);

									if (goliath.size() >= carrier.size() * 4)
										canAttack = true;
								}
							}
						}
						else
							canAttack = true;
					}

					if (canAttack) {
						mainStrategy = AttackAll;

						if (scanForAttackAll == false && INFO.getAvailableScanCount() > 0)
						{
							ComsatStationManager::Instance().useScan(INFO.getSecondChokePosition(E));
							scanForAttackAll = true;
							return;
						}
					}

				}

				// 정말 이길 수 없다고 판단될때, 그냥 가진 병력가지고 진출하기
				if (INFO.getCompletedCount(Terran_Goliath, S) <= 4 && INFO.getCompletedCount(Protoss_Carrier, E) >= 8) {
					if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) + INFO.getCompletedCount(Terran_Vulture, S) >= 10) {
						mainStrategy = AttackAll;
						finalAttack = true;
						return;
					}
				}

			}*/
			
			{

				double rushThreshhold = 1.3;

				// 적이 세번째 멀티를 안먹었으면 병력을 모았을 것이므로 좀더 안전하게 한다.
				if (EMB == Toss_2base_zealot_dra) {
					if (scanForcheckMulti == false && INFO.getAvailableScanCount() > 0)
					{
						if (INFO.getSecondExpansionLocation(E) && !bw->isVisible((TilePosition)INFO.getSecondExpansionLocation(E)->Center())) {
							ComsatStationManager::Instance().useScan(INFO.getSecondExpansionLocation(E)->Center());
							scanForcheckMulti = true;
						}
					}

					rushThreshhold = 1.5;
				}
				else if (EMB == Toss_mass_production) {
					rushThreshhold = 1.2;
				}

				int Tankcnt = INFO.getAllCount(Terran_Siege_Tank_Siege_Mode, S)
					+ INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S);
				int Marinecnt = INFO.getCompletedCount(Terran_Marine, S);
				if (SM.getMyBuild() == MyBuildTypes::Protoss_TemplarKiller &&TIME >= 24 * 60 * 6.2)
				{
					
					mainStrategy = AttackAll;
					return;
				}
				else if ((SM.getMyBuild() == MyBuildTypes::Protoss_CarrierKiller || SM.getMyBuild() == MyBuildTypes::Protoss_CannonKiller || SM.getMyBuild() == MyBuildTypes::Protoss_ZealotKiller || SM.getMyBuild() == MyBuildTypes::Protoss_DragoonKiller) && S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) == 2 || S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) == 3)
				{

					if (!INFO.getUnits(Protoss_Arbiter, E).empty()) {
						if (INFO.getAllCount(Terran_Science_Vessel, S) == 0 && INFO.getAvailableScanCount() < 5) {
							return;
						}
					}
					
					// 다크가 보이면 스캔이 있거나 베슬이 있어야 나간다.
					//	if (!INFO.getUnits(Protoss_Dark_Templar, E).empty()) {
						//	if (INFO.getAllCount(Terran_Science_Vessel, S) == 0 && INFO.getAvailableScanCount() < 2) {
					//			return;
						//	}
					//	}
					
					UnitInfo *armory = INFO.getClosestTypeUnit(S, MYBASE, Terran_Armory);
					mainStrategy = AttackAll;
					/*myAttackUnitSupply >= (int)(enemyAttackUnitSupply * 1.6) || */
					if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) ||
						(armory != nullptr && armory->unit()->getRemainingUpgradeTime() > 0 && armory->unit()->getRemainingUpgradeTime() < 24 * 30))
					{
						

						if (scanForAttackAll == false && INFO.getAvailableScanCount() > 0)
						{
							ComsatStationManager::Instance().useScan(INFO.getSecondChokePosition(E));
							scanForAttackAll = true;
						}

						return;
					}
				}
				else if ((SM.getMyBuild() == MyBuildTypes::Protoss_MineKiller) &&INFO.getAllCount(Terran_Siege_Tank_Tank_Mode,S)>=1)
				{

					mainStrategy = AttackAll;
					return;
				}
			}
		}

		uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);
		
			for (auto c : commandCenterList)
			{
				if (INFO.getFirstExpansionLocation(S) != nullptr && c->pos().getApproxDistance(INFO.getFirstExpansionLocation(S)->getPosition()) < 6 * TILE_SIZE)
				{
					mainStrategy = WaitToFirstExpansion;
					return;
				}
			}
		
		
	}
}


bool StrategyManager::useML()
{
	
		return false;

	bool upgradeCheck = false;

	if (ESM.getEnemyMainBuild() == Toss_fast_carrier || ESM.getEnemyMainBuild() == Toss_arbiter_carrier)
	{
		if (S->hasResearched(TechTypes::Tank_Siege_Mode) && S->getUpgradeLevel(UpgradeTypes::Charon_Boosters))
			upgradeCheck = true;
	}
	else {
		UnitInfo *armory = INFO.getClosestTypeUnit(S, MYBASE, Terran_Armory);

		/*myAttackUnitSupply >= (int)(enemyAttackUnitSupply * 1.6) || */
		if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) ||
				(armory != nullptr && armory->unit()->getRemainingUpgradeTime() > 0 && armory->unit()->getRemainingUpgradeTime() < 24 * 30))
			upgradeCheck = true;
	}

	if (upgradeCheck) {
		clock_t str = clock();

		if (TIME % PERIOD == 24)
			SUPER.update(S);	// 아군 유닛 맵 업데이트
		else if (TIME % PERIOD == 48)
			SUPER.update(E);	// 적군 유닛 맵 업데이트
		else if (TIME % PERIOD == 72) {
			SHM.write();		// 공유메모리에 유닛 정보 쓰기

			// 최초 1번 보내졌는지 확인
			sendStart = true;
		}
		else if (TIME % PERIOD == 0)  // <<< 메모리 읽는 주기 변경하는 곳
		{
			// 공유메모리에서 승률 읽기, 해당 게임에서 정보 보냈는지 확인 후 읽음
			if (sendStart)
			{
				SHM.read();

				cout << "[" << TIME << "]" << "### 현재 승률 = " << SHM.getWinningRate() << endl;

				if (SHM.getWinningRate() > BASE_WINNING_RATE)  // <<< 승률 바꾸는 곳
				{
					mainStrategy = AttackAll;

					if (scanForAttackAll == false && INFO.getAvailableScanCount() > 0)
					{
						ComsatStationManager::Instance().useScan(INFO.getSecondChokePosition(E));
						scanForAttackAll = true;
					}
				}

			}
		}

		clock_t end = clock();

		if (end - str > 55)
			SHM.setWinningRate(-1);
	}

	return true;

}

//人口管理======================================================================
void StrategyManager::executeSupplyManagement()
{
	// 1초에 한번만 실행
	if (TIME % 24 != 0) {
		return;
	}

	// 생산/건설 중인 Supply를 센다
	int onBuildingSupplyCount = 0;

	onBuildingSupplyCount += CM.getConstructionQueueItemCount(Terran_Supply_Depot) * Terran_Supply_Depot.supplyProvided();

	// 건설중인 커맨드가 있다면 완성이 다 되가는 시점에서 건설중인 서플 건물에 포함
	uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

	for (auto c : commandCenterList)
	{
		if (!c->isComplete() && c->hp() > 875)
		{
			onBuildingSupplyCount += c->type().supplyProvided();
		}
	}


	if (S->supplyUsed() + onBuildingSupplyCount >= 400)
	{
		return;
	}

	// 게임에서는 서플라이 값이 200까지 있지만, BWAPI 에서는 서플라이 값이 400까지 있다
	// 저글링 1마리가 게임에서는 서플라이를 0.5 차지하지만, BWAPI 에서는 서플라이를 1 차지한다
	if (S->supplyTotal() < 400)
	{
		// 서플라이가 다 꽉찼을때 새 서플라이를 지으면 지연이 많이 일어나므로, supplyMargin (게임에서의 서플라이 마진 값의 2배)만큼 부족해지면 새 서플라이를 짓도록 한다
		// 이렇게 값을 정해놓으면, 게임 초반부에는 서플라이를 너무 일찍 짓고, 게임 후반부에는 서플라이를 너무 늦게 짓게 된다
		int supplyMargin = 0;

		// 여기에 건물의 상태와 미네랄 사정을 봐서 supplyMargin을 늘릴 필요가 있다.
		int numberOfCommandCenter = getTrainBuildingCount(Terran_Command_Center);
		int numberOfBarracks = getTrainBuildingCount(Terran_Barracks);
		int numberOfFactory = getTrainBuildingCount(Terran_Factory);
		int numberofStarport = getTrainBuildingCount(Terran_Starport);

		supplyMargin = (numberOfBarracks * 1 + numberOfCommandCenter * 1 + numberOfFactory * 2 + numberofStarport * 3) * 2;

		double velocity = 0;
		int supplyCount = INFO.getAllCount(Terran_Supply_Depot, S);
		int remaingMineral = S->minerals();

		if (supplyCount == 0)
		{
			velocity = 2;
		}
		else if (supplyCount == 1)
		{
			if (ESM.getEnemyInitialBuild() <= Zerg_9_Drone)
			{
				velocity = 1.0;

				if (BasicBuildStrategy::Instance().cancelSupplyDepot)
				{
					if (INFO.getAllCount(Terran_Bunker, S) < 1)
					{
						velocity = 0.5;
					}
				}
			}
			else
			{
				velocity = 2.0;
			}
		}
		else if (supplyCount == 2)
			velocity = 1.0;
		else if (supplyCount == 3)
			velocity = 1.3;
		else if (supplyCount >= 4)
			velocity = 1.5;

		supplyMargin = (int)(supplyMargin * velocity);

		// currentSupplyShortage 를 계산한다
		int currentSupplyShortage = S->supplyUsed() + supplyMargin - S->supplyTotal();

		// 함수 초입에서 SD가 400이 넘을 경우를 고려하더라도 margin과 사용중인 SD를 더할 경우는 초과될 수 있음을 고려 deduction 로직 추가
		if (currentSupplyShortage + S->supplyTotal() > 400) {
			currentSupplyShortage = currentSupplyShortage + S->supplyTotal() - 400;
		}

		//cout << "supplyMargin :: " << supplyMargin << endl;
		if (currentSupplyShortage > 0) {
			//cout << "currentSupplyShortage : " << currentSupplyShortage << ", onBuildingSupplyCount : " << onBuildingSupplyCount << endl;

			if (currentSupplyShortage > onBuildingSupplyCount) 
			{
				bool needBarricade = false;

				for (auto b : INFO.getBuildings(Terran_Barracks, S))
				{
					if (b->getState() == "Barricade")
					{
						needBarricade = true;
					}
				}

				for (auto b : INFO.getBuildings(Terran_Engineering_Bay, S))
				{
					if (b->getState() == "Barricade")
					{
						needBarricade = true;
					}
				}

				// SecondChokePoint 에 바리케이트 서플이 필요한 경우
				// 일단 주석 처리함.
				/*
				if (needBarricade && INFO.getOccupiedBaseLocations(S).size() > 1
				&& TerranConstructionPlaceFinder::Instance().getSupplysPositionInSCP().isValid())
				{
				bool isToEnqueue = false;

				if (!BM.buildQueue.isEmpty()) {
				BuildOrderItem currentItem = BM.buildQueue.getHighestPriorityItem();

				if (currentItem.metaType.isUnit() && currentItem.metaType.getUnitType() == Terran_Supply_Depot) {
				isToEnqueue = false;
				}
				}

				if (isToEnqueue) {
				cout << "바리게이트 서플 추가 요청" << endl;
				BM.buildQueue.queueAsHighestPriority(Terran_Supply_Depot, BuildOrderItem::SeedPositionStrategy::SecondChokePoint);
				}
				}
				else*/
				{
					// 적군에 의해서 SD의 부족이 많이 발생하면 현재 Mineral을 감안하여 다수의 SCV들이 SD를 빨리 짓는 로직
					if (currentSupplyShortage > 2 * Terran_Supply_Depot.supplyProvided()) 
					{
						int neededTobuildByForce = currentSupplyShortage / Terran_Supply_Depot.supplyProvided();

						for (int i = 0; i < min(neededTobuildByForce, remaingMineral / 100); i++) {
							if (CM.getConstructionQueueItemCount(Terran_Factory)
									+ BM.buildQueue.getItemCount(Terran_Factory) < neededTobuildByForce)
							{
								BM.buildQueue.queueAsHighestPriority(Terran_Supply_Depot);
							}
						}
					}
					else {
						if (supplyCount <= 4)
						{
							// BuildQueue 내에 SupplyProvider 가 있지 않으면 enqueue 한다
							bool isToEnqueue = true;

							if (!BM.buildQueue.isEmpty())
							{
								if (BM.buildQueue.existItem(Terran_Supply_Depot))
								{
									isToEnqueue = false;
								}
							}

							if (isToEnqueue) {
								BM.buildQueue.queueAsHighestPriority(Terran_Supply_Depot, true);
							}
						}
						else
						{
							// 将SP放到建造队列中
							bool isToEnqueue = true;

							if (!BM.buildQueue.isEmpty()) 
							{
								BuildOrderItem currentItem = BM.buildQueue.getHighestPriorityItem();

								if (currentItem.metaType.isUnit() && currentItem.metaType.getUnitType() == Terran_Supply_Depot) {
									isToEnqueue = false;
								}
							}

							if (isToEnqueue) {
						
								BM.buildQueue.queueAsHighestPriority(Terran_Supply_Depot, true);
							}
						}
					}
				}
			}
		}
	}
}

void StrategyManager::searchForEliminate()
{
	if (TIME % 24 != 0) {
		return;
	}

	Position targetPosition = Positions::None;

	uList enemys = INFO.getBuildingsInRadius(E, Positions::Origin, 0, true, true, true);

	bool isAir = false;

	for (auto eu : enemys)
	{
		if (eu->pos() != Positions::Unknown)
		{
			targetPosition = eu->pos();
			isAir = eu->unit()->isFlying();
			break;
		}
	}

	// 엘리 로직에 Air가 있는 경우 무조건 Wraith를 생산한다.
	if (isAir && INFO.getAllCount(Terran_Wraith, S) == 0)
	{
		if (S->supplyUsed() > 390) // 추가 생산을 못하는 수준
		{
			for (auto u : INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S))
			{
				UnitInfo *closestVulture = INFO.getClosestTypeUnit(S, u->pos(), Terran_Vulture);

				if (closestVulture != nullptr)
					CommandUtil::attackUnit(u->unit(), closestVulture->unit());
			}
		}

		for (auto star : INFO.getTypeBuildingsInRadius(Terran_Starport, S))
		{
			if (star->isComplete())
			{
				if (!star->unit()->isTraining() && Broodwar->canMake(Terran_Wraith, star->unit()))
				{
					star->unit()->train(Terran_Wraith);
				}
			}
		}
	}

	for (auto u : INFO.getUnitsInRadius(S, Positions::Origin, 0, true, true, false))
	{
		if (u->getState() != "Eliminate" && u->getState() != "Defence") {
			// 임시 방편, 캐리어 때문에 상대가 프로토스이고 내 유닛이 골리앗이면 Eli로직으로 안가도록
			if (INFO.enemyRace != Races::Protoss || u->unit()->getType() != Terran_Goliath) {
				u->setState(new EliminateState());
			}
		}

		if (u->unit()->isSieged())
			u->unit()->unsiege();
	}

	if (targetPosition == Positions::None) {

		for (auto u : INFO.getUnitsInRadius(S, Positions::Origin, 0, true, true, false))
			searchAllMap(u->unit());

	}
	else {

		for (auto u : INFO.getUnitsInRadius(S, Positions::Origin, 0, true, true, false)) {

			if (isAir) {
				if (u->unit()->getType() == Terran_Goliath || u->unit()->getType() == Terran_Wraith)
					u->unit()->attack(targetPosition);
				else
					searchAllMap(u->unit());
			}
			else
				u->unit()->attack(targetPosition);

		}

	}

}

void StrategyManager::searchAllMap(Unit u) {

	Position targetPosition = Positions::None;
	UnitCommand command = u->getLastCommand();
	bool enemyRemoved = false;

	// 서치하려는 곳이 이미 보이고 있음에도 적 병력이 없는 경우.
	if (command.getTargetPosition().isValid() && bw->isVisible((TilePosition)command.getTargetPosition())) {
		if (INFO.getUnitsInRadius(E, command.getTargetPosition(), 10 * TILE_SIZE, true, true, true, true).empty() &&
				INFO.getBuildingsInRadius(E, command.getTargetPosition(), 10 * TILE_SIZE, true, true, true).empty())
			enemyRemoved = true;
	}

	if (u->isIdle() || enemyRemoved) {

		if (searchPoint < 400)
		{
			targetPosition = map400[searchPoint];
			searchPoint++;
		}
		else
		{
			searchPoint = 0;
		}

		u->attack(targetPosition);
	}
}

bool StrategyManager::getNeedSecondExpansion()
{
	return needSecondExpansion;
}

bool StrategyManager::getNeedThirdExpansion()
{
	return needThirdExpansion;
}

bool StrategyManager::getNeedAdvanceWait()
{
	return advanceWait;
}
//设置我的建造类型=============================
void StrategyManager::setMyBuild()
{
	string enemyName = BWAPI::Broodwar->enemy()->getName();


	if (INFO.enemyRace == Races::Terran)
	{
		if (EIB == Terran_1fac_1star ||
			EIB == Terran_1fac_2star ||
			EIB == Terran_2fac_1star ||
			EMB == Terran_1fac_1star ||
			EMB == Terran_1fac_2star ||
			EMB == Terran_2fac_1star ||
			EMB == Terran_2fac
			)
			myBuild = MyBuildTypes::Terran_TankGoliath;
		else
			myBuild = MyBuildTypes::Terran_VultureTankWraith;
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
	

		if (EIB == UnknownBuild)
			myBuild = MyBuildTypes::Protoss_DragoonKiller;
		else
		{
			
			if (EMB == Toss_dark &&TIME < 3.1 * 60 * 24)
				myBuild = MyBuildTypes::Protoss_TemplarKiller;


			else if ((EIB == Toss_1g_forward || EIB == Toss_2g_forward || EIB == Toss_2g_zealot)&& myBuild != MyBuildTypes::Protoss_TemplarKiller)
				myBuild = MyBuildTypes::Protoss_ZealotKiller;

			else if (EIB == Toss_pure_double || EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier)
			{
				if(INFO.getMapPlayerLimit() == 2)
					myBuild = MyBuildTypes::Protoss_MineKiller;

				else 
				{
					if (EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier)
					 myBuild = MyBuildTypes::Protoss_CarrierKiller;
					
					else
						myBuild = MyBuildTypes::Protoss_CannonKiller;
				}
			}


			else if (EIB == Toss_forge || Toss_cannon_rush)
				myBuild = MyBuildTypes::Protoss_CannonKiller;

			else 
			{
				if (myBuild = MyBuildTypes::Protoss_DragoonKiller)
					myBuild = MyBuildTypes::Protoss_DragoonKiller;

			}

		}
	}
	else if (INFO.enemyRace == Races::Zerg)
	{

	}
}

/*
작성자 : 최현진
작성일 : 2018-01-30
기능 : 첫 번 째 초크포인트를 막아서 적 정찰을 막음
Prameter : 해당 유닛으로 길막(ex : Marine, SCV)
상세 : Chokepoint의 길이에 따라 유닛 1마리 혹은 2마리로 막음(2마리 이상은 사용X)
*/
void StrategyManager::blockFirstChokePoint(UnitType type)
{
	WalkPosition cpEnd1 = theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition())->ChokePoints().at(0)->Pos(BWEM::ChokePoint::end1);
	WalkPosition cpMiddle = theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition())->ChokePoints().at(0)->Pos(BWEM::ChokePoint::middle);
	WalkPosition cpEnd2 = theMap.GetArea(INFO.getMainBaseLocation(S)->getTilePosition())->ChokePoints().at(0)->Pos(BWEM::ChokePoint::end2);

	Position cpEnd1Pos = Position(cpEnd1);
	Position cpMiddlePos = Position(cpMiddle);
	Position cpEnd2Pos = Position(cpEnd2);

	//디버깅 테스트용
	/*Broodwar->drawText(CoordinateType::Map, cpEnd1Pos.x, cpEnd1Pos.y, "End1");
	Broodwar->drawText(CoordinateType::Map, cpMiddlePos.x, cpMiddlePos.y, "Middle");
	int a = cpEnd1Pos.getDistance(cpEnd2Pos);
	char width[20];
	itoa(a, width, 10);
	Broodwar->drawText(CoordinateType::Map, cpMiddlePos.x, cpMiddlePos.y, width);
	Broodwar->drawText(CoordinateType::Map, cpEnd2Pos.x, cpEnd2Pos.y, "End2");*/

	for (auto &unit : S->getUnits())
	{
		if (!unit->isCompleted()) continue;

		if (unit->getType() == type)
		{
			if (m1 == nullptr)
				m1 = unit;
			else if (m2 == nullptr && unit != m1)
				m2 = unit;
		}
	}

	if (cpEnd1Pos.getDistance(cpEnd2Pos) > 50) // Chokepoint 길이가 50초과인 경우 마린 2마리
	{
		if (m1 != nullptr)
			if (m1->getDistance((cpEnd1Pos + cpMiddlePos) / 2) > 1)
			{
				m1->move((cpEnd1Pos + cpMiddlePos) / 2);
			}
			else
			{
				m1->holdPosition();
			}

		if (m2 != nullptr)
			if (m2->getDistance((cpEnd2Pos + cpMiddlePos) / 2) > 1)
			{
				m2->move((cpEnd2Pos + cpMiddlePos) / 2);
			}
			else
			{
				m2->holdPosition();
			}
	}
	else // Chokepoint 길이가 50 이하인 경우 마린 1마리
	{
		if (m1 != nullptr)
			if (m1->getDistance(getDefencePos(cpMiddlePos, cpEnd1Pos, INFO.getMainBaseLocation(S)->getPosition())) > 1)
			{
				m1->move(getDefencePos(cpMiddlePos, cpEnd1Pos, INFO.getMainBaseLocation(S)->getPosition()));
			}
			else
			{
				m1->holdPosition();
			}
	}
}
//建造完成以及建造血量大于60％的建筑=============================
int StrategyManager::getTrainBuildingCount(UnitType type)
{
	uList buildings = INFO.getBuildings(type, S);
	int count = 0;

	for (auto b : buildings)
	{
		// 완성되지 않은 건물의 경우 hp가 60% 이상일때부터 카운팅 (너무 빨리 서플 증설하지 않도록)
		if (!b->isComplete() && b->hp() < b->type().maxHitPoints() * 0.6)
			continue;

		count++;
	}

	return count;
}
//检查升级==========================================================
void StrategyManager::checkUpgrade()
{
	if (TIME % 24 != 0)
		return;

	if ((INFO.enemyRace == Races::Protoss) &&
			(ESM.getEnemyInitialBuild() == Toss_1g_dragoon || ESM.getEnemyInitialBuild() == Toss_2g_dragoon ||
			 ESM.getEnemyInitialBuild() == Toss_1g_double || ESM.getEnemyInitialBuild() == UnknownBuild))
	{
		// 탱크가 없으면 탱크 1개 먼저 찍는다
		// 탱크 1개 이상인데 시즈업이 안되어 있는 상태면 시즈업 먼저 찍는다.
		if (BasicBuildStrategy::Instance().isResearchingOrResearched(TechTypes::Tank_Siege_Mode)) 
		{
			needUpgrade = false;
			return;
		}

		// 생산중이거나 생산된 탱크가 존재하고 머신샵이 달려있고 시즈모드업이 안되어있고
		// 적 빌드를 알거나, 앞마당을 확인했으면 업그레이드 먼저 하자.(참고 : BasicBuildStrategy의 엔베 올리는 부분)
		if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0 && INFO.getCompletedCount(Terran_Machine_Shop, S) > 0 &&
				!BasicBuildStrategy::Instance().isResearchingOrResearched(TechTypes::Tank_Siege_Mode)
				&& !(EIB != Toss_cannon_rush && (EMB == UnknownMainBuild || EMB == Toss_dark) && !HasEnemyFirstExpansion()))
			needUpgrade = true;
		else
			needUpgrade = false;
	}

	return;
}
//检查是否用雷达==========================================================
void StrategyManager::checkUsingScan()
{
	if (TIME % 24 != 0)
		return;

	if (INFO.getAvailableScanCount() < 1)
		return;

	if (INFO.getMainBaseLocation(E) != nullptr && INFO.getMainBaseLocation(E)->GetLastVisitedTime() == 0)
	{
		ComsatStationManager::Instance().useScan(INFO.getMainBaseLocation(E)->Center());
		return;
	}

	if (INFO.enemyRace == Races::Zerg)
	{

	}
	else if (INFO.enemyRace == Races::Terran)
	{

	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		if (ESM.getEnemyMainBuild() != Toss_arbiter_carrier
				&& lastUsingScanTime + 24 * 60 * 1.5 < TIME
				&& INFO.getAvailableScanCount() > 1) // 여유 있을때는 쓰자.
		{

			INFO.setNextScanPointOfMainBase();

			if (INFO.getMainBaseLocation(E) != nullptr && !bw->isVisible((TilePosition)INFO.getMainBaseLocation(E)->Center())) {
				ComsatStationManager::Instance().useScan((Position)INFO.getScanPositionOfMainBase());
				lastUsingScanTime = TIME;
			}

		}
	}

	checkEnemyMultiBase();
}
//设置进攻地址==================================================
void StrategyManager::setAttackPosition()
{
	if (TIME % 24 != 0)
		return;

	const Base *currentAttackBase = nullptr;

	if (mainAttackPosition != Positions::Unknown)
	{
		Base *nearestBase = INFO.getNearestBaseLocation(mainAttackPosition, true);

		if (nearestBase && nearestBase->GetOccupiedInfo() == enemyBase)
			currentAttackBase = nearestBase;
	}

	// Special Case : 맵 중앙 멀티는 항상 우선순위로 Main Attack Position을 설정한다.
	for (auto eBase : INFO.getOccupiedBaseLocations(E))
	{
		if (eBase->Center().getApproxDistance(theMap.Center()) < 10 * TILE_SIZE)
		{
			currentAttackBase = eBase;
		}
	}

	//Position standardPos = (mainAttackPosition == Positions::Unknown) ? MYBASE : mainAttackPosition;
	Position standardPos = MYBASE;
	int closestDistance = INT_MAX;
	Position closestPosition = Positions::Unknown;

	// 0. 제일 가까운 상대 베이스 지정
	if (currentAttackBase) // 현재 메인 공격지점이 있는 경우
	{
		bool isExistResourceDepot = false;

		Position currentBasePos = currentAttackBase->Center();

		if (INFO.enemyRace == Races::Zerg)
		{
			if (INFO.getTypeBuildingsInRadius(Zerg_Hatchery, E, currentBasePos, 8 * TILE_SIZE).size()
					+ INFO.getTypeBuildingsInRadius(Zerg_Lair, E, currentBasePos, 8 * TILE_SIZE).size()
					+ INFO.getTypeBuildingsInRadius(Zerg_Hive, E, currentBasePos, 8 * TILE_SIZE).size() > 0)
				isExistResourceDepot = true;
		}
		else
		{
			if (INFO.getTypeBuildingsInRadius(INFO.getBasicResourceDepotBuildingType(INFO.enemyRace), E, currentBasePos, 8 * TILE_SIZE).size() > 0)
				isExistResourceDepot = true;
		}

		if (isExistResourceDepot)
		{
			mainAttackPosition = currentBasePos;
		}
		else // 如果把目前的主要攻击点轰出去，
		{
			if (INFO.getBuildingsInRadius(E, mainAttackPosition, 2 * TILE_SIZE, true, false, true).size() == 0)
			{
				uMap enemyBuildings = INFO.getBuildings(E);

				Position sameAreaBuilding = Positions::None;

				for (auto &enemyBuilding : enemyBuildings)
				{
					if (enemyBuilding.second->pos() == Positions::Unknown)
						continue;

					if (enemyBuilding.second->getLift())
						continue;

					if (isSameArea(enemyBuilding.second->pos(), mainAttackPosition))
					{
						mainAttackPosition = enemyBuilding.second->pos();
						sameAreaBuilding = mainAttackPosition;
						break;
					}
				}

				// Occupied가 12 TILE 기준이기 때문에 Same Area 없으면 다시 거리 기준으로 한번 더 본다.
				if (sameAreaBuilding == Positions::None)
				{
					for (auto &enemyBuilding : enemyBuildings)
					{
						if (enemyBuilding.second->pos() == Positions::Unknown)
							continue;

						if (enemyBuilding.second->getLift())
							continue;

						int dist = getGroundDistance(currentBasePos, enemyBuilding.second->pos());

						if (dist >= 0 && dist < 12 * TILE_SIZE)
						{
							mainAttackPosition = enemyBuilding.second->pos();
							break;
						}
					}
				}
			}
		}

		// 相对 Main Attack Position从离这儿最远的地方开始打碎。. 18.09.15
		standardPos = mainAttackPosition;
		int farDistance = 0;
		Position farPosition = Positions::Unknown;

		for (auto eBase : INFO.getOccupiedBaseLocations(E))
		{
			// 已经排除岛屿了
			if (eBase->isIsland())
				continue;

			int tempDist = getGroundDistance(standardPos, eBase->Center());

			if (tempDist == -1)
				continue;

			if (currentAttackBase != nullptr && eBase == currentAttackBase)
				continue;

			// 적의 Main Base를 밀고 있는데 멀티가 적의 앞마당이면 어차피 밀기때문에 Second Attack Position으로 설정하지 않는다.
			if (currentAttackBase == INFO.getMainBaseLocation(E) && INFO.getFirstExpansionLocation(E) && eBase == INFO.getFirstExpansionLocation(E))
				continue;

			// 적의 Main과 앞마당이 뒤집히는 경우가 있음.
			if (INFO.getFirstExpansionLocation(E) && currentAttackBase == INFO.getFirstExpansionLocation(E) && eBase == INFO.getMainBaseLocation(E))
				continue;

			// 적의 멀티가 내 선두 골탱보다 적 본진에서 가까우면 잡지 말아보자
			UnitInfo *forwardTank = TM.getFrontTankFromPos(currentAttackBase->Center());

			if (forwardTank)
			{
				int gDist = getGroundDistance(forwardTank->pos(), currentAttackBase->Center());
				int baseGDist = getGroundDistance(currentAttackBase->Center(), eBase->Center());

				if (gDist >= 0 && baseGDist >= 0 && gDist > baseGDist)
					continue;
			}

			// 안드로메다와 같이 적의 앞마당과 본진 사이에 멀티가 있는 경우 Second Attack Position으로 설정하지 않는다.
			if (currentAttackBase == INFO.getMainBaseLocation(E) && isSameArea(eBase->Center(), currentAttackBase->Center()))
				continue;

			if (INFO.enemyRace == Races::Zerg)
			{
				if (INFO.getTypeBuildingsInRadius(Zerg_Hatchery, E, eBase->Center(), 8 * TILE_SIZE).size()
						+ INFO.getTypeBuildingsInRadius(Zerg_Lair, E, eBase->Center(), 8 * TILE_SIZE).size()
						+ INFO.getTypeBuildingsInRadius(Zerg_Hive, E, eBase->Center(), 8 * TILE_SIZE).size() == 0)
					continue;
			}
			else
			{
				if (INFO.getTypeBuildingsInRadius(INFO.getBasicResourceDepotBuildingType(INFO.enemyRace), E, eBase->Center(), 8 * TILE_SIZE).size() == 0)
					continue;
			}

			if (tempDist > farDistance)
			{
				farDistance = tempDist;
				farPosition = eBase->Center();
			}
		}

		if (farPosition != Positions::Unknown)
			secondAttackPosition = farPosition;
		else
		{
			secondAttackPosition = Positions::Unknown;
			needAttackMulti = false;
		}
	}
	else // 현재 메인공격지점이 없는 경우.
	{
		if (mainAttackPosition != Positions::Unknown && theMap.Center().getApproxDistance(mainAttackPosition) < 10 * TILE_SIZE) 
		{
			mainAttackPosition = INFO.getMainBaseLocation(E)->Center();
			return;
		}

		secondAttackPosition = Positions::Unknown;
		needAttackMulti = false;

		standardPos = mainAttackPosition;
		closestDistance = INT_MAX;
		closestPosition = Positions::Unknown;

		// 0. 제일 가까운 상대 베이스 지정

		for (auto eBase : INFO.getOccupiedBaseLocations(E))
		{
			// 일단 섬은 제외
			if (eBase->isIsland())
				continue;

			int tempDist = getGroundDistance(standardPos, eBase->Center());

			if (closestDistance > tempDist)
			{
				closestPosition = eBase->Center();
				closestDistance = tempDist;
			}
		}

		if (closestPosition != Positions::Unknown) // 설정할 Base가 있으면 일단 설정
		{
			mainAttackPosition = closestPosition;
			return;
		}
		else // 설정할 Base도 없으면
		{
			uMap enemyBuildings = INFO.getBuildings(E);
			// 3. 상대 모든 건물에 대해서 가까운 건물 순서로 지정
			bool isExistEnemyBuilding = false;
			closestDistance = INT_MAX;
			Position tempBuildingPos = Positions::None;

			for (auto &enemyBuilding : enemyBuildings)
			{
				if (enemyBuilding.second->pos() == Positions::Unknown)
					continue;

				// 떠다니는 건물 무시
				if (enemyBuilding.second->getLift())
					continue;

				int dist = getGroundDistance(MYBASE, enemyBuilding.second->pos());

				if (closestDistance > dist)
				{
					tempBuildingPos = enemyBuilding.second->pos();
					closestDistance = dist;
					isExistEnemyBuilding = true;
				}
			}

			if (isExistEnemyBuilding)
			{
				//printf("[isExistEnemyBuilding]-- 건물 위치 (%d, %d)\n", closestPosition.x / TILE_SIZE, closestPosition.y / TILE_SIZE);
				mainAttackPosition = tempBuildingPos;
				return;
			}

			// 모든 스타팅 체크해봤는지 확인, 초반 정찰 실패했을 때 엘리 로직 타지 않기 위해
			bool completedSearch = true;

			for (auto sl : INFO.getStartLocations())
			{
				if (sl->GetLastVisitedTime() == 0)
				{
					completedSearch = false;
					break;
				}
			}

			// 4. 내가 아는 상대 건물이 없고, 모든 스타팅 포인트를 다 확인해본 상태에서 예상지역 확인
			if (completedSearch)
			{
				// 엘리 로직 필요
				mainStrategy = Eliminate;
			}
			else
			{
				// 예상 지역으로 공격지점 설정
				mainAttackPosition = INFO.getMainBaseLocation(E)->Center();
			}
		}
	}
}

void StrategyManager::checkAdvanceWait()
{
	if (!advanceWait)
	{
		if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) >= 2 && INFO.getCompletedCount(Terran_Goliath, S) >= 4)
			advanceWait = true;
	}
}

void StrategyManager::setDrawLinePosition()
{
	UnitInfo *firstTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

	if (firstTank == nullptr)
	{
		drawLinePosition = waitLinePosition;
		return;
	}

	// 최초 설정은 FirstWaitLinePosition 으로
	if (INFO.getAllCount(Terran_Command_Center, S) < 3)
	{
		Position tempLinePosition = Positions::Unknown;
		int tankToFirstWaitLineDistance = 0;

		//theMap.GetPath(firstTank->pos(), INFO.getFirstWaitLinePosition(), &tankToFirstWaitLineDistance);
		tankToFirstWaitLineDistance = firstTank->pos().getApproxDistance(INFO.getFirstWaitLinePosition());

		if (tankToFirstWaitLineDistance > 12 * TILE_SIZE)
		{
			tempLinePosition = getDirectionDistancePosition(firstTank->pos(), INFO.getFirstWaitLinePosition(), 12 * TILE_SIZE);
		}
		else
		{
			tempLinePosition = INFO.getFirstWaitLinePosition();
		}

		//// 구한 drawLinePosition 이 걸어서 갈 수 없는 곳이면 첫번째 탱크쪽으로 보정
		//if (!bw->isWalkable((WalkPosition)tempLinePosition))
		//{
		//	for (int i = 0; i < 2; i++)
		//	{
		//		tempLinePosition = getDirectionDistancePosition(tempLinePosition, firstTank->pos(), 3 * TILE_SIZE);

		//		if (bw->isWalkable((WalkPosition)tempLinePosition))
		//			break;
		//	}
		//}

		//if (drawLinePosition != Positions::Unknown && drawLinePosition.isValid())
		//{
		//	if (drawLinePosition.getApproxDistance(tempLinePosition) < 5 * TILE_SIZE)
		//		return;
		//}

		if (tempLinePosition.getApproxDistance(INFO.getFirstWaitLinePosition()) < 5 * TILE_SIZE)
		{
			drawLinePosition = INFO.getFirstWaitLinePosition();
		}
		else
		{
			drawLinePosition = tempLinePosition;
		}

		int SCPToFirstWaitLineDistance = 0;
		int SCPToFirstTankDistance = 0;

		theMap.GetPath(INFO.getSecondChokePosition(S), INFO.getFirstWaitLinePosition(), &SCPToFirstWaitLineDistance);
		theMap.GetPath(INFO.getSecondChokePosition(S), firstTank->pos(), &SCPToFirstTankDistance);

		if (SCPToFirstTankDistance < SCPToFirstWaitLineDistance - 2.5 * TILE_SIZE)
		{
			//cout << "## 아직 FirstWaitLine 까지 못갔어.." << endl;
			return;
		}
		else
		{
			//cout << "@@ 더 나가즈아아아아아" << endl;
		}
	}

	int tankDistance = 0;
	int firstWaitLineDistance = 0;
	Position basePosition = Positions::Unknown;

	theMap.GetPath(INFO.getFirstExpansionLocation(S)->getPosition(), firstTank->pos(), &tankDistance);
	theMap.GetPath(INFO.getFirstExpansionLocation(S)->getPosition(), INFO.getFirstWaitLinePosition(), &firstWaitLineDistance);

	if (tankDistance < firstWaitLineDistance)
		basePosition = INFO.getFirstWaitLinePosition();
	else
		basePosition = firstTank->pos();

	const CPPath &path = theMap.GetPath(firstTank->pos(), mainAttackPosition);

	if (path.empty())
	{
		// 중간에 ChokePoint 없으면 바로 mainAttackPosition 을 타겟으로
		if (basePosition.getApproxDistance(mainAttackPosition) < 15 * TILE_SIZE)
		{
			drawLinePosition = mainAttackPosition;
		}
		else
		{
			drawLinePosition = getDirectionDistancePosition(basePosition, mainAttackPosition, 15 * TILE_SIZE);
		}
	}
	else
	{
		auto c = path.begin();

		// 제일 가까운 ChokePoint 랑 가까우면 다음 ChokePoint 를 타겟으로 계산
		if (basePosition.getApproxDistance((Position)(*c)->Center()) < 6 * TILE_SIZE)
		{
			c++;

			// 다음 ChokePoint 가 없으면 mainAttackPosition 을 기준으로
			if (c == path.end())
			{
				drawLinePosition = getDirectionDistancePosition(basePosition, mainAttackPosition, 15 * TILE_SIZE);
			}
			else
			{
				drawLinePosition = getDirectionDistancePosition(basePosition, (Position)(*c)->Center(), 15 * TILE_SIZE);
			}
		}
		else
		{
			drawLinePosition = getDirectionDistancePosition(basePosition, (Position)(*c)->Center(), 15 * TILE_SIZE);
		}

		// 구한 drawLinePosition 이 걸어서 갈 수 없는 곳이면 맵의 센터쪽으로 보정
		if (!bw->isWalkable((WalkPosition)drawLinePosition))
		{
			for (int i = 0; i < 3; i++)
			{
				drawLinePosition = getDirectionDistancePosition(drawLinePosition, (Position)theMap.Center(), 3 * TILE_SIZE);

				if (bw->isWalkable((WalkPosition)drawLinePosition))
					break;
			}
		}

		// 구한 pos 이 벽이랑 너무 가까우면 센터쪽으로 보정
		if (theMap.GetMiniTile((WalkPosition)drawLinePosition).Altitude() < 100)
		{
			for (int i = 0; i < 2; i++)
			{
				drawLinePosition = getDirectionDistancePosition(drawLinePosition, (Position)theMap.Center(), 3 * TILE_SIZE);

				if (bw->isWalkable((WalkPosition)drawLinePosition) && theMap.GetMiniTile((WalkPosition)drawLinePosition).Altitude() > 100)
					break;
			}
		}
	}
}

bool StrategyManager::needWait(UnitInfo *firstTank)
{
	/// 상대 앞마당 제외 모든 멀티 부시기 전까지 무리해서 상대 앞마당으로 공격가지 않고 기다려야 해
	if (firstTank == nullptr)
		return false;

	bool needWait = false;

	//cout << "### First Tank = " << firstTank->pos() / 32 << endl;

	// 3번째 커맨드 건설 전까진 FirstWaitLinePosition 에서 조금 더 나가고 그만 나갈래
	if (INFO.getAllCount(Terran_Command_Center, S) < 3)
	{
		int SCPToFirstWaitLineDistance = 0;
		int SCPToFirstTankDistance = 0;

		theMap.GetPath(INFO.getSecondChokePosition(S), INFO.getFirstWaitLinePosition(), &SCPToFirstWaitLineDistance);
		theMap.GetPath(INFO.getSecondChokePosition(S), firstTank->pos(), &SCPToFirstTankDistance);

		// 첫번째 탱크 위치가 FirstWaitLinePosition 보다 8타일 더 나갔으면 여기까지
		if (SCPToFirstTankDistance > SCPToFirstWaitLineDistance + 8 * TILE_SIZE)
			return true;
		else
			return false;
	}

	// 6팩, 드랍쉽 3기 완성되기 전까진 센터 이상 안나갈래
	if ((INFO.getCompletedCount(Terran_Factory, S) + INFO.getDestroyedCount(Terran_Factory, S)) < 6
			&& (INFO.getCompletedCount(Terran_Dropship, S) + INFO.getDestroyedCount(Terran_Dropship, S)) < 3)
	{
		Position mainBase = Positions::Unknown;
		Position mySCP = INFO.getSecondChokePosition(S);
		Position enemySCP = INFO.getSecondChokePosition(E);
		Position center = theMap.Center();
		Position firstWaitLine = INFO.getFirstWaitLinePosition();
		UnitInfo *firstTank = TM.getFrontTankFromPos(SM.getMainAttackPosition());

		if (INFO.getMainBaseLocation(S) != nullptr)
			mainBase = INFO.getMainBaseLocation(S)->getPosition();

		if (firstTank != nullptr && mainBase != Positions::Unknown && firstWaitLine != Positions::Unknown && mySCP.isValid() && enemySCP.isValid())
		{
			int distanceFirstTank = 0;
			int distanceFirstWaitLine = 0;
			bool leftAndRight = false;

			if (abs(mySCP.x - enemySCP.x) > abs(mySCP.y - enemySCP.y))
				leftAndRight = true;

			theMap.GetPath(mainBase, firstTank->pos(), &distanceFirstTank);
			theMap.GetPath(mainBase, firstWaitLine, &distanceFirstWaitLine);

			if (!centerIsMyArea)
			{
				centerIsMyArea = (leftAndRight ? abs(center.x - firstTank->pos().x) < 8 * TILE_SIZE
								  : abs(center.y - firstTank->pos().y) < 8 * TILE_SIZE)
								 && distanceFirstTank > distanceFirstWaitLine + 6 * TILE_SIZE;
			}
			else
			{
				if (distanceFirstTank <= distanceFirstWaitLine + 6 * TILE_SIZE)
					centerIsMyArea = false;
			}
		}

		if (!centerIsMyArea)
			return false;
		else
			return true;
	}

	int distance = 0;
	const CPPath &path = theMap.GetPath(firstTank->pos(), INFO.getFirstExpansionLocation(E)->getPosition(), &distance);

	if (path.empty())
	{
		// 중간 초크포인트가 없으면 25타일 밖에서 병력들 대기
		if (distance < 25 * TILE_SIZE)
		{
			needWait = true;
			//cout << "@@ empty, distance = " << distance / 32 << endl;
		}
	}
	else if (path.size() == 1)
	{
		// SecondChokePoint 랑 가까운 곳이면 여기를 기점으로 Stop
		auto chokePoint = path.begin();

		theMap.GetPath(firstTank->pos(), (Position)(*chokePoint)->Center(), &distance);

		if (distance < 25 * TILE_SIZE)
		{
			needWait = true;
			//cout << "@@ path 1, distance = " << distance / 32 << endl;
		}

	}
	else if (path.size() == 2)
	{
		// 좁은 초크포인트가 있고 상대 SecondChokePoint 랑 가까운 곳이면 여기를 기점으로 Stop
		const ChokePoint *secondChokePoint = nullptr;
		const ChokePoint *thirdChokePoint = nullptr;

		if ((*path.begin())->Center().getApproxDistance((WalkPosition)INFO.getFirstExpansionLocation(E)->getPosition())
				< (*path.rbegin())->Center().getApproxDistance((WalkPosition)INFO.getFirstExpansionLocation(E)->getPosition()))
		{
			secondChokePoint = (*path.begin());
			thirdChokePoint = (*path.rbegin());
		}
		else
		{
			secondChokePoint = (*path.rbegin());
			thirdChokePoint = (*path.begin());
		}

		//cout << "@@ distance = " << Position(secondChokePoint->Center()).getApproxDistance((Position)thirdChokePoint->Center()) << endl;

		if (Position(secondChokePoint->Center()).getApproxDistance((Position)thirdChokePoint->Center()) < 20 * TILE_SIZE)
		{
			Position start = (Position)(*thirdChokePoint->Geometry().begin());
			Position end = (Position)(*thirdChokePoint->Geometry().rbegin());

			if (start.getApproxDistance(end) < 4 * TILE_SIZE)
			{
				theMap.GetPath(firstTank->pos(), (Position)thirdChokePoint->Center(), &distance);

				if (distance < 13 * TILE_SIZE)
				{
					needWait = true;
					//cout << "@@ path 2, distanceFT = " << distance / 32 << ", sec = " << (TilePosition)secondChokePoint->Center() << ", third = " << (TilePosition)thirdChokePoint->Center() << endl;
				}
			}
		}
	}
	else if (path.size() == 3)
	{
		const ChokePoint *closestChokePoint = nullptr;

		closestChokePoint = (*path.begin());

		if (INFO.getSecondChokePoint(E) != nullptr
				&& Position(closestChokePoint->Center()).getApproxDistance(INFO.getSecondChokePosition(E)) < 30 * TILE_SIZE)
		{
			//cout << "@@ path 3 이상, distance = " << Position(closestChokePoint->Center()).getApproxDistance(INFO.getSecondChokePosition(E)) / 32 << endl;

			Position start = (Position)(*closestChokePoint->Geometry().begin());
			Position end = (Position)(*closestChokePoint->Geometry().rbegin());

			if (start.getApproxDistance(end) < 4 * TILE_SIZE)
			{
				theMap.GetPath(firstTank->pos(), (Position)closestChokePoint->Center(), &distance);

				if (distance < 13 * TILE_SIZE)
				{
					needWait = true;
					//cout << "@@ path 3, distanceFT = " << distance / 32 << ", sec = " << (TilePosition)closestChokePoint->Center() << endl;
				}
			}
		}
	}
	else
	{
		// path 4 이상..
	}

	// 상대방 다른 멀티 다 부시기 전까지 안들어가고 기다릴꺼야, 나는 3멀티 이상
	if (needWait)
	{
		if (INFO.getOccupiedBaseLocations(E).size() > 2)
		{
			cout << "***** 더이상 들어가면 안돼!!! *****" << endl;
			surround = true;

			if (secondAttackPosition != Positions::Unknown && multiBreakDeathPosition == Positions::Unknown)
			{
				needAttackMulti = true;
			}
			else
			{
				needAttackMulti = false;
			}

			return true;
		}
	}
	else
	{
		surround = false;
	}

	needAttackMulti = false;

	return false;
}

bool StrategyManager::moreUnits(UnitInfo *firstTank)
{
	int myVultureCountInLine = 0;
	int myTankCountInLine = 0;
	int myGoliathCountInLine = 0;

	for (auto g : INFO.getTypeUnitsInRadius(Terran_Goliath, S, waitLinePosition, 30 * TILE_SIZE, true))
		myVultureCountInLine++;

	// SiegeLineState만
	for (auto t : INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, waitLinePosition, 30 * TILE_SIZE, true))
		if (t->getState() == "SiegeLineState")
			myTankCountInLine++;

	for (auto g : INFO.getTypeUnitsInRadius(Terran_Goliath, S, waitLinePosition, 30 * TILE_SIZE, true))
		if (g->getState() == "VsTerran" || g->getState() == "ProtectTank")
			myGoliathCountInLine++;

	int enemyVultureCountInLine = INFO.getTypeUnitsInRadius(Terran_Vulture, E, drawLinePosition, 30 * TILE_SIZE, true).size();
	int enemyTankCountInLine = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, drawLinePosition, 30 * TILE_SIZE, true).size();
	int enemyGoliathCountInLine = INFO.getTypeUnitsInRadius(Terran_Goliath, E, drawLinePosition, 30 * TILE_SIZE, true).size();
	int enemyWraithCountInLine = INFO.getTypeUnitsInRadius(Terran_Wraith, E, drawLinePosition, 30 * TILE_SIZE, true).size();

	// 상대 레이스 있는데 내 골리앗 없으면 Wait
	if (enemyWraithCountInLine && !myGoliathCountInLine)
	{
		cout << "[" << TIME << "]" << "=== 레이스 잡을 골리앗 없어서 ㅠ Wait - " << myTankCountInLine << ":" << enemyTankCountInLine << endl;

		return false;
	}

	// 내가 지상 공2업이 먼저 됐고, 상대가 2업 전이라면 내 탱크에 가중치 부여
	double higherUpgrade = 1.0;

	if (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) >= 2 && E->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) < 2)
		higherUpgrade = 1.2;

	// 라인 기준 상대 탱크가 나보다 더 많으면 Wait
	if ((int)(myTankCountInLine * higherUpgrade) < (int)(enemyTankCountInLine * 1.3))
	{
		if ((myTankCountInLine * higherUpgrade) + (myGoliathCountInLine * 0.5) + (myVultureCountInLine * 0.3)
				< (enemyTankCountInLine * 1.3) + (enemyVultureCountInLine * 0.3) + (enemyGoliathCountInLine * 0.5))
		{
			// 조합된 유닛을 비교해서 그래도 내가 부족하면 Wait
			cout << "[" << TIME << "]" << "=== 상대 탱크가 많아서 Wait - " << myTankCountInLine << ":" << enemyTankCountInLine << endl;

			return false;
		}
	}
	else
	{
		// 내가 많아도 내 탱크가 너무 적을땐 Wait
		if (myTankCountInLine < 3)
		{
			cout << "[" << TIME << "]" << "=== 내 탱크가 너무 적어서 Wait - " << myTankCountInLine << ":" << enemyTankCountInLine << endl;

			return false;
		}
	}

	//// 첫번째 탱크 기준으로 적과 나의 탱크수 비교
	//int myTankCountAroundTheFirstTank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S, firstTank->pos(), 30 * TILE_SIZE, true).size();
	//int enemyTankCountAroundTheFirstTank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, firstTank->pos(), 30 * TILE_SIZE, true).size();

	//if (myTankCountAroundTheFirstTank < enemyTankCountAroundTheFirstTank)
	//{
	//	cout << "[" << TIME << "]" << "=== 첫번째 탱크 기준 상대 탱크가 많아서 Wait - " << myTankCountAroundTheFirstTank << ":" << enemyTankCountAroundTheFirstTank << endl;

	//	return false;
	//}

	return true;
}

void StrategyManager::checkSecondExpansion()
{
	if (keepMultiDeathPosition != Positions::Unknown)
		bw->drawCircleMap(keepMultiDeathPosition, 15 * TILE_SIZE, Colors::Cyan);

	if (TIME % 24 != 0)
		return;

	/// 2번째 멀티 먹어야 할 타이밍인지 확인
	if (INFO.getAllCount(Terran_Command_Center, S) < 2)
		return;

	if (!S->hasResearched(TechTypes::Tank_Siege_Mode))
		return;

	if (INFO.getSecondExpansionLocation(S) == nullptr)
		return;

	if (!INFO.getFirstWaitLinePosition().isValid())
		return;

	int myUnitCount = INFO.getUnitsInRadius(S, INFO.getFirstWaitLinePosition(), 20 * TILE_SIZE, true, true).size();
	int enemyUnitCount = INFO.getUnitsInRadius(E, INFO.getFirstWaitLinePosition(), 20 * TILE_SIZE, true, true).size();

	// FirstWaitLinePosition 주변에 적 유닛들이 나보다 많으면 안먹어
	if (enemyUnitCount > 0 && myUnitCount <= enemyUnitCount)
		return;

	// FirstWaitLinePosition 주변에 적 유닛들이 없으면
	if (INFO.enemyRace == Races::Terran)
	{
		if (keepMultiDeathPosition != Positions::Unknown)
		{
			int eVulture = INFO.getTypeUnitsInRadius(Terran_Vulture, E, keepMultiDeathPosition, 10 * TILE_SIZE, true).size();
			int eTank = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, E, keepMultiDeathPosition, 15 * TILE_SIZE, true).size();
			int eWraith = INFO.getTypeUnitsInRadius(Terran_Wraith, E, keepMultiDeathPosition, 10 * TILE_SIZE, true).size();
			int eGoliath = INFO.getTypeUnitsInRadius(Terran_Goliath, E, keepMultiDeathPosition, 10 * TILE_SIZE, true).size();

			int enemyPoint = eTank * 10 + eVulture * 3 + eGoliath * 5 + eWraith * 3;

			if (enemyPoint >= 30) {
				needSecondExpansion = false;
				return;
			}

			keepMultiDeathPosition = Positions::Unknown;
		}

		if (needSecondExpansion == false && S->minerals() > 600 && INFO.getAllCount(Terran_Factory, S) >= 4
				&& GM.getUsableGoliathCnt() > 3 && TM.getUsableTankCnt() > 5)
		{
			cout << "2멀티먹자아아아아" << endl;
			needSecondExpansion = true;
		}
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		if (needSecondExpansion == false && S->minerals() > 600
				&& (INFO.getCompletedCount(Terran_Goliath, S) + INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S)) > 10)
		{
			cout << "2멀티먹자아아아아" << endl;
			needSecondExpansion = true;
		}
	}
	else
	{
		if (ESM.getEnemyMainBuild() == Toss_fast_carrier)
		{
			if (needSecondExpansion == false && S->minerals() > 600 && INFO.getCompletedCount(Terran_Factory, S) >= 4
					&& INFO.getCompletedCount(Terran_Goliath, S) > 4)
			{
				cout << "2멀티먹자아아아아" << endl;
				needSecondExpansion = true;
			}
		}
	}

	if (needSecondExpansion == false && INFO.getMainBaseLocation(S) != nullptr
			&& INFO.getAverageMineral(INFO.getMainBaseLocation(S)->getPosition()) < 450)
	{
		if (INFO.enemyRace == Races::Protoss && SM.getMainStrategy() != AttackAll)
			return;

		cout << "2멀티먹자아아아아" << endl;
		needSecondExpansion = true;
	}

	// 내 마인 제거
	if (needSecondExpansion)
	{
		killMine(INFO.getSecondExpansionLocation(S));
	}
}

void StrategyManager::checkThirdExpansion()
{
	if (TIME % 24 != 0)
		return;

	/// 3번째 멀티 먹어야 할 타이밍인지 확인
	if (INFO.getCompletedCount(Terran_Command_Center, S) < 3)
		return;

	if (INFO.getThirdExpansionLocation(S) == nullptr)
		return;

	if (!INFO.getThirdExpansionLocation(S)->Center().isValid())
		return;

	if (INFO.enemyRace == Races::Terran)
	{
		int myUnitCountInFirstWaitLine = INFO.getUnitsInRadius(S, INFO.getFirstWaitLinePosition(), 20 * TILE_SIZE, true, true).size();
		int enemyUnitCountInFirstWaitLine = INFO.getUnitsInRadius(E, INFO.getFirstWaitLinePosition(), 20 * TILE_SIZE, true, true).size();

		// FirstWaitLinePosition 주변에 적 유닛들이 나보다 많으면 안먹어
		if (enemyUnitCountInFirstWaitLine > 0 && myUnitCountInFirstWaitLine <= enemyUnitCountInFirstWaitLine)
			return;

		UnitInfo *firstTank = TM.getFrontTankFromPos(getMainAttackPosition());
		int firstTankDistanceFromEnemySCP = 0;

		if (firstTank != nullptr && INFO.getSecondChokePosition(E).isValid())
		{
			firstTankDistanceFromEnemySCP = getGroundDistance(firstTank->pos(), INFO.getSecondChokePosition(E));
		}

		// 지상 메카닉 공2업 완료되었거나 센터를 장악했고 병력 여유가 있을 때 3멀티
		if (needThirdExpansion == false && S->minerals() > 600
				&& (S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) >= 2 || centerIsMyArea || surround
					|| firstTankDistanceFromEnemySCP < 30 * TILE_SIZE)
				&& GM.getUsableGoliathCnt() > 4 && TM.getUsableTankCnt() > 7)
		{

			cout << "3멀티먹자아아아아" << endl;
			needThirdExpansion = true;
		}
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		if (needThirdExpansion == false && S->minerals() > 600
				&& (INFO.getCompletedCount(Terran_Vulture, S)
					+ INFO.getCompletedCount(Terran_Goliath, S)
					+ INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S)) > 20)
		{
			cout << "3멀티먹자아아아아" << endl;
			needThirdExpansion = true;
		}
	}
	else
	{
		if (needThirdExpansion == false && INFO.getCompletedCount(Terran_Command_Center, S) >= 3)
		{
			cout << "3멀티먹자아아아아" << endl;
			needThirdExpansion = true;
		}
	}

	if (needSecondExpansion == false && INFO.getFirstExpansionLocation(S) != nullptr
			&& INFO.getAverageMineral(INFO.getFirstExpansionLocation(S)->getPosition()) < 450)
	{
		cout << "3멀티먹자아아아아" << endl;
		needThirdExpansion = true;
	}

	// 내 마인 제거
	if (needThirdExpansion)
	{
		killMine(INFO.getThirdExpansionLocation(S));
	}
}

bool StrategyManager::needAdditionalExpansion()
{
	if (INFO.getOccupiedBaseLocations(S).size() < 4)
		return false;

	// 10초에 한번씩 멀티 예상 지점에 마인 체크
	if (TIME % (24 * 10) != 0)
		return false;

	Base *firstMulti = INFO.getFirstMulti(S);
	Base *firstGasMulti = INFO.getFirstMulti(S, true);

	// 더이상 멀티 먹을 곳이 없다면
	if (firstGasMulti == nullptr && firstMulti == nullptr)
		return false;

	// 멀티 후보 1, 2 지역까지 내 마인 제거
	for (int i = 1; i < 3; i++)
	{
		Base *multi = INFO.getFirstMulti(S, false, false, i);

		if (multi == nullptr)
			return false;

		killMine(multi);
	}

	if (TIME % (24 * 60) != 0)
		return false;

	if (INFO.enemyRace == Races::Terran)
	{
		if (surround || centerIsMyArea)
		{
			// 2분에 1번씩 멀티 추가
			if (TIME % (24 * 60 * 2) != 0)
				return true;
		}
		else
		{
			// 센터 점령 못했으면 그래도 3분에 1번씩은 멀티 던져보기
			if (TIME % (24 * 60 * 3) != 0)
				return true;
		}
	}
	else
	{
		// vsZ, vsP 자원 채취하는 곳이 4곳 보다 적어질 경우 멀티 추가
		if (INFO.getActivationMineralBaseCount() < 4)
		{
			// 4분에 1번씩
			if (TIME % (24 * 60 * 2 * 2) != 0)
				return true;
		}
	}

	return false;
}

void StrategyManager::checkKeepExpansion()
{
	needKeepMultiSecond = false;
	needKeepMultiThird = false;

	needKeepMultiSecondAir = false;
	needKeepMultiThirdAir = false;

	needMineSecond = false;
	needMineThird = false;

	if (INFO.getSecondExpansionLocation(S) && INFO.getSecondExpansionLocation(S)->GetOccupiedInfo() == myBase) // 내 베이스
	{
		if (INFO.getUnitsInRadius(E, INFO.getSecondExpansionLocation(S)->Center(), 12 * TILE_SIZE, true, false, true, true).size())
			needKeepMultiSecond = true;

		if (INFO.getUnitsInRadius(E, INFO.getSecondExpansionLocation(S)->Center(), 12 * TILE_SIZE, false, true, true, true).size())
			needKeepMultiSecondAir = true;

		if (INFO.getSecondExpansionLocation(S)->GetMineCount() < 8)
			needMineSecond = true;

		if (INFO.getSecondExpansionLocation(S)->GetMineCount() > 11)
			needMineSecond = false;
	}

	if (INFO.getThirdExpansionLocation(S) && INFO.getThirdExpansionLocation(S)->GetOccupiedInfo() == myBase) // 내 베이스
	{
		if (INFO.getUnitsInRadius(E, INFO.getThirdExpansionLocation(S)->Center(), 12 * TILE_SIZE, true, false, true, true).size())
			needKeepMultiThird = true;

		if (INFO.getUnitsInRadius(E, INFO.getThirdExpansionLocation(S)->Center(), 12 * TILE_SIZE, false, true, true, true).size())
			needKeepMultiThirdAir = true;

		if (INFO.getThirdExpansionLocation(S)->GetMineCount() < 8)
			needMineThird = true;

		if (INFO.getThirdExpansionLocation(S)->GetMineCount() > 11)
			needMineThird = false;
	}
}

void StrategyManager::killMine(Base *base)
{
	if (base == nullptr)
		return;

	uList mine = INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, base->getPosition(), 5 * TILE_SIZE);

	if (mine.empty())
		return;

	for (auto m : mine)
	{
		Position newPos = m->pos();
		// 커맨드 센터
		Position c = (Position)base->getTilePosition();
		Position cr = c + (Position)Terran_Command_Center.tileSize();

		if (newPos.x >= c.x - 16 && newPos.y >= c.y - 16
				&& newPos.x <= cr.x + 16 && newPos.y <= cr.y + 16)
		{
			if (m->getKillMe() != MustKill && m->getKillMe() != MustKillAssigned)
			{
				m->setKillMe(MustKill);
				continue;
			}
		}

		// 컴셋
		Position cs = Position(cr.x, c.y + 32);
		Position csr = cs + (Position)Terran_Comsat_Station.tileSize();

		if (newPos.x >= cs.x - 16 && newPos.y >= cs.y - 16
				&& newPos.x <= csr.x + 16 && newPos.y <= csr.y + 16)
		{
			if (m->getKillMe() != MustKill && m->getKillMe() != MustKillAssigned)
			{
				m->setKillMe(MustKill);
				continue;
			}
		}
	}
}

// [TO DO] BuildQueue의 아이팀이 DeadLock 이 걸렸을때,
// 이곳에서 해당 UnitType을 만들지 말지 결정.
bool MyBot::StrategyManager::decideToBuildWhenDeadLock(UnitType *)
{
	return true;
}

MyBuildType StrategyManager::getMyBuild()
{
	return myBuild;
}

bool StrategyManager::getNeedUpgrade()
{
	return needUpgrade;
}

bool StrategyManager::getNeedTank()
{
	return needTank;
}

Position StrategyManager::getMainAttackPosition()
{
	return mainAttackPosition;
}

Position StrategyManager::getSecondAttackPosition()
{
	return secondAttackPosition;
}

Position StrategyManager::getDrawLinePosition()
{
	return drawLinePosition;
}

Position StrategyManager::getWaitLinePosition()
{
	return waitLinePosition;
}

void StrategyManager::decideDropship()
{
	if (INFO.enemyRace == Races::Terran)
	{
		

		if (INFO.getCompletedCount(Terran_Dropship, S) >= 3 && TM.enoughTankForDrop() && GM.enoughGoliathForDrop()) //DrawLine tank > 8, gol > 6  && INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 12 && INFO.getCompletedCount(Terran_Goliath, S) > 8)
		{
			dropshipMode = true;
		}
		else
		{
			dropshipMode = false;
		}
	}
	else if (INFO.enemyRace == Races::Protoss)
	{


		if (INFO.getCompletedCount(Terran_Dropship, S) >= 1 && TM.enoughTankForDrop()) //DrawLine tank > 8, gol > 6  && INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) > 12 && INFO.getCompletedCount(Terran_Goliath, S) > 8)
		{
			dropshipMode = true;
		}
		else
		{
			dropshipMode = false;
		}
	}
}

bool StrategyManager::checkTurretFirst() {

	// 5분 이내에는 터렛을 먼저 짓는다.
	if (INFO.enemyRace == Races::Protoss)
		if (TIME < 5 * 24 * 60) {
			if (CM.existConstructionQueueItem(Terran_Missile_Turret) ||
					BM.buildQueue.existItem(Terran_Missile_Turret))

				// AvailableMineral에 이미 터렛 값이 저장되어 있을텐데..
				if (TrainManager::Instance().getAvailableMinerals() < 60) {
					return true;
				}
		}

	return false;

}

// 초반에 SCV랑 함께 수비하러 가는 조건으로 시즈업 되기전의 탱크가 전진했을때
void StrategyManager::checkNeedDefenceWithScv()
{
	needDefenceWithScv = false;

	if (S->hasResearched(TechTypes::Tank_Siege_Mode) || INFO.hasRearched(TechTypes::Spider_Mines))
		return;

	vector<UnitType> types = { Terran_Siege_Tank_Tank_Mode, Terran_Goliath };
	UnitInfo *myFrontUnit = INFO.getClosestTypeUnit(S, mainAttackPosition, types, 0, false, true);

	if (myFrontUnit == nullptr)
		return;

	// 내 본진에 있는 경우는 일단 돌아와 수비 실패임.
	if (!isSameArea(myFrontUnit->pos(), MYBASE))
	{
		if (INFO.getClosestTypeUnit(E, myFrontUnit->pos(), Terran_Siege_Tank_Tank_Mode, 15 * TILE_SIZE, true) == nullptr)
			return;

		if (INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, S) + INFO.getCompletedCount(Terran_Goliath, S) <=
				INFO.getCompletedCount(Terran_Vulture, E) / 2 + INFO.getCompletedCount(Terran_Siege_Tank_Tank_Mode, E) + INFO.getCompletedCount(Terran_Goliath, E))
			needDefenceWithScv = true;
	}
}