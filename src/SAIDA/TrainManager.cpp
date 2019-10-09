#include "TrainManager.h"
#include "EnemyStrategyManager.h"

using namespace MyBot;

TrainManager::TrainManager()
{
	reservedMinerals = 0;
	reservedGas = 0;
	saveGas = false;
	waitToProduce = false;
	nextVessleTime = 0;
}

TrainManager &TrainManager::Instance()
{
	static TrainManager instance;
	return instance;
}

void TrainManager::update()
{
	if (TIME % 4 != 0)
		return;

	// �ʹ� Upgrade �켱 ������ �α� ���� ����
	if (SM.getNeedUpgrade())
	{
		return;
	}

	if (S->supplyUsed() >= 32 && INFO.getAllCount(Terran_Factory, S) == 0 &&
			(EIB != Zerg_4_Drone && EIB != Zerg_5_Drone && EIB != Zerg_sunken_rush && EIB != Toss_cannon_rush && EIB != Terran_bunker_rush && ESM.getEnemyGasRushRefinery() == nullptr))
	{
		return;
	}

	reservedMinerals = 0;
	reservedGas = 0;
	waitToProduce = false;

	try {
		factoryTraining();
	}
	catch (SAIDA_Exception e) {
		Logger::error("factoryTraining Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("factoryTraining Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("factoryTraining Unknown Error.\n");
		throw;
	}

	try {
		commandCenterTraining();
	}
	catch (SAIDA_Exception e) {
		Logger::error("commandCenterTraining Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("commandCenterTraining Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("commandCenterTraining Unknown Error.\n");
		throw;
	}

	try {
		barracksTraining();
	}
	catch (SAIDA_Exception e) {
		Logger::error("barracksTraining Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("barracksTraining Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("barracksTraining Unknown Error.\n");
		throw;
	}

	try {
		starportTraining();
	}
	catch (SAIDA_Exception e) {
		Logger::error("starportTraining Error. (ErrorCode : %x, Eip : %p)\n", e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("starportTraining Error. (Error : %s)\n", e.what());
		throw e;
	}
	catch (...) {
		Logger::error("starportTraining Unknown Error.\n");
		throw;
	}
}

bool TrainManager::hasEnoughResources(UnitType unitType)
{
	// ���� �������� �������� BuildQueue �� �߰��Ǿ� ����� �ڿ��� ���� ���꿡 ���� �ڿ��� ������ ��� ������ �ڿ��� ��
	return (unitType.mineralPrice() <= getAvailableMinerals()) && (unitType.gasPrice() <= getAvailableGas());
}

void TrainManager::addReserveResources(UnitType unitType)
{
	reservedMinerals += unitType.mineralPrice();
	reservedGas += unitType.gasPrice();
}

int TrainManager::getAvailableMinerals()
{
	return BM.getAvailableMinerals() - BM.getFirstItemMineral() - reservedMinerals;
}

int TrainManager::getAvailableGas()
{
	return BM.getAvailableGas() - BM.getFirstItemGas() - reservedGas;
}

const Base *TrainManager::getEscapeBase(UnitInfo *commandCenter) {

	const Base *base = nullptr;

	bool mainCommandExist = false;
	bool firstExpansionCommandExist = false;

	for (auto c : INFO.getBuildings(Terran_Command_Center, S))
	{
		if (c->id() == commandCenter->unit()->getID())
			continue;

		if (INFO.getMainBaseLocation(S) && c->unit()->getDistance(INFO.getMainBaseLocation(S)->getPosition()) < 5 * TILE_SIZE)
			mainCommandExist = true;
		else if (INFO.getFirstExpansionLocation(S) && c->unit()->getDistance(INFO.getFirstExpansionLocation(S)->getPosition()) < 5 * TILE_SIZE)
			firstExpansionCommandExist = true;
	}

	if (!mainCommandExist)
	{
		if (INFO.getMainBaseLocation(S) && INFO.isBaseSafe(INFO.getMainBaseLocation(S)) && INFO.isBaseHasResourses(INFO.getMainBaseLocation(S), 0, 0))
			base = INFO.getMainBaseLocation(S);
	}
	else if (!firstExpansionCommandExist)
	{
		if (INFO.getFirstExpansionLocation(S) && INFO.isBaseSafe(INFO.getFirstExpansionLocation(S)) && INFO.isBaseHasResourses(INFO.getFirstExpansionLocation(S), 0, 0))
			base = INFO.getFirstExpansionLocation(S);
	}
	else {
		base = INFO.getClosestSafeEmptyBase(commandCenter);
	}

	return base;
}

void TrainManager::commandCenterTraining()
{
	uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);

	if (commandCenterList.empty())
		return;

	// 9 Drone ���� �� ��������
	//	if (INFO.enemyRace == Races::Zerg && EIB <= Zerg_9_Drone &&
	//			INFO.getTypeUnitsInRadius(Zerg_Zergling, E, INFO.getMainBaseLocation(S)->getPosition(), 10 * TILE_SIZE).size())
	//	{
	//		return;
	//	}

	// �ʹ� �ϲ۷��� ���� ���� 1�� �����ϱ� ���� �ϲ� ���� �ߴ�
	if (needStopTrainToBarracks())
		return;

	if (INFO.enemyRace == Races::Protoss)
	{
		if (needStopTrainToFactory())
		{
			// cout << "@@@ ���丮 ������ �����ؾ��ؼ� �ϲ� ���� ���� ��" << endl;
			return;
		}
	}

	// SCV�� �ִ� 60�� �ѱ��� �ʴ´�. ���»����ؾߵ�.
	int curScvCnt = INFO.getAllCount(Terran_SCV, S);

	// �ʿ��� SCV Count ���.
	int maxScvNeedCount = getMaxScvNeedCount();

	// ���߿� ���ִ� Ŀ�ǵ尡 �ִ°�
	bool isExistLiftAndMoveCommandCenter = false;

	for (auto c : commandCenterList)
	{
		if (c->getState() == "LiftAndMove")
		{
			isExistLiftAndMoveCommandCenter = true;
			break;
		}
	}


	for (auto c : commandCenterList)
	{
		if (!c->isComplete())
			continue;

		string state = c->getState();

		// �츮 ���� Ŀ�ǵ弾�Ͱ� �ƴ϶�� �����Ȳ �Ǵ��Ͽ� ����
		if (state != "LiftAndMove") {
			// ���� ���̽��� ������ ��Ȳ�� �ȵȴٸ�
			if (c->isComplete() && isTimeToMoveCommandCenter(c))
			{
				// ���� �ִ� ��ġ�� �߰� ��Ƽ �����̾����� �� �߰� ��Ƽ ����Ʈ���� ���� ����
				Position pos = c->pos();

				auto targetBase = find_if(INFO.getAdditionalExpansions().begin(), INFO.getAdditionalExpansions().end(), [pos](Base * base) {
					return pos == base->getPosition();
				});

				if (targetBase != INFO.getAdditionalExpansions().end())
					INFO.removeAdditionalExpansionData(c->unit());

				const Base *b = getEscapeBase(c);

				if (b) {
					// addon�Ǽ����̰ų� ���� �Ʒ����̸� ����Ѵ�
					if (c->unit()->canCancelAddon())
						c->unit()->cancelAddon();

					for (word i = 0, size = c->unit()->getTrainingQueue().size(); i < size; i++) {
						if (c->unit()->canCancelTrain())
							c->unit()->cancelTrain();
					}

					// ������ ������ ���󰣴�
					c->setState(new CommandCenterLiftAndMoveState(getEscapeBase(c)->getTilePosition()));
					state = c->getState();
				}
			}
		}

		if (state == "New" || state == "Idle")
		{
			// �ո��� ������� ���� Ŀ�ǵ����� �Ǵ�
			if (c->isComplete() && c->unit()->getDistance(MYBASE) > 5 * TILE_SIZE && isSameArea(c->pos(), MYBASE)
					&& INFO.getCompletedCount(Terran_Command_Center, S) == 2) {


				if (INFO.getFirstExpansionLocation(S) && INFO.isTimeToMoveCommandCenterToFirstExpansion()) {
					c->setState(new CommandCenterLiftAndMoveState(INFO.getFirstExpansionLocation(S)->getTilePosition()));
					c->action();
				}
				else // �ϴ� ����.
				{
					if (!c->getLift())
						c->unit()->lift();
				}

				continue;
			}

			UnitType trainUnit = Terran_SCV;

			// �ڿ� ����
			if (!hasEnoughResources(trainUnit))
				continue;

			// ���� �� ���� ��ó�� �ְ� Factory�� �ȵ��� ������ SCV�� ���� ����...
			if (waitToProduce && INFO.enemyInMyArea().size())
				continue;

			// �Ļ������̼� ������ ������ ���º���
			else if (INFO.getCompletedCount(Terran_Academy, S) >= 1 && c->unit()->getAddon() == nullptr && c->unit()->canBuildAddon()
					 && hasEnoughResources(Terran_Comsat_Station) && isSafeComsatPosition(c)) {
				c->setState(new CommandCenterBuildAddonState());
				addReserveResources(Terran_Comsat_Station);
			}
			// SCV ���� �� ������ �̱�
			else if (curScvCnt <= maxScvNeedCount && c->unit()->canTrain())
			{
				if (INFO.getCompletedCount(Terran_Command_Center, S) > 2
						&& ScvManager::Instance().depotHasEnoughMineralWorkers(c->unit())) // �ո��� �̻��� ��
					continue;

				if (!SM.checkTurretFirst()) {
					c->setState(new CommandCenterTrainState(1));
					addReserveResources(trainUnit);
				}
			}
			// Ŀ�ǵ弾�� ��ó�� �ִ� �ڿ��� �� �������� Ŀ�ǵ弾�� �ű��.
			else {
				if (!INFO.isBaseHasResourses(INFO.getNearestBaseLocation(c->pos()), 0, 0) && INFO.getActivationMineralBaseCount() < 1
						&& !isExistLiftAndMoveCommandCenter && TIME % 24 * 30 == 0)
				{
					Base *multiBase = nullptr;

					for (int i = 1; i < 4; i++)
					{
						multiBase = INFO.getFirstMulti(S, false, false, i);

						if (multiBase != nullptr)
							break;
					}

					// ��Ƽ �� �� �ִ� ��ġ ������
					if (multiBase) {
						isExistLiftAndMoveCommandCenter = true;
						c->setState(new CommandCenterLiftAndMoveState(multiBase->getTilePosition()));
					}
				}
			}
		}

		c->action();
	}
}

void TrainManager::barracksTraining()
{
	// 9 Drone ���� �� ��������
	//if (INFO.enemyRace == Races::Zerg && EIB <= Zerg_9_Drone &&
	//		INFO.getCompletedCount(Terran_Factory, S) > 0 && INFO.getAllCount(Terran_Vulture, S) == 0)
	//	return;

	if (waitToProduce && INFO.enemyInMyArea().size())
		return;

	uList barracksList = INFO.getBuildings(Terran_Barracks, S);

	if (barracksList.empty())
		return;

	int marineCount = INFO.getAllCount(Terran_Marine, S);
	int liftThreshold = 0;

	// Marine ���� Threshold
	if (INFO.enemyRace == Races::Terran) {
		if ((EIB == Terran_bunker_rush || EIB == Terran_1b_forward || EIB == Terran_2b_forward) && EMB == UnknownMainBuild)
			liftThreshold = max(INFO.getCompletedCount(Terran_Marine, E) + 1, 3);
		else if (INFO.getCompletedCount(Terran_Marine, E) >= 3)
			liftThreshold = min(INFO.getCompletedCount(Terran_Marine, E) - 1, 3);
		else if (EIB == Terran_4scv && EMB == UnknownMainBuild)
			liftThreshold = 2;
		else
			liftThreshold = 1;
	}
	else if (INFO.enemyRace == Races::Zerg) {
		if (EIB <= Zerg_9_Drone || EMB == Zerg_main_hydra || EMB == Zerg_main_lurker)
			liftThreshold = 4;
		else if (EIB == Zerg_9_Hat)
			liftThreshold = 3;
		else if (EIB == Zerg_12_Pool)
			liftThreshold = 2;
		else if (EIB == Zerg_4_Drone_Real && EMB == UnknownMainBuild)
			liftThreshold = 2;
		else
			liftThreshold = 1;
	}
	else {

		if (EIB == Toss_forge)
			liftThreshold = 0;
		else if (EMB == Toss_1base_fast_zealot) {
			if ((INFO.getUnits(Terran_Vulture, S).size() + INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S).size()
					+ INFO.getUnits(Terran_Goliath, S).size() < 8))
				liftThreshold = 4;
			else
				liftThreshold = 2;

		}
		else if (EIB == Toss_2g_forward || EIB == Toss_1g_forward
				 || EMB == Toss_Scout || ESM.getEnemyGasRushRefinery() != nullptr) {

			if (INFO.getUnits(Terran_Vulture, S).size() + INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S).size()
					+ INFO.getUnits(Terran_Goliath, S).size() < 5)
				liftThreshold = 4;
			else
				liftThreshold = 2;
		}
		else if (EMB == Toss_dark || (EIB != Toss_cannon_rush && EMB == UnknownMainBuild))
		{
			if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0 && INFO.getAllCount(Terran_Engineering_Bay, S) > 0)
				liftThreshold = 4;
			else
				liftThreshold = 2;
		}
		else if (EIB <= Toss_2g_dragoon) {
			if ((INFO.getUnits(Terran_Vulture, S).size() + INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S).size()
					+ INFO.getUnits(Terran_Goliath, S).size() < 5)
					&& (EMB == Toss_range_up_dra_push || EMB == Toss_2gate_dra_push)
					&& (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) > 0))
				liftThreshold = 4;
			else
				liftThreshold = 2;

		}
		else
			liftThreshold = 2;

		// �ʹ� 1����Ʈ ù ��� Ÿ�̹� ���� ������ 1�� �̻� ���̸� �ּ� 3���� ����
		if (TIME < (24 * 60 * 5) && (INFO.getAllCount(Protoss_Zealot, E) + INFO.getDestroyedCount(Protoss_Zealot, E) >= 1))
			liftThreshold = max(liftThreshold, 3);
	}

	if (INFO.getUnits(Terran_Vulture, S).size() + INFO.getUnits(Terran_Goliath, S).size()
			+ INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S).size() > 8)
		liftThreshold = 0;

	for (auto b : barracksList)
	{
		string state = b->getState();

		// ���� ������� �跰�̶�� Idle �� ����
		if (state == "New" && b->isComplete()) {
			b->setState(new BarrackIdleState());
		}

		// Idle �϶� ��������
		if (state == "Idle")
		{
			UnitType trainUnit = Terran_Marine;

			// �ڿ� ����
			if (!hasEnoughResources(trainUnit))
				continue;

			// 2���� ~ 2����Ʈ ���̰� �ƴ� ��쿡�� 2��° ������ ���丮 ������ ����
			if (INFO.enemyRace == Races::Protoss &&
					(EIB != UnknownBuild && EIB >= Toss_pure_double))
			{
				if (marineCount && INFO.getAllCount(Terran_Command_Center, S) < 2)
					continue;
			}

			if (marineCount < liftThreshold)
			{
				if (INFO.enemyRace == Races::Protoss)
				{
					uList bunkerList = INFO.getTypeBuildingsInRadius(Terran_Bunker, S, INFO.getFirstExpansionLocation(S)->getPosition(), 10 * TILE_SIZE, false);
					uList barricadeBarrack;

					if (!bunkerList.empty())
						barricadeBarrack = INFO.getTypeBuildingsInRadius(Terran_Barracks, S, (*bunkerList.begin())->pos(), 6 * TILE_SIZE);

					// �ϼ��� ��Ŀ �ְ� ���� 2�� ����ư� �ٸ�����Ʈ �跰 ���ٸ�
					if ((EIB == Toss_1g_double || EIB == Toss_2g_dragoon || EMB == Toss_range_up_dra_push || EMB == Toss_2gate_dra_push)
							&& !bunkerList.empty() && barricadeBarrack.empty()
							&& marineCount >= 2 && liftThreshold != 3)
					{
						setBarricadeBarrack(barracksList);
						continue;
					}
				}

				if (needStopTrainToFactory())
				{
					// cout << "@@@ ������ �����ؾ��ؼ� �跰 ���� ���� ��" << endl;
					continue;
				}

				b->setState(new BarrackTrainState());
				b->action(trainUnit);
				addReserveResources(trainUnit);
				continue;
			}
			else
			{
				if (INFO.enemyRace == Races::Protoss)
				{
					uList bunkerList = INFO.getTypeBuildingsInRadius(Terran_Bunker, S, INFO.getFirstExpansionLocation(S)->getPosition(), 10 * TILE_SIZE, false);

					if ((EIB == Toss_1g_double || EIB == Toss_2g_dragoon || EMB == Toss_range_up_dra_push || EMB == Toss_2gate_dra_push)
							&& !bunkerList.empty())
					{
						// ��� Ǫ�÷��϶� �ϼ��� ��Ŀ ������ �ٸ�����Ʈ
						setBarricadeBarrack(barracksList);
					}
					else
					{
						// �ٸ� �����϶� �ո��� Ŀ�ǵ� �ö󰡰� ���� �ٸ�����Ʈ
						if (!INFO.getTypeBuildingsInRadius(Terran_Command_Center, S, INFO.getFirstExpansionLocation(S)->getPosition(), 5 * TILE_SIZE).empty())
							setBarricadeBarrack(barracksList);
					}

					if (SM.getMainStrategy() == AttackAll)
					{
						b->setState(new BarrackLiftAndMoveState());
					}
				}
				else if (INFO.enemyRace == Races::Terran)
				{
					if (INFO.getCompletedCount(Terran_Marine, S) + INFO.getDestroyedCount(Terran_Marine, S) > 0
							&& ((EIB >= Terran_1b_double && EIB <= Terran_pure_double)
								|| INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S)))
					{
						b->setState(new BarrackLiftAndMoveState());
					}
				}
				else //zerg�� ���
				{
					if (SM.getMainStrategy() == AttackAll) {
						if (INFO.getCompletedCount(Terran_Command_Center, S) > 1)
						{
							b->setState(new BarrackLiftAndMoveState());
						}
					}
				}
			}
		}

		// �ٸ����̵��ε�, ������ �ʿ��ϴٸ� Idle�� ��ü
		if (state == "Barricade") {
			//			cout << "@@ �ٸ�����Ʈ��. --- marineCount = " << marineCount << ", liftThreshold = " << liftThreshold << endl;

			if (!b->unit()->isLifted()/* && INFO.getUnitsInRadius(E, b->pos(), 10 * TILE_SIZE, true).size() <= 2*/) {
				if (marineCount < liftThreshold) {
					b->setState(new BarrackIdleState());
				}
			}
		}

		b->action();
	}
}

