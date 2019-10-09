#include "BuildOrderQueue.h"
#include "SaidaUtil.h"

using namespace MyBot;

BuildOrderQueue::BuildOrderQueue()
	: highestPriority(0),
	  lowestPriority(0),
	  defaultPrioritySpacing(10),
	  numSkippedItems(0)
{

}
// 清除所有队列
void BuildOrderQueue::clearAll()
{
	// clear the queue
	queue.clear();

	// reset the priorities
	highestPriority = 0;
	lowestPriority = 0;
}

bool BuildOrderQueue::removeQueue(UnitType _unitType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy, bool isSame) {
	bool removed = false;

	for (auto iter = queue.begin(); iter != queue.end(); iter++) {
		if (isSame && iter->metaType.getUnitType() == _unitType && iter->seedLocationStrategy == _seedPositionStrategy) {
			removed = true;
			// remove the back element of the vector
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
		// if the list is not empty, set the highest accordingly
		highestPriority = queue.empty() ? 0 : queue.back().priority;
		lowestPriority = queue.empty() ? 0 : lowestPriority;
	}

	return removed;
}
//得到最高优先级的物品===============================================================
BuildOrderItem BuildOrderQueue::getHighestPriorityItem()
{
	// reset the number of skipped items to zero
	numSkippedItems = 0;

	// the queue will be sorted with the highest priority at the back
	return queue.back();
}
//得到下一件的物品======================================================================
BuildOrderItem BuildOrderQueue::getNextItem()
{
	assert(queue.size() - 1 - numSkippedItems >= 0);

	// the queue will be sorted with the highest priority at the back
	return queue[queue.size() - 1 - numSkippedItems];
}

//得到物品位置（元）========================================================================
int BuildOrderQueue::getItemCount(MetaType queryType, TilePosition queryTilePosition)
{
	// queryTilePosition 을 입력한 경우, 거리의 maxRange. 타일단위
	int maxRange = 16;

	int itemCount = 0;

	size_t reps = queue.size();

	// for each unit in the queue
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
//得到物品位置（单位）========================================================================
int BuildOrderQueue::getItemCount(UnitType         _unitType, TilePosition queryTilePosition)
{
	return getItemCount(MetaType(_unitType), queryTilePosition);
}
//得到物品位置（科技）========================================================================
int BuildOrderQueue::getItemCount(TechType         _techType)
{
	return getItemCount(MetaType(_techType));
}
//得到物品位置（升级）========================================================================
int BuildOrderQueue::getItemCount(UpgradeType      _upgradeType)
{
	return getItemCount(MetaType(_upgradeType));
}

// 计算该类型的物品是否存在（元）========================================================================
bool BuildOrderQueue::existItem(MetaType queryType, TilePosition queryTilePosition)
{
	// queryTilePosition 을 입력한 경우, 거리의 maxRange. 타일단위
	int maxRange = 16;

	size_t reps = queue.size();

	// for each unit in the queue
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
// 计算该类型的物品是否存在（单位）========================================================================
bool BuildOrderQueue::existItem(UnitType         _unitType, TilePosition queryTilePosition)
{
	return existItem(MetaType(_unitType), queryTilePosition);
}
// 计算该类型的物品是否存在（科技）========================================================================
bool BuildOrderQueue::existItem(TechType         _techType)
{
	return existItem(MetaType(_techType));
}
// 计算该类型的物品是否存在（升级）========================================================================
bool BuildOrderQueue::existItem(UpgradeType      _upgradeType)
{
	return existItem(MetaType(_upgradeType));
}
// 略过当前物品========================================================================
void BuildOrderQueue::skipCurrentItem()
{
	// make sure we can skip
	if (canSkipCurrentItem()) {
		// skip it
		numSkippedItems++;
	}
}
// 能略过当前物品========================================================================
bool BuildOrderQueue::canSkipCurrentItem()
{
	// does the queue have more elements
	bool bigEnough = queue.size() > (size_t)(1 + numSkippedItems);

	if (!bigEnough)
	{
		return false;
	}

	// is the current highest priority item not blocking a skip
	bool highestNotBlocking = !queue[queue.size() - 1 - numSkippedItems].blocking;

	// this tells us if we can skip
	return highestNotBlocking;
}
// 物品编队========================================================================
void BuildOrderQueue::queueItem(BuildOrderItem b)
{
	// 队列内不要有6个以上的同一栋建筑。 
	if (getItemCount(b.metaType) > 6) {
		return;
	}

	// if the queue is empty, set the highest and lowest priorities
	if (queue.empty())
	{
		highestPriority = b.priority;
		lowestPriority = b.priority;
	}

	// push the item into the queue
	if (b.priority <= lowestPriority)
	{
		queue.push_front(b);
	}
	else
	{
		queue.push_back(b);
	}

	// if the item is somewhere in the middle, we have to sort again
	if ((queue.size() > 1) && (b.priority < highestPriority) && (b.priority > lowestPriority))
	{
		// sort the list in ascending order, putting highest priority at the top
		sort(queue.begin(), queue.end());
	}

	// update the highest or lowest if it is beaten
	highestPriority = (b.priority > highestPriority) ? b.priority : highestPriority;
	lowestPriority  = (b.priority < lowestPriority)  ? b.priority : lowestPriority;
}

void BuildOrderQueue::queueAsHighestPriority(MetaType                _metaType, bool blocking, int _producerID)
{
	// the new priority will be higher
	int newPriority = highestPriority + defaultPrioritySpacing;

	// queue the item
	queueItem(BuildOrderItem(_metaType, newPriority, blocking, _producerID));
}


// buildQueue 에 넣는다. 기본적으로 blocking 모드 (다른 것을 생산하지 않고, 이것을 생산 가능하게 될 때까지 기다리는 모드)
// SeedPositionStrategy 를 갖고 결정
void BuildOrderQueue::queueAsHighestPriority(MetaType                _metaType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy, bool _blocking) {
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(_metaType, _seedPositionStrategy, newPriority, _blocking));
}
// SeedPositionStrategy 를 갖고 결정
void BuildOrderQueue::queueAsHighestPriority(UnitType         _unitType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy, bool _blocking) {
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(MetaType(_unitType), _seedPositionStrategy, newPriority, _blocking));
}

//设置为编队中的最高优先级（元单位）=====================================================================================
void BuildOrderQueue::queueAsHighestPriority(MetaType                _metaType, TilePosition _seedPosition, bool _blocking)
{
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(_metaType, _seedPosition, newPriority, _blocking));
}

