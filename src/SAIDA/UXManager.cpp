#include "UXManager.h"

using namespace MyBot;

UXManager::UXManager()
{

}

UXManager &UXManager::Instance()
{
	static UXManager instance;
	return instance;
}


void UXManager::onStart()
{
}

void UXManager::update() {
	uList tanks = INFO.getTypeUnitsInRadius(Terran_Siege_Tank_Tank_Mode, S);

	for (auto t : tanks) {
		if (t->unit()->isSelected()) {
			Color c1 = checkZeroAltitueAroundMe(t->pos(), 6) ? Colors::Red : Colors::Blue;
			Color c2 = isBlocked(t->unit()) ? Colors::Red : Colors::Blue;

			bw->drawCircleMap(t->pos(), 6, c1);
			bw->drawCircleMap(t->pos(), 32, c2);

			break;
		}
	}

	if (TIME > 300)
	{
		bw->drawCircleMap(SM.getDrawLinePosition(), 5, Colors::Red);
		bw->drawTextMap(SM.getDrawLinePosition(), "DrawLinePosition");
		bw->drawCircleMap(SM.getWaitLinePosition(), 5, Colors::Yellow);
		bw->drawTextMap(SM.getWaitLinePosition(), "WaitLinePosition");
		bw->drawCircleMap(INFO.getFirstWaitLinePosition(), 5, Colors::Green);
		bw->drawTextMap(INFO.getFirstWaitLinePosition(), "FirstWaitLinePosition");
	}

	if (Config::Debug::DrawBWEMInfo)
	{
		drawBWEMResultOnMap(theMap);
	}

	// 정찰 및 전략
	if (Config::Debug::DrawScoutInfo)
	{
		drawScoutInformation(220, 320);
	}

	// 공격
	if (Config::Debug::DrawUnitTargetInfo)
	{
		drawUnitTargetOnMap();

		// 미사일, 럴커의 보이지않는 공격등을 표시
		drawBulletsOnMap();
	}

	//cout << 14;

	// draw tile position of mouse cursor
	if (Config::Debug::DrawMouseCursorInfo)
	{
		int mouseX = Broodwar->getMousePosition().x + Broodwar->getScreenPosition().x;
		int mouseY = Broodwar->getMousePosition().y + Broodwar->getScreenPosition().y;

		/*
		정찰TEST
		int c = bw->getUnitsInRadius(Position(mouseX, mouseY), 1 * TILE_SIZE, Filter::IsBuilding || Filter::IsNeutral).size();
		Broodwar->drawTextMap(mouseX + 50, mouseY + 50, "%d", c);*/

		if (theMap.Size().x - mouseX / 32 < 5)
			mouseX -= 100;

		if (theMap.Size().y - mouseY / 32 < 6)
			mouseY -= 40;

		Broodwar->drawTextMap(mouseX + 20, mouseY, "(%d, %d)", (int)(mouseX / TILE_SIZE), (int)(mouseY / TILE_SIZE));

		if (BWAPI::Broodwar->getFrameCount() > 10) {
			WalkPosition w = WalkPosition((int)(mouseX / 8), (int)(mouseY / 8));

			if (w.isValid()) {
				if (!theMap.GetArea(w) || theMap.GetArea(w)->MaxAltitude() <= 0) {
					BWAPI::Broodwar->drawTextMap(mouseX + 20, mouseY + 12, "(altitude : %d, height : %d)", theMap.GetMiniTile(w).Altitude(), bw->getGroundHeight((TilePosition)w));
				}
				else {
					BWAPI::Broodwar->drawTextMap(mouseX + 20, mouseY + 12, "(altitude : %d, %f%%, height : %d)", theMap.GetMiniTile(w).Altitude(), (double)theMap.GetMiniTile(w).Altitude() / theMap.GetArea(w)->MaxAltitude() * 100, bw->getGroundHeight((TilePosition)w));
				}
			}
		}
	}

	if (Config::Debug::DrawMyUnit || Config::Debug::DrawEnemyUnit)
	{
		drawAllUnitData(20, 20);
	}

	drawUnitStatus();
}