int TrainManager::getBaseVultureCount(InitialBuildType eib, MainBuildType emb) {
	if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
		{
			if (S->getUpgradeLevel(UpgradeTypes::Ion_Thrusters) && S->hasResearched(TechTypes::Spider_Mines))
				return S->supplyUsed() / 100 + 3;
			else if (EnemyStrategyManager::Instance().getEnemyInitialBuild() == Terran_2b_forward)
				return 3;
			else
				return 1;
		}
		else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
			return 3;
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		if (eib <= Zerg_9_Balup || emb == Zerg_main_zergling) {
			if (E->getUpgradeLevel(UpgradeTypes::Metabolic_Boost) || E->isUpgrading(UpgradeTypes::Metabolic_Boost))
				return 4;
			// ���ĸ� �ּ� 2���� ~ 4���� ���� �� ���¿� ���缭 �̴´�.
			else if (INFO.getCompletedCount(Zerg_Zergling, E) > INFO.getAllCount(Terran_Vulture, S) * 4)
				return 4;
		}


		return 2;
	}
	else
	{
		if (emb == Toss_fast_carrier || emb == Toss_arbiter_carrier)
			return 2;
		else
		{
			if (INFO.getCompletedCount(Terran_Factory, S) >= 2)
				return 4;
		}

		if (eib == Toss_1g_forward)
			return 2;
		else if (eib == Toss_2g_zealot || eib == Toss_2g_forward)
			return 4;

		if (eib == Toss_1g_double || eib == Toss_pure_double)
			return 1;

		// 3�� 15��(1����Ʈ ù ��� Ÿ�̹�) ���� ������ 2���� �̻� ���̸� ���� 1���� ���� �� �ֵ�� �߰��ϵ���
		if (TIME < (24 * 60 * 3 + 15) && (INFO.getAllCount(Protoss_Zealot, E) + INFO.getDestroyedCount(Protoss_Zealot, E) >= 2))
			return 1;
	}

	return 0;
}

