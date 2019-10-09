#pragma once
#include "../common.h"

namespace Micro
{

class MicroManager
{
	BWAPI::Unitset		_units;

protected:
	

	virtual void        executeMicro(const BWAPI::Unitset & targets) = 0;
	bool				buildScarabOrInterceptor(BWAPI::Unit u) const;
	bool                checkPositionWalkable(BWAPI::Position pos);
	bool                unitNearEnemy(BWAPI::Unit unit);
	bool                unitNearChokepoint(BWAPI::Unit unit) const;

	bool				mobilizeUnit(BWAPI::Unit unit) const;      // unsiege or unburrow
	bool				immobilizeUnit(BWAPI::Unit unit) const;    // siege or burrow
	bool				unstickStuckUnit(BWAPI::Unit unit);
	void                execute();
	void                drawOrderText();

public:
						MicroManager();
    virtual				~MicroManager(){}
	const BWAPI::Unitset & getUnits() const;
	bool				containsType(BWAPI::UnitType type) const;
	BWAPI::Position     calcCenter() const;
	void                update();
	void				setUnits(const BWAPI::Unitset & u);
	void				regroup(const BWAPI::Position & regroupPosition) const;

};
}