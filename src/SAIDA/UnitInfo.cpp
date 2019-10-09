
#include "Common.h"
#include "GameCommander.h"
#include "UnitInfo.h"
#include "UnitState\State.h"
#include "InformationManager.h"

using namespace MyBot;

UnitInfo::UnitInfo(Unit unit)
{
	m_unit = unit;
	m_RemainingBuildTime = getRemainingBuildFrame(m_unit);
	m_type = unit->getType();
	m_unitID = unit->getID();
	m_player = unit->getPlayer();
	m_lastPositionTime = TIME;
	m_lastPosition = unit->getPosition();
	m_lastSeenPosition = m_lastPosition;
	m_completed = unit->isCompleted();
	m_hide = true;
	pState = new InitialNewState();
	pState->unit = m_unit;
	m_lifted = false;
	m_vPosition = Positions::None;
	m_avgEnemyPosition = Positions::None;
	m_veryFrontEnemUnit = nullptr;
	m_isGatheringMinerals = false;
	m_beingRepaired = false;
	m_LastCommandFrame = 0;
	m_marineInBunker = 0;
	m_morphing = unit->isMorphing();
	m_burrowed = m_unit->isBurrowed();
	m_canBurrow = m_unit->canBurrow(false);
	m_killMe = 0;
	m_isPowered = true;
	m_dangerMine = false;
	m_blockedCnt = 0;

	m_HP = 0;
	m_Shield = 0;
	m_expectedDamage = 0;
	m_lastSiegeOrUnsiegeTime = 0;

	if (m_unit->getPlayer() == S) {
		m_spaceRemaining = m_unit->getSpaceRemaining();
		m_energy = m_unit->getEnergy();
	}
	else {
		m_spaceRemaining = 0;
		m_energy = m_type.maxEnergy();
	}

	m_lastNearUnitFrame = 0;
	m_nearUnitFrameCnt = 0;
	m_completeTime = m_completed ? TIME : TIME + m_RemainingBuildTime;
}

// 중립같은 것들에 대해 관리할때 사용.
UnitInfo::UnitInfo(Unit unit, Position pos) {
	m_unit = unit;
	m_type = unit->getType();
	m_unitID = unit->getID();
	m_lastPosition = pos;
	m_lastSeenPosition = m_lastPosition;
	m_completed = true;
	m_hide = false;
	pState = nullptr;

	m_RemainingBuildTime = 0;
	m_player = unit->getPlayer();
	m_lastPositionTime = TIME;

	m_lifted = false;
	m_vPosition = Positions::None;
	m_avgEnemyPosition = Positions::None;
	m_veryFrontEnemUnit = nullptr;
	m_isGatheringMinerals = false;
	m_beingRepaired = false;
	m_LastCommandFrame = 0;
	m_marineInBunker = 0;
	m_morphing = unit->isMorphing();
	m_burrowed = m_unit->isBurrowed();
	m_canBurrow = m_unit->canBurrow(false);
	m_killMe = 0;
	m_isPowered = true;
	m_dangerMine = false;
	m_blockedCnt = 0;

	m_HP = 0;
	m_Shield = 0;
	m_expectedDamage = 0;
	m_lastSiegeOrUnsiegeTime = 0;
	m_energy = 0;
	m_spaceRemaining = 0;
	m_lastNearUnitFrame = 0;
	m_nearUnitFrameCnt = 0;
	m_completeTime = 0;
}