void UXManager::drawAllUnitData(int x, int y)
{
	Broodwar->drawTextScreen(x, y, "\x04 <UnitData>");

	map<UnitType, uList> &allSUnit = INFO.getUnitData(S).getUnitTypeMap();
	map<UnitType, uList> &allSBuild = INFO.getUnitData(S).getBuildingTypeMap();
	uMap &sUnit = INFO.getUnits(S);
	uMap &sBuild = INFO.getBuildings(S);

	map<UnitType, uList> &allEUnit = INFO.getUnitData(E).getUnitTypeMap();
	map<UnitType, uList> &allEBuild = INFO.getUnitData(E).getBuildingTypeMap();
	uMap &eUnit = INFO.getUnits(E);
	uMap &eBuild = INFO.getBuildings(E);

	map<UnitType, uList>::iterator iter;

	if (Config::Debug::DrawMyUnit)
	{
		int gap = 10;

		for (iter = allSUnit.begin(); iter != allSUnit.end(); iter++)
		{
			if (iter->second.empty())
				continue;

			Broodwar->drawTextScreen(x, y + gap, "%s : %d", iter->first.getName().c_str(), iter->second.size());
			gap += 10;
		}

		Broodwar->drawTextScreen(x, y + gap, "AllUnit : %d", sUnit.size());
		gap += 20;

		for (iter = allSBuild.begin(); iter != allSBuild.end(); iter++)
		{
			if (iter->second.empty())
				continue;

			Broodwar->drawTextScreen(x, y + gap, "%s : %d", iter->first.getName().c_str(), iter->second.size());
			gap += 10;
		}

		Broodwar->drawTextScreen(x, y + gap, "AllBuildings : %d", sBuild.size());

		x += 180;

		gap = 0;
		Broodwar->drawTextScreen(x, y + gap, "\x04 <Unit Create/All>");
		gap += 10;

		for (iter = allSUnit.begin(); iter != allSUnit.end(); iter++)
		{
			if (INFO.getCompletedCount(iter->first, S) + INFO.getAllCount(iter->first, S) == 0)
				continue;

			Broodwar->drawTextScreen(x, y + gap, "%s : %d/%d", iter->first.getName().c_str(), INFO.getCompletedCount(iter->first, S), INFO.getAllCount(iter->first, S));
			gap += 10;
		}

		gap += 10;
		Broodwar->drawTextScreen(x, y + gap, "\x04 <Building Create/All>");
		gap += 10;

		for (iter = allSBuild.begin(); iter != allSBuild.end(); iter++)
		{
			if (INFO.getCompletedCount(iter->first, S) + INFO.getAllCount(iter->first, S) == 0)
				continue;

			Broodwar->drawTextScreen(x, y + gap, "%s : %d/%d", iter->first.getName().c_str(), INFO.getCompletedCount(iter->first, S), INFO.getAllCount(iter->first, S));
			gap += 10;
		}
	}
	else
	{
		int gap = 10;

		for (iter = allEUnit.begin(); iter != allEUnit.end(); iter++)
		{
			if (iter->second.empty())
				continue;

			Broodwar->drawTextScreen(x, y + gap, "%s : %d", iter->first.getName().c_str(), iter->second.size());
			gap += 10;
		}

		Broodwar->drawTextScreen(x, y + gap, "AllUnit : %d", eUnit.size());
		gap += 20;

		for (iter = allEBuild.begin(); iter != allEBuild.end(); iter++)
		{
			if (iter->second.empty())
				continue;

			Broodwar->drawTextScreen(x, y + gap, "%s : %d", iter->first.getName().c_str(), iter->second.size());
			gap += 10;
		}

		Broodwar->drawTextScreen(x, y + gap, "AllBuildings : %d", eBuild.size());

		x += 180;

		gap = 0;
		Broodwar->drawTextScreen(x, y + gap, "\x04 <Unit Complete/All>");
		gap += 10;

		for (iter = allEUnit.begin(); iter != allEUnit.end(); iter++)
		{
			if (INFO.getCompletedCount(iter->first, E) + INFO.getAllCount(iter->first, E) == 0)
				continue;

			Broodwar->drawTextScreen(x, y + gap, "%s : %d/%d", iter->first.getName().c_str(), INFO.getCompletedCount(iter->first, E), INFO.getAllCount(iter->first, E) );
			gap += 10;
		}

		gap += 10;
		Broodwar->drawTextScreen(x, y + gap, "\x04 <Building Complete/All>");
		gap += 10;

		for (iter = allEBuild.begin(); iter != allEBuild.end(); iter++)
		{
			if (INFO.getCompletedCount(iter->first, E) + INFO.getAllCount(iter->first, E) == 0)
				continue;

			Broodwar->drawTextScreen(x, y + gap, "%s : %d/%d", iter->first.getName().c_str(), INFO.getCompletedCount(iter->first, E), INFO.getAllCount(iter->first, E));
			gap += 10;
		}
	}
}

void UXManager::drawGameInformationOnScreen(int x, int y)
{
	Broodwar->drawTextScreen(x, y, "\x04Players:");
	Broodwar->drawTextScreen(x + 50, y, "%c%s(%s) \x04vs. %c%s(%s)",
							 S->getTextColor(), S->getName().c_str(), INFO.selfRace.c_str(),
							 E->getTextColor(), E->getName().c_str(), INFO.enemyRace.c_str());
	y += 12;

	Broodwar->drawTextScreen(x, y, "\x04Map:");
	Broodwar->drawTextScreen(x + 50, y, "\x03%s (%d x %d size)", Broodwar->mapFileName().c_str(), Broodwar->mapWidth(), Broodwar->mapHeight());
	Broodwar->setTextSize();
	y += 12;

	Broodwar->drawTextScreen(x, y, "\x04Time:");
	Broodwar->drawTextScreen(x + 50, y, "\x04%d", Broodwar->getFrameCount());
	Broodwar->drawTextScreen(x + 90, y, "\x04%4d:%3d", (int)(Broodwar->getFrameCount() / (23.8 * 60)), (int)((int)(Broodwar->getFrameCount() / 23.8) % 60));
}

void UXManager::drawAPM(int x, int y)
{
	int bwapiAPM = Broodwar->getAPM();
	Broodwar->drawTextScreen(x, y, "APM : %d", bwapiAPM);
}

void UXManager::drawPlayers()
{
	Playerset players = Broodwar->getPlayers();

	for (auto p : players)
		Broodwar << "Player [" << p->getID() << "]: " << p->getName() << " is in force: " << p->getForce()->getName() << endl;
}

void UXManager::drawForces()
{
	Forceset forces = Broodwar->getForces();

	for (auto f : forces)
	{
		Playerset players = f->getPlayers();
		Broodwar << "Force " << f->getName() << " has the following players:" << endl;

		for (auto p : players)
			Broodwar << "  - Player [" << p->getID() << "]: " << p->getName() << endl;
	}
}

