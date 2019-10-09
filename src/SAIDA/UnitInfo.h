#pragma once

#include "Common.h"
#include "UnitManager\State.h"

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

	
		void		setDamage(Unit attacker);

		
		void		setLift(bool l) {
			m_lifted = l;
		}
		bool		getLift() {
			return m_lifted;
		}
		bool		isDangerMine() {
			return m_dangerMine;
		}

		
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

	
		string getState() {
			return pState->getName();
		}
		void setState(State *state);
		
		void changeState(State *state);
		void action();
		void action(Unit targetUnit);
		void action(Unit targetUnit, Unit targetUnit2);
		void action(Position targetPosition);
		void action(TilePosition targetPosition);
		void action(BWEM::Base *targetPosition);
		void action(UnitType unitType);

		
		Unit	getTarget() {
			return pState->getTargetUnit();
		}
		Position		getTargetPos() {
			return pState->getTargetPos();
		}

		PosChange posChange(UnitInfo *uInfo);

		
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

		int								m_expectedDamage; 

		int								m_lastPositionTime; 
		Position						m_lastSeenPosition; 
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
		int								m_blockedCnt; 
		
		int								m_nearUnitFrameCnt;
		int								m_lastNearUnitFrame;

		
		bool							m_lifted;

		
		int							m_marineInBunker;

		
		bool							m_isGatheringMinerals;

		vector<Unit>					m_enemiesTargetMe;
		Position						m_avgEnemyPosition;
		Unit							m_veryFrontEnemUnit;
		
		State							*pState;

		
		int								m_lastSiegeOrUnsiegeTime;

	};
}