#include "InformationManager.h"
#include "GridArea.h"

using namespace MyBot;

GridArea::GridArea(const Area *wholeArea, int N)
{
	TilePosition topLeft = wholeArea->TopLeft();
	TilePosition bottomRight = wholeArea->BottomRight();

	int width = bottomRight.x - topLeft.x;
	int height = bottomRight.y - topLeft.y;

	if (N <= 0)	N = 1;


	if (N > min(width, height) / 3)	N = min(width, height) / 3;

	sArea.resize(N);

	for (int i = 0; i < N; i++)	sArea[i].resize(N);

	int wbase = (bottomRight.x - topLeft.x) / N;
	int wremain = bottomRight.x - topLeft.x - N * wbase;

	TilePosition myBase = S->getStartLocation();

	if (myBase.x <= (topLeft.x + bottomRight.x) / 2)
	{
		isMyBaseRight = false;

		X.push_back(topLeft.x);

		for (int i = 1; i <= N; i++)
		{
			X.push_back(X[i - 1] + wbase + (wremain >= 0 ? 1 : 0)), wremain--;
		}
	}
	else
	{
		isMyBaseRight = true;

		X.push_back(bottomRight.x);

		for (int i = 1; i <= N; i++)
		{
			X.push_back(X[i - 1] - wbase - (wremain >= 0 ? 1 : 0)), wremain--;
		}
	}

	int hbase = (bottomRight.y - topLeft.y) / N;
	int hremain = bottomRight.y - topLeft.y - N * hbase;

	Y.push_back(topLeft.y);

	for (int i = 1; i <= N; i++)
	{
		Y.push_back(Y[i - 1] + hbase + (hremain >= 0 ? 1 : 0)), hremain--;
	}

	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			TilePosition tl, br;
			tl.x = X[i];
			tl.y = Y[j];
			br.x = X[i + 1];
			br.y = Y[j + 1];

			sArea[i][j].setTopLeft(tl);
			sArea[i][j].setBottomRight(br);
		}
	}
}

int GridArea::getDist(TilePosition base, GridAreaCell gac)
{
	TilePosition mid = gac.center();
	return (int)sqrt((mid.x - base.x) * (mid.x - base.x) + (mid.y - base.y) * (mid.y - base.y));
}

vector<GridAreaCell *> GridArea::getEnemyBoundary(int margin)
{
	const BWEM::Base *ebase = INFO.getMainBaseLocation(E);

	if (ebase != nullptr)
	{
		TilePosition base = ebase->Location();
		GridAreaCell cell = getNearestCell(E);
		int dist = getDist(base, cell) + margin;		// margin(기본 5) tile 정도 밖을 리턴

		Broodwar->drawCircleMap(Position(base), dist * 32, BWAPI::Colors::Yellow);

		return getBoundary(base, dist);
	}

	return vector<GridAreaCell *>();
}


GridAreaCell GridArea::getNearestCell(Player p)
{
	int N = sArea.size();
	const BWEM::Base *baseLoc = INFO.getMainBaseLocation(p);
	GridAreaCell ret = sArea[N / 2][N / 2];		

	if (baseLoc != nullptr)
	{
		TilePosition base = baseLoc->Location();

		int dist = 100000;


		for (int i = 0; i < N; i++)
			for (int j = 0; j < N; j++)
			{
				if (sArea[i][j].areaStatus() == AreaStatus::NeutralArea || sArea[i][j].areaStatus() == AreaStatus::UnknownArea)
				{
					int newD = getDist(base, sArea[i][j]);

					if (newD < dist)
					{
						dist = newD;
						ret = sArea[i][j];
					}
				}
			}
	}

	return ret;
}

GridAreaCell GridArea::getFarthestCell(Player p)
{
	int N = sArea.size();
	const BWEM::Base *baseLoc = INFO.getMainBaseLocation(p);
	GridAreaCell ret = sArea[N / 2][N / 2];		

	AreaStatus status = AreaStatus::EnemyArea;

	if (p == S)
	{
		status = AreaStatus::SelfArea;
	}

	if (baseLoc != nullptr)
	{
		TilePosition base = baseLoc->Location();

		int dist = 0;

	
		for (int i = 0; i < N; i++)
			for (int j = 0; j < N; j++)
			{
				if (sArea[i][j].areaStatus() == status || sArea[i][j].areaStatus() == AreaStatus::CombatArea)
				{
					int newD = getDist(base, sArea[i][j]);

					if (newD > dist)
					{
						dist = newD;
						ret = sArea[i][j];
					}
				}
			}
	}

	return ret;
}

vector<GridAreaCell *> GridArea::getBoundary(TilePosition base, int dist)
{
	int N = sArea.size();
	vector<GridAreaCell *> cells;

	
	if (dist > 0)
	{
		for (int j = 0; j < N; j++)
		{
			for (int i = 0; i < N - 1; i++)
			{
				int d1 = getDist(base, sArea[i][j]);
				int d2 = getDist(base, sArea[i + 1][j]);

				if (d1 <= dist && d2 >= dist)
				{
					if (bw->isWalkable(WalkPosition(sArea[i + 1][j].center())))
						cells.push_back(&sArea[i + 1][j]);

			

					break;
				}
				else if (d1 >= dist && d2 <= dist)
				{
					if (bw->isWalkable(WalkPosition(sArea[i][j].center())))
						cells.push_back(&sArea[i][j]);

				

					break;
				}
				else if (d1 >= dist && d2 >= dist && (d2 - dist) <= sArea[i + 1][j].size())
				{
					if (bw->isWalkable(WalkPosition(sArea[i + 1][j].center())))
						cells.push_back(&sArea[i + 1][j]);

					

					break;
				}
			}
		}
	}

	return cells;
}