void UXManager::drawBuildOrderQueueOnScreen(int x, int y)
{
	Broodwar->drawTextScreen(x, y, "\x04 <Build Order>");

	/*
	deque< BuildOrderItem >* queue = BuildManager::Instance().buildQueue.getQueue();
	size_t reps = queue->size() < 24 ? queue->size() : 24;
	for (size_t i(0); i<reps; i++) {
		const MetaType & type = (*queue)[queue->size() - 1 - i].metaType;
		Broodwar->drawTextScreen(x, y + 10 + (i * 10), " %s", type.getName().c_str());
	}
	*/

	const deque<BuildOrderItem> *buildQueue = BuildManager::Instance().buildQueue.getQueue();
	int itemCount = 0;

	for (auto itr = buildQueue->rbegin(); itr != buildQueue->rend(); itr++) {
		BuildOrderItem currentItem = *itr;
		Broodwar->drawTextScreen(x, y + 10 + (itemCount * 10), " %s %d", currentItem.metaType.getName().c_str(), currentItem.blocking);
		itemCount++;

		if (itemCount >= 24) break;
	}
}


void UXManager::drawBuildStatusOnScreen(int x, int y)
{
	// 건설 / 훈련 중인 유닛 진행상황 표시
	vector<Unit> unitsUnderConstruction;

	for (auto &unit : S->getUnits())
	{
		if (unit != nullptr && unit->isBeingConstructed())
		{
			unitsUnderConstruction.push_back(unit);
		}
	}

	// sort it based on the time it was started
	sort(unitsUnderConstruction.begin(), unitsUnderConstruction.end(), CompareWhenStarted());

	Broodwar->drawTextScreen(x, y, "\x04 <Build Status>");

	size_t reps = unitsUnderConstruction.size() < 10 ? unitsUnderConstruction.size() : 10;

	string prefix = "\x07";

	for (auto &unit : unitsUnderConstruction)
	{
		y += 10;
		UnitType t = unit->getType();

		if (t == UnitTypes::Zerg_Egg)
		{
			t = unit->getBuildType();
		}

		Broodwar->drawTextScreen(x, y, " %s%s (%d)", prefix.c_str(), t.getName().c_str(), unit->getRemainingBuildTime());
	}

	// Tech Research 표시

	// Upgrade 표시

}


void UXManager::drawConstructionQueueOnScreenAndMap(int x, int y)
{
	Broodwar->drawTextScreen(x, y, "\x04 <Construction Status>");

	int yspace = 0;

	const vector<ConstructionTask> *constructionQueue = ConstructionManager::Instance().getConstructionQueue();

	for (const auto &b : *constructionQueue)
	{
		string constructionState = "";

		if (b.status == ConstructionStatus::Unassigned)
		{
			Broodwar->drawTextScreen(x, y + 10 + ((yspace) * 10), "\x03 %s - No Worker", b.type.getName().c_str());
		}
		else if (b.status == ConstructionStatus::Assigned)
		{
			if (b.constructionWorker == nullptr) {
				Broodwar->drawTextScreen(x, y + 10 + ((yspace) * 10), "\x03 %s - Assigned Worker Null", b.type.getName().c_str());
			}
			else {
				Broodwar->drawTextScreen(x, y + 10 + ((yspace) * 10), "\x03 %s - Assigned Worker %d, Position (%d,%d)", b.type.getName().c_str(), b.constructionWorker->getID(), b.finalPosition.x, b.finalPosition.y);
			}

			int x1 = b.finalPosition.x * 32;
			int y1 = b.finalPosition.y * 32;
			int x2 = (b.finalPosition.x + b.type.tileWidth()) * 32;
			int y2 = (b.finalPosition.y + b.type.tileHeight()) * 32;

			Broodwar->drawLineMap(b.constructionWorker->getPosition().x, b.constructionWorker->getPosition().y, (x1 + x2) / 2, (y1 + y2) / 2, Colors::Orange);
			Broodwar->drawBoxMap(x1, y1, x2, y2, Colors::Red, false);
		}
		else if (b.status == ConstructionStatus::WaitToAssign) {
			Broodwar->drawTextScreen(x, y + 10 + ((yspace) * 10), "\x03 %s - Wait To Assign", b.type.getName().c_str());
		}
		else if (b.status == ConstructionStatus::UnderConstruction)
		{
			Broodwar->drawTextScreen(x, y + 10 + ((yspace) * 10), "\x03 %s - Under Construction id : %d", b.type.getName().c_str(), b.constructionWorker->getID());
		}

		yspace++;
	}
}



void UXManager::drawUnitIdOnMap() {
	for (auto &unit : S->getUnits())
	{
		Broodwar->drawTextMap(unit->getPosition().x, unit->getPosition().y - 12, "\x07%d", unit->getID());
	}

	for (auto &unit : E->getUnits())
	{
		Broodwar->drawTextMap(unit->getPosition().x, unit->getPosition().y - 12, "\x06%d", unit->getID());
	}

	if (MapDrawer::showHide) {
		for (auto &unit : INFO.getUnits(E)) {
			if (unit.second->isHide() && unit.second->pos() != Positions::Unknown)
				Broodwar->drawTextMap(unit.second->pos().x, unit.second->pos().y - 12, "\x01%d", unit.second->id());
		}

		for (auto &unit : INFO.getBuildings(E)) {
			if (unit.second->isHide() && unit.second->pos() != Positions::Unknown)
				Broodwar->drawTextMap(unit.second->pos().x, unit.second->pos().y - 12, "\x01%d", unit.second->id());
		}
	}

	for (auto &unit : BWAPI::Broodwar->neutral()->getUnits())
	{
		BWAPI::Broodwar->drawTextMap(unit->getPosition().x, unit->getPosition().y - 12, "\x04%d", unit->getID());
	}
}

