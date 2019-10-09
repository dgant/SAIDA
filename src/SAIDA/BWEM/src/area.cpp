//////////////////////////////////////////////////////////////////////////
//
// This file is part of the BWEM Library.
// BWEM is free software, licensed under the MIT/X11 License.
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2015, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "area.h"
#include "mapImpl.h"
#include "graph.h"
#include "neutral.h"
#include "winutils.h"
#include <map>


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
	//                                  class Area
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////


	Area::Area(Graph *pGraph, id areaId, WalkPosition top, int miniTiles)
		: m_pGraph(pGraph), m_id(areaId), m_top(top), m_miniTiles(miniTiles)
	{
		bwem_assert(areaId > 0);

		auto &topMiniTile = GetMap()->GetMiniTile(top);
		bwem_assert(topMiniTile.AreaId() == areaId);

		m_maxAltitude = topMiniTile.Altitude();
	}


	Area::Area(const Area &Other)
		: m_pGraph(Other.m_pGraph)
	{
		bwem_assert(false);
	}


	Map *Area::GetMap() const
	{
		return m_pGraph->GetMap();
	}


	TilePosition Area::BoundingBoxSize() const
	{
		return m_bottomRight - m_topLeft + 1;
	}


	const std::vector<ChokePoint> &Area::ChokePoints(const Area *pArea) const
	{
		auto it = m_ChokePointsByArea.find(pArea);
		bwem_assert(it != m_ChokePointsByArea.end());
		return *it->second;
	}

	bool Area::isNeighbouringArea(const Area *pArea) const {
		auto it = m_ChokePointsByArea.find(pArea);
		return it != m_ChokePointsByArea.end();
	}

	void Area::AddGeyser(Geyser *pGeyser)
	{
		bwem_assert(pGeyser && !contains(m_Geysers, pGeyser));
		m_Geysers.push_back(pGeyser);
	}


	void Area::AddMineral(Mineral *pMineral)
	{
		bwem_assert(pMineral && !contains(m_Minerals, pMineral));
		m_Minerals.push_back(pMineral);
	}


	void Area::OnMineralDestroyed(const Mineral *pMineral)
	{
		bwem_assert(pMineral);

		auto iMineral = find(m_Minerals.begin(), m_Minerals.end(), pMineral);

		if (iMineral != m_Minerals.end())
			fast_erase(m_Minerals, distance(m_Minerals.begin(), iMineral));

		for (Base &base : Bases())
			base.OnMineralDestroyed(pMineral);
	}


	void Area::AddChokePoints(Area *pArea, vector<ChokePoint> *pChokePoints)
	{
		bwem_assert(!m_ChokePointsByArea[pArea] && pChokePoints);

		m_ChokePointsByArea[pArea] = pChokePoints;

		for (const auto &cp : *pChokePoints)
			m_ChokePoints.push_back(&cp);
	}



	vector<int> Area::ComputeDistances(const ChokePoint *pStartCP, const vector<const ChokePoint *> &TargetCPs) const
	{
		bwem_assert(!contains(TargetCPs, pStartCP));

		TilePosition start = GetMap()->BreadthFirstSearch(TilePosition(pStartCP->PosInArea(ChokePoint::middle, this)),
		[this](const Tile & tile, TilePosition) {
			return tile.AreaId() == Id();
		},	// findCond
		[](const Tile &,          TilePosition) {
			return true;
		});					// visitCond

		vector<TilePosition> Targets;

		for (const ChokePoint *cp : TargetCPs)
			Targets.push_back(GetMap()->BreadthFirstSearch(TilePosition(cp->PosInArea(ChokePoint::middle, this)),
			[this](const Tile & tile, TilePosition) {
			return tile.AreaId() == Id();
		},	// findCond
		[](const Tile &,          TilePosition) {
			return true;
		}));					// visitCond

		return ComputeDistances(start, Targets);
	}



	vector<int> Area::ComputeDistances(TilePosition start, const vector<TilePosition> &Targets) const
	{
		const Map *pMap = GetMap();
		vector<int> Distances(Targets.size());

		Tile::UnmarkAll();

		multimap<int, TilePosition> ToVisit;	
		ToVisit.emplace(0, start);

		int remainingTargets = Targets.size();

		while (!ToVisit.empty())
		{
			int currentDist = ToVisit.begin()->first;
			TilePosition current = ToVisit.begin()->second;
			const Tile &currentTile = pMap->GetTile(current, check_t::no_check);
			bwem_assert(currentTile.InternalData() == currentDist);
			ToVisit.erase(ToVisit.begin());
			currentTile.SetInternalData(0);										
			currentTile.SetMarked();

			for (int i = 0 ; i < (int)Targets.size() ; ++i)
				if (current == Targets[i])
				{
					Distances[i] = int(0.5 + currentDist * 32 / 10000.0);
					--remainingTargets;
				}

			if (!remainingTargets) break;

			for (TilePosition delta : {
						TilePosition(-1, -1), TilePosition(0, -1), TilePosition(+1, -1),
						TilePosition(-1,  0),                      TilePosition(+1,  0),
						TilePosition(-1, +1), TilePosition(0, +1), TilePosition(+1, +1)
					})
			{
				const bool diagonalMove = (delta.x != 0) && (delta.y != 0);
				const int newNextDist = currentDist + (diagonalMove ? 14142 : 10000);

				TilePosition next = current + delta;

				if (pMap->Valid(next))
				{
					const Tile &nextTile = pMap->GetTile(next, check_t::no_check);

					if (!nextTile.Marked())
					{
						if (nextTile.InternalData())	
						{
							if (newNextDist < nextTile.InternalData())		
							{	
								auto range = ToVisit.equal_range(nextTile.InternalData());
								auto iNext = find_if(range.first, range.second, [next]
								(const pair<int, TilePosition> &e) {
									return e.second == next;
								});
								bwem_assert(iNext != range.second);

								ToVisit.erase(iNext);
								nextTile.SetInternalData(newNextDist);
								ToVisit.emplace(newNextDist, next);
							}
						}
						else if ((nextTile.AreaId() == Id()) || (nextTile.AreaId() == -1))
						{
							nextTile.SetInternalData(newNextDist);
							ToVisit.emplace(newNextDist, next);
						}
					}
				}
			}
		}

		bwem_assert(!remainingTargets);

		for (auto e : ToVisit)
			pMap->GetTile(e.second, check_t::no_check).SetInternalData(0);

		return Distances;
	}


	void Area::UpdateAccessibleNeighbours()
	{
		m_AccessibleNeighbours.clear();

		for (auto it : ChokePointsByArea())
			if (any_of(it.second->begin(), it.second->end(), [](const ChokePoint & cp) {
			return !cp.Blocked();
			}))
		m_AccessibleNeighbours.push_back(it.first);
	}


	void Area::AddTileInformation(const BWAPI::TilePosition t, const Tile &tile)
	{
		++m_tiles;

		if (tile.Buildable()) ++m_buildableTiles;

		if (tile.GroundHeight() == 1) ++m_highGroundTiles;

		if (tile.GroundHeight() == 2) ++m_veryHighGroundTiles;

		if (t.x < m_topLeft.x)     m_topLeft.x = t.x;

		if (t.y < m_topLeft.y)     m_topLeft.y = t.y;

		if (t.x > m_bottomRight.x) m_bottomRight.x = t.x;

		if (t.y > m_bottomRight.y) m_bottomRight.y = t.y;
	}


	void Area::PostCollectInformation()
	{
	}





	int Area::ComputeBaseLocationScore(TilePosition location) const
	{
		const Map *pMap = GetMap();
		const TilePosition dimCC = UnitType(Terran_Command_Center).tileSize();

		int sumScore = 0;

		for (int dy = 0 ; dy < dimCC.y ; ++dy)
			for (int dx = 0 ; dx < dimCC.x ; ++dx)
			{
				const Tile &tile = pMap->GetTile(location + TilePosition(dx, dy), check_t::no_check);

				if (!tile.Buildable()) return -1;

				if (tile.InternalData() == -1) return -1;		

				if (tile.AreaId() != Id()) return -1;

				if (tile.GetNeutral() && tile.GetNeutral()->IsStaticBuilding()) return -1;

				sumScore += tile.InternalData();
			}

		return sumScore;
	}



	bool Area::ValidateBaseLocation(TilePosition location, vector<Mineral *> &BlockingMinerals) const
	{
		const Map *pMap = GetMap();
		const TilePosition dimCC = UnitType(Terran_Command_Center).tileSize();

		BlockingMinerals.clear();

		for (int dy = -3 ; dy < dimCC.y + 3 ; ++dy)
			for (int dx = -3 ; dx < dimCC.x + 3 ; ++dx)
			{
				TilePosition t = location + TilePosition(dx, dy);

				if (pMap->Valid(t))
				{
					const Tile &tile = pMap->GetTile(t, check_t::no_check);

					if (Neutral *n = tile.GetNeutral())
					{
						if (n->IsGeyser()) return false;

						if (Mineral *m = n->IsMineral())
							if (m->InitialAmount() <= 8) BlockingMinerals.push_back(m);
							else return false;
					}
				}
			}

		for (const Base &base : Bases())
			if (roundedDist(base.Location(), location) < min_tiles_between_Bases) return false;

		return true;
	}


	void Area::CreateBases()
	{
		const TilePosition dimCC = UnitType(Terran_Command_Center).tileSize();
		const Map *pMap = GetMap();



		vector<Ressource *> RemainingRessources;

		for (Mineral *m : Minerals())	if ((m->InitialAmount() >= 40) && !m->Blocking()) RemainingRessources.push_back(m);

		for (Geyser *g : Geysers())	if ((g->InitialAmount() >= 300) && !g->Blocking()) RemainingRessources.push_back(g);

		m_Bases.reserve(min(100, (int)RemainingRessources.size()));

		while (!RemainingRessources.empty())
		{

			TilePosition topLeftRessources     = {numeric_limits<int>::max(), numeric_limits<int>::max()};
			TilePosition bottomRightRessources = {numeric_limits<int>::min(), numeric_limits<int>::min()};

			for (const Ressource *r : RemainingRessources)
			{
				makeBoundingBoxIncludePoint(topLeftRessources, bottomRightRessources, r->TopLeft());
				makeBoundingBoxIncludePoint(topLeftRessources, bottomRightRessources, r->BottomRight());
			}

			TilePosition topLeftSearchBoundingBox = topLeftRessources - dimCC - max_tiles_between_CommandCenter_and_ressources;
			TilePosition bottomRightSearchBoundingBox = bottomRightRessources + 1 + max_tiles_between_CommandCenter_and_ressources;
			makePointFitToBoundingBox(topLeftSearchBoundingBox, TopLeft(), BottomRight() - dimCC + 1);
			makePointFitToBoundingBox(bottomRightSearchBoundingBox, TopLeft(), BottomRight() - dimCC + 1);

			for (const Ressource *r : RemainingRessources)
				for (int dy = -dimCC.y - max_tiles_between_CommandCenter_and_ressources ; dy < r->Size().y + dimCC.y + max_tiles_between_CommandCenter_and_ressources ; ++dy)
					for (int dx = -dimCC.x - max_tiles_between_CommandCenter_and_ressources ; dx < r->Size().x + dimCC.x + max_tiles_between_CommandCenter_and_ressources ; ++dx)
					{
						TilePosition t = r->TopLeft() + TilePosition(dx, dy);

						if (pMap->Valid(t))
						{
							const Tile &tile = pMap->GetTile(t, check_t::no_check);
							int dist = (distToRectangle(center(t), r->TopLeft(), r->Size()) + 16) / 32;
							int score = max(max_tiles_between_CommandCenter_and_ressources + 3 - dist, 0);

							if (r->IsGeyser()) score *= 3;		

							if (tile.AreaId() == Id()) tile.SetInternalData(tile.InternalData() + score);	
						}
					}

			for (const Ressource *r : RemainingRessources)
				for (int dy = -3 ; dy < r->Size().y + 3 ; ++dy)
					for (int dx = -3 ; dx < r->Size().x + 3 ; ++dx)
					{
						TilePosition t = r->TopLeft() + TilePosition(dx, dy);

						if (pMap->Valid(t))
							pMap->GetTile(t, check_t::no_check).SetInternalData(-1);
					}


			TilePosition bestLocation;
			int bestScore = 0;
			vector<Mineral *> BlockingMinerals;

			for (int y = topLeftSearchBoundingBox.y ; y <= bottomRightSearchBoundingBox.y ; ++y)
				for (int x = topLeftSearchBoundingBox.x ; x <= bottomRightSearchBoundingBox.x ; ++x)
				{
					int score = ComputeBaseLocationScore(TilePosition(x, y));

					if (score > bestScore)
						if (ValidateBaseLocation(TilePosition(x, y), BlockingMinerals))
						{
							bestScore = score;
							bestLocation = TilePosition(x, y);
						}
				}

			for (const Ressource *r : RemainingRessources)
				for (int dy = -dimCC.y - max_tiles_between_CommandCenter_and_ressources ; dy < r->Size().y + dimCC.y + max_tiles_between_CommandCenter_and_ressources ; ++dy)
					for (int dx = -dimCC.x - max_tiles_between_CommandCenter_and_ressources ; dx < r->Size().x + dimCC.x + max_tiles_between_CommandCenter_and_ressources ; ++dx)
					{
						TilePosition t = r->TopLeft() + TilePosition(dx, dy);

						if (pMap->Valid(t)) pMap->GetTile(t, check_t::no_check).SetInternalData(0);
					}

			if (!bestScore) break;

			vector<Ressource *> AssignedRessources;

			for (Ressource *r : RemainingRessources)
				if (distToRectangle(r->Pos(), bestLocation, dimCC) + 2 <= max_tiles_between_CommandCenter_and_ressources * 32)
					AssignedRessources.push_back(r);

			really_remove_if(RemainingRessources, [&AssignedRessources](const Ressource * r) {
				return contains(AssignedRessources, r);
			});

			if (AssignedRessources.empty())
			{
				//bwem_assert(false);
				break;
			}

			m_Bases.emplace_back(this, bestLocation, AssignedRessources, BlockingMinerals);
		}
	}

	////////////////////////////////////////////////
	void Area::calcBoundaryVertices() const
	{
		set<Position> unsortedVertices;

		for (int x(0); x < Map::Instance().Size().x; ++x)
			for (int y(0); y < Map::Instance().Size().y; ++y)
			{
				TilePosition tp(x, y);

				if (Map::Instance().GetArea(tp) != this)
				{
					continue;
				}

				TilePosition t1(tp.x + 1, tp.y);
				TilePosition t2(tp.x, tp.y + 1);
				TilePosition t3(tp.x - 1, tp.y);
				TilePosition t4(tp.x, tp.y - 1);

				bool surrounded = true;

				if (tp.x == 0 || tp.y == 0
						|| (t1.isValid() && (Map::Instance().GetArea(t1) != this || !Broodwar->isBuildable(t1)))
						|| (t2.isValid() && (Map::Instance().GetArea(t2) != this || !Broodwar->isBuildable(t2)))
						|| (t3.isValid() && (Map::Instance().GetArea(t3) != this || !Broodwar->isBuildable(t3)))
						|| (t4.isValid() && (Map::Instance().GetArea(t4) != this || !Broodwar->isBuildable(t4))))
				{
					surrounded = false;
				}

				if (!surrounded && Broodwar->isBuildable(tp))
				{
					unsortedVertices.insert(Position(tp) + Position(16, 16));
				}
			}


		if (unsortedVertices.empty()) {
			return;
		}

		vector<Position> sortedVertices;
		Position current = *unsortedVertices.begin();

		sortedVertices.push_back(current);
		unsortedVertices.erase(current);

		while (!unsortedVertices.empty())
		{
			double bestDist = 1000000;
			Position bestPos;

			for (const Position &pos : unsortedVertices)
			{
				double dist = pos.getDistance(current);

				if (dist < bestDist)
				{
					bestDist = dist;
					bestPos = pos;
				}
			}

			current = bestPos;
			sortedVertices.push_back(bestPos);
			unsortedVertices.erase(bestPos);
		}

		int distanceThreshold = 10;

		while (true)
		{
			int maxFarthest = 0;
			int maxFarthestStart = 0;
			int maxFarthestEnd = 0;

			for (int i(0); i < (int)sortedVertices.size(); ++i)
			{
				int farthest = 0;
				int farthestIndex = 0;

				for (size_t j(1); j < sortedVertices.size() / 2; ++j)
				{
					int jindex = (i + j) % sortedVertices.size();

					if (sortedVertices[i].getDistance(sortedVertices[jindex]) < distanceThreshold)
					{
						farthest = j;
						farthestIndex = jindex;
					}
				}

				if (farthest > maxFarthest)
				{
					maxFarthest = farthest;
					maxFarthestStart = i;
					maxFarthestEnd = farthestIndex;
				}
			}

			if (maxFarthest < 4)
			{
				break;
			}

			double dist = sortedVertices[maxFarthestStart].getDistance(sortedVertices[maxFarthestEnd]);

			vector<Position> temp;

			for (size_t s(maxFarthestEnd); s != maxFarthestStart; s = (s + 1) % sortedVertices.size())
			{
				temp.push_back(sortedVertices[s]);
			}

			sortedVertices = temp;
		}

		m_boundaryVertices = sortedVertices;
	}
} 