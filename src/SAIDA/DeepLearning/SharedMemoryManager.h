#pragma once
#include "../Common.h"
#include "../InformationManager.h"
#include "../DeepLearning/Supervised.h"
#include "../StrategyManager.h"

#define SHM	SharedMemoryManager::Instance()
#define MAX_SHM_SIZE 1024 * 1024 * 5 // 5M

namespace MyBot
{
	struct findUnitType
	{
		findUnitType(UnitType type) : unitType(type) {}
		bool operator()(const pair<string, int **> &feature) const
		{
			return feature.first == unitType.getName();
		}
	private:
		UnitType unitType;
	};

	class SharedMemory
	{
	public:
		SharedMemory(HANDLE h, char *p, LPCSTR n, size_t s)
		{
			hFileMap = h;
			pData = p;
			MapName = n;
			Size = s;
		};

		HANDLE hFileMap;
		char *pData;
		LPCSTR MapName;
		size_t Size;
	};

	class SharedMemoryManager
	{
	public:
		/// static singleton ��ü�� �����մϴ�
		static SharedMemoryManager &Instance();

		SharedMemoryManager() {
			communication = false;
			//CreateMemoryMap(&shm);
		}

		~SharedMemoryManager() {
			//FreeMemoryMap(&shm);
		}

		void read();
		void write();

		double getWinningRate();
		void setWinningRate(double winningRate);
		bool communication;
	private:
		// shared memory �߰��Ҷ����� ���� �߰� �� ������ �Ҹ��� �߰� �ʿ���.
		SharedMemory shm = SharedMemory(0, NULL, "SAIDA_IPC", MAX_SHM_SIZE);

		bool CreateMemoryMap(SharedMemory *shm);
		bool FreeMemoryMap(SharedMemory *shm);

		bool setSupervisedData_old();
		bool setSupervisedData();

		bool isCreateSAIDAIPC = false;
		double winningRate = 0;
		deque<double> winingRateQueue;
	};
}