void UXManager::drawHideUnitOnMap() {
	int mouseX = Broodwar->getMousePosition().x + Broodwar->getScreenPosition().x;
	int mouseY = Broodwar->getMousePosition().y + Broodwar->getScreenPosition().y;

	int gap = 0;

	for (auto a : bw->getUnitsOnTile(TilePosition(mouseX / 32, mouseY / 32))) {
		Broodwar->drawTextMap(mouseX - 150, mouseY + gap, "%s", a->getType().c_str());

		gap += 18;
	}

	for (auto &unit : INFO.getUnits(E)) {
		if (unit.second->isHide() && unit.second->pos() != Positions::Unknown)
			Broodwar->drawTextMap(unit.second->pos().x, unit.second->pos().y - 12, "\x01%s", unit.second->type().c_str());
	}

	for (auto &unit : INFO.getBuildings(E)) {
		if (unit.second->isHide() && unit.second->pos() != Positions::Unknown)
			Broodwar->drawTextMap(unit.second->pos().x, unit.second->pos().y - 12, "\x01%s", unit.second->type().c_str());
	}
}

void UXManager::drawWorkerCountOnMap()
{
	uList center = INFO.getUnitData(S).getBuildingVector(Terran_Command_Center);

	for (auto c : center)
	{
		if (c == nullptr || !c->isComplete() || c->getLift())
			continue;

		int scvCnt = ScvManager::Instance().getAssignedScvCount(c->unit());
		int x = c->pos().x - 64;
		int y = c->pos().y - 32;

		Broodwar->drawTextMap(x, y, "\x04 Workers: %d", scvCnt);
	}

	map<Unit, int>::iterator iter;

	for (iter = ScvManager::Instance().getMineralScvCountMap().begin(); iter != ScvManager::Instance().getMineralScvCountMap().end(); iter++)
	{
		int x = iter->first->getPosition().x;
		int y = iter->first->getPosition().y;
		Broodwar->drawTextMap(x, y, "Scv Cnt: %d", iter->second);
	}

	for (iter = ScvManager::Instance().getRefineryScvCountMap().begin(); iter != ScvManager::Instance().getRefineryScvCountMap().end(); iter++)
	{
		int x = iter->first->getPosition().x;
		int y = iter->first->getPosition().y;
		Broodwar->drawTextMap(x, y, "Scv Cnt: %d", iter->second);
	}
}

