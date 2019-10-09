#include "CommandCenterState.h"
#include "../InformationManager.h"

using namespace MyBot;

State *CommandCenterTrainState::action()
{
	if (unit->isTraining())
		return nullptr;

	if (!count)
		return new CommandCenterIdleState();

	if (INFO.enemyRace == Races::Terran && S->supplyUsed() + Terran_SCV.supplyRequired() > 388 + (INFO.getAllCount(Terran_Dropship, S) * 4))
		return nullptr;

	if (bw->canMake(Terran_SCV, unit)) {
		if (unit->train(Terran_SCV))
			count--;
	}

	return nullptr;
}

State *CommandCenterBuildAddonState::action()
{
	if (unit->getAddon() && !unit->isTraining())
	{
		return new CommandCenterIdleState();
	}

	if (unit->canBuildAddon())
	{
		unit->buildAddon(Terran_Comsat_Station);
	}

	return nullptr;
}

MyBot::CommandCenterLiftAndMoveState::CommandCenterLiftAndMoveState(TilePosition p)
{
	if (p == TilePositions::None || p == TilePositions::Unknown || p == TilePositions::Invalid) {
		if (INFO.getFirstExpansionLocation(S) != nullptr)
			targetPosition = (TilePosition)INFO.getFirstExpansionLocation(S)->Center();
	}
	else {
		targetPosition = p;
	}
}

State *CommandCenterLiftAndMoveState::action()
{
	// ������ ��ġ�� �� ���� ������ ����
	if (targetPosition.isValid())
	{
		uList delMine = INFO.getTypeUnitsInRadius(Terran_Vulture_Spider_Mine, S, (Position)targetPosition, 5 * TILE_SIZE);

		for (auto m : delMine)
			m->setKillMe(MustKill);
	}

	if (!unit->isLifted() && unit->canLift() && unit->getDistance((Position)targetPosition) > 16)
	{
		unit->lift();
		return nullptr;
	}

	Position pos = (Position)targetPosition;

	// ���� ���� �ִ� ��ġ�� �̹� ��/�� ���� ���ɵ� ���¸� ��ġ ����
	auto targetMyBase = find_if(INFO.getOccupiedBaseLocations(S).begin(), INFO.getOccupiedBaseLocations(S).end(), [pos](const Base * base) {
		return pos == base->getPosition();
	});

	auto targetEnemyBase = find_if(INFO.getOccupiedBaseLocations(E).begin(), INFO.getOccupiedBaseLocations(E).end(), [pos](const Base * base) {
		return pos == base->getPosition();
	});

	if (targetMyBase != INFO.getOccupiedBaseLocations(S).end() || targetEnemyBase != INFO.getOccupiedBaseLocations(E).end())
	{
		Base *multiBase = nullptr;

		for (int i = 1; i < 4; i++)
		{
			multiBase = INFO.getFirstMulti(S, false, false, i);

			if (multiBase != nullptr)
				break;
		}

		// ��Ƽ �� �� �ִ� ��ġ ������
		if (multiBase)
		{
			targetPosition = multiBase->getTilePosition();
		}
		else
		{
			targetPosition = INFO.getMainBaseLocation(S)->getTilePosition();
		}
	}

	if (unit->isLifted())
	{
		if (ui == nullptr)
			ui = INFO.getUnitInfo(unit, S);

		if (ui->getVeryFrontEnemyUnit() != nullptr)
		{
			moveBackPostion(ui, ui->getVeryFrontEnemyUnit()->getPosition(), 5 * TILE_SIZE);
			return nullptr;
		}

		if (unit->getDistance((Position)targetPosition) < 16)
		{
			if (unit->canLand())
			{
				return new CommandLandingState((Position)targetPosition);
			}
		}
		else {
			CommandUtil::move(unit, (Position)targetPosition);
		}
	}

	return nullptr;
}

MyBot::CommandLandingState::CommandLandingState(Position p)
{
	targetPosition = p;
	nextLandTime = TIME;
}

State *CommandLandingState::action()
{
	// �̹� ��������
	if (!unit->isLifted() && unit->canLift()) {
		// ���� ��ġ�� ����, �ո���, Second/Third Expansion �� �ƴ� ��� �߰� ��Ƽ�� �߰�
		Base *multiBase = nullptr;

		for (Base *baseLocation : INFO.getBaseLocations())
		{
			if (baseLocation->getPosition() == targetPosition)
			{
				multiBase = baseLocation;
				break;
			}
		}

		if (INFO.getMainBaseLocation(S) != nullptr && multiBase == INFO.getMainBaseLocation(S))
			return new CommandCenterIdleState();

		if (INFO.getFirstExpansionLocation(S) != nullptr && multiBase == INFO.getFirstExpansionLocation(S))
			return new CommandCenterIdleState();

		if (INFO.getSecondExpansionLocation(S) != nullptr && multiBase == INFO.getSecondExpansionLocation(S))
			return new CommandCenterIdleState();

		if (INFO.getThirdExpansionLocation(S) != nullptr && multiBase == INFO.getThirdExpansionLocation(S))
			return new CommandCenterIdleState();

		auto targetBase = find_if(INFO.getAdditionalExpansions().begin(), INFO.getAdditionalExpansions().end(), [multiBase](Base * base) {
			return multiBase == base;
		});

		if (targetBase == INFO.getAdditionalExpansions().end())
			INFO.setAdditionalExpansions(multiBase);

		return new CommandCenterIdleState();
	}
	else {
		if (unit->canLand() && bw->canBuildHere((TilePosition)targetPosition, Terran_Command_Center)) {
			if (TIME >= nextLandTime)
			{
				unit->land((TilePosition)targetPosition);
				nextLandTime += 5 * 24;
			}
		}
	}

	return nullptr;
}
