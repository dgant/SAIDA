#include "SharedMemoryManager.h"

using namespace MyBot;

SharedMemoryManager &SharedMemoryManager::Instance() {
	static SharedMemoryManager instance;
	return instance;
}

void SharedMemoryManager::read()
{
	char data[10];
	char *ptr = shm.pData;

	word waitingCnt = 0;

	while (ptr && ptr[0] != 'S' && waitingCnt++ < 40)
		Sleep(1);

	if (waitingCnt >= 40) {
		winningRate = -1;
		return;
	}

	if (ptr && ptr[0] == 'S')
	{
		communication = true;
		memset(data, NULL, sizeof(data));

		memcpy(data, shm.pData, sizeof(data));

		//cout << "data : " << endl;
		//cout << data << endl;

		char *ch = strtok(data, ";");

		if (ch != NULL)
		{
			ch = strtok(NULL, ";");

			double tmp = atof(ch);
			winingRateQueue.push_back(tmp);

			if (winingRateQueue.size() > 3) {
				winingRateQueue.pop_front();
			}

			if (winingRateQueue.size() == 3) {
				winningRate = (winingRateQueue.at(0) + winingRateQueue.at(1) + winingRateQueue.at(2)) / 3;
			}

			//cout << "winningRate : " << winningRate << endl;
		}
	}
}

void SharedMemoryManager::write()
{
	setSupervisedData();
}

double SharedMemoryManager::getWinningRate()
{
	if (winningRate == -1)
		return -1;

	if (winingRateQueue.size() < 3)
		return 0;

	return winningRate * 100;
}

void SharedMemoryManager::setWinningRate(double winningRate_)
{
	winningRate = winningRate_;
}

bool SharedMemoryManager::CreateMemoryMap(SharedMemory *shm)
{
	// 기 오픈된 공유메모리가 있는지 확인.
	shm->hFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, shm->MapName);

	// 공유메모리 존재 안하는 경우 생성.
	if (!shm->hFileMap) {
		std::cout << "공유메모리 오픈 실패" << std::endl;
		shm->hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, shm->Size, shm->MapName);
	}

	// 생성 실패 시 종료 (메모리 부족, 보안 등)
	if (!shm->hFileMap) {
		std::cout << "공유메모리 생성 실패!" << std::endl;
		return false;
	}
	else
	{
		std::cout << "공유메모리 생성" << std::endl;
		isCreateSAIDAIPC = true;
	}

	if ((shm->pData = (char *)MapViewOfFile(shm->hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, shm->Size)) == NULL) {
		std::cout << "공유메모리 닫기" << std::endl;
		CloseHandle(shm->hFileMap);
		return false;
	}
	else
	{
		std::cout << "공유메모리 주소 획득" << std::endl;

		// 내가 열었으면 초기화
		if (isCreateSAIDAIPC)
		{
			memset(shm->pData, NULL, shm->Size);
		}
	}

	return true;
}

bool SharedMemoryManager::FreeMemoryMap(SharedMemory *shm)
{
	if (shm && shm->hFileMap)
	{
		if (shm->pData)
		{
			UnmapViewOfFile(shm->pData);
		}

		if (shm->hFileMap)
		{
			CloseHandle(shm->hFileMap);
		}

		return true;
	}

	return false;
}

bool SharedMemoryManager::setSupervisedData_old()
{
	char basicData[MAX_PATH];
	char *p = NULL;
	int myUnitAndBuildingCount = 0;
	int enemyUnitAndBuildingCount = 0;
	int mapX = 0;
	int mapY = 0;

	memset(basicData, NULL, sizeof(basicData));
	memset(shm.pData, NULL, MAX_SHM_SIZE);

	p = shm.pData;

	myUnitAndBuildingCount = SUPER.getUnitAndBuildingCount(S);
	enemyUnitAndBuildingCount = SUPER.getUnitAndBuildingCount(E);

	mapX = theMap.Size().x;
	mapY = theMap.Size().y;

	// C;frame;mapX;mapY;myRace:myUnitAndBuildingCount;enemyRace:enemyUnitAndBuildingCount;unitType:0 0 ...,unitType:0 0 ...;
	// ex.. C;1592;128;128;T:34;P:32;Terran_Firebat:0 0 1 0 ...
	wsprintf(basicData, "C;%d;%d;%d;%c;%d;%c;%d;", TIME, mapX, mapY,
			 INFO.selfRace.c_str()[0], myUnitAndBuildingCount, INFO.enemyRace.c_str()[0], enemyUnitAndBuildingCount);

	strncpy(p, basicData, strlen(basicData));
	//strncpy_s(p, MAX_SHM_SIZE, basicData, _TRUNCATE);
	p += strlen(basicData);

	list<UnitType> allUnitAndBuilding;

	for (auto type : SUPER.getUnitList(S))
	{
		allUnitAndBuilding.push_back(type);
	}

	for (auto type : SUPER.getBuildingList(S))
	{
		allUnitAndBuilding.push_back(type);
	}

	for (auto unitType : allUnitAndBuilding)
	{
		pair<std::string, int **> featureMap;

		for (auto &f : SUPER.getFeatureMap(S))
		{
			if (f.first == unitType.getName())
				featureMap = f;
		}

		//map<std::string, int **>::iterator featureMap = find_if(SUPER.getFeatureMap(S).begin(), SUPER.getFeatureMap(S).end(), findUnitType(unitType));
		//auto featureMap = find_if(SUPER.getFeatureMap(S).begin(), SUPER.getFeatureMap(S).end(), [unitType](const pair<string, int **> &feature) {
		//	return feature.first == unitType.getName();
		//});

		int len = strlen((featureMap).first.c_str());

		strncpy(p, (featureMap).first.c_str(), len);
		p += len;
		//			p++;
		*p = ':';
		p++;

		for (int i = 0; i < mapX; i++)
		{
			for (int j = 0; j < mapY; j++)
			{
				string unitCount = to_string(((featureMap).second)[i][j]);
				strncpy(p, unitCount.c_str(), strlen(unitCount.c_str()));
				p += strlen(unitCount.c_str());
				//				p++;
				*p = 0x20; // space
				p++;
			}
		}

		p--;
		*p = ',';
		p++;
	}

	allUnitAndBuilding.clear();

	for (auto type : SUPER.getUnitList(E))
	{
		allUnitAndBuilding.push_back(type);
	}

	for (auto type : SUPER.getBuildingList(E))
	{
		allUnitAndBuilding.push_back(type);
	}

	for (auto unitType : allUnitAndBuilding)
	{
		pair<std::string, int **> featureMap;

		for (auto &f : SUPER.getFeatureMap(E))
		{
			if (f.first == unitType.getName())
				featureMap = f;
		}

		//map<std::string, int **>::iterator featureMap = find_if(SUPER.getFeatureMap(S).begin(), SUPER.getFeatureMap(S).end(), findUnitType(unitType));
		//auto featureMap = find_if(SUPER.getFeatureMap(S).begin(), SUPER.getFeatureMap(S).end(), [unitType](const pair<string, int **> &feature) {
		//	return feature.first == unitType.getName();
		//});

		int len = strlen((featureMap).first.c_str());

		strncpy(p, (featureMap).first.c_str(), len);
		p += len;
		//			p++;
		*p = ':';
		p++;

		for (int i = 0; i < mapX; i++)
		{
			for (int j = 0; j < mapY; j++)
			{
				string unitCount = to_string(((featureMap).second)[i][j]);
				strncpy(p, unitCount.c_str(), strlen(unitCount.c_str()));
				p += strlen(unitCount.c_str());
				//				p++;
				*p = 0x20; // space
				p++;
			}
		}

		p--;
		*p = ',';
		p++;
	}

	p--;
	*p = ';';
	p++;
	*p = '\0';

	//	cout << shm.pData << endl;

	return false;
}

