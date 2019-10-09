#pragma once

#include "Common.h"
#include "UnitState\State.h"

namespace MyBot
{
	enum PosChange
	{
		Stop = 0,
		Closer = 1,
		Farther = 2
	};

	enum KillMe
	{
		NoKill = 0,
		KillMe = 1,
		MustKill = 2,
		AssignedKill = 3,
		MustKillAssigned
	};

	class UnitInfo
	{
	public :
		UnitInfo();
		~UnitInfo();
		UnitInfo(Unit);
		UnitInfo(Unit, Position);

		void initFrame();
		void Update();

		Unit		unit() const {
			return m_unit;
		}
		UnitType type() const {
			return m_type;
		}
		Position	pos() const {
			return m_lastPosition;
		}
		Position	vPos() const {
			return m_vPosition;
		}
		Position	vPos(int frame) const {
			return m_lastPosition + Position((int)(m_unit->getVelocityX() * frame), (int)(m_unit->getVelocityY() * frame));
		}
		Player		player() const {
			return m_player;
		}
		int			id() const {
			return m_unitID;
		}
		int			hp() const {
			return m_HP + m_Shield;
		}
		int			shield() const {
			return m_Shield;
		}
		bool		isComplete() const {
			return m_completed;
		}
		int			getRemainingBuildTime() const {
			return m_RemainingBuildTime;
		}
		bool		isMorphing() const {
			return m_morphing;
		}
		bool		isBurrowed() const {
			return m_burrowed;
		}
		void		setFrame(int f = 0) {
			m_LastCommandFrame = TIME + f;
		}
		int			frame() const {
			return m_LastCommandFrame;
		}
		int			expectedDamage() const {
			return m_expectedDamage;
		}
		bool		isBeingRepaired() const {
			return m_beingRepaired;
		}
		int		getKillMe() const {
			return m_killMe;
		}
		bool		isPowered() const {
			return m_isPowered;
		}

		int		getMarinesInBunker() const {
			return m_marineInBunker;
		}

		void	recudeEnergy(int energy) {
			m_energy -= min(m_energy, (double)energy);
		}

		int		getEnergy() const {
			return (int)m_energy;
		}

		// Attacker가 this unit을 공격할때 Damage를 계산해서 누적해줌
		void		setDamage(Unit attacker);

		// Building 용
		void		setLift(bool l) {
			m_lifted = l;
		}
		bool		getLift() {
			return m_lifted;
		}
		bool		isDangerMine() {
			return m_dangerMine;
		}

		//enemy 용
		const bool isHide() const {
			return m_hide;
		}

		const bool operator == (Unit unit) const
		{
			return m_unitID == unit->getID();
		}
		const bool operator == (const UnitInfo &rhs) const
		{
			return (m_unitID == rhs.m_unitID);
		}
		const bool operator < (const UnitInfo &rhs) const
		{
			return (m_unitID < rhs.m_unitID);
		}

		// 상태
		string getState() {
			return pState->getName();
		}
		void setState(State *state);
		// action 호출 후 state를 변경
		void changeState(State *state);
		void action();
		void action(Unit targetUnit);
		void action(Unit targetUnit, Unit targetUnit2);
		void action(Position targetPosition);
		void action(TilePosition targetPosition);
		void action(BWEM::Base *targetPosition);
		void action(UnitType unitType);

		// Specific Target
		Unit	getTarget() {
			return pState->getTargetUnit();
		}
		Position		getTargetPos() {
			return pState->getTargetPos();
		}

		PosChange posChange(UnitInfo *uInfo);

		// 클락킹 유닛은 포함되지 않음.
		vector<Unit> &getEnemiesTargetMe() {
			return m_enemiesTargetMe;
		}
		Position	getAvgEnemyPos() {
			return m_avgEnemyPosition;
		}
		Unit		getVeryFrontEnemyUnit() {
			return m_veryFrontEnemUnit;
		}
		void		clearAvgEnemyPos() {
			m_avgEnemyPosition = Positions::None;
		}
		void		clearVeryFrontEnemyUnit() {
			m_veryFrontEnemUnit = nullptr;
		}
		bool		isGatheringMinerals() {
			return m_isGatheringMinerals;
		}
		void		setGatheringMinerals() {
			m_isGatheringMinerals = true;
		}
		void		setMarineInBunker() {
			m_marineInBunker++;
		}
		void		setKillMe(int  i = 1) {
			m_killMe = i;
		}
		int			getLastPositionTime() {
			return m_lastPositionTime;
		}
		Position	getLastSeenPosition() {
			return m_lastSeenPosition;
		}
		int			getSpaceRemaining() {
			return m_spaceRemaining;
		}
		void		addSpaceRemaining(int space) {
			m_spaceRemaining += space;
		}
		void		delSpaceRemaining(int space) {
			m_spaceRemaining -= space;
		}
		void		initspaceRemaining()	{
			m_spaceRemaining = m_unit->getSpaceRemaining();
		}
		int			getLastSiegeOrUnsiegeTime() {
			return m_lastSiegeOrUnsiegeTime;
		}
		void		setLastSiegeOrUnsiegeTime(int m_lastSiegeOrUnsiegeTime_) {
			m_lastSiegeOrUnsiegeTime = m_lastSiegeOrUnsiegeTime_;
		}

		int			getCompleteTime() {
			return m_completeTime;
		}

		Position			getIdlePosition() {
			return pState->getIdlePos();
		}

		bool		isBlocked() const {
			return m_blockedCnt > 25;
		}

	private :
		Unit							m_unit;
		UnitType						m_type;
		Player							m_player;

		int								m_unitID;
		int								m_RemainingBuildTime;
		int								m_HP;
		bool							m_beingRepaired;
		int								m_Shield;
		double							m_energy;
		int								m_LastCommandFrame;

		int								m_expectedDamage; // 공격 받을 Damage를 예상하기 위함.

		int								m_lastPositionTime; // m_lastPosition 에 처음 도달한 시간
		Position						m_lastSeenPosition; // m_lastPosition 과 동일하나 Unknown 으로 바껴도 변하지 않음.
		Position						m_lastPosition;
		Position						m_vPosition;
		bool							m_completed;
		bool							m_morphing;
		bool							m_burrowed;
		bool							m_canBurrow;
		bool							m_hide;
		int								m_killMe;
		int								m_spaceRemaining;
		bool							m_isPowered;
		int								m_completeTime;
		bool							m_dangerMine;
		queue<bool>						m_blockedQueue;
		int								m_blockedCnt; // 120 frame 간 길막을 당한 횟수
		// burrow 유닛 unknown position 으로 변경시키기 위해 사용.
		int								m_nearUnitFrameCnt;
		int								m_lastNearUnitFrame;

		// 건물의 lift 확인
		bool							m_lifted;

		// 벙커 Marine 확인
		int							m_marineInBunker;

		// 행동 확인
		// 일꾼의 경우 실제로 미네랄을 채취하고 있는지 여부 리턴
		bool							m_isGatheringMinerals;

		vector<Unit>					m_enemiesTargetMe;
		Position						m_avgEnemyPosition;
		Unit							m_veryFrontEnemUnit;
		// 상태
		State							*pState;

		// For Tank
		int								m_lastSiegeOrUnsiegeTime;

	};
}