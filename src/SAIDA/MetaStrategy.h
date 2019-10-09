

#include <BWAPI.h>
#include "Common.h"
#include "UnitData.h"
#include "InformationManager.h"
#include "StrategyManager.h"
/*
#include "Xelnaga.h"
#include "Skynet.h"
#include "NUSBotModule.h"
*/
using namespace BWAPI;
using namespace std;

/** Base class for meta strategies */
class MetaStrategy {

private:   
	InitialBuildType enemyInitialBuild;
	MainBuildType enemyMainBuild;
	vector<pair<InitialBuildType, int>> eibHistory;
	vector<pair<MainBuildType, int>> embHistory;
	Unit enemyGasRushRefinery;

	float nexusBuildSpd_per_frmCnt = (float)(Protoss_Nexus.maxHitPoints() + Protoss_Nexus.maxShields()) / Protoss_Nexus.buildTime(); // hp per frame count
	float commandBuildSpd_per_frmCnt = (float)Terran_Command_Center.maxHitPoints() / Terran_Command_Center.buildTime(); // hp per frame count
	//UnitInfo *secondNexus = nullptr;
	//UnitInfo *secondCommandCenter = nullptr;

	/*
	void setEnemyMainBuild(MainBuildType st);
	void setEnemyInitialBuild(InitialBuildType ib, bool force = false);
	void clearBuildConstructQueue();
	//bool isTheUnitNearMyBase(uList unitList);
	void checkDarkDropHadCome();
	*/

	

protected:
	//name of behavior in previous frame
    //string oldBehaviorName;

	//boost::mt19937 rng; //mersenne twister random number generator

	/** Active behavior (corresponds to a bot) */
	BWAPI::AIModule* currentStrategy;

	/** Name of this meta strategy */
	string name;

	/** Maps behaviors to their respective names */
    std::map<BWAPI::AIModule*, string> strategyNames;

    /** Maps behavior names to their respectives AIModules */
    std::map<string, BWAPI::AIModule*> portfolio;

	/** Returns a strategy uniformly at random */
	BWAPI::AIModule* randomUniform(); 

	/** Initializes all values in a string->float map */
	template<typename T>
	void initializeMap(std::map<string, T>& map, T initialValue) { //implementation in header prevents linking errors
		vector<string> &portfolioNames = Configuration::getInstance()->portfolioNames;
		vector<string>::iterator it;

		for (it = portfolioNames.begin(); it != portfolioNames.end(); it++) {
			map[*it] = initialValue;
		}
	}


public:

	MetaStrategy();
    ~MetaStrategy();

	/**
	* Loads an AI module given its name
	* (DLL must be located in the directory specified in Config)
	*/
	//float nexusBuildSpd_per_frmCnt = (float)(Protoss_Nexus.maxHitPoints() + Protoss_Nexus.maxShields()) / Protoss_Nexus.buildTime(); // hp per frame count
	//float commandBuildSpd_per_frmCnt = (float)Terran_Command_Center.maxHitPoints() / Terran_Command_Center.buildTime(); // hp per frame count
	//UnitInfo *secondNexus = nullptr;
	//UnitInfo *secondCommandCenter = nullptr;

	double centerToBaseOfMarine = 0;
	double centerToBaseOfZ = 0;
	double centerToBaseOfSCV = 0;
	double centerToBaseOfProbe = 0;
	int timeToBuildBarrack = 0;
	int timeToBuildGateWay = 0;
	int waitTimeForDrop = 0;
	void setEnemyMainBuild(MainBuildType st);
	void setEnemyInitialBuild(InitialBuildType ib, bool force = false);
	void clearBuildConstructQueue();
	void checkEnemyInitialBuild();
//	bool isTheUnitNearMyBase(uList unitList);
	void checkDarkDropHadCome();
	//clearBuildConstructQueue

};