int GridArea::findpos(vector<int> arr, int v)
{
	if (arr.size() == 0 || arr[0] > v || arr[arr.size() - 1] < v)	return -1;

	int s = 0, e = arr.size() - 1;

	while (s <= e)
	{
		int m = (s + e) / 2;

		if (arr[m] == v)	break;
		else if (arr[m] < v)	s = m + 1;
		else
		{
			e = m - 1;
		}
	}

	return e;
}

int GridArea::findposR(vector<int> arr, int v)
{
	if (arr.size() == 0 || arr[0] < v || arr[arr.size() - 1] > v)	return -1;

	int e = 0;

	for (word i = 0; i < arr.size() - 1; i++)
	{
		if (arr[i] > v && v >= arr[i + 1])
		{
			e = i;
		}
	}

	if (e == 0)
	{
		return -1;
	}

	return e;
}

bool GridArea::hasMine(Position pos)
{
	vector<int>	Xs(X), Ys(Y);

	for (int i = 0; i < (int)X.size(); i++) Xs[i] *= TILEPOSITION_SCALE, Ys[i] *= TILEPOSITION_SCALE;

	word xind = findpos(Xs, pos.x);
	word yind = findpos(Ys, pos.y);

	if (xind < 0 || yind < 0 || xind >= sArea.size() || yind >= sArea.size()) return false;

	if (sArea[xind][yind].mineCount() > 0) return true;

	return false;
}

void GridArea::update()
{
	if (Broodwar->getFrameCount() < 300) return;

	initGridArea();

	uMap myU = INFO.getUnits(S);
	uMap myB = INFO.getBuildings(S);
	uMap enU = INFO.getUnits(E);
	uMap enB = INFO.getBuildings(E);

	int N = sArea.size();
	vector<int>	Xs(X), Ys(Y);

	for (int i = 0; i < (int)X.size(); i++) Xs[i] *= TILEPOSITION_SCALE, Ys[i] *= TILEPOSITION_SCALE;

	// My Units
	for (auto &u : myU)
	{
		Position upos = u.second->pos();
		TilePosition tp(upos);

		if (!Broodwar->isExplored(tp) || upos == Positions::Unknown || upos == Positions::Invalid)	continue;

		int xind = 0;

		if (isMyBaseRight)
		{
			xind = findposR(Xs, upos.x);
		}
		else
		{
			xind = findpos(Xs, upos.x);
		}

		int yind = findpos(Ys, upos.y);

		if (xind < 0 || yind < 0 || xind >= N || yind >= N)	continue;

		int cnt = sArea[xind][yind].myUcnt();
		sArea[xind][yind].setMyUcnt(cnt + 1);

		if (u.second->type() == UnitTypes::Terran_Vulture_Spider_Mine)
		{


			sArea[xind][yind].setMineCount(sArea[xind][yind].mineCount() + 1);
		}
	}

	// My Buildings
	for (auto &u : myB)
	{
		Position upos = u.second->pos();
		TilePosition tp(upos);

		if (!Broodwar->isExplored(tp) || upos == Positions::Unknown || upos == Positions::Invalid)	continue;

		int xind = findpos(Xs, upos.x);
		int yind = findpos(Ys, upos.y);

		if (xind < 0 || yind < 0 || xind >= N || yind >= N)	continue;

		int cnt = sArea[xind][yind].myBcnt();
		sArea[xind][yind].setMyBcnt(cnt + 1);
	}


	for (auto &u : enU)
	{
		Position upos = u.second->pos();
		TilePosition tp(upos);

		if (!Broodwar->isExplored(tp) || upos == Positions::Unknown || upos == Positions::Invalid)	continue;

		int xind = findpos(Xs, upos.x);
		int yind = findpos(Ys, upos.y);

		if (xind < 0 || yind < 0 || xind >= N || yind >= N)	continue;

		int cnt = sArea[xind][yind].enUcnt();
		sArea[xind][yind].setEnUcnt(cnt + 1);
	}

	for (auto &u : enB)
	{
		Position upos = u.second->pos();
		TilePosition tp(upos);

		if (!Broodwar->isExplored(tp) || upos == Positions::Unknown || upos == Positions::Invalid)	continue;

		int xind = findpos(Xs, upos.x);
		int yind = findpos(Ys, upos.y);

		if (xind < 0 || yind < 0 || xind >= N || yind >= N)	continue;

		int cnt = sArea[xind][yind].enBcnt();
		sArea[xind][yind].setEnBcnt(cnt + 1);
	}

	for (int i = 0; i < (int)sArea.size(); i++)
		for (int j = 0; j < (int)sArea[i].size(); j++)
		{
			int mycnt = sArea[i][j].myBcnt() + sArea[i][j].myUcnt();
			int encnt = sArea[i][j].enBcnt() + sArea[i][j].enUcnt();

			if (mycnt * encnt > 0)
			{
				sArea[i][j].setAreaStatus(AreaStatus::CombatArea);
			}
			else if (mycnt)
			{
				sArea[i][j].setAreaStatus(AreaStatus::SelfArea);
			}
			else if (encnt)
			{
				sArea[i][j].setAreaStatus(AreaStatus::EnemyArea);
			}
			else if (Broodwar->isExplored(sArea[i][j].center()))
			{
				sArea[i][j].setAreaStatus(AreaStatus::NeutralArea);
			}
		}
}