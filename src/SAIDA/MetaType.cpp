#include "MetaType.h"

using namespace MyBot;

MetaType::MetaType ()
	: _type(MetaTypes::Default)
	, _race(Races::None)
{
}

MetaType::MetaType(const string &name)
	: _type(MetaTypes::Default)
	, _race(Races::None)
{
	string inputName(name);
	replace(inputName.begin(), inputName.end(), '_', ' ');

	for (const UnitType &unitType : UnitTypes::allUnitTypes())
	{
		
		string typeName = unitType.getName();
		replace(typeName.begin(), typeName.end(), '_', ' ');

		if (typeName == inputName)
		{
			*this = MetaType(unitType);
			return;
		}

		
		const string &raceName = unitType.getRace().getName();

		if ((typeName.length() > raceName.length()) && (typeName.compare(raceName.length() + 1, typeName.length(), inputName) == 0))
		{
			*this = MetaType(unitType);
			return;
		}
	}

	for (const TechType &techType : TechTypes::allTechTypes())
	{
		string typeName = techType.getName();
		replace(typeName.begin(), typeName.end(), '_', ' ');

		if (typeName == inputName)
		{
			*this = MetaType(techType);
			return;
		}
	}

	for (const UpgradeType &upgradeType : UpgradeTypes::allUpgradeTypes())
	{
		string typeName = upgradeType.getName();
		replace(typeName.begin(), typeName.end(), '_', ' ');

		if (typeName == inputName)
		{
			*this = MetaType(upgradeType);
			return;
		}
	}


}

MetaType::MetaType (UnitType t)
	: _unitType(t)
	, _type(MetaTypes::Unit)
	, _race(t.getRace())
{
}

MetaType::MetaType (TechType t)
	: _techType(t)
	, _type(MetaTypes::Tech)
	, _race(t.getRace())
{
}

MetaType::MetaType (UpgradeType t)
	: _upgradeType(t)
	, _type(MetaTypes::Upgrade)
	, _race(t.getRace())
{
}

const size_t &MetaType::type() const
{
	return _type;
}

bool MetaType::isUnit() const
{
	return _type == MetaTypes::Unit;
}

bool MetaType::isTech() const
{
	return _type == MetaTypes::Tech;
}

bool MetaType::isUpgrade() const
{
	return _type == MetaTypes::Upgrade;
}

const Race &MetaType::getRace() const
{
	return _race;
}

bool MetaType::isBuilding()	const
{
	return _type == MetaTypes::Unit && _unitType.isBuilding();
}

bool MetaType::isRefinery()	const
{
	return isBuilding() && _unitType.isRefinery();
}

const UnitType &MetaType::getUnitType() const
{
	return _unitType;
}

const TechType &MetaType::getTechType() const
{
	return _techType;
}

const UpgradeType &MetaType::getUpgradeType() const
{
	return _upgradeType;
}

int MetaType::supplyRequired()
{
	if (isUnit())
	{
		return _unitType.supplyRequired();
	}
	else
	{
		return 0;
	}
}

int MetaType::mineralPrice() const
{
	return isUnit() ? _unitType.mineralPrice() : (isTech() ? _techType.mineralPrice() : _upgradeType.mineralPrice());
}

int MetaType::gasPrice() const
{
	return isUnit() ? _unitType.gasPrice() : (isTech() ? _techType.gasPrice() : _upgradeType.gasPrice());
}

UnitType MetaType::whatBuilds() const
{
	return isUnit() ? _unitType.whatBuilds().first : (isTech() ? _techType.whatResearches() : _upgradeType.whatUpgrades());
}

string MetaType::getName() const
{
	if (isUnit())
	{
		return _unitType.getName();
	}
	else if (isTech())
	{
		return _techType.getName();
	}
	else if (isUpgrade())
	{
		return _upgradeType.getName();
	}
	else
	{
		return "LOL";
	}
}