#include "BuildOrderManager.h"
#include "../StormbreakerUtil.h"

using namespace MyBot;

BuildOrderQueue::BuildOrderQueue()
	: highestPriority(0),
	  lowestPriority(0),
	  defaultPrioritySpacing(10),
	  numSkippedItems(0)
{

}

void BuildOrderQueue::clearAll()
{

	queue.clear();

	
	highestPriority = 0;
	lowestPriority = 0;
}

bool BuildOrderQueue::removeQueue(UnitType _unitType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy, bool isSame) {
	bool removed = false;

	for (auto iter = queue.begin(); iter != queue.end(); iter++) {
		if (isSame && iter->metaType.getUnitType() == _unitType && iter->seedLocationStrategy == _seedPositionStrategy) {
			removed = true;
			
			queue.erase(iter);
			break;
		}
		else if (!isSame && iter->metaType.getUnitType() == _unitType && iter->seedLocationStrategy != _seedPositionStrategy) {
			removed = true;
			queue.erase(iter);
			break;
		}
	}

	if (removed) {
		
		highestPriority = queue.empty() ? 0 : queue.back().priority;
		lowestPriority = queue.empty() ? 0 : lowestPriority;
	}

	return removed;
}

BuildOrderItem BuildOrderQueue::getHighestPriorityItem()
{

	numSkippedItems = 0;

	
	return queue.back();
}

BuildOrderItem BuildOrderQueue::getNextItem()
{
	assert(queue.size() - 1 - numSkippedItems >= 0);

	
	return queue[queue.size() - 1 - numSkippedItems];
}


int BuildOrderQueue::getItemCount(MetaType queryType, TilePosition queryTilePosition)
{
	
	int maxRange = 16;

	int itemCount = 0;

	size_t reps = queue.size();

	
	for (size_t i(0); i < reps; i++) {

		const MetaType &item = queue[i].metaType;
		TilePosition itemPosition = queue[i].seedLocation;
		BuildOrderItem::SeedPositionStrategy itemPositionStrategy = queue[i].seedLocationStrategy;

		if (queryType.isUnit() && item.isUnit()) {
			if (item.getUnitType() == queryType.getUnitType())
			{
				if (queryType.getUnitType().isBuilding() && queryTilePosition.isValid())
				{
					if (itemPosition.isValid()) {
						if (itemPosition.getApproxDistance(queryTilePosition) <= maxRange) {
							itemCount++;
						}
					}
					else {
						if (itemPositionStrategy == BuildOrderItem::SeedPositionStrategy::MainBaseLocation) {
							if (isSameArea(queryTilePosition, INFO.getMainBaseLocation(S)->getTilePosition())) {
								itemCount++;
							}
						}
						else if (itemPositionStrategy == BuildOrderItem::SeedPositionStrategy::FirstChokePoint) {
							if (INFO.getFirstChokePoint(S) && ((TilePosition)INFO.getFirstChokePosition(S)).getApproxDistance(queryTilePosition) <= maxRange) {
								itemCount++;
							}
						}
						else if (itemPositionStrategy == BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation) {
							if (INFO.getFirstExpansionLocation(S) && isSameArea(queryTilePosition, INFO.getFirstExpansionLocation(S)->getTilePosition())) {
								itemCount++;
							}
						}
						else if (itemPositionStrategy == BuildOrderItem::SeedPositionStrategy::SecondChokePoint) {
							if (INFO.getSecondChokePoint(S) && ((TilePosition)INFO.getSecondChokePosition(S)).getApproxDistance(queryTilePosition) <= maxRange) {
								itemCount++;
							}
						}
						else if (itemPositionStrategy == BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation) {
							if (INFO.getSecondExpansionLocation(S) && isSameArea(queryTilePosition, INFO.getSecondExpansionLocation(S)->getTilePosition())) {
								itemCount++;
							}
						}
						else if (itemPositionStrategy == BuildOrderItem::SeedPositionStrategy::ThirdExpansionLocation) {
							if (INFO.getThirdExpansionLocation(S) && isSameArea(queryTilePosition, INFO.getThirdExpansionLocation(S)->getTilePosition())) {
								itemCount++;
							}
						}
					}
				}
				else
				{
					itemCount++;
				}
			}
		}
		else if (queryType.isTech() && item.isTech()) {
			if (item.getTechType() == queryType.getTechType()) {
				itemCount++;
			}
		}
		else if (queryType.isUpgrade() && item.isUpgrade()) {
			if (item.getUpgradeType() == queryType.getUpgradeType()) {
				itemCount++;
			}
		}
	}

	return itemCount;
}
int BuildOrderQueue::getItemCount(UnitType         _unitType, TilePosition queryTilePosition)
{
	return getItemCount(MetaType(_unitType), queryTilePosition);
}