int TrainManager::getMaxVultureCount() {
	if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
		{
			return min((getAvailableMinerals() / 500) * 6 + 1, 24);
		}
	}

	return 35;
}

void TrainManager::factoryTraining()
{
	uList factoryList = INFO.getBuildings(Terran_Factory, S);

	if (factoryList.empty())
		return;

	int machineShopCount = INFO.getAllCount(Terran_Machine_Shop, S);
	int vultureAllCount = INFO.getAllCount(Terran_Vulture, S) + INFO.getDestroyedCount(Terran_Vulture, S);
	int vultureCount = INFO.getAllCount(Terran_Vulture, S);
	int tankCount = INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S);
	int goliathCount = INFO.getAllCount(Terran_Goliath, S);
	int baseVultureCount = getBaseVultureCount(EIB, EMB);
	int baseTankCount = 0;
	int baseGoliathCount = 0;
	int maxVultureCount = getMaxVultureCount();
	int maxTankCount = 35;
	int maxGoliathCount = INT_MAX;
	int mashineShopCount = 0;
	int currentFrameSetAddOnCount = 0;

	// Tank ���� Threshold
	if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
			baseTankCount = 2;
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		if (EMB == Zerg_main_lurker)
			baseTankCount = 0;
		else if ( EMB == Zerg_main_hydra_mutal)
			baseTankCount = 3;
		else if (EMB == Zerg_main_hydra)
			baseTankCount = 5;
	}
	else
	{
		if (EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier) {
			baseTankCount = max(4, INFO.getCompletedCount(Protoss_Dragoon, E) / 2);
		}
	}

	// Goliath ���� Threshold
	if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
		{
			if (tankCount < 8)
				baseGoliathCount = 4;
			else
			{
				double ratio = 0.3;

				if (INFO.getAllCount(Terran_Wraith, E) + INFO.getAllCount(Terran_Battlecruiser, E))
					ratio = 0.6;

				baseGoliathCount = (int)(tankCount * ratio + 4);
			}

			// ��Ʋũ���� ������ �� �񸮾� ���� �� �߰� ����
			baseGoliathCount += INFO.getCompletedCount(Terran_Battlecruiser, E);
			maxGoliathCount = tankCount + INFO.getCompletedCount(Terran_Battlecruiser, E);
		}
		else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
		{
			if (INFO.getCompletedCount(Terran_Valkyrie, E) > 0 || INFO.getCompletedCount(Terran_Goliath, E) > 2)
				baseGoliathCount = 2;
			else
				baseGoliathCount = 0;
		}
	}
	else if (INFO.enemyRace == Races::Zerg)
	{
		baseGoliathCount = S->supplyUsed() < 200 ? 6 : 12;

		if (EMB == Zerg_main_queen_hydra)
			baseGoliathCount += INFO.getCompletedCount(Zerg_Mutalisk, E) / 2;
	}
	else
	{
		if (EMB == Toss_arbiter)
			baseGoliathCount = 2 + INFO.getCompletedCount(Protoss_Arbiter, E);
		else if (EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier) {
			int maxGcnt = EMB == Toss_fast_carrier ? 12 : 4;
			int carrierCnt = INFO.getCompletedCount(Protoss_Carrier, E);
			baseGoliathCount = max(maxGcnt, carrierCnt * (int)(S->getUpgradeLevel(UpgradeTypes::Terran_Vehicle_Weapons) >= 2 ? 3.5 : 5));

			if (INFO.getCompletedCount(Protoss_Arbiter, E) > 0) {
				baseGoliathCount = max(baseGoliathCount, 2 + INFO.getCompletedCount(Protoss_Arbiter, E));
			}

		}
		else if (EMB == Toss_dark_drop || EMB == Toss_drop)
			baseGoliathCount = baseGoliathCount > 2 ? baseGoliathCount : 2;

		if (INFO.getAllCount(Protoss_Scout, E) != 0) {
			baseGoliathCount = baseGoliathCount > INFO.getAllCount(Protoss_Scout, E) * 2 ? baseGoliathCount : INFO.getAllCount(Protoss_Scout, E) * 2;
		}
	}

	// �ӽż� ī��Ʈ
	if (INFO.enemyRace == Races::Zerg)
	{
		mashineShopCount = 1;

		if (EMB == Zerg_main_queen_hydra || (INFO.getAllCount(Zerg_Defiler, E) + INFO.getAllCount(Zerg_Defiler_Mound, E)))
			mashineShopCount = 2;
	}
	else if (INFO.enemyRace == Races::Terran)
	{
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
		{
			mashineShopCount = 1;

			if (INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) >= 2 && INFO.getAllCount(Terran_Goliath, S) >= 4)
				mashineShopCount = 2;

			// ��밡 ���� 3�ֵ�� ��ũ�� ��� ���缭 3�ֵ��
			if (INFO.getAllCount(Terran_Command_Center, S) == 2 && INFO.getAllCount(Terran_Machine_Shop, E) >= 3)
				mashineShopCount = 3;
		}
		else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
		{
			if (!SM.getNeedTank())
				mashineShopCount = 1;
			else
				mashineShopCount = 3;
		}
	}
	else
	{
		if (EMB == Toss_fast_carrier)
		{
			mashineShopCount = 1;
		}
		else
		{
			if (INFO.getCompletedCount(Terran_Factory, S) > 2 || vultureCount >= 4)
				mashineShopCount = 2;
			else
				mashineShopCount = 1;
		}
	}

	// �߰� ��Ƽ ����� �ӽż� �ϳ� �߰�
	if (INFO.getActivationGasBaseCount() >= 3)
	{
		if (INFO.enemyRace == Races::Zerg)
		{
			if (INFO.getAllCount(Zerg_Defiler, E) + INFO.getAllCount(Zerg_Defiler_Mound, E))
			{
				mashineShopCount += 1;
			}
		}
		else if (INFO.enemyRace == Races::Terran)
		{
			mashineShopCount = min(4, INFO.getActivationGasBaseCount());
		}
		else
			mashineShopCount += 1;
	}

	// Factory with Machinshop ���� ó�����ֱ� ����.
	int addonIndex = 0;

	for (auto f : factoryList)
	{
		if (f->unit()->getAddon())
		{
			string state = f->getState();

			// ���� ������� ���丮��� Idle �� ����
			if (state == "New" && f->isComplete()) {
				f->setState(new FactoryIdleState());
			}

			// �ּ� ���� ���� �� �ӽż� �߰�
			if (state == "Idle")
			{
				UnitType trainUnit = Terran_Vulture;

				// �ڿ�����, �ӽż��� 50/50 �̶� ���߿� ���� �ʿ��غ���
				// �ڿ� �����ϸ� �ϲ� ���� ����. // ���� �켱.
				if (!hasEnoughResources(trainUnit))
				{
					waitToProduce = true;
					return;
				}

				// �⺻ ���� ���� �����.
				if (vultureAllCount >= baseVultureCount)
				{
					// ���� ����
					if (INFO.enemyRace == Races::Zerg)
					{
						if (EMB == Zerg_main_lurker)
						{
							if (S->hasResearched(TechTypes::Spider_Mines) == false)
								trainUnit = Terran_Vulture;
							else
								trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : Terran_Vulture;
						}
						else if (EMB == Zerg_main_hydra || EMB == Zerg_main_hydra_mutal || EMB == Zerg_main_queen_hydra)
						{
							trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : Terran_Vulture;
						}
						else
						{
							if (INFO.getCompletedCount(Terran_Goliath, S) >= 8)
							{
								trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : Terran_Vulture;
							}
							else
							{
								if (INFO.getCompletedCount(Terran_Armory, S) == 0)
								{
									trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
								}
								else
								{
									trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : None;

									if (trainUnit == None)
									{
										// ���� �������� �񸮾� ������ ��� �̳׶� üũ�ϰ� ���� ����
										trainUnit = getAvailableGas() >= Terran_Goliath.gasPrice() ? None : hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
									}
								}
							}
						}

						if (baseTankCount > tankCount)
						{
							if (baseGoliathCount > goliathCount
									&& (EMB == Zerg_main_mutal || EMB == Zerg_main_hydra_mutal))
								trainUnit = Terran_Goliath;
							else
								trainUnit = Terran_Siege_Tank_Tank_Mode;
						}
						else
						{
							if (INFO.getCompletedCount(Terran_Armory, S) && baseGoliathCount > goliathCount)
								trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath :
											(getAvailableGas() >= Terran_Goliath.gasPrice() ? None :
											 hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None);
						}

						// ���� ���̺��ϱ� ���� ���� ����
						if (saveGas)
							trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
					}
					else if (INFO.enemyRace == Races::Terran)
					{
						if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
						{
							if (INFO.getCompletedCount(Terran_Armory, S) > 0 && baseGoliathCount > goliathCount && baseTankCount <= tankCount)
							{
								trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : None;
							}
							else
							{
								trainUnit = hasEnoughResources(Terran_Siege_Tank_Tank_Mode) ? Terran_Siege_Tank_Tank_Mode : None;
							}
						}
						else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
						{
							if (SM.getNeedTank())
							{
								trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : Terran_Vulture;

								// Tank ���� Gas�� ������ Mineral�� ������ ��� Tank ������ ���� ���� Factory���� Vulture ������ �ߴ��Ѵ�.
								if (trainUnit == Terran_Siege_Tank_Tank_Mode && getAvailableMinerals() < Terran_Siege_Tank_Tank_Mode.mineralPrice())
									return;
							}
							else
							{
								trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;;
							}
						}
					}
					else // �����佺
					{
						// Tank�� ��� Gas�� ������ ��츸 Vulture�� �����Ѵ�.
						//ĳ������ ���,
						if (EMB == Toss_fast_carrier || EMB == Toss_arbiter_carrier
								|| EMB == Toss_Scout || EMB == Toss_arbiter) {
							// ù��° �ֵ�¿����� Tank ���� �̴´�.
							if (((addonIndex == 0 && tankCount >= baseTankCount) || addonIndex != 0) && goliathCount < baseGoliathCount && INFO.getCompletedCount(Terran_Armory, S) > 0) {
								trainUnit = getAvailableGas() >= Terran_Goliath.gasPrice() ? Terran_Goliath : Terran_Vulture;
							}
							else {
								trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : Terran_Vulture;
							}
						}
						else if ((EIB == Toss_1g_dragoon || EIB == Toss_2g_dragoon ||
								  EIB == Toss_1g_double || EIB == UnknownBuild) && tankCount < 2)
						{
							// Tank ���� ���¸� Tank �̴´�.
							trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : None;
						}
						else if ((EIB == Toss_2g_zealot || EIB == Toss_1g_forward || EIB == Toss_2g_forward) && (EMB == Toss_1base_fast_zealot || EMB == UnknownMainBuild))
						{
							if (INFO.getAllCount(Protoss_Zealot, E) > INFO.getAllCount(Terran_Vulture, S) * 2 && INFO.getAllCount(Protoss_Dragoon, E) == 0 )
								trainUnit = Terran_Vulture;
							else
								trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : Terran_Vulture;
						}
						else {
							if (tankCount > 8 && tankCount * 0.7 > vultureCount) // ��ũ�� ������ ���� ����
								trainUnit = Terran_Vulture;
							else
								trainUnit = getAvailableGas() >= Terran_Siege_Tank_Tank_Mode.gasPrice() ? Terran_Siege_Tank_Tank_Mode : Terran_Vulture;
						}
					}
				}

				// �Ĺݿ� ��ũ�� �ʹ� �������°� �����ϱ� ���� �߰�
				if (trainUnit == Terran_Siege_Tank_Tank_Mode && maxTankCount <= INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S))
					trainUnit = Terran_Vulture;

				f->setState(new FactoryTrainState());
				f->action(trainUnit);
				addReserveResources(trainUnit);
				continue;
			}
			else if (state == "Train")
			{
				// ���� ���� ����Ǹ� Idle�� �ٲ�
				f->action();
			}
			else if (state == "BuildAddon")
			{
				f->action();
			}
			else
			{

			}

			addonIndex++;

		}

	}

	// AddOn �ٴµ� Delay�� �־ ���� üũ��.
	for (auto f : factoryList)
	{
		string state = f->getState();

		// ���� ������� ���丮��� Idle �� ����
		if (state == "BuildAddon") {
			currentFrameSetAddOnCount++;
		}
	}

	for (auto f : factoryList)
	{
		if (f->unit()->getAddon() == nullptr)
		{
			string state = f->getState();

			// ���� ������� ���丮��� Idle �� ����
			if (state == "New" && f->isComplete()) {
				f->setState(new FactoryIdleState());
			}

			// �ֵ�� �ڸ��� �� �ǹ��� ���� �� LiftAndLand ���·� ���丮�� ���� �ȴ�.
			if (state == "LiftAndLand")
			{
				if (f->unit()->isLifted() || !f->unit()->canLift())
				{
					f->action(moveFirstFactoryPos);
					continue;
				}
				else
				{
					f->setState(new FactoryIdleState());
					cout << "to idle f->unit()->getTilePosition() : " << f->unit()->getTilePosition() << endl;
				}
			}

			// �ּ� ���� ���� �� �ӽż� �߰�
			if (state == "Idle")
			{
				UnitType trainUnit = Terran_Vulture;

				// �ڿ�����, �ӽż��� 50/50 �̶� ���߿� ���� �ʿ��غ���
				// �ڿ� �����ϸ� �ϲ� ���� ����. // ���� �켱.
				if (!hasEnoughResources(trainUnit))
				{
					waitToProduce = true;
					return;
				}

				// �⺻ ���� ���� �����.
				if (vultureAllCount >= baseVultureCount)
				{
					if (machineShopCount + currentFrameSetAddOnCount < mashineShopCount)
					{
						bool needMachineShop = true;

						// �߾� ���۸� ���� �����ϰ� �ӽż� �߰�
						if (INFO.enemyRace == Races::Zerg)
						{
							if (EIB <= Zerg_9_Balup || EMB == Zerg_main_zergling)
							{
								if (INFO.getCompletedCount(Terran_Vulture, S) < baseVultureCount && INFO.getDestroyedCount(Zerg_Zergling, E) < 10)
									needMachineShop = false;
							}
						}
						else if (INFO.enemyRace == Races::Protoss)
						{
							if (EIB == Toss_1g_forward
									|| EIB == Toss_2g_zealot
									|| EIB == Toss_2g_forward)
							{
								if (INFO.getCompletedCount(Terran_Vulture, S) < baseVultureCount && INFO.getAllCount(Protoss_Cybernetics_Core, E) == 0)
									needMachineShop = false;
							}
						}

						// �ӽż� �߰�
						if (f->unit()->getAddon() == nullptr && hasEnoughResources(Terran_Machine_Shop) && needMachineShop)
						{
							if (!bw->canBuildHere(f->unit()->getTilePosition(), Terran_Machine_Shop, f->unit())
									&& !bw->getUnitsInRectangle((Position)(f->unit()->getTilePosition() + TilePosition(4, 1)), (Position)(f->unit()->getTilePosition() + TilePosition(6, 3)), Filter::IsBuilding).empty())
							{
								if (isFirstFactory())
								{
									if (f->getState() != "LiftAndLand")
									{
										findAndSaveFirstFactoryPos(f->unit());
										f->setState(new FactoryLiftAndLandState());
										f->action(moveFirstFactoryPos);
									}

									continue;
								}
							}
							else
							{
								f->setState(new FactoryBuildAddonState());
								currentFrameSetAddOnCount++;
								f->action();
								addReserveResources(Terran_Machine_Shop);
								continue;
							}
						}
					}

					// ���� ����
					if (INFO.enemyRace == Races::Zerg)
					{
						if (INFO.getCompletedCount(Terran_Armory, S) == 0)
						{
							trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
						}
						else
						{
							if (EMB == Zerg_main_queen_hydra)
							{
								if (goliathCount <= baseGoliathCount)
									trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : Terran_Vulture;
								else
									trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
							}
							else
							{
								trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : None;

								if (trainUnit == None)
								{
									// ���� �������� �񸮾� ������ ��� �̳׶� üũ�ϰ� ���� ����
									trainUnit = getAvailableGas() >= Terran_Goliath.gasPrice() ? None : hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
								}
							}
						}

						// ���� ���̺��ϱ� ���� ���� ����
						if (saveGas)
							trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
					}
					else if (INFO.enemyRace == Races::Terran)
					{
						if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
						{
							if (S->hasResearched(TechTypes::Spider_Mines))
							{
								// �Ĺݱ��� ���� �� ���� ����ϱ� ���� �߰�
								if (baseVultureCount > vultureCount)
								{
									trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
								}
								else
								{
									if (getAvailableGas() < 200)
										trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
									else
										trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
								}
							}
							else
							{
								trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : None;
							}

							// ���� ���̺��ϱ� ���� ���� ����
							if (saveGas)
								trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
						}
						else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
						{
							if (!SM.getNeedTank())
							{
								trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
							}
							else
							{
								trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
							}
						}
					}
					else // �����佺
					{
						// Tank�� ��� Gas�� ������ ��츸 Vulture�� �����Ѵ�.
						if (goliathCount < baseGoliathCount && INFO.getCompletedCount(Terran_Armory, S) > 0)
						{
							trainUnit = hasEnoughResources(Terran_Goliath) ? Terran_Goliath : None;

							// ���� �������� �񸮾� ������ ��� �̳׶� üũ�ϰ� ���� ����
							if (trainUnit == None)
								trainUnit = getAvailableGas() >= Terran_Goliath.gasPrice() ? None : hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
						}
						else
						{
							// �񸮾� �ʿ����� ���� ��� ���ĸ�
							trainUnit = hasEnoughResources(Terran_Vulture) ? Terran_Vulture : None;
						}
					}
				}

				if (INFO.enemyRace == Races::Terran) {
					// ������ �Ĺݿ� �������� ���� �ʹ� ���� ���ﵵ��
					if (trainUnit == Terran_Vulture && maxVultureCount <= INFO.getAllCount(Terran_Vulture, S)
							|| (S->hasResearched(TechTypes::Spider_Mines) && S->isUpgrading(UpgradeTypes::Ion_Thrusters) && maxVultureCount >= tankCount + goliathCount))
						continue;

					// �Ĺݿ� ��ũ���� �񸮾��� �� �������°� �����ϱ� ���� �߰�
					else if (trainUnit == Terran_Goliath && maxGoliathCount <= INFO.getAllCount(Terran_Goliath, S)
							 && S->getUpgradeLevel(UpgradeTypes::Charon_Boosters))
						continue;
				}

				f->setState(new FactoryTrainState());
				f->action(trainUnit);
				addReserveResources(trainUnit);
				continue;
			}
			else if (state == "Train")
			{
				// ���� ���� ����Ǹ� Idle�� �ٲ�
				f->action();
			}
			else if (state == "BuildAddon")
			{
				f->action();
			}
			else
			{

			}
		}
	}
}

