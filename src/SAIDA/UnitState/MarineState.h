#pragma once

#include "../Common.h"
#include "../UnitData.h"
#include "State.h"
#include "../UnitManager/MarineManager.h"
#include "../InformationManager.h"
#include "../EnemyStrategyManager.h"
#include "../StrategyManager.h"

namespace MyBot
{
	
//����========================================
	class MarineIdleState : public State
	{
	public:
		virtual string getName() override {
			return "Idle";
		}
		virtual State *action() override;
	};
//========================================
	class MarineKillDState : public State
	{
	public:
		virtual string getName() override {
			return "KillD";
		}
		virtual State *action() override;
	};
//���========================================
	class MarineScoutState : public State
	{
	public:
		virtual string getName() override {
			return "Scout";
		}
		virtual State *action() override;
	private:
		Base *targetBase = nullptr;
		UnitInfo *me = nullptr;
	};
//��========================================
	class MarineKitingState : public State
	{
	public:
		virtual string getName() override {
			return "Kiting";
		}
	};
//����========================================
	class MarineKitingMultiState : public State
	{
	public:
		virtual string getName() override {
			return "KitingMulti";
		}
	};
//����========================================
	class MarineKeepMultiState : public State
	{
	public:
		virtual string getName() override {
			return "KeepMulti";
		}
	};
//Ǳˮ==========================================
	class MarineDiveState : public State
	{
	public:
		MarineDiveState() {}
		~MarineDiveState() {}

		virtual string getName() override {
			return "Diving";
		}
		virtual State *action() override;

	private:
		int direction = 1;
	};
//�ƶ�==========================================
	class MarineMoveState : public State
	{
	public:
		virtual string getName() override {
			return "Move";
		}
	};
//����==========================================
	class MarineAttackState : public State
	{
	public:
		MarineAttackState(Position targetPos) {
			m_targetUnit = nullptr;
			m_targetPos = targetPos;
		}
		MarineAttackState(UnitInfo *target) {
			m_targetUnit = target->unit();
			m_targetPos = target->pos();
		}
		virtual string getName() override {
			return "Attack";
		}
		virtual State *action() override;
	};
//Ѳ��==========================================
	class MarinePatrolState : public State
	{
	public:
		virtual string getName() override {
			return "Patrol";
		}
	};
//ɱscouter==========================================
	class MarineKillScouterState : public State
	{
	public:
		virtual string getName() override {
			return "KillScouter";
		}
		virtual State *action(Position lastScouterPosition) override;
	};
//����==========================================
	class MarineDefenceState : public State
	{
	public:
		MarineDefenceState() {}
		~MarineDefenceState() {}
		virtual string getName() override {
			return "Defence";
		}
		virtual State *action() override;


		State *bunkerRushDefence();
		UnitInfo *getMarineDefenceTargetUnit(const Unit &unit);
		bool shuttleDefence(const Unit &unit);
		bool darkDefence(const Unit &unit, const UnitInfo *closeUnit);
	};
//zealot����==========================================
	class MarineZealotDefenceState : public State
	{
	public:
		virtual string getName() override {
			return "ZealotDefence";
		}
		virtual State *action() override;
	};
//firstchoke����==========================================
	class MarineFirstChokeDefenceState : public State
	{
	public:
		MarineFirstChokeDefenceState(Position p);
		virtual string getName() override {
			return "FirstChokeDefence";
		}
		virtual State *action() override;
	private:
		Position defencePosition;
	};
//����==========================================
	class MarineGoGoGoState : public State
	{
	public:
		virtual string getName() override {
			return "GoGoGo";
		}
		virtual State *action() override;
	};
//��ɱũ��==========================================
	class MarineKillWorkerFirstState : public State
	{
	public:
		MarineKillWorkerFirstState() {}
		~MarineKillWorkerFirstState() {}

		virtual string getName() override {
			return "KillWorkerFirst";
		}
		virtual State *action() override;
	private:
		bool checkBase = false;
	};
//�ֿ�Ǳˮ==========================================
	class MarineDive2State : public State
	{
	public:
		MarineDive2State() {}
		~MarineDive2State() {}

		virtual string getName() override {
			return "Diving2";
		}
		virtual State *action() override;

	private:
		int direction = 1;
	};
//���£�վߣ==========================================
	class MarineIronmanState : public State
	{
	public:
		MarineIronmanState() {}
		~MarineIronmanState() {}

		virtual string getName() override {
			return "Ironman";
		}
		virtual State *action() override;
	private:
		
	};
//����һ��============================================
	class MarineGatherState : public State
	{
	public:
		Position waitnearsecondchokepoint = Positions::None;
		MarineGatherState() {}
		~MarineGatherState() {}

		virtual string getName() override 
		{
			return "Gather";
		}
		virtual State *action() override;
	private:

	};

}