bool SharedMemoryManager::setSupervisedData()
{
	char basicData[MAX_PATH / 4];
	char *p = NULL;
	int myUnitAndBuildingCount = 0;
	int enemyUnitAndBuildingCount = 0;
	int mapX = 0;
	int mapY = 0;

	memset(basicData, NULL, sizeof(basicData));
	memset(shm.pData, NULL, MAX_SHM_SIZE);

	p = shm.pData;

	myUnitAndBuildingCount = INFO.getUnits(S).size() + INFO.getBuildings(S).size();
	enemyUnitAndBuildingCount = INFO.getUnits(E).size() + INFO.getBuildings(E).size();

	mapX = theMap.Size().x * TILE_SIZE;
	mapY = theMap.Size().y * TILE_SIZE;

	// C;Frame;mapX;mapY;myRace:myUnitAndBuildingCount;enemyRace:enemyUnitAndBuildingCount;unitType:x:y,unitType:x:y,...;
	// ex.. C;1208;128;128;T:34;P:32;Terran_Firebat:34:108, ...
	wsprintf(basicData, "S;%d;%d;%d;%c;%d;%c;%d;", TIME, mapX, mapY,
			 INFO.selfRace.c_str()[0], myUnitAndBuildingCount, INFO.enemyRace.c_str()[0], enemyUnitAndBuildingCount);

	strncpy(p, basicData, strlen(basicData));
	//strncpy_s(p, MAX_SHM_SIZE, basicData, _TRUNCATE);
	p += strlen(basicData);

	for (auto &myUnit : INFO.getUnits(S))
	{
		memset(basicData, NULL, sizeof(basicData));
		wsprintf(basicData, "%s:%d:%d,", myUnit.second->type().getName().c_str(), myUnit.second->pos().x, myUnit.second->pos().y);
		strncpy(p, basicData, strlen(basicData));
		p += strlen(basicData);
	}

	for (auto &myUnit : INFO.getBuildings(S))
	{
		memset(basicData, NULL, sizeof(basicData));
		wsprintf(basicData, "%s:%d:%d,", myUnit.second->type().getName().c_str(), myUnit.second->pos().x, myUnit.second->pos().y);
		strncpy(p, basicData, strlen(basicData));
		p += strlen(basicData);
	}

	*p = ';';
	p++;

	for (auto &enemyUnit : INFO.getUnits(E))
	{
		memset(basicData, NULL, sizeof(basicData));
		wsprintf(basicData, "%s:%d:%d,", enemyUnit.second->type().getName().c_str(), enemyUnit.second->getLastSeenPosition().x, enemyUnit.second->getLastSeenPosition().y);
		strncpy(p, basicData, strlen(basicData));
		p += strlen(basicData);
	}

	for (auto &enemyUnit : INFO.getBuildings(E))
	{
		memset(basicData, NULL, sizeof(basicData));
		wsprintf(basicData, "%s:%d:%d,", enemyUnit.second->type().getName().c_str(), enemyUnit.second->getLastSeenPosition().x, enemyUnit.second->getLastSeenPosition().y);
		strncpy(p, basicData, strlen(basicData));
		p += strlen(basicData);
	}

	p--;
	*p = ';';
	p++;
	*p = '\0';

	// 데이터를 모두 쓴 다음에 플래그를 바꿔준다.
	p = shm.pData;
	*p = 'C';

	return false;
}