void TrainManager::starportTraining()
{
	uList starportList = INFO.getBuildings(Terran_Starport, S);
	uList controlTowerList = INFO.getBuildings(Terran_Control_Tower, S);
	int wraithCount = INFO.getAllCount(Terran_Wraith, S);
	int dropshipCount = INFO.getAllCount(Terran_Dropship, S);
	int vesselCount = INFO.getAllCount(Terran_Science_Vessel, S);
	int maxVesselCount = 0;
	int baseWraithCount = 0;
	int maxWraithCount = 0;
	int maxDropshipCount = 0;
	int needConrolTowerCount = 0;

	int en_wraithCount = INFO.getAllCount(Terran_Wraith, E);

	if (starportList.empty())
		return;

	if (INFO.enemyRace == Races::Zerg)
	{
		maxVesselCount = 5;
	}
	else if (INFO.enemyRace == Races::Protoss)
	{
		maxVesselCount = 3;
	}
	else
	{
		maxVesselCount = 2;
	}

	if (INFO.enemyRace == Races::Terran) {
		if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
		{
			needConrolTowerCount = 1;
			maxDropshipCount = 3; //Dropship TEST
		}
		else if (SM.getMyBuild() == MyBuildTypes::Terran_VultureTankWraith)
		{
			if (INFO.getCompletedCount(Terran_Valkyrie, E) > 0 || INFO.getCompletedCount(Terran_Goliath, E) > 2)
			{
				baseWraithCount = 0;
				maxWraithCount = 0;
				needConrolTowerCount = 0;
			}
			else
			{
				// wraith ���� Threshold
				baseWraithCount = 1;
				// wraith �ִ� ����
				maxWraithCount = 3;
				// �ʿ��� ��Ʈ��Ÿ��
				needConrolTowerCount = (en_wraithCount > 0) ? 1 : 0;
			}
		}
	}
	else
	{
		needConrolTowerCount = 1;
	}

	if (SM.getMainStrategy() == Eliminate) {
		baseWraithCount = baseWraithCount > 1 ? baseWraithCount : 1;
		maxWraithCount = maxWraithCount > 3 ? maxWraithCount : 3;
	}

	for (auto s : starportList)
	{
		string state = s->getState();

		if (state == "New" && s->isComplete()) {
			s->setState(new StarportIdleState());
		}

		// �ּ� ���̽� ���� �� �ӽż� �߰�
		if (state == "Idle")
		{
			//if (wraithCount >= maxWraithCount && vesselCount >= maxVesselCount)
			//	continue;

			UnitType trainUnit = UnitTypes::None;

			// �⺻ ���̽� ���� �����.
			if (wraithCount < baseWraithCount)
			{
				trainUnit = Terran_Wraith;
			}
			else
			{
				if ((int)controlTowerList.size() < needConrolTowerCount)
				{
					// ��Ʈ��Ÿ�� �߰�
					if (s->unit()->getAddon() == nullptr && hasEnoughResources(Terran_Control_Tower))
					{
						s->setState(new StarportBuildAddonState());
						s->action();
					}

					continue;
				}

				if (INFO.enemyRace == Races::Terran)
				{
					if (SM.getMyBuild() == MyBuildTypes::Terran_TankGoliath)
					{
						if (dropshipCount < maxDropshipCount)
							trainUnit = hasEnoughResources(Terran_Dropship) ? Terran_Dropship : None;
						else
						{
							if (vesselCount < maxVesselCount)
								trainUnit = Terran_Science_Vessel;
						}
					}
				}
				else if (INFO.enemyRace == Races::Protoss)
				{
					if (vesselCount < maxVesselCount) {
						if (INFO.getCompletedCount(Terran_Science_Facility, S) == 0
								|| INFO.getCompletedCount(Terran_Starport, S) == 0
								|| INFO.getCompletedCount(Terran_Control_Tower, S) == 0)
						{
							continue;
						}

						// ĳ���� ����� �� ���� ���� �� �� ����
						if (EMB == Toss_fast_carrier && INFO.getActivationGasBaseCount() < 3)
							continue;

						trainUnit = Terran_Science_Vessel;
					}
				}
				else
				{
					if (vesselCount < maxVesselCount) {
						if (INFO.getCompletedCount(Terran_Science_Facility, S) == 0
								|| INFO.getCompletedCount(Terran_Starport, S) == 0
								|| INFO.getCompletedCount(Terran_Control_Tower, S) == 0)
						{
							continue;
						}

						trainUnit = Terran_Science_Vessel;
					}
				}
			}

			// �ڿ� ����
			if (trainUnit == Terran_Science_Vessel && nextVessleTime < TIME)
			{
				if (!hasEnoughResources(trainUnit) && trainUnit.gasPrice() > getAvailableGas())
				{
					saveGas = true;
					continue;
				}
			}

			// ���� ���� ����
			if (trainUnit == UnitTypes::None)
				continue;

			s->setState(new StarportTrainState());
			s->action(trainUnit);
			addReserveResources(trainUnit);

			if (trainUnit == Terran_Science_Vessel && saveGas)
			{
				saveGas = false;
				nextVessleTime = TIME + (24 * 60 * 2); // ���� ������� 2�� �ĺ���
				cout << "## ���� ������� 2�еڿ�.." << nextVessleTime << endl;
			}

			continue;
		}
		else if (state == "Train")
		{
			// ���� ���� ����Ǹ� Idle�� �ٲ�
			s->action();
		}
		else if (state == "BuildAddon")
		{
			s->action();
		}
		else
		{

		}
	}
}