void UnitInfo::Update()
{
	if (m_unit->exists() && !m_unit->isLoaded())
	{
		bool isShowThisFrame = m_hide;

		m_hide = false;

		// 공격을 하고 있을때는 블락킹 처리를 하지 않는다.
		if (m_lastPosition == m_unit->getPosition() && m_vPosition != m_lastPosition && m_unit->getGroundWeaponCooldown() == 0) {
			m_blockedQueue.push(true);
			m_blockedCnt++;

			if (m_blockedQueue.size() > 120) {
				if (m_blockedQueue.front())
					m_blockedCnt--;

				m_blockedQueue.pop();
			}
		}
		else {
			m_blockedQueue.push(false);

			if (m_blockedQueue.size() > 120) {
				if (m_blockedQueue.front())
					m_blockedCnt--;

				m_blockedQueue.pop();
			}
		}

		if (m_lastPosition != m_unit->getPosition()) {
			m_lastPositionTime = TIME;
			m_lastPosition = m_unit->getPosition();
			m_lastSeenPosition = m_lastPosition;
		}
		else if (m_unit->getAirWeaponCooldown() || m_unit->getGroundWeaponCooldown() || m_unit->getSpellCooldown() || m_unit->isConstructing() || m_unit->isHoldingPosition() || m_isGatheringMinerals) {
			m_lastPositionTime = TIME;
		}

		m_vPosition = m_lastPosition + Position((int)(m_unit->getVelocityX() * 8), (int)(m_unit->getVelocityY() * 8));
		m_completed = m_unit->isCompleted();
		m_morphing = m_unit->isMorphing();

		if (m_unit->getHitPoints() > m_HP)
			m_beingRepaired = true;
		else
			m_beingRepaired = false;

		m_HP = m_unit->getHitPoints();
		m_Shield = m_unit->getShields();
		m_type = m_unit->getType();
		m_expectedDamage = 0;
		m_isPowered = m_unit->isPowered();

		m_RemainingBuildTime = getRemainingBuildFrame(m_unit);

		if (!m_completed)
			m_completeTime = TIME + m_RemainingBuildTime;

		if (m_unit->getPlayer() == S) {
			m_energy = m_unit->getEnergy();
			m_spaceRemaining = m_unit->getSpaceRemaining();
		}
		else {
			m_energy = min((double)E->maxEnergy(m_type), m_energy + 0.03125);

			if (isShowThisFrame) {
				// 지상유닛이고 주변이 모두 visiable 이고 unload 할 건물 or 유닛이 있으면.
				if (!m_type.isFlyer() && !m_type.isBuilding()) {
					bool isUnloaded = true;

					for (auto direction : {
								TilePosition(1, 0), TilePosition(-1, 0), TilePosition(0, 1), TilePosition(0, -1)
							}) {
						TilePosition t = m_unit->getTilePosition() + direction;

						if (t.isValid() && !bw->isVisible(t)) {
							isUnloaded = false;
							break;
						}
					}

					if (isUnloaded) {
						UnitInfo *ui = nullptr;

						// unload 할 수 있는 건물 유닛 찾기
						if (INFO.enemyRace == Races::Zerg)
							ui = INFO.getClosestTypeUnit(E, m_unit->getPosition(), Zerg_Overlord, 320);
						else if (INFO.enemyRace == Races::Protoss)
							ui = INFO.getClosestTypeUnit(E, m_unit->getPosition(), Protoss_Shuttle, 320);
						else {
							ui = INFO.getClosestTypeUnit(E, m_unit->getPosition(), Terran_Dropship, 320);

							if (ui == nullptr)
								ui = INFO.getClosestTypeUnit(E, m_unit->getPosition(), Terran_Bunker, 320);
						}

						if (ui)
							ui->m_spaceRemaining = min(ui->m_spaceRemaining + m_type.spaceRequired(), ui->type().spaceProvided());
					}
				}
				// load 가능한 건물이 나타난 경우 가득 찼다고 가정.
				else if (m_type == Zerg_Overlord || m_type == Protoss_Shuttle || m_type == Terran_Dropship || m_type == Terran_Bunker)
					m_spaceRemaining = 0;
			}
		}

		if (m_type == Terran_Vulture_Spider_Mine)
		{
			if (m_unit->getPlayer() == S)
			{
				m_dangerMine = false;

				for (auto eu : INFO.getUnitsInRadius(E, m_lastPosition, 3 * TILE_SIZE, true, false, false))
				{
					if (!eu->type().isWorker() && eu->type() != Terran_Vulture && eu->type() != Protoss_Archon)
					{
						m_dangerMine = true;
						break;
					}
				}
			}
			else
				m_burrowed = true;
		}

		if ( m_type.isBurrowable())
		{
			if (m_canBurrow == true && m_unit->canBurrow(false) == false)
				m_burrowed = true;
			else if (m_canBurrow == false && m_unit->canBurrow(false) == true)
				m_burrowed = false;

			m_canBurrow = m_unit->canBurrow(false);
		}

		if (m_lifted != m_unit->isFlying())
		{
			if (m_lifted == true)
				GameCommander::Instance().onUnitLanded(m_unit);
			else
				GameCommander::Instance().onUnitLifted(m_unit);

			m_lifted = m_unit->isFlying();
		}

		/// Enemy Bunker 의 Marine Count Clear
		if (TIME % 12 == 0  && m_unit->getPlayer() != S && m_type == Terran_Bunker)
		{
			int gap;

			if (E->getUpgradeLevel(UpgradeTypes::U_238_Shells))
				gap = 8;
			else
				gap = 7;

			if (INFO.getClosestUnit(S, m_lastPosition, AllKind, gap * TILE_SIZE, true) != nullptr)
				m_marineInBunker = 0;
		}

		if (m_unit->getPlayer() != S && m_type.canAttack())
		{
			if (m_unit->getOrderTarget() != nullptr && m_unit->getOrderTarget()->exists() && m_unit->getOrderTarget()->getPlayer() == S)
			{
				if (INFO.getUnitInfo(m_unit->getOrderTarget(), S) != nullptr)
				{
					INFO.getUnitInfo(m_unit->getOrderTarget(), S)->getEnemiesTargetMe().push_back(m_unit);
				}
			}

			else if (m_unit->getTarget() != nullptr && m_unit->getTarget()->exists() && m_unit->getTarget()->getPlayer() == S)
			{
				if (INFO.getUnitInfo(m_unit->getTarget(), S) != nullptr)
				{
					INFO.getUnitInfo(m_unit->getTarget(), S)->getEnemiesTargetMe().push_back(m_unit);
				}
			}

			else if (!m_unit->isDetected()) {
				for (auto s : INFO.getUnitsInRadius(S, m_unit->getPosition(), m_unit->getType().groundWeapon().maxRange() + 32)) {
					if (abs(atan2(s->pos().y - m_unit->getPosition().y, s->pos().x - m_unit->getPosition().x) - m_unit->getAngle()) < 0.035)
						s->getEnemiesTargetMe().push_back(m_unit);
				}
			}
		}

		// TODO 위치 이동. 불필요하게 여러번 세팅될 수 있음.
		if (!m_enemiesTargetMe.empty())
		{
			m_avgEnemyPosition = UnitUtil::GetAveragePosition(m_enemiesTargetMe);
			m_veryFrontEnemUnit = UnitUtil::GetClosestEnemyTargetingMe(m_unit, m_enemiesTargetMe);
		}
	}
	else {
		// 이전 프레임에 보였다가 갑자기 안보이는 경우
		if (!m_hide) {
			// 지상유닛이고 주변에 load 할 건물, 유닛이 있으면.
			if (!m_type.isFlyer() && !m_type.isBuilding()) {
				UnitInfo *ui = nullptr;

				// load 할 수 있는 건물 유닛 찾기
				if (INFO.enemyRace == Races::Zerg)
					ui = INFO.getClosestTypeUnit(E, m_lastPosition, Zerg_Overlord, 100);
				else if (INFO.enemyRace == Races::Protoss)
					ui = INFO.getClosestTypeUnit(E, m_lastPosition, Protoss_Shuttle, 100);
				else {
					ui = INFO.getClosestTypeUnit(E, m_lastPosition, Terran_Dropship, 100);

					if (ui == nullptr)
						ui = INFO.getClosestTypeUnit(E, m_lastPosition, Terran_Bunker, 100);
				}

				if (ui)
					ui->m_spaceRemaining = max(ui->m_spaceRemaining - m_type.spaceRequired(), 0);
			}
		}

		m_hide = true;

		if (m_type.isBuilding() && !m_completed) {
			if (!--m_RemainingBuildTime) {
				m_completed = true;
			}
		}

		m_energy = min((double)E->maxEnergy(m_type), m_energy + 0.03125);

		if (m_lastPosition != Positions::Unknown) {
			// 저그의 burrow 가능한 유닛들이나 vulture 의 spider mine 동작.
			if (m_burrowed)
			{
				// 터렛, 베슬, 스캔 시야 안인 경우
				if (ComsatStationManager::Instance().inDetectedArea(m_type, m_lastPosition)
						|| ComsatStationManager::Instance().inTheScanArea(m_lastPosition)) {
					if (m_lastNearUnitFrame + 1 == TIME)
						m_nearUnitFrameCnt++;
					else
						m_nearUnitFrameCnt = 1;

					m_lastNearUnitFrame = TIME;

					if (m_nearUnitFrameCnt > 50) {
						cout << "[" << TIME << "] " << m_type << " position changed to unknown. (near detecting unit)" << endl;
						m_lastPosition = Positions::Unknown;
					}
				}
				else {
					// 럴커 주변에 지상유닛이 있는데도 안보이면 없다고 판단.
					if (m_type == Zerg_Lurker)
					{
						if (INFO.getUnitsInRadius(S, m_lastPosition, 5 * TILE_SIZE, true, false, true).size())
						{
							if (m_lastNearUnitFrame + 1 == TIME)
								m_nearUnitFrameCnt++;
							else
								m_nearUnitFrameCnt = 1;

							m_lastNearUnitFrame = TIME;

							if (m_nearUnitFrameCnt > 70) {
								cout << TIME << " : lurker position changed to unknown" << endl;
								m_lastPosition = Positions::Unknown;
							}
						}
					}
					// 벌쳐, 일꾼을 제외한 지상유닛이 근처에 있는데도 안보이면 없어짐.
					else if (m_type == Terran_Vulture_Spider_Mine) {
						uList selfUnit = INFO.getUnitsInRadius(S, m_lastPosition, 3 * TILE_SIZE, true, false, false);

						for (auto eu : selfUnit) {
							if (eu->type() != Terran_Vulture && eu->type() != Protoss_Archon) {
								if (m_lastNearUnitFrame + 1 == TIME)
									m_nearUnitFrameCnt++;
								else
									m_nearUnitFrameCnt = 1;

								m_lastNearUnitFrame = TIME;

								break;
							}
						}

						if (m_nearUnitFrameCnt > 70) {
							cout << TIME << " : mine position changed to unknown" << endl;
							m_lastPosition = Positions::Unknown;
						}
					}
				}
			}
			else
			{
				// Visible 아닌 적이 있는 경우에만 Unknown 처리 한다.
				if (bw->isVisible(TilePosition(m_lastPosition)) && bw->isVisible(TilePosition(m_vPosition)))
					m_lastPosition = Positions::Unknown;
			}
		}
	}

	m_isGatheringMinerals = false;
}