void UXManager::drawScoutInformation(int x, int y)
{
	// get the enemy base location, if we have one
	const BWEM::Base *enemyBaseLocation = INFO.getMainBaseLocation(E);

	if (enemyBaseLocation != nullptr) {
		if (INFO.isEnemyBaseFound)
			Broodwar->drawTextScreen(x, y, "Enemy MainBaseLocation : (%d, %d) checked!", enemyBaseLocation->getTilePosition().x, enemyBaseLocation->getTilePosition().y);
		else
			Broodwar->drawTextScreen(x, y, "Enemy MainBaseLocation : (%d, %d)", enemyBaseLocation->getTilePosition().x, enemyBaseLocation->getTilePosition().y);
	}
	else {
		Broodwar->drawTextScreen(x, y, "Enemy MainBaseLocation : Unknown");
	}

	int currentScoutStatus = ScoutManager::Instance().getScoutStatus();
	string scoutStatusString;

	switch (currentScoutStatus) {
		case ScoutStatus::MovingToAnotherBaseLocation:
			scoutStatusString = "Moving To Another Base Location";
			break;

		case ScoutStatus::MoveAroundEnemyBaseLocation:
			scoutStatusString = "Move Around Enemy BaseLocation";
			break;

		case ScoutStatus::FinishFirstScout:
			scoutStatusString = "Finished First Scout";
			break;

		case ScoutStatus::LookAroundMyMainBase:
			scoutStatusString = "Look Around My Main Base";
			break;

		case ScoutStatus::WaitAtMySecondChokePoint:
			scoutStatusString = "Wait At My Second ChokePoint";
			break;

		case ScoutStatus::CheckEnemyFirstExpansion:
			scoutStatusString = "Check Enemy First Expansion";
			break;

		case ScoutStatus::WaitAtEnemyFirstExpansion:
			scoutStatusString = "Wait At Enemy First Expansion";
			break;

		case ScoutStatus::AssignScout:
			scoutStatusString = "Assign Scout";
			break;

		case ScoutStatus::NoScout:
			scoutStatusString = "No Scout";
			break;

		default:
			scoutStatusString = "No Scout";
			break;
	}

	Broodwar->drawTextScreen(x, y + 10, "%s", scoutStatusString.c_str());

	Unit scoutUnit = ScoutManager::Instance().getScoutUnit();

	if (scoutUnit) {

		Broodwar->drawTextScreen(x, y + 20, "Scout Unit : %s %d (%d, %d)", scoutUnit->getType().getName().c_str(), scoutUnit->getID(), scoutUnit->getTilePosition().x, scoutUnit->getTilePosition().y);

		Position scoutMoveTo = scoutUnit->getTargetPosition();

		if (scoutMoveTo && scoutMoveTo != Positions::None && scoutMoveTo.isValid()) {

			double currentScoutTargetDistance;

			if (currentScoutStatus == ScoutStatus::MovingToAnotherBaseLocation) {

				if (scoutUnit->getType().isFlyer()) {
					currentScoutTargetDistance = (int)(scoutUnit->getPosition().getDistance(scoutMoveTo));
				}
				else {
					currentScoutTargetDistance = scoutUnit->getTilePosition().getApproxDistance(TilePosition(scoutMoveTo.x / TILE_SIZE, scoutMoveTo.y / TILE_SIZE));
				}

				Broodwar->drawTextScreen(x, y + 30, "Target = (%d, %d) Distance = %4.0f",
										 scoutMoveTo.x / TILE_SIZE, scoutMoveTo.y / TILE_SIZE,
										 currentScoutTargetDistance);

			}

			/*
			else if (currentScoutStatus == ScoutStatus::MoveAroundEnemyBaseLocation) {

			vector<Position> vertices = ScoutManager::Instance().getEnemyRegionVertices();
			for (size_t i(0); i < vertices.size(); ++i)
			{
			Broodwar->drawCircleMap(vertices[i], 4, Colors::Green, false);
			Broodwar->drawTextMap(vertices[i], "%d", i);
			}

			Broodwar->drawCircleMap(scoutMoveTo, 5, Colors::Red, true);
			}
			*/
		}
	}

	bw->setTextSize(Text::Size::Small);
	bw->drawTextScreen(450, 16, "%cSelf Strategy : %s", Text::Yellow, SM.getMainStrategy().c_str());

	if (SM.getMainAttackPosition() == Positions::Unknown)
		bw->drawTextScreen(450, 28, "%cMainAttackPosition : Unknown", Text::Yellow);
	else
		bw->drawTextScreen(450, 28, "%cMainAttackPosition : (%d, %d)", Text::Yellow, SM.getMainAttackPosition().x / 32, SM.getMainAttackPosition().y / 32);

	if (SM.getSecondAttackPosition() == Positions::Unknown)
		bw->drawTextScreen(450, 40, "%cSecondAttackPosition : Unknown", Text::Yellow);
	else
		bw->drawTextScreen(450, 40, "%cSecondAttackPosition : (%d, %d)", Text::Yellow, SM.getSecondAttackPosition().x / 32, SM.getSecondAttackPosition().y / 32);

	if (SM.getDrawLinePosition() == Positions::Unknown)
		bw->drawTextScreen(450, 52, "%cDrawLinePosition : Unknown", Text::Yellow);
	else
		bw->drawTextScreen(450, 52, "%cDrawLinePosition : (%d, %d)", Text::Yellow, SM.getDrawLinePosition().x / 32, SM.getDrawLinePosition().y / 32);

	if (SM.getWaitLinePosition() == Positions::Unknown)
		bw->drawTextScreen(450, 64, "%cWaitLinePosition : Unknown", Text::Yellow);
	else
		bw->drawTextScreen(450, 64, "%cWaitLinePosition : (%d, %d)", Text::Yellow, SM.getWaitLinePosition().x / 32, SM.getWaitLinePosition().y / 32);

	bw->drawTextScreen(450, 76, "%cEnemy Initialbuild : %s", Text::Yellow, ESM.getEnemyInitialBuild().c_str());
	bw->drawTextScreen(450, 88, "%cEnemy Mainbuild : %s", Text::Yellow, ESM.getEnemyMainBuild().c_str());

	bw->setTextSize(Text::Size::Large);
	if (SHM.getWinningRate() > 0)
		bw->drawTextScreen(450, 106, "%cWinning Rate : %.1f%%", Text::Yellow, SHM.getWinningRate());
	bw->setTextSize();   // Set text size back to default
}

void UXManager::drawUnitStatus()
{
	for (auto &u : INFO.getUnits(S)) {
		if (u.second->type() == Terran_Vulture_Spider_Mine)
			continue;

		if (!Config::Debug::DrawUnitStatus && !u.first->isSelected())
			continue;

		int x = u.second->pos().x;
		int y = u.second->pos().y;

		if (theMap.Size().x - x / 32 < 5)
			x -= 100;

		if (theMap.Size().y - y / 32 < 6)
			y -= 40;

		if (!Config::Debug::DrawLastCommandInfo) {
			Broodwar->drawTextMap(x, y, "State : %s", u.second->getState().c_str());
		}
		else if (Config::Debug::DrawLastCommandInfo || u.first->isSelected()) {
			if (u.first->getLastCommand().getTarget())
				Broodwar->drawTextMap(x, y, "State : %s\nCmd : %s, targetId : %d, frame : %d", u.second->getState().c_str(), u.first->getLastCommand().getType().c_str(), u.first->getLastCommand().getTarget()->getID(), u.first->getLastCommandFrame());
			else if (u.first->getLastCommand().getTargetTilePosition().isValid())
				Broodwar->drawTextMap(x, y, "State : %s\nCmd : %s(%d, %d), frame : %d", u.second->getState().c_str(), u.first->getLastCommand().getType().c_str(), u.first->getLastCommand().getTargetTilePosition().x, u.first->getLastCommand().getTargetTilePosition().y, u.first->getLastCommandFrame());
			else
				Broodwar->drawTextMap(x, y, "State : %s\nCmd : %s, frame : %d", u.second->getState().c_str(), u.first->getLastCommand().getType().c_str(), u.first->getLastCommandFrame());
		}
	}

	for (auto &u : INFO.getBuildings(S)) {
		int x = u.second->pos().x - 32;
		int y = u.second->pos().y + 15;

		if (!Config::Debug::DrawUnitStatus && !u.first->isSelected())
			continue;

		if (!Config::Debug::DrawLastCommandInfo)
			Broodwar->drawTextMap(x, y, "State : %s", u.second->getState().c_str());
		else if (Config::Debug::DrawLastCommandInfo || u.first->isSelected()) {
			if (u.first->getLastCommand().getTarget())
				Broodwar->drawTextMap(x, y, "State : %s\nCmd : %s, targetId : %d, frame : %d", u.second->getState().c_str(), u.first->getLastCommand().getType().c_str(), u.first->getLastCommand().getTarget()->getID(), u.first->getLastCommandFrame());
			else if (u.first->getLastCommand().getTargetTilePosition().isValid())
				Broodwar->drawTextMap(x, y, "State : %s\nCmd : %s(%d, %d), frame : %d", u.second->getState().c_str(), u.first->getLastCommand().getType().c_str(), u.first->getLastCommand().getTargetTilePosition().x, u.first->getLastCommand().getTargetTilePosition().y, u.first->getLastCommandFrame());
			else
				Broodwar->drawTextMap(x, y, "State : %s\nCmd : %s, frame : %d", u.second->getState().c_str(), u.first->getLastCommand().getType().c_str(), u.first->getLastCommandFrame());
		}
	}
}