int TrainManager::getMaxScvNeedCount()
{
	uList commandCenterList = INFO.getBuildings(Terran_Command_Center, S);
	int maxScvNeedCount = 0;

	for (auto c : commandCenterList)
	{
		// Command�� Mineral * 2 ��ū �߰�
		maxScvNeedCount += ScvManager::Instance().getDepotMineralSize(c->unit()) * 2;
		maxScvNeedCount += 3; // for Refinery
		maxScvNeedCount += 1; // for Spare(Build, Scout, Repair)

		// Command�� �ϼ����� �������� �ִٸ� �ϼ� �� Rebalancing�� ���� 10�� margine�� �߰�
		if (!c->isComplete())
		{
			maxScvNeedCount += 10;
			continue;
		}
	}

	return min(maxScvNeedCount, 60);
}
/*
Position TrainManager::getBarrackEnemyChokePosition()
{
	Position targetPos = Positions::None;

	if (INFO.getSecondChokePoint(E) == nullptr)
		targetPos = INFO.getMainBaseLocation(E)->Center();
	else
	{
		const ChokePoint *secondChokePoint = INFO.getSecondChokePoint(E);

		int dist1 = INFO.getMainBaseLocation(E)->Center().getApproxDistance((Position)secondChokePoint->Pos(BWEM::ChokePoint::end1));
		int dist2 = INFO.getMainBaseLocation(E)->Center().getApproxDistance((Position)secondChokePoint->Pos(BWEM::ChokePoint::end2));

		targetPos = dist1 > dist2 ? (Position)secondChokePoint->Pos(BWEM::ChokePoint::end1) : (Position)secondChokePoint->Pos(BWEM::ChokePoint::end2);

		Position tempPos = getCirclePosFromPosByDegree(targetPos, (Position)secondChokePoint->Center(), 180);

		if (!tempPos.isValid())
			tempPos.makeValid();

		if (targetPos.getApproxDistance(tempPos) > 3 * TILE_SIZE)
			targetPos = getMiddlePositionByDist(targetPos, tempPos, 3 * TILE_SIZE);
		else
			targetPos = tempPos;

		if (!targetPos.isValid())
			targetPos.makeValid();
	}

	return targetPos;
}

void TrainManager::setBarrackLiftState(UnitInfo *b)
{
	// ���ݹ�����
	if (b->unit()->isUnderAttack() || b->getVeryFrontEnemyUnit() != nullptr)
	{
		b->setState(new BarrackLiftAndMoveState());

		if (b->getVeryFrontEnemyUnit() != nullptr)
		{
			int safeDist = b->getVeryFrontEnemyUnit()->getType().airWeapon().maxRange() > TILE_SIZE ? b->getVeryFrontEnemyUnit()->getType().airWeapon().maxRange() / TILE_SIZE : TILE_SIZE;
			b->action(getBackPostion(b->pos(), b->getVeryFrontEnemyUnit()->getPosition(), safeDist + 1));
		}
		else
		{
			moveBackPostion(b, b->pos(), 2);
		}
	}
	else if (INFO.getSecondChokePosition(S).isValid() && (b->hp() * 100) / b->type().maxHitPoints() < 33)
	{
		b->setState(new BarrackLiftAndMoveState());
		b->action(INFO.getSecondChokePosition(S));
	}
	else if (TIME % 24 == 0)
	{
		if (INFO.enemyRace == Races::Terran)
		{
			if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion)
			{
				if (b->pos().getApproxDistance(INFO.getMainBaseLocation(E)->getPosition()) > 10)
				{
					b->setState(new BarrackLiftAndMoveState());
					// �� ���κ��̽� �ι�° ��ũ����Ʈ(���̶�� ���κ��̽�)�� �����Ϸ� ��
					b->action(getBarrackEnemyChokePosition());
				}
			}
			else
			{
				b->setState(new BarrackLiftAndMoveState());

				uList tankList = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);

				// ��ũ��ü�� ��� �����ǿ��� ��ũ ��ü ��� ���� 4Ÿ�� ������ �̵�
				if (!tankList.empty())
				{
					b->action(get1stUnitFrontPosition(tankList));
					// ���� ��ũ�� �������� ������ ���� �� �Լ��� ����
					//b->action(getAvgTankFrontPosition(tankList));
				}
				else
					b->action(getBarrackEnemyChokePosition());
			}

		}
		else if (INFO.enemyRace == Races::Protoss && SM.getMainStrategy() != WaitToBase && SM.getMainStrategy() != WaitToFirstExpansion)
		{
			b->setState(new BarrackLiftAndMoveState());
			uList tankList = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);

			// ���� ��ũ �����ǿ��� ��ũ ���� 4Ÿ�� ������ �̵�
			if (!tankList.empty())
			{
				b->action(get1stUnitFrontPosition(tankList));
			}
			else if (INFO.getSecondChokePosition(S).isValid())
				b->action(INFO.getSecondChokePosition(S));
		}
		else
		{
			if (INFO.getSecondChokePosition(S).isValid() && b->pos().getApproxDistance(INFO.getSecondChokePosition(S)) > 10)
			{
				b->setState(new BarrackLiftAndMoveState());
				b->action(INFO.getSecondChokePosition(S));
			}
		}

	}
}

void TrainManager::actionBarrackLiftAndMoveState(UnitInfo *b)
{

	if (b->unit()->isUnderAttack() || b->getVeryFrontEnemyUnit() != nullptr)
	{
		if (b->getVeryFrontEnemyUnit() != nullptr)
		{
			int safeDist = b->getVeryFrontEnemyUnit()->getType().airWeapon().maxRange() > TILE_SIZE ? b->getVeryFrontEnemyUnit()->getType().airWeapon().maxRange() / TILE_SIZE : TILE_SIZE;
			b->action(getBackPostion(b->pos(), b->getVeryFrontEnemyUnit()->getPosition(), safeDist + 1));
		}
		else
		{
			moveBackPostion(b, b->pos(), 2);
		}
	}
	else if ((b->hp() * 100) / b->type().maxHitPoints() < 33)
	{
		b->action(INFO.getSecondChokePosition(S));
	}
	// �ǹ� Ư�� ����Ʈ���� �̵�
	else if (TIME % 48 == 0)
	{
		if (INFO.enemyRace == Races::Terran)
		{
			if (SM.getMainStrategy() == WaitToBase || SM.getMainStrategy() == WaitToFirstExpansion)
			{
				// �� ���κ��̽� �����Ϸ� ��
				b->action(getBarrackEnemyChokePosition());
			}
			else
			{
				uList tankList = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);

				// ���� ��ũ �����ǿ��� ��ũ ���� 4Ÿ�� ������ �̵�
				if (!tankList.empty())
				{
					b->action(get1stUnitFrontPosition(tankList));
					// ���� ��ũ�� �������� ������ ���� �� �Լ��� ����
					//b->action(getAvgTankFrontPosition(tankList));
				}
				else
					b->action(getBarrackEnemyChokePosition());
			}

		}
		else if (INFO.enemyRace == Races::Protoss && SM.getMainStrategy() != WaitToBase && SM.getMainStrategy() != WaitToFirstExpansion)
		{
			uList tankList = INFO.getUnits(Terran_Siege_Tank_Tank_Mode, S);
			uList vultureList = INFO.getUnits(Terran_Vulture, S);
			uList goliathList = INFO.getUnits(Terran_Goliath, S);

			// ���� ������ ���� 4Ÿ�� ������ �̵�
			if (!tankList.empty())
				b->action(get1stUnitFrontPosition(tankList));
			else if (!vultureList.empty())
				b->action(get1stUnitFrontPosition(vultureList));
			else if (!goliathList.empty())
				b->action(get1stUnitFrontPosition(goliathList));
			else
				b->action(INFO.getSecondChokePosition(S));
		}
		else
			b->action(INFO.getSecondChokePosition(S));
	}
}


Position TrainManager::get1stUnitFrontPosition(uList unitList)
{
	int dist = INT_MAX;
	UnitInfo *firstUnit = nullptr;

	for (auto t : unitList)
	{
		int tempDist = INT_MAX;
		theMap.GetPath(t->pos(), SM.getMainAttackPosition(), &tempDist);

		if (tempDist >= 0 && dist > tempDist)
		{
			firstUnit = t;
			dist = tempDist;
		}
	}

	return targetPos;
}
*/
bool TrainManager::isFirstFactory()
{
	if (INFO.getTypeBuildingsInRadius(Terran_Factory, S, INFO.getMainBaseLocation(S)->getPosition(), 0, true, true).size() <= 1)
		return true;
	else
		return false;
}

