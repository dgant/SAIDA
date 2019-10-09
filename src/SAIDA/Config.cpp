#include "Config.h"
#include "Common.h"

namespace Config
{
	// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
	// 봇 이름 및 파일 경로 기본값 변경

	namespace Files
	{
		std::string LogFilename;
		std::string TimeoutFilename;
		std::string ErrorLogFilename;
		std::string ReadDirectory = "bwapi-data\\read\\";
		std::string WriteDirectory = "bwapi-data\\write\\";
	}

	// BasicBot 1.1 Patch End //////////////////////////////////////////////////

	namespace BWAPIOptions
	{
		int SetLocalSpeed = 0;
		int SetFrameSkip = 0;
		bool EnableUserInput = true;
		bool EnableCompleteMapInformation = false;
	}

	namespace Tools
	{
		extern int MAP_GRID_SIZE = 32;
	}

	namespace Debug
	{
		bool DrawGameInfo = true;
		bool DrawScoutInfo = true;
		bool DrawMouseCursorInfo = true;
		bool DrawBWEMInfo = true;
		bool DrawUnitTargetInfo = false;
		// 아래는 둘중 한가지만
		bool DrawMyUnit = false;
		bool DrawEnemyUnit = false;
		bool DrawLastCommandInfo = false;
		bool DrawUnitStatus = true;
	}


	// 기본 설정 정보
	namespace Propeties
	{
		/// 자원 측정시에 사용되는 measure duration 정보이며 단위는 seconds
		int duration = 1;
		bool recoring = true;
		enum columns { REMAINING_GAS, REMAINING_MINERAL, GATHERED_GAS, GATHERED_MINERAL, NUMBER_OF_SCV_FOR_GAS, NUMBER_OF_SCV_OF_FOR_MINERAL };

	}

}