void UXManager::drawUnitTargetOnMap()
{
	for (auto &unit : S->getUnits())
	{
		if (unit != nullptr && unit->isCompleted() && !unit->getType().isBuilding() && !unit->getType().isWorker())
		{
			Unit targetUnit = unit->getTarget();

			if (targetUnit != nullptr && targetUnit->getPlayer() != S) {
				Broodwar->drawCircleMap(unit->getPosition(), dotRadius, Colors::Red, true);
				Broodwar->drawCircleMap(targetUnit->getTargetPosition(), dotRadius, Colors::Red, true);
				Broodwar->drawLineMap(unit->getPosition(), targetUnit->getTargetPosition(), Colors::Red);
			}
			else if (unit->isMoving()) {
				Broodwar->drawCircleMap(unit->getPosition(), dotRadius, Colors::Orange, true);
				Broodwar->drawCircleMap(unit->getTargetPosition(), dotRadius, Colors::Orange, true);
				Broodwar->drawLineMap(unit->getPosition(), unit->getTargetPosition(), Colors::Orange);
			}

		}
	}
}

// Bullet 을 Line 과 Text 로 표시한다. Cloaking Unit 의 Bullet 표시에 쓰인다
void UXManager::drawBulletsOnMap()
{
	for (auto &b : Broodwar->getBullets())
	{
		Position p = b->getPosition();
		double velocityX = b->getVelocityX();
		double velocityY = b->getVelocityY();

		// 아군 것이면 녹색, 적군 것이면 빨간색
		Broodwar->drawLineMap(p, p + Position((int)velocityX, (int)velocityY), b->getPlayer() == S ? Colors::Green : Colors::Red);

		Broodwar->drawTextMap(p, "%c%s", b->getPlayer() == S ? Text::Green : Text::Red, b->getType().c_str());
	}
}

