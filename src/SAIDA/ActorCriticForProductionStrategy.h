#pragma once
#include <iostream>
#include <fstream>
#include <iomanip>
#include "DeepLearningTools/network.h"
#include "DeepLearningTools/LinearRegression.h"
#include "DeepLearningTools/SumTree.h"
#include "ACUnitType.h"
typedef		unsigned char		UnitCountType;
typedef std::pair<int, int> IntPair;
typedef std::pair<ACMetaType, UnitCountType> ACMetaPair;
typedef std::vector<ACMetaPair> ACMetaPairVector;
struct sampleParsedInfo
{
	vector<double> modelInputs;
	int actualChosen;
	double targetQValue;
};


class ActorCriticForProductionStrategy
{
	ActorCriticForProductionStrategy();
	~ActorCriticForProductionStrategy() {}

	BWAPI::Race					selfRace;
	BWAPI::Race					enemyRace;

	std::vector<ACMetaType> 		actions;
	std::vector<ACMetaType>		getMetaVector(std::string buildString);

	std::vector<std::string>							stateActions;
	std::map<std::string, std::pair<BWAPI::UnitType, BWAPI::UnitType>>			actionBuildingTarget;
	std::map<std::string, std::map<std::string, std::vector<std::string>>>		featureNames;
	int													previousScore;
	std::map<std::string, std::map<std::string, double>>						parameterValue;
	std::map<std::string, std::map<std::string, int>>						featureValue;
	Network						policyModel;
	Network						QValueModel;
	Network						targetQValueModel;

	Network						stateValueModel;
	std::string initReadResourceFolder;
	std::string readResourceFolder;
	std::string writeResourceFolder;

	std::map<std::string, ACMetaType> highLevelActions;
	std::map<std::string, std::vector<std::string>>	features;
	std::vector<std::string>	NNInputAction;
	std::vector<std::string>	NNOutputAction;
	std::map<std::string, int>	actionsOutputIndexMapping;

	std::map<std::string, double> featureValues;
	std::vector<std::vector<std::string>> trainingData;
	int							lastExpandTime;
	std::string					previousChosenAction;
	double						iterationTrain(int miniBatch);
	double						tmpReward;
	int							tdStep;
	int							replayLength;

	int							curActionTime;
	void						featureWeightInit();
	void						experienceDataInit();


	SumTree						replayData;
	double						getSamplePriority(std::vector<std::string>& sample);
	sampleParsedInfo			sampleParse(std::vector<std::string>& sample);




	std::vector<std::vector<std::string>> testSetData;
	double						testSetAvgQValue;
	void						tesetSetPerformance();

	std::map<std::string, std::map<std::string, double>>						parameterCumulativeGradient;
	void						featureCumulativeGradientInit();



	std::vector<std::vector<std::string>>	curEpisodeData;
	double						discountRate;
	std::string					getCategory(std::vector<std::string>& categoryRange, int curValue, std::string prefix);
	double						getNormalizedValue(std::vector<std::string>& categoryRange, int curValue);
	void						GetCurrentStateFeature();
	std::string					opponentWinrate;
	void						setArmyUpgrade();

	int							playMatchCount;
	bool 						muteBuilding;
	bool						isInBuildingMutalisk;
	std::string                       filePath_tmp = "";
	std::string					curBuildingAction;
	int							nextDefenseBuildTime;

public:

	static	ActorCriticForProductionStrategy &	Instance();
	void						update(int reward);
	void						baseExpand();
	void                        RLModelInitial();
	int							getScore();

	std::string					getCurrentBuildingAction() { return curBuildingAction; }

	string					    getTargetUnit();

	ACMetaType					getTargetDQN();
	void						trainModels(int reward);
	void						buildingFinish(BWAPI::UnitType u);
	bool                        isGasRequireMeet(int price);
	bool                        isMineRequireMeet(int price);
                  
	void						featureWeightSave();
	void						featureGradientSave();
	void						experienceDataSave();


	void						setReward(BWAPI::UnitType u, bool killEnemy);

};