void TrainManager::findAndSaveFirstFactoryPos(Unit factory)
{
	moveFirstFactoryPos = ReserveBuilding::Instance().getReservedPosition(Terran_Factory, factory, Terran_Machine_Shop);

	ReserveBuilding::Instance().assignTiles(moveFirstFactoryPos, Terran_Factory);
}

bool TrainManager::isTimeToMoveCommandCenter(UnitInfo *c)
{
	// Ŀ�ǵ� ���� hp�� 50% �̻��̸� ��ƾ��
	if (c->hp() < c->type().maxHitPoints() * 50 / 100)
	{
		// 50% ���϶� ���� ���� �°� ���� �ʴٸ� ��ƾ��
		if (c->getVeryFrontEnemyUnit() != nullptr || c->unit()->isUnderAttack())
			return true;
	}

	return false;
}

void TrainManager::setBarricadeBarrack(uList &bList)
{
	if (SM.getMainStrategy() != AttackAll && INFO.getFirstExpansionLocation(S))
	{
		for (auto b : bList) {
			if (b->getState() == "Idle") {
				b->setState(new BarrackBarricadeState());
				b->action();
				break;
			}
		}
	}
	else
	{
		for (auto b : bList) {
			if (b->getState() == "Barricade") {
				b->unit()->lift();
				b->setState(new BarrackIdleState());
				break;
			}
		}
	}

}