void UnitInfo::initFrame() {
	if (!m_enemiesTargetMe.empty())
	{
		m_enemiesTargetMe.clear();
		m_avgEnemyPosition = Positions::None;
		m_veryFrontEnemUnit = nullptr;
	}
}

void UnitInfo::setDamage(Unit attacker)
{
	m_expectedDamage += getDamage(attacker, m_unit);
}

void UnitInfo::setState(State *state)
{
	if (pState)
		delete pState;

	pState = state;
	pState->unit = m_unit;
	m_lastPositionTime = TIME;
}

void UnitInfo::changeState(State *state)
{
	if (state != nullptr) {
		if (this->type() == Terran_SCV) {
			ScvManager::Instance().setStateToSCV(this, state);
		}
		else if (this->type() == Terran_Siege_Tank_Tank_Mode || this->type() == Terran_Siege_Tank_Siege_Mode) {
			TankManager::Instance().setStateToTank(this, state);
		}
		else if (this->type() == Terran_Goliath) {
			GoliathManager::Instance().setStateToGoliath(this, state);
		}

		else {
			if (pState)
				delete pState;

			pState = state;
			pState->unit = m_unit;
			m_lastPositionTime = TIME;
		}
	}
}

void UnitInfo::action()
{
	try {
		changeState(pState->action());
	}
	catch (SAIDA_Exception e) {
		Logger::error("%s State action Error. (ErrorCode : %x, Eip : %p)\n", pState->getName().c_str(), e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("%s State action Error. (Error : %s)\n", pState->getName().c_str(), e.what());
		throw e;
	}
	catch (...) {
		Logger::error("%s State action Unknown Error.\n", pState->getName().c_str());
		throw;
	}
}

void UnitInfo::action(Unit unit)
{
	try {
		changeState(pState->action(unit));
	}
	catch (SAIDA_Exception e) {
		Logger::error("%s State action Error. (ErrorCode : %x, Eip : %p)\n", pState->getName().c_str(), e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("%s State action Error. (Error : %s)\n", pState->getName().c_str(), e.what());
		throw e;
	}
	catch (...) {
		Logger::error("%s State action Unknown Error.\n", pState->getName().c_str());
		throw;
	}
}

void MyBot::UnitInfo::action(Unit targetUnit, Unit targetUnit2)
{
	try {
		changeState(pState->action(targetUnit, targetUnit2));
	}
	catch (SAIDA_Exception e) {
		Logger::error("%s State action Error. (ErrorCode : %x, Eip : %p)\n", pState->getName().c_str(), e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("%s State action Error. (Error : %s)\n", pState->getName().c_str(), e.what());
		throw e;
	}
	catch (...) {
		Logger::error("%s State action Unknown Error.\n", pState->getName().c_str());
		throw;
	}
}

void UnitInfo::action(UnitType unitType)
{
	try {
		changeState(pState->action(unitType));
	}
	catch (SAIDA_Exception e) {
		Logger::error("%s State action Error. (ErrorCode : %x, Eip : %p)\n", pState->getName().c_str(), e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("%s State action Error. (Error : %s)\n", pState->getName().c_str(), e.what());
		throw e;
	}
	catch (...) {
		Logger::error("%s State action Unknown Error.\n", pState->getName().c_str());
		throw;
	}
}

void UnitInfo::action(Position targetPosition)
{
	try {
		changeState(pState->action(targetPosition));
	}
	catch (SAIDA_Exception e) {
		Logger::error("%s State action Error. (ErrorCode : %x, Eip : %p)\n", pState->getName().c_str(), e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("%s State action Error. (Error : %s)\n", pState->getName().c_str(), e.what());
		throw e;
	}
	catch (...) {
		Logger::error("%s State action Unknown Error.\n", pState->getName().c_str());
		throw;
	}
}

void UnitInfo::action(TilePosition targetPosition)
{
	try {
		changeState(pState->action(targetPosition));
	}
	catch (SAIDA_Exception e) {
		Logger::error("%s State action Error. (ErrorCode : %x, Eip : %p)\n", pState->getName().c_str(), e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("%s State action Error. (Error : %s)\n", pState->getName().c_str(), e.what());
		throw e;
	}
	catch (...) {
		Logger::error("%s State action Unknown Error.\n", pState->getName().c_str());
		throw;
	}
}

void UnitInfo::action(Base *targetPosition)
{
	try {
		changeState(pState->action(targetPosition));
	}
	catch (SAIDA_Exception e) {
		Logger::error("%s State action Error. (ErrorCode : %x, Eip : %p)\n", pState->getName().c_str(), e.getSeNumber(), e.getExceptionPointers()->ContextRecord->Eip);
		throw e;
	}
	catch (const exception &e) {
		Logger::error("%s State action Error. (Error : %s)\n", pState->getName().c_str(), e.what());
		throw e;
	}
	catch (...) {
		Logger::error("%s State action Unknown Error.\n", pState->getName().c_str());
		throw;
	}
}

PosChange UnitInfo::posChange(UnitInfo *uInfo)
{
	if (uInfo->isHide())
		return PosChange::Stop;

	if (m_lastPosition.getApproxDistance(uInfo->pos()) > m_lastPosition.getApproxDistance(uInfo->vPos()) + 10)
		return PosChange::Closer;

	if (m_lastPosition.getApproxDistance(uInfo->pos()) + 10 < m_lastPosition.getApproxDistance(uInfo->vPos()) )
		return PosChange::Farther;

	return PosChange::Stop;
}


UnitInfo::UnitInfo()
{
}

UnitInfo::~UnitInfo()
{
	if (pState)
	{
		delete pState;
	}
}