//设置为编队中的最高优先级（单位）=====================================================================================
void BuildOrderQueue::queueAsHighestPriority(UnitType         _unitType, TilePosition _seedPosition, bool _blocking)
{
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(MetaType(_unitType), _seedPosition, newPriority, _blocking));
}
//设置为编队中的最高优先级（科技）=====================================================================================
void BuildOrderQueue::queueAsHighestPriority(TechType         _techType, bool blocking, int _producerID) {
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(MetaType(_techType), newPriority, blocking, _producerID));
}
//设置为编队中的最高优先级（升级）=====================================================================================
void BuildOrderQueue::queueAsHighestPriority(UpgradeType      _upgradeType, bool blocking, int _producerID) {
	int newPriority = highestPriority + defaultPrioritySpacing;
	queueItem(BuildOrderItem(MetaType(_upgradeType), newPriority, blocking, _producerID));
}
//设置为编队中的最低优先级=======================================================================================
void BuildOrderQueue::queueAsLowestPriority(MetaType                _metaType, bool blocking, int _producerID)
{
	// the new priority will be higher
	int newPriority = lowestPriority - defaultPrioritySpacing;

	if (newPriority < 0) {
		newPriority = 0;
	}

	// queue the item
	queueItem(BuildOrderItem(_metaType, newPriority, blocking, _producerID));
}

// buildQueue 에 넣는다. 기본적으로 blocking 모드 (다른 것을 생산하지 않고, 이것을 생산 가능하게 될 때까지 기다리는 모드)
// SeedPositionStrategy 를 갖고 결정
void BuildOrderQueue::queueAsLowestPriority(MetaType                _metaType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy, bool _blocking) {
	queueItem(BuildOrderItem(_metaType, _seedPositionStrategy, 0, _blocking));
}
// SeedPositionStrategy 를 갖고 결정
void BuildOrderQueue::queueAsLowestPriority(UnitType         _unitType, BuildOrderItem::SeedPositionStrategy _seedPositionStrategy, bool _blocking) {
	queueItem(BuildOrderItem(MetaType(_unitType), _seedPositionStrategy, 0, _blocking));
}
// SeedPosition 을 갖고 결정
void BuildOrderQueue::queueAsLowestPriority(MetaType                _metaType, TilePosition _seedPosition, bool _blocking)
{
	queueItem(BuildOrderItem(_metaType, _seedPosition, 0, _blocking));
}

// SeedPosition 을 갖고 결정
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
	// remove the back element of the vector
	queue.pop_back();

	// if the list is not empty, set the highest accordingly
	highestPriority = queue.empty() ? 0 : queue.back().priority;
	lowestPriority  = queue.empty() ? 0 : lowestPriority;
}

void BuildOrderQueue::removeCurrentItem()
{
	// remove the back element of the vector
	queue.erase(queue.begin() + queue.size() - 1 - numSkippedItems);
	
	//assert((int)(queue.size()) < size);

	// if the list is not empty, set the highest accordingly
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