void MyBot::UXManager::drawBWEMResultOnMap(const Map &theMap)
{
	if (MapDrawer::showFrontier)
		for (auto &f : theMap.RawFrontier())
			bw->drawBoxMap(Position(f.second), Position(f.second + 1), MapDrawer::Color::frontier, bool("isSolid"));

	for (int y = 0; y < theMap.Size().y; ++y)
		for (int x = 0; x < theMap.Size().x; ++x)
		{
			TilePosition t(x, y);
			const Tile &tile = theMap.GetTile(t, check_t::no_check);

			if (MapDrawer::showUnbuildable && !tile.Buildable()) {
				// 좌측 상단 시작점인지 검사
				bool isLeftTop = false;

				if (x == 0 || y == 0) {
					isLeftTop = true;
				}
				else {
					const Tile &beforeTile = theMap.GetTile(t - 1, check_t::no_check);

					isLeftTop = beforeTile.Buildable();
				}

				// 우측 하단 끝점까지 찾아서 그리기
				if (isLeftTop) {
					int i;

					for (i = 1; x + i < theMap.Size().x && y + i < theMap.Size().y && !theMap.GetTile(t + i, check_t::no_check).Buildable(); i++);

					BWAPI_ext::drawLineMap(Position(t), Position(t + i), MapDrawer::Color::unbuildable);
				}

				// 우측 상단 시작점인지 검사
				bool isRightTop = false;

				if (x == 0 || y == theMap.Size().y - 1) {
					isRightTop = true;
				}
				else {
					const Tile &beforeTile = theMap.GetTile(TilePosition(x - 1, y + 1), check_t::no_check);

					isRightTop = beforeTile.Buildable();
				}

				// 좌측 하단 끝점까지 찾아서 그리기
				if (isRightTop) {
					int i;

					for (i = 1; x + i < theMap.Size().x && y - i > 0 && !theMap.GetTile(TilePosition(x + i, y - i), check_t::no_check).Buildable(); i++);

					BWAPI_ext::drawLineMap(Position(TilePosition(t.x, t.y + 1)), Position(TilePosition(x + i, y - i + 1)), MapDrawer::Color::unbuildable);
				}
			}

			if (MapDrawer::showGroundHeight && (tile.GroundHeight() > 0))
			{
				auto col = tile.GroundHeight() == 1 ? MapDrawer::Color::highGround : MapDrawer::Color::veryHighGround;

				//bw->drawBoxMap(Position(t), Position(t)+6, col, bool("isSolid"));
				if (tile.Doodad()) bw->drawTriangleMap(center(t) + Position(0, 5), center(t) + Position(-3, 2), center(t) + Position(+3, 2), Colors::White);
			}
		}

	for (int y = 0; y < theMap.WalkSize().y; ++y)
		for (int x = 0; x < theMap.WalkSize().x; ++x)
		{
			WalkPosition w(x, y);
			const MiniTile &miniTile = theMap.GetMiniTile(w, check_t::no_check);

			if (MapDrawer::showSeas && miniTile.Sea()) {
				// 좌측 상단 시작점인지 검사
				bool isLeftTop = false;

				if (x == 0 || y == 0) {
					isLeftTop = true;
				}
				else {
					const MiniTile &beforeMiniTile = theMap.GetMiniTile(w - 1, check_t::no_check);

					isLeftTop = !beforeMiniTile.Sea();
				}

				// 우측 하단 끝점까지 찾아서 그리기
				if (isLeftTop) {
					int i;

					for (i = 1; x + i < theMap.WalkSize().x && y + i < theMap.WalkSize().y && theMap.GetMiniTile(w + i, check_t::no_check).Sea(); i++);

					BWAPI_ext::drawLineMap(Position(w), Position(w + i), MapDrawer::Color::sea);
				}

				// 우측 상단 시작점인지 검사
				bool isRightTop = false;

				if (x == 0 || y == theMap.WalkSize().y - 1) {
					isRightTop = true;
				}
				else {
					const MiniTile &beforeMiniTile = theMap.GetMiniTile(WalkPosition(x - 1, y + 1), check_t::no_check);

					isRightTop = !beforeMiniTile.Sea();
				}

				// 좌측 하단 끝점까지 찾아서 그리기
				if (isRightTop) {
					int i;

					for (i = 1; x + i < theMap.WalkSize().x && y - i > 0 && theMap.GetMiniTile(WalkPosition(x + i, y - i), check_t::no_check).Sea(); i++);

					BWAPI_ext::drawLineMap(Position(WalkPosition(w.x, w.y + 1)), Position(WalkPosition(x + i, y - i + 1)), MapDrawer::Color::sea);
				}
			}
			else if (MapDrawer::showLakes && miniTile.Lake())
				drawDiagonalCrossMap(Position(w), Position(w + 1), MapDrawer::Color::lakes);
		}

	if (MapDrawer::showCP) {
		for (const Area &area : theMap.Areas())
			for (const ChokePoint *cp : area.ChokePoints())
				for (ChokePoint::node end : {
							ChokePoint::end1, ChokePoint::end2
						})
					bw->drawLineMap(Position(cp->Pos(ChokePoint::middle)), Position(cp->Pos(end)), MapDrawer::Color::cp);

		if (INFO.getFirstChokePoint(S) != nullptr) {
			Broodwar->drawTextMap(INFO.getFirstChokePosition(S), "My First ChokePoint");
		}

		if (INFO.getSecondChokePoint(S) != nullptr) {
			Broodwar->drawTextMap(INFO.getSecondChokePosition(S), "My Second ChokePoint");
		}

		if (INFO.getFirstChokePoint(E) != nullptr) {
			Broodwar->drawTextMap(INFO.getFirstChokePosition(E), "Enemy First ChokePoint");
		}

		if (INFO.getSecondChokePoint(E) != nullptr) {
			Broodwar->drawTextMap(INFO.getSecondChokePosition(E), "Enemy Second ChokePoint");
		}
	}

	if (MapDrawer::showMinerals)
		for (auto &m : theMap.Minerals())
		{
			bw->drawBoxMap(Position(m->TopLeft()), Position(m->TopLeft() + m->Size()), MapDrawer::Color::minerals);

			if (m->Blocking())
				drawDiagonalCrossMap(Position(m->TopLeft()), Position(m->TopLeft() + m->Size()), MapDrawer::Color::minerals);
		}

	if (MapDrawer::showGeysers)
		for (auto &g : theMap.Geysers())
			bw->drawBoxMap(Position(g->TopLeft()), Position(g->TopLeft() + g->Size()), MapDrawer::Color::geysers);

	if (MapDrawer::showStaticBuildings)
		for (auto &s : theMap.StaticBuildings())
		{
			bw->drawBoxMap(Position(s->TopLeft()), Position(s->TopLeft() + s->Size()), MapDrawer::Color::staticBuildings);

			if (s->Blocking())
				drawDiagonalCrossMap(Position(s->TopLeft()), Position(s->TopLeft() + s->Size()), MapDrawer::Color::staticBuildings);
		}

	for (const Area &area : theMap.Areas())
		for (const Base &base : area.Bases())
		{
			if (MapDrawer::showBases) {
				bw->drawBoxMap(Position(base.Location()), Position(base.Location() + UnitType(Terran_Command_Center).tileSize()), MapDrawer::Color::bases);

				if (INFO.getMainBaseLocation(S) != nullptr) {
					Broodwar->drawTextMap(INFO.getMainBaseLocation(S)->getPosition(), "My MainBaseLocation");
				}

				if (INFO.getFirstExpansionLocation(S) != nullptr) {
					Broodwar->drawTextMap(INFO.getFirstExpansionLocation(S)->getPosition(), "My First ExpansionLocation");
				}

				if (INFO.getMainBaseLocation(E) != nullptr) {
					Broodwar->drawTextMap(INFO.getMainBaseLocation(E)->getPosition(), "Enemy MainBaseLocation");
				}

				if (INFO.getFirstExpansionLocation(E) != nullptr) {
					Broodwar->drawTextMap(INFO.getFirstExpansionLocation(E)->getPosition(), "Enemy First ExpansionLocation");
				}
			}

			if (MapDrawer::showAssignedRessources)
			{
				vector<Ressource *> AssignedRessources(base.Minerals().begin(), base.Minerals().end());
				AssignedRessources.insert(AssignedRessources.end(), base.Geysers().begin(), base.Geysers().end());

				for (const Ressource *r : AssignedRessources)
					bw->drawLineMap(base.Center(), r->Pos(), MapDrawer::Color::assignedRessources);
			}
		}

	if (MapDrawer::showAreas)
		for (const Area &area : theMap.Areas()) {
			TilePosition t = (area.TopLeft() + area.BottomRight()) / 2;
			bw->drawBoxMap(Position(area.TopLeft().x * 32, area.TopLeft().y * 32), Position((area.BottomRight().x + 1) * 32, (area.BottomRight().y + 1) * 32), MapDrawer::Color::areas);
			bw->drawTextMap(Position(t.x * 32, t.y * 32), "areaId : %d\n(altitude : %d, size : %d)", area.Id(), area.MaxAltitude(), area.MiniTiles());
		}

	/*if (MapDrawer::showSmallAreas)
	{
		GridArea *gridArea = INFO.getCenterArea();

		int N = gridArea->getGridAreaVec().size();

		TilePosition tl, br;

		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
			{
				tl = gridArea->getGridArea(i, j).topLeft();
				br = gridArea->getGridArea(i, j).bottomRight();

				Position center = (Position(tl) + Position(br)) / 2;
				int radius = min((Position(br).x - Position(tl).x) / 2, (Position(br).y - Position(tl).y) / 2);

				BWAPI::Color color = BWAPI::Colors::Purple;

				switch (gridArea->getGridArea(i, j).areaStatus())
				{
					case AreaStatus::SelfArea:
						color = BWAPI::Colors::Green;
						break;

					case AreaStatus::EnemyArea:
						color = BWAPI::Colors::Blue;
						break;

					case AreaStatus::CombatArea:
						color = BWAPI::Colors::Red;
						break;
				}

				Broodwar->drawCircleMap(center, radius, color);
				Broodwar->drawBoxMap(Position(tl), Position(br), color);
				Broodwar->drawTextMap(center, "(%d, %d) : %d mines", i, j, gridArea->getGridArea(i, j).mineCount());

				center.y += 15;
				Broodwar->drawTextMap(center, "myU = %d, enU = %d", gridArea->getGridArea(i, j).myUcnt(), gridArea->getGridArea(i, j).enUcnt());

				center.y += 15;
				Broodwar->drawTextMap(center, "myB = %d, enB = %d", gridArea->getGridArea(i, j).myBcnt(), gridArea->getGridArea(i, j).enBcnt());
			}
		}

		vector<GridAreaCell *> eb = gridArea->getEnemyBoundary();

		for (auto cell : eb)
		{
			Position ptl(cell->topLeft());
			Position pbr(cell->bottomRight());
			Broodwar->drawLineMap(ptl, pbr, BWAPI::Colors::Yellow);
			Broodwar->drawLineMap(Position(pbr.x, ptl.y), Position(ptl.x, pbr.y), BWAPI::Colors::Yellow);
		}
	}*/

	if (MapDrawer::showSupply)
		TerranConstructionPlaceFinder::Instance().drawMap();

	if (MapDrawer::showReserved) {
		// 건물 건설 장소 예약 지점
		vector< Building > _reserveList = ReserveBuilding::Instance().getReserveList();

		for (Building b : _reserveList) {
			bw->drawTextMap(Position(b.TopLeft().x * 32 + 1, b.getTilePosition().y * 32), "%s", b.toString().c_str());
			bw->drawBoxMap((Position)b.TopLeft(), (Position)(b.BottomRight()), MapDrawer::Color::reserved);

			for (auto space : b.getSpaceList()) {
				bw->drawBoxMap((Position)space, (Position)(space + TilePosition(1, 1)), MapDrawer::Color::reserved);
			}
		}

		vector< Building > _avoidList = ReserveBuilding::Instance().getAvoidList();

		for (Building b : _avoidList) {
			bw->drawTextMap(Position(b.TopLeft().x * 32 + 1, b.getTilePosition().y * 32), "%s", b.toString().c_str());
			bw->drawBoxMap((Position)b.TopLeft(), (Position)(b.BottomRight()), MapDrawer::Color::reserved);
		}

		TerranConstructionPlaceFinder::Instance().drawSecondChokePointReserveBuilding();
	}

	if (MapDrawer::showStatus) {
		drawGameInformationOnScreen(5, 5);
		drawBuildOrderQueueOnScreen(80, 60);
		drawBuildStatusOnScreen(200, 60);
		drawConstructionQueueOnScreenAndMap(200, 150);
		drawWorkerCountOnMap();
	}

	if (MapDrawer::showId) {
		drawUnitIdOnMap();
	}
	else {
		if (MapDrawer::showHide) {
			drawHideUnitOnMap();
		}
	}
}