bool TrainManager::isSafeComsatPosition(UnitInfo *depot)
{
	// ����, �ո��� ���� ������ ��ġ�� Ŀ�ǵ弾�Ϳ� �Ļ� �߰� ��
	// �ش� ������ �������� �Ǵ� �� �Ļ� �߰�
	// (�ַ� ��ũ������ �Ļ��� ��� �ı��Ǵ� ���� �����ϱ� ���� �߰�)

	if (isSameArea(depot->pos(), (Position)MYBASE))
		return true;

	if (INFO.getFirstExpansionLocation(S) && isSameArea(depot->pos(), INFO.getFirstExpansionLocation(S)->getPosition()))
		return true;

	// �Ļ� Center ��ġ
	Position basePosition = (Position)(depot->unit()->getTilePosition() + TilePosition(5, 2));
	uList enemyList = INFO.getUnitsInRadius(E, basePosition, 15 * TILE_SIZE, true, true, true, true);

	// �Ļ� ��ġ�� �����ϸ� false
	if (getDamageAtPosition(basePosition, Terran_Comsat_Station, enemyList, false))
		return false;

	return true;
}

bool TrainManager::needStopTrainToFactory()
{
	bool stopTrain = false;
	int initialCombatUnitCreateCount = INFO.getAllCount(Terran_Vulture, S) + INFO.getDestroyedCount(Terran_Vulture, S)
									   + INFO.getAllCount(Terran_Siege_Tank_Tank_Mode, S) + INFO.getDestroyedCount(Terran_Siege_Tank_Tank_Mode, S);

	// �ʹ� ���丮 ���� ���� 2�� ������ �� ���� ���丮 �ִ��� ���� �ʵ��� ���� ���� ���� ���� �߰�
	if (initialCombatUnitCreateCount < 2)
	{
		for (auto f : INFO.getBuildings(Terran_Factory, S))
		{
			if (!f->isComplete())
				continue;

			// addon �� �ִ� ���
			if (f->unit()->getAddon()) {
				if (getAvailableMinerals() <= 150)
				{
					if (f->unit()->getAddon()->isCompleted()) {
						// ������ �Ȼ̰� �ְų� ���� �� �Ϸ� �Ǵ� ��� ���� ���� ����
						if (f->unit()->getTrainingQueue().empty() || f->unit()->isTraining() && f->unit()->getRemainingTrainTime() <= 130)
							stopTrain = true;
					}
					// addon �� �� �ϼ��Ǵ� ��� ���� ���� ����
					else if (!f->unit()->getAddon()->isCompleted() && f->unit()->getAddon()->getRemainingBuildTime() <= 130)
						stopTrain = true;
				}
			}
			// addon �� ���� ���
			else {
				if (getAvailableMinerals() <= 75)
				{
					// �������̰ų� ���׷��̵� ���� �ƴ� ���
					if (f->unit()->getTrainingQueue().empty())
						stopTrain = true;
					// ���� �Ϸ� ��
					else if (f->unit()->isTraining() && f->unit()->getRemainingTrainTime() <= 130)
						stopTrain = true;
				}
			}
		}
	}

	return stopTrain;
}

