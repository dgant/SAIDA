#pragma once

#include "Common.h"
#include "UnitData.h"

namespace MyBot
{
	enum AreaStatus
	{
		UnknownArea,
		NeutralArea,
		SelfArea,
		EnemyArea,
		CombatArea
	}; // 모름(0)/중립(1)/내꺼(2)/적꺼(3)/전투(4) : 둘다 유닛이 있어 전투 발생/진행 가능 지역

	class GridAreaCell
	{
	public:
		GridAreaCell() {}
		~GridAreaCell() {}

		bool isValid()
		{
			return width() > 0 && height() > 0;
		}
		int width() const {
			return m_bottomRight.x - m_topLeft.x;
		}
		int height() const {
			return m_bottomRight.y - m_topLeft.y;
		}
		int size() const {
			return min(width(), height());
		}

		TilePosition center() const {
			return (m_topLeft + m_bottomRight) / 2;
		}

		TilePosition topLeft() const {
			return m_topLeft;
		}
		void setTopLeft(BWAPI::TilePosition topLeft) {
			m_topLeft = topLeft;
		}

		TilePosition bottomRight() const {
			return m_bottomRight;
		}
		void setBottomRight(BWAPI::TilePosition bottomRight) {
			m_bottomRight = bottomRight;
		}

		int mineCount() const {
			return m_mineCount;
		}
		void setMineCount(int cnt) {
			m_mineCount = cnt;
		}

		AreaStatus areaStatus() const {
			return m_areaStatus;
		}
		void setAreaStatus(AreaStatus st) {
			m_areaStatus = st;
		}

		int reservedMineCount() {
			return m_reservedMineCount;
		}
		void setReservedMineCount(int count) {
			m_reservedMineCount = count;
		}
		void addReservedMineCount() {
			m_reservedMineCount++;
		}

		int myUcnt() const {
			return m_myUcnt;
		}
		void setMyUcnt(int cnt) {
			m_myUcnt = cnt;
		}

		int myBcnt() const {
			return m_myBcnt;
		}
		void setMyBcnt(int cnt) {
			m_myBcnt = cnt;
		}

		int enUcnt() const {
			return m_enUcnt;
		}
		void setEnUcnt(int cnt) {
			m_enUcnt = cnt;
		}

		int enBcnt() const {
			return m_enBcnt;
		}
		void setEnBcnt(int cnt) {
			m_enBcnt = cnt;
		}
	private:
		TilePosition				m_topLeft = { 1000, 1000 };
		TilePosition				m_bottomRight = { -1000, -1000 };
		int m_reservedMineCount = 0;
		int m_mineCount = 0;
		int m_myUcnt = 0;
		int m_myBcnt = 0;
		int m_enUcnt = 0;
		int m_enBcnt = 0;

		AreaStatus m_areaStatus = UnknownArea;
	};

	class GridArea
	{
		vector<int> X, Y;
		vector<vector<GridAreaCell>> sArea;
		int getDist(TilePosition base, GridAreaCell gac);
		int findpos(vector<int> arr, int v);
		int findposR(vector<int> arr, int v);

	public:
		GridArea(const Area *wholeArea, int N = 8);

		vector<vector<GridAreaCell>> &getGridAreaVec() {
			return sArea;
		}

		// base로 부터 가장 가까운 미점령 Cell을 리턴
		GridAreaCell getNearestCell(Player p);

		// base로 부터 가장 먼 점령지 Cell을 리턴
		GridAreaCell getFarthestCell(Player p);

		// base로 부터 radius 반경 경계에 해당하는 GridAreaCell들을 리턴함
		vector<GridAreaCell *> getBoundary(TilePosition base, int radius);

		// 적이 점령한 지역 경계에 해당하는 GridAreaCell들을 리턴함
		vector<GridAreaCell *> getEnemyBoundary(int margin = 5);

		GridAreaCell getGridArea(int i, int j) {
			if (i < 0) i = 0;

			if (j < 0) j = 0;

			if (i >= (int)sArea.size()) i = sArea.size() - 1;

			if (j >= (int)sArea.size()) j = sArea.size() - 1;

			return sArea[i][j];
		}

		void initGridArea()
		{
			for (int i = 0; i < (int)sArea.size(); i++)
				for (int j = 0; j < (int)sArea[i].size(); j++)
				{
					sArea[i][j].setMineCount(0);
					sArea[i][j].setAreaStatus(AreaStatus::UnknownArea);
					sArea[i][j].setMyUcnt(0);
					sArea[i][j].setMyBcnt(0);
					sArea[i][j].setEnUcnt(0);
					sArea[i][j].setEnBcnt(0);
				}
		}

		void initReservedMineCount()
		{
			for (int i = 0; i < (int)sArea.size(); i++)
				for (int j = 0; j < (int)sArea[i].size(); j++)
				{
					sArea[i][j].setReservedMineCount(0);
				}
		}

		bool hasMine(Position pos);

		void update();

	private:
		bool isMyBaseRight = false; // 내 본진이 중앙에서 어느쪽인지, false = left, true = right
	};
}