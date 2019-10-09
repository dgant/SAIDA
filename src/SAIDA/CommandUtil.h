#pragma once

#include "Common.h"

namespace MyBot
{
	struct Rect
	{
		int x, y;
		int height, width;
	};

	namespace CommandUtil
	{
		bool attackUnit(BWAPI::Unit attacker, BWAPI::Unit target, bool repeat = false);

		bool attackMove(BWAPI::Unit attacker, const BWAPI::Position &targetPosition, bool repeat = false);

		void move(BWAPI::Unit attacker, const BWAPI::Position &targetPosition, bool repeat = false);

		void rightClick(BWAPI::Unit unit, BWAPI::Unit target, bool repeat = false, bool rightClickOnly = false);
		void rightClick(BWAPI::Unit unit, BWAPI::Position target, bool repeat = false);

		void repair(BWAPI::Unit unit, BWAPI::Unit target, bool repeat = false);
		void backMove(BWAPI::Unit unit, BWAPI::Unit target, bool attackingMove = false, bool repeat = false);

		void patrol(BWAPI::Unit patroller, const BWAPI::Position &targetPosition, bool repeat = false);
		void goliathHold(BWAPI::Unit holder, BWAPI::Unit target = nullptr, bool repeat = false);
		void hold(BWAPI::Unit holder, bool repeat = false);
		void holdControll(BWAPI::Unit unit, BWAPI::Unit target, BWAPI::Position targetPosition, bool targetUnit = false);
		bool build(BWAPI::Unit builder, BWAPI::UnitType building, BWAPI::TilePosition buildPosition);
		void gather(BWAPI::Unit worker, BWAPI::Unit target);
	};


	namespace UnitUtil
	{
		bool IsCombatUnit(BWAPI::Unit unit);
		bool IsValidUnit(BWAPI::Unit unit);
		bool CanAttack(BWAPI::Unit attacker, BWAPI::Unit target);
		double CalculateLTD(BWAPI::Unit attacker, BWAPI::Unit target);

		int GetAttackRange(BWAPI::Unit attacker, BWAPI::Unit target);
		int GetAttackRange(BWAPI::Unit attacker, bool isTargetFlying);
		int GetAttackRange(BWAPI::UnitType attackerType, BWAPI::Player attackerPlayer, bool isFlying);


		BWAPI::WeaponType GetWeapon(BWAPI::Unit attacker, BWAPI::Unit target);
		BWAPI::WeaponType GetWeapon(BWAPI::Unit attacker, bool isTargetFlying);
		BWAPI::WeaponType GetWeapon(BWAPI::UnitType attackerType, bool isFlying);

		size_t GetAllUnitCount(BWAPI::UnitType type);

		BWAPI::Unit GetClosestUnitTypeToTarget(BWAPI::UnitType type, BWAPI::Position target);
		double GetDistanceBetweenTwoRectangles(Rect &rect1, Rect &rect2);

		BWAPI::Position GetAveragePosition(std::vector<BWAPI::Unit>  units);
		BWAPI::Unit GetClosestEnemyTargetingMe(BWAPI::Unit myUnit, std::vector<BWAPI::Unit>  units);

		BWAPI::Position getPatrolPosition(BWAPI::Unit attackUnit, BWAPI::WeaponType weaponType, BWAPI::Position targetPos);
	};
}