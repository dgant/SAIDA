//////////////////////////////////////////////////////////////////////////
//
// This file is part of the BWEM Library.
// BWEM is free software, licensed under the MIT/X11 License.
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2015, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "base.h"
#include "graph.h"
#include "mapImpl.h"
#include "neutral.h"
#include "bwapiExt.h"


using namespace BWAPI;
using namespace BWAPI::UnitTypes::Enum;
namespace {
	auto &bw = Broodwar;
}

using namespace std;

namespace BWEM {

	using namespace detail;
	using namespace BWAPI_ext;


	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class Base
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////


	Base::Base(Area *pArea, const TilePosition &location, const vector<Ressource *> &AssignedRessources, const vector<Mineral *> &BlockingMinerals)
		: m_pArea(pArea),
		  m_pMap(pArea->GetMap()),
		  m_location(location),
		  m_center(Position(location) + Position(UnitType(Terran_Command_Center).tileSize()) / 2),
		  m_BlockingMinerals(BlockingMinerals)
	{
		bwem_assert(!AssignedRessources.empty());

		for (Ressource *r : AssignedRessources)
			if		(Mineral *m = r->IsMineral())	m_Minerals.push_back(m);
			else if (Geyser *g = r->IsGeyser())		m_Geysers.push_back(g);
	}

	Base::Base(const Base &Other)
		: m_pMap(Other.m_pMap),
		  m_pArea(Other.m_pArea),
		  m_location(Other.Location()),
		  m_center(Position(Other.Location()) + Position(UnitType(Terran_Command_Center).tileSize()) / 2),
		  m_BlockingMinerals(Other.BlockingMinerals())
	{
		for (auto m : Other.Minerals())		m_Minerals.push_back(m);

		for (auto g : Other.Geysers())		m_Geysers.push_back(g);
	}

	void Base::SetStartingLocation(const TilePosition &actualLocation)
	{
		m_starting = true;
		m_location = actualLocation;
		m_center = Position(actualLocation) + Position(UnitType(Terran_Command_Center).tileSize()) / 2;
	}


	void Base::OnMineralDestroyed(const Mineral *pMineral)
	{
		bwem_assert(pMineral);

		auto iMineral = find(m_Minerals.begin(), m_Minerals.end(), pMineral);

		if (iMineral != m_Minerals.end())
			fast_erase(m_Minerals, distance(m_Minerals.begin(), iMineral));

		iMineral = find(m_BlockingMinerals.begin(), m_BlockingMinerals.end(), pMineral);

		if (iMineral != m_BlockingMinerals.end())
			fast_erase(m_BlockingMinerals, distance(m_BlockingMinerals.begin(), iMineral));
	}

	////////////////////////////////////////////////////////////////////////////
	// SAIDA Ãß°¡
	bool Base::isIsland() const {
		return m_pArea->isIsland();
	}

	void Base::setBaseLocationInfo() const {
		m_isMineralsLeftOfBase = m_location.x > m_bottomMineral->getTilePosition().x;
		m_isMineralsTopOfBase = m_location.y > m_bottomMineral->getTilePosition().y;

		m_isBaseLeftOfArea = m_pArea->BottomRight().x - m_location.x > m_location.x + 3 - m_pArea->TopLeft().x;
		m_isBaseTopOfArea = m_pArea->BottomRight().y - m_location.y > m_location.y + 2 - m_pArea->TopLeft().y;
	}

} // namespace BWEM