bool TrainManager::needStopTrainToBarracks()
{
	bool stopTrain = false;

	if (TIME < 24 * 60 * 5 && (EIB == Terran_4scv || EIB == Toss_4probes || EIB == Zerg_4_Drone_Real)
			&& INFO.getCompletedCount(Terran_Barracks, S) && INFO.getAllCount(Terran_Marine, S) < 1 && INFO.getAllCount(Terran_SCV, S) >= 2)
	{
		// cout << "@@@ �跰 ������ �����ؾ��ؼ� �ϲ� ���� ���� ��" << endl;
		stopTrain = true;
	}

	return stopTrain;
}

Position TrainManager::getBarricadePosition()
{
	if (barricadePosition == Positions::None) {

		Position eBase = (Position)INFO.getMainBaseLocation(E)->Center();
		Position secondChoke = (Position)INFO.getSecondChokePoint(S)->Pos(INFO.getSecondChokePoint(S)->end2);

		if (eBase.getApproxDistance(secondChoke) >
				eBase.getApproxDistance((Position)INFO.getSecondChokePoint(S)->Pos(INFO.getSecondChokePoint(S)->end1))) {
			secondChoke = (Position)INFO.getSecondChokePoint(S)->Pos(INFO.getSecondChokePoint(S)->end1);
		}

		// �ո���� ���� ��ũ����Ʈ�� �� ��� ( ex : ����Ƽ���̼�)
		const pair<const Area *, const Area *> &areas = INFO.getSecondChokePoint(S)->GetAreas();
		const vector<ChokePoint> &secondCP = areas.first->ChokePoints(areas.second);
		WalkPosition pos = WalkPositions::Origin;

		for (auto &cp : secondCP) {
			pos += cp.Center();
		}

		if (INFO.getFirstExpansionLocation(S)->getPosition().getApproxDistance((Position)INFO.getSecondChokePoint(S)->Center()) > 340)
			pos = (pos * 2 + (WalkPosition)INFO.getFirstExpansionLocation(S)->getPosition() * 2) / (secondCP.size() * 2 + 2);
		else
			pos = (WalkPosition)secondChoke;

		barricadePosition = (Position)pos;
	}

	return barricadePosition;

}

void TrainManager::setBarricadePosition(Position p)
{
	barricadePosition = p;
}