int BuildOrderQueue::getItemCount(TechType         _techType)
{
	return getItemCount(MetaType(_techType));
}

int BuildOrderQueue::getItemCount(UpgradeType      _upgradeType)
{
	return getItemCount(MetaType(_upgradeType));
}


bool BuildOrderQueue::existItem(MetaType queryType, TilePosition queryTilePosition)
{
	
	int maxRange = 16;

	size_t reps = queue.size();

	
	for (size_t i(0); i < reps; i++) {

		const MetaType &item = queue[i].metaType;
		TilePosition itemPosition = queue[i].seedLocation;

		if (queryType.isUnit() && item.isUnit()) {
			if (item.getUnitType() == queryType.getUnitType())
			{
				if (queryType.getUnitType().isBuilding() && queryTilePosition != TilePositions::None)
				{
					if (itemPosition.getApproxDistance(queryTilePosition) <= maxRange) {
						return true;
					}
				}
				else
				{
					return true;
				}
			}
		}
		else if (queryType.isTech() && item.isTech()) {
			if (item.getTechType() == queryType.getTechType()) {
				return true;
			}
		}
		else if (queryType.isUpgrade() && item.isUpgrade()) {
			if (item.getUpgradeType() == queryType.getUpgradeType()) {
				return true;
			}
		}
	}

	return false;
}
bool BuildOrderQueue::existItem(UnitType         _unitType, TilePosition queryTilePosition)
{
	return existItem(MetaType(_unitType), queryTilePosition);
}

bool BuildOrderQueue::existItem(TechType         _techType)
{
	return existItem(MetaType(_techType));
}

bool BuildOrderQueue::existItem(UpgradeType      _upgradeType)
{
	return existItem(MetaType(_upgradeType));
}

void BuildOrderQueue::skipCurrentItem()
{
	
	if (canSkipCurrentItem()) {
		
		numSkippedItems++;
	}
}

bool BuildOrderQueue::canSkipCurrentItem()
{
	
	bool bigEnough = queue.size() > (size_t)(1 + numSkippedItems);

	if (!bigEnough)
	{
		return false;
	}

	
	bool highestNotBlocking = !queue[queue.size() - 1 - numSkippedItems].blocking;

	
	return highestNotBlocking;
}

void BuildOrderQueue::queueItem(BuildOrderItem b)
{

	if (getItemCount(b.metaType) > 6) {
		return;
	}

	
	if (queue.empty())
	{
		highestPriority = b.priority;
		lowestPriority = b.priority;
	}

	
	if (b.priority <= lowestPriority)
	{
		queue.push_front(b);
	}
	else
	{
		queue.push_back(b);
	}

	
	if ((queue.size() > 1) && (b.priority < highestPriority) && (b.priority > lowestPriority))
	{
		
		sort(queue.begin(), queue.end());
	}

	
	highestPriority = (b.priority > highestPriority) ? b.priority : highestPriority;
	lowestPriority  = (b.priority < lowestPriority)  ? b.priority : lowestPriority;
}

void BuildOrderQueue::queueAsHighestPriority(MetaType                _metaType, bool blocking, int _producerID)
{
	
	int newPriority = highestPriority + defaultPrioritySpacing;

	
	queueItem(BuildOrderItem(_metaType, newPriority, blocking, _producerID));
}



void BuildOrderQueue::queueAsHighestPriority(MetaType                _metaType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy, bool _blocking) {
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(_metaType, _seedPositionStrategy, newPriority, _blocking));
}

void BuildOrderQueue::queueAsHighestPriority(UnitType         _unitType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy, bool _blocking) {
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(MetaType(_unitType), _seedPositionStrategy, newPriority, _blocking));
}

void BuildOrderQueue::queueAsHighestPriority(MetaType                _metaType, TilePosition _seedPosition, bool _blocking)
{
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(_metaType, _seedPosition, newPriority, _blocking));
}

void BuildOrderQueue::queueAsHighestPriority(UnitType         _unitType, TilePosition _seedPosition, bool _blocking)
{
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(MetaType(_unitType), _seedPosition, newPriority, _blocking));
}

void BuildOrderQueue::queueAsHighestPriority(TechType         _techType, bool blocking, int _producerID) {
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(MetaType(_techType), newPriority, blocking, _producerID));
}
void BuildOrderQueue::queueAsHighestPriority(UpgradeType      _upgradeType, bool blocking, int _producerID) {
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(MetaType(_upgradeType), newPriority, blocking, _producerID));
}

void BuildOrderQueue::queueAsLowestPriority(MetaType                _metaType, bool blocking, int _producerID)
{
	
	int newPriority = lowestPriority - defaultPrioritySpacing;

	if (newPriority < 0) {
		newPriority = 0;
	}

	
	queueItem(BuildOrderItem(_metaType, newPriority, blocking, _producerID));
}


void BuildOrderQueue::queueAsLowestPriority(MetaType                _metaType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy, bool _blocking) {
	queueItem(BuildOrderItem(_metaType, _seedPositionStrategy, 0, _blocking));
}

void BuildOrderQueue::queueAsLowestPriority(UnitType         _unitType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy, bool _blocking) {
	queueItem(BuildOrderItem(MetaType(_unitType), _seedPositionStrategy, 0, _blocking));
}

void BuildOrderQueue::queueAsLowestPriority(MetaType                _metaType, TilePosition _seedPosition, bool _blocking)
{
	queueItem(BuildOrderItem(_metaType, _seedPosition, 0, _blocking));
}


void BuildOrderQueue::queueAsLowestPriority(UnitType         _unitType, TilePosition _seedPosition, bool _blocking)
{
	queueItem(BuildOrderItem(MetaType(_unitType), _seedPosition, 0, _blocking));
}

void BuildOrderQueue::queueAsLowestPriority(TechType         _techType, bool blocking, int _producerID) {
	queueItem(BuildOrderItem(MetaType(_techType), 0, blocking, _producerID));
}
void BuildOrderQueue::queueAsLowestPriority(UpgradeType      _upgradeType, bool blocking, int _producerID) {
	queueItem(BuildOrderItem(MetaType(_upgradeType), 0, blocking, _producerID));
}

void BuildOrderQueue::removeHighestPriorityItem()
{
	
	queue.pop_back();

	
	highestPriority = queue.empty() ? 0 : queue.back().priority;
	lowestPriority  = queue.empty() ? 0 : lowestPriority;
}

void BuildOrderQueue::removeCurrentItem()
{

	queue.erase(queue.begin() + queue.size() - 1 - numSkippedItems);

	

	
	highestPriority = queue.empty() ? 0 : queue.back().priority;
	lowestPriority  = queue.empty() ? 0 : lowestPriority;
}

size_t BuildOrderQueue::size()
{
	return queue.size();
}

bool BuildOrderQueue::isEmpty()
{
	return queue.empty();
}

BuildOrderItem BuildOrderQueue::operator [] (int i)
{
	return queue[i];
}

const deque< BuildOrderItem > *BuildOrderQueue::getQueue()
{
	return &queue;
}
