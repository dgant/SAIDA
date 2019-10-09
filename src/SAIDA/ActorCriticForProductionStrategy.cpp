
#include "ActorCriticForProductionStrategy.h"
#include "InformationManager.h"
using namespace MyBot;
// constructor
ActorCriticForProductionStrategy::ActorCriticForProductionStrategy():
selfRace(BWAPI::Broodwar->self()->getRace()), enemyRace(BWAPI::Broodwar->enemy()->getRace()), initReadResourceFolder("./bwapi-data/AI/"),readResourceFolder("./bwapi-data/read/"),
writeResourceFolder("./bwapi-data/write/")
{
	//output_Actions:output of AC model
	if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Terran)
	{

		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_Marine));
		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_Ghost));
		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_Vulture));
		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_Goliath));
		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode));
		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_SCV));
		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_Wraith));
		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_Command_Center));
		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_Dropship));
		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_Battlecruiser));
		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_Refinery));
		actions.push_back(ACMetaType(BWAPI::UnitTypes::Terran_Nuclear_Missile));
		actions.push_back(ACMetaType(BWAPI::UpgradeTypes::Terran_Ship_Weapons));
	}
	tmpReward = 0;
	tdStep = 30;

	highLevelActions = decltype(highLevelActions)
	{
		//Unit
		{ "Unit_Terran_Covert_Ops", ACMetaType(BWAPI::UnitTypes::Terran_Covert_Ops)},
		{ "Unit_Terran_Starport", ACMetaType(BWAPI::UnitTypes::Terran_Starport) },
		{ "Unit_Terran_Science_Facility", ACMetaType(BWAPI::UnitTypes::Terran_Science_Facility) },
		{ "Unit_Terran_Machine_Shop", ACMetaType(BWAPI::UnitTypes::Terran_Machine_Shop) },
		{ "Unit_Terran_Bunker", ACMetaType(BWAPI::UnitTypes::Terran_Bunker) },

		//TechResearch
		{ "Tech_Tank_Siege_Mode", ACMetaType(BWAPI::TechTypes::Tank_Siege_Mode) },
		{ "Tech_Defensive_Matrix", ACMetaType(BWAPI::TechTypes::Defensive_Matrix) },
		{ "Tech_Mind_Control,", ACMetaType(BWAPI::TechTypes::Mind_Control) },
		{ "Tech_Dark_Archon_Meld", ACMetaType(BWAPI::TechTypes::Dark_Archon_Meld) },
		{ "Tech_Tank_Siege_Mode", ACMetaType(BWAPI::TechTypes::Tank_Siege_Mode) },
		{ "Tech_Psionic_Storm", ACMetaType(BWAPI::TechTypes::Psionic_Storm) },
		{ "Tech_Personnel_Cloaking", ACMetaType(BWAPI::TechTypes::Personnel_Cloaking) },
		{ "Tech_Scanner_Sweep", ACMetaType(BWAPI::TechTypes::Scanner_Sweep) },

		//Upgrade
		{ "Upgrade_Terran_Infantry_Armor", ACMetaType(BWAPI::UpgradeTypes::Terran_Infantry_Armor) },
		{ "Upgrade_Terran_Vehicle_Plating", ACMetaType(BWAPI::UpgradeTypes::Terran_Vehicle_Plating) },
		{ "Upgrade_Terran_Ship_Plating", ACMetaType(BWAPI::UpgradeTypes::Terran_Ship_Plating) },
		{ "Upgrade_Terran_Infantry_Weapons", ACMetaType(BWAPI::UpgradeTypes::Terran_Infantry_Weapons) },
		{ "Upgrade_Terran_Vehicle_Weapons", ACMetaType(BWAPI::UpgradeTypes::Terran_Vehicle_Weapons) },
		{ "Upgrade_Metabolic_Boost", ACMetaType(BWAPI::UpgradeTypes::Metabolic_Boost) },
		{ "Upgrade_Metasynaptic_Node", ACMetaType(BWAPI::UpgradeTypes::Metasynaptic_Node) },

		//Wait
		{ "Wait_doNothing", ACMetaType() }
	};

	for (auto entry : highLevelActions)
	{
		NNOutputAction.push_back(entry.first);
	}

	for (size_t i = 0; i < NNOutputAction.size(); i++)
	{
		actionsOutputIndexMapping[NNOutputAction[i]] = i;
	}

	//state feature:input of the AC model
	features = decltype(features)
	{
		//state feature
		{ "time_", { "60" } },
		{ "enemyRace_z", { "1" } },
		{ "enemyRace_t", { "1" } },
		{ "enemyRace_p", { "1" } },
		{ "enemyRace_unknow", { "1" } },

		{ "ourSupplyUsed_", { "200" } },

		//economy
		{ "our_gas_base_count", { "3" } },
		{ "our_mineral_base_count", { "2" } },
		//battleUnit
		//self
		{ "our_Terran_Marine", { "12" } },
		{ "our_Terran_Ghost", { "12" } },
		{ "our_Terran_Vulture", { "12" } },
		{ "our_Terran_Goliath", { "12" } },
		{ "our_Terran_SCV", { "12" } },
		{ "our_Terran_Wraith", { "12" } },
		{ "our_Terran_Science_Vessel", { "12" } },
		{ "our_Terran_Siege_Tank_Tank_Mode", { "12" } },
		{ "our_Terran_Science_Facility", { "12" } },
		
		//enemy
		{ "enemyP_cannon_", { "12" } },
		{ "enemyT_missile_", { "12" } },
		{ "enemyT_Bunker_", { "12" } },
		{ "enemyZ_Sunken_", { "12" } },
		{ "enemyZ_Spore_", { "12" } },

		{ "enemyZ_Zergling_", { "120" } },
		{ "enemyZ_Mutalisk_", { "36" } },
		{ "enemyZ_Hydra_", { "48" } },
		{ "enemyZ_Lurker_", { "20" } },
		{ "enemyZ_Scourage", { "36" } },
		{ "enemyZ_Ultralisk", { "20" } },
		{ "enemyZ_Defiler", { "12" } },

		{ "enemyP_Zealot_", { "36" } },
		{ "enemyP_Dragon_", { "36" } },
		{ "enemyP_High_temple_", { "12" } },
		{ "enemyP_Carrier_", { "24" } },
		{ "enemyP_Corsair_", { "30" } },
		{ "enemyP_Shuttle_", { "9" } },
		
		{ "enemyT_Bunker_", { "30" } },
		{ "enemyT_missle_", { "36" } },
		
		{ "enemyT_Goliath_", { "30" } },
		{ "enemyT_Marine_", { "36" } },
		{ "enemyT_Tank_", { "30" } },
		{ "enemyT_Vulture_", { "30" } },
		{ "enemyT_Dropship_", { "9" } },
		{ "enemyT_Valkyrie_", { "18" } },
		{ "enemyT_Science_", { "9" } },
		{ "enemyT_Firebat_", { "30" } },
		{ "enemyT_Terran_Medic_", { "12" } }
	};
	/*
	for (auto entry : highLevelActions)
	{
	if (entry.first.find("Building_") != std::string::npos
	|| entry.first.find("Expand_") != std::string::npos
	|| entry.first.find("Tech_") != std::string::npos
	|| entry.first.find("Upgrade_") != std::string::npos)
	{
	features["building_during_production_" + entry.first] = { "1" };
	}
	}

	*/

	for (auto entry : features)
	{
		NNInputAction.push_back(entry.first);
	}
	discountRate = 0.99;
	muteBuilding = true;
	isInBuildingMutalisk = false;
	nextDefenseBuildTime = 0;
	replayLength = 6;
	replayData.setCapacity(10000);
	experienceDataInit();
}

void ActorCriticForProductionStrategy::RLModelInitial()
{
	//初始化策略网络
	string enemyName = BWAPI::Broodwar->enemy()->getName();
	if (policyModel.deserialize("policy", readResourceFolder) == -1)
	{
		if (policyModel.deserialize("policy", initReadResourceFolder) == -1)
		{
			BWAPI::Broodwar->printf("policyModel first init!");
			// linear model initial
			policyModel.setDepth(3);
			policyModel.setNodeCountAtLevel(100, 0, Relu);
			policyModel.setNodeCountAtLevel(80, 1, Relu);
			policyModel.setNodeCountAtLevel(NNOutputAction.size(), 2, SOFTMAX);//此处softmax仅做逐点exp处理，不做求和归一化
			policyModel.setNumInputs(NNInputAction.size());
			policyModel.linkNeurons();
			policyModel.setLearningRate(0.00025);
			policyModel.initNetwork();
		}
	}
	//初始化状态价值网络
	if (stateValueModel.deserialize("stateValue", readResourceFolder) == -1)
	{
		if (stateValueModel.deserialize("stateValue", initReadResourceFolder) == -1)
		{
			BWAPI::Broodwar->printf("stateValueModel init!");
			// linear model initial
			stateValueModel.setDepth(3);
			stateValueModel.setNodeCountAtLevel(100, 0, Relu);
			stateValueModel.setNodeCountAtLevel(80, 1, Relu);
			stateValueModel.setNodeCountAtLevel(1, 2, SUM);
			stateValueModel.setNumInputs(NNInputAction.size());
			stateValueModel.linkNeurons();
			stateValueModel.setLearningRate(0.00025);
			stateValueModel.initNetwork();
		}
	}
}
void ActorCriticForProductionStrategy::setReward(BWAPI::UnitType u, bool addReward)
{
	if (u == BWAPI::UnitTypes::Terran_Dropship || u == BWAPI::UnitTypes::Terran_Vulture)
	{
		return;
	}

	if (addReward)
	{
		if (u.isWorker())
		{
			tmpReward += double(u.supplyRequired()) / 20;
		}
		//expand reward
		else if (u.isResourceDepot())
		{
			tmpReward += 0.5;
		}
		else if (u == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine)
		{
			tmpReward += 0.1;
		}
		else if (u.isBuilding() && (u.groundWeapon() != BWAPI::WeaponTypes::None || u.airWeapon() != BWAPI::WeaponTypes::None ||
			u == BWAPI::UnitTypes::Terran_Bunker))
		{
			tmpReward += 0.15;
		}
		//encourage kill unit
		else
		{
			tmpReward += double(u.supplyRequired()) / 40;
		}
	}

	else
	{
		if (u.isWorker())
		{
			tmpReward -= double(u.supplyRequired()) / 20;
		}
		else if (u.isResourceDepot())
		{
			tmpReward -= 0.5;
		}
		else
		{
			tmpReward -= double(u.supplyRequired()) / 80;
		}

	}


}
void ActorCriticForProductionStrategy::experienceDataSave()
{
	string filePath;
	string enemyName = BWAPI::Broodwar->enemy()->getName();
	filePath = writeResourceFolder + "RL_data";
	filePath += enemyName;
	fstream historyModel;
	historyModel.open(filePath_tmp.c_str(), ios::out);
	std::vector<std::vector<std::string>> subData;

	vector<treeReturnData> experienceData = replayData.getData();
	vector<treeReturnData> experienceData_tmp;

	/*
	//save test dataSet
	if (experienceData.size() > 1000)
	{
	filePath = "./bwapi-data/write/testSetData";
	fstream test;
	test.open(filePath.c_str(), ios::in);
	//if do not has this file, init
	if (!test.is_open())
	{
	fstream testSetData;
	testSetData.open(filePath.c_str(), ios::out);
	int count = 0;
	for (auto tmp : experienceData)
	{
	vector<string> rlData = tmp.second;
	count += 1;
	if (count % 5 != 0)
	{
	continue;
	}
	for (size_t i = 0; i < rlData.size(); i++)
	{
	testSetData << rlData[i];
	if (i != rlData.size() - 1)
	{
	testSetData << "|";
	}
	}
	testSetData << endl;
	}
	testSetData.close();
	}
	}
	*/


	for (auto tmp : experienceData_tmp)
	{
		vector<string> rlData = tmp.data;
		for (size_t i = 0; i < rlData.size(); i++)
		{
			historyModel << rlData[i];
			if (i != rlData.size() - 1)
			{
				historyModel << "|";
			}
		}
		historyModel << "|" << tmp.priority << endl;
	}
	historyModel.close();

	/*
	//save for offline test training
	if (trainingData.size() > 0)
	{
	fstream historyFile;
	string filePath = "./bwapi-data/total_match";
	historyFile.open(filePath.c_str(), ios::app);
	for (size_t instance = 0; instance < trainingData.size(); instance++)
	{
	for (size_t i = 0; i < trainingData[instance].size(); i++)
	{
	historyFile << trainingData[instance][i];
	if (i != trainingData.size() - 1)
	{
	historyFile << "|";
	}
	}
	historyFile << "&";
	}
	historyFile << endl;
	historyFile.close();
	}

	// save test set performance data
	if (testSetData.size() > 0)
	{
	fstream historyFile;
	string filePath = "./bwapi-data/write/performace_data";
	historyFile.open(filePath.c_str(), ios::app);
	historyFile << testSetAvgQValue;
	historyFile << endl;
	historyFile.close();
	}
	*/
}


void ActorCriticForProductionStrategy::featureWeightSave()
{
	/*
	string filePath;
	if (curMode == Develop)
	{
	filePath = "./bwapi-data/write/feature_value";
	}
	else
	{
	string enemyName = BWAPI::Broodwar->enemy()->getName();
	filePath = "./bwapi-data/write/feature_value";
	filePath += enemyName;
	}

	fstream historyModel;

	historyModel << std::fixed << std::setprecision(5);

	historyModel.open(filePath.c_str(), ios::out);

	bool featureValid = true;

	for (std::map<std::string, std::map<std::string, double>> ::iterator it = parameterValue.begin(); it != parameterValue.end();)
	{
	if (featureNames.find(it->first) == featureNames.end())
	{
	parameterValue.erase(it++);
	featureValid = false;
	}
	else
	{
	it++;
	}
	}
	if (featureValid == false)
	{
	fstream lossFile;
	string lossfilePath = "./bwapi-data/write/debug_file_parweight";
	lossFile.open(lossfilePath.c_str(), ios::app);
	lossFile << "1" << endl;
	lossFile.close();
	//return maxAction;
	}


	// file format:
	// feature_name1:value
	// feature_name2:value
	for (auto feature_categoy : parameterValue)
	{
	for (auto& feature : feature_categoy.second)
	{
	//weird bug...
	if (isnan(feature.second))
	{
	fstream lossFile;
	string lossfilePath = "./bwapi-data/write/debug_file";
	lossFile.open(lossfilePath.c_str(), ios::app);
	lossFile << feature.first << ":" << feature.second << endl;
	lossFile.close();

	continue;
	}
	historyModel << feature_categoy.first << ":" << feature.first << ":" << feature.second;
	historyModel << endl;
	}
	}
	historyModel.close();
	*/
}


void ActorCriticForProductionStrategy::experienceDataInit()
{
	string filePath;
	string enemyName = BWAPI::Broodwar->enemy()->getName();
	filePath = readResourceFolder + "RL_data";

	fstream historyModel;

	historyModel.open(filePath.c_str(), ios::in);

	if (historyModel.is_open())
	{
		string content;
		while (getline(historyModel, content))
		{
			if (content == "")
				continue;
			std::stringstream instance(content);
			string entry;
			std::vector<string> itemList;
			while (getline(instance, entry, '|'))
			{
				itemList.push_back(entry);
			}
			double priority = std::stod(itemList.back());
			itemList.pop_back();

			if (itemList.size() != replayLength)
			{
				continue;
			}

			replayData.add(priority, itemList);
		}
	}
	else
	{
		BWAPI::Broodwar->printf("do not has experience data !");
	}
	historyModel.close();
	playMatchCount = 50;
}



double ActorCriticForProductionStrategy::getNormalizedValue(std::vector<std::string>& categoryRange, int curValue)
{
	int rangeValue = std::stoi(categoryRange[0]);
	//scale to [-1, 1] range
	return (curValue / double(rangeValue)) * 2 - 1;
}


void ActorCriticForProductionStrategy::GetCurrentStateFeature()
{
	//state general
	int featurecount = 0;
	int curTimeMinuts = BWAPI::Broodwar->getFrameCount() / (24 * 60);
	featureValues["time_"] = getNormalizedValue(features["time_"], curTimeMinuts);
	BWAPI::UnitType enemyWork = BWAPI::UnitTypes::None;
	int featureCount = 0;

	std::string enemyRaceName;
	if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Zerg)
	{
		enemyRaceName = "enemyRace_z";
		featureValues["enemyRace_z"] = 1;
		enemyWork = BWAPI::UnitTypes::Zerg_Drone;
	}
	else if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran)
	{
		enemyRaceName = "enemyRace_t";
		featureValues["enemyRace_t"] = 1;
		enemyWork = BWAPI::UnitTypes::Terran_SCV;
	}
	else if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Protoss)
	{
		enemyRaceName = "enemyRace_p";
		featureValues["enemyRace_p"] = 1;
		enemyWork = BWAPI::UnitTypes::Protoss_Probe;
	}
	else
	{
		enemyRaceName = "enemyRace_unknow";
		featureValues["enemyRace_unknow"] = 1;
	}
	//enemy
	int supplyUsed = BWAPI::Broodwar->self()->supplyUsed();
	featureValues["ourSupplyUsed_"] = getNormalizedValue(features["ourSupplyUsed_"], supplyUsed);
	//state buildings
	//self buildings
	/*
	auto x = INFO;
	featureCount = INFO.getAllCount(BWAPI::UnitTypes::Terran_Bunker, S);
	*/
	featurecount = INFO.getActivationGasBaseCount();
	featureValues["our_gas_base_count"] = getNormalizedValue(features["our_gas_base_count"], featurecount);

	featurecount = INFO.getActivationMineralBaseCount();
	featureValues["our_mineral_base_count"] = getNormalizedValue(features["our_mineral_base_count"], featurecount);

	featurecount = INFO.getAllCount(BWAPI::UnitTypes::Terran_Marine, S);
	featureValues["our_Terran_Marine"] = getNormalizedValue(features["our_Terran_Marine"], featurecount);

	featurecount = INFO.getAllCount(BWAPI::UnitTypes::Terran_Ghost, S);
	featureValues["our_Terran_Ghost"] = getNormalizedValue(features["our_Terran_Ghost"], featurecount);

	featurecount = INFO.getAllCount(BWAPI::UnitTypes::Terran_Vulture, S);
	featureValues["our_Terran_Vulture"] = getNormalizedValue(features["our_Terran_Vulture"], featurecount);

	featurecount = INFO.getAllCount(BWAPI::UnitTypes::Terran_Goliath, S);
	featureValues["our_Terran_Goliath"] = getNormalizedValue(features["our_Terran_Goliath"], featurecount);

	featurecount = INFO.getAllCount(BWAPI::UnitTypes::Terran_SCV, S);
	featureValues["our_Terran_SCV"] = getNormalizedValue(features["our_Terran_SCV"], featurecount);

	featurecount = INFO.getAllCount(BWAPI::UnitTypes::Terran_Wraith, S);
	featureValues["our_Terran_Wraith"] = getNormalizedValue(features["our_Terran_Wraith"], featurecount);

	featurecount = INFO.getAllCount(BWAPI::UnitTypes::Terran_Science_Vessel, S);
	featureValues["our_Terran_Science_Vessel"] = getNormalizedValue(features["our_Terran_Science_Vessel"], featurecount);

	featurecount = INFO.getAllCount(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, S);
	featureValues["our_Terran_Siege_Tank_Tank_Mode"] = getNormalizedValue(features["our_Terran_Siege_Tank_Tank_Mode"], featurecount);


	featurecount = INFO.getAllCount(BWAPI::UnitTypes::Terran_Science_Facility, S);
	featureValues["our_Terran_Science_Facility"] = getNormalizedValue(features["our_Terran_Science_Facility"], featurecount);
	//enemy state
	int enemy_feature_count = 0;
	if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Zerg)
	{
		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Zerg_Sunken_Colony, E);
		featureValues["enemyZ_Sunken_"] = getNormalizedValue(features["enemyZ_Sunken_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Zerg_Spore_Colony, E);
		featureValues["enemyZ_Spore_"] = getNormalizedValue(features["enemyZ_Spore_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Zerg_Zergling, E);
		featureValues["enemyZ_Zergling_"] = getNormalizedValue(features["enemyZ_Zergling_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Zerg_Mutalisk, E);
		featureValues["enemyZ_Mutalisk_"] = getNormalizedValue(features["enemyZ_Mutalisk_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Zerg_Hydralisk, E);
		featureValues["enemyZ_Hydra_"] = getNormalizedValue(features["enemyZ_Hydra_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Zerg_Lurker, E);
		featureValues["enemyZ_Lurker_"] = getNormalizedValue(features["enemyZ_Lurker_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Zerg_Scourge, E);
		featureValues["enemyZ_Scourage"] = getNormalizedValue(features["enemyZ_Scourage"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Zerg_Ultralisk, E);
		featureValues["enemyZ_Ultralisk"] = getNormalizedValue(features["enemyZ_Ultralisk"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Zerg_Defiler, E);
		featureValues["enemyZ_Defiler"] = getNormalizedValue(features["enemyZ_Defiler"], enemy_feature_count);

	}
	else if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran)
	{
		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Terran_Missile_Turret, E);
		featureValues["enemyT_missile_"] = getNormalizedValue(features["enemyT_missile_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Terran_Bunker, E);
		featureValues["enemyT_Bunker_"] = getNormalizedValue(features["enemyT_Bunker_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Terran_Goliath, E);
		featureValues["enemyT_Goliath_"] = getNormalizedValue(features["enemyT_Goliath_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Terran_Marine, E);
		featureValues["enemyT_Marine_"] = getNormalizedValue(features["enemyT_Marine_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode, E);
		featureValues["enemyT_Tank_"] = getNormalizedValue(features["enemyT_Tank_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Terran_Vulture, E);
		featureValues["enemyT_Vulture_"] = getNormalizedValue(features["enemyT_Vulture_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Terran_Dropship, E);
		featureValues["enemyT_Dropship_"] = getNormalizedValue(features["enemyT_Dropship_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Terran_Valkyrie, E);
		featureValues["enemyT_Valkyrie_"] = getNormalizedValue(features["enemyT_Valkyrie_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Terran_Science_Facility, E);
		featureValues["enemyT_Science_"] = getNormalizedValue(features["enemyT_Science_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Terran_Firebat, E);
		featureValues["enemyT_Firebat_"] = getNormalizedValue(features["enemyT_Firebat_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Terran_Medic, E);
		featureValues["enemyT_Terran_Medic_"] = getNormalizedValue(features["enemyT_Terran_Medic_"], enemy_feature_count);

	}
	else if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Protoss)
	{
		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Protoss_Citadel_of_Adun, E);
		featureValues["enemyP_cannon_"] = getNormalizedValue(features["enemyP_cannon_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Protoss_Zealot, E);
		featureValues["enemyP_Zealot_"] = getNormalizedValue(features["enemyP_Zealot_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Protoss_Dragoon, E);
		featureValues["enemyP_Dragon_"] = getNormalizedValue(features["enemyP_Dragon_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Protoss_High_Templar, E);
		featureValues["enemyP_High_temple_"] = getNormalizedValue(features["enemyP_High_temple_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Protoss_Carrier, E);
		featureValues["enemyP_Carrier_"] = getNormalizedValue(features["enemyP_Carrier_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Protoss_Corsair, E);
		featureValues["enemyP_Corsair_"] = getNormalizedValue(features["enemyP_Corsair_"], enemy_feature_count);

		enemy_feature_count = INFO.getAllCount(BWAPI::UnitTypes::Protoss_Shuttle, E);
		featureValues["enemyP_Shuttle_"] = getNormalizedValue(features["enemyP_Shuttle_"], enemy_feature_count);
	}
}
bool ActorCriticForProductionStrategy::isGasRequireMeet(int price)
{
	if (price > 0
		&& BWAPI::Broodwar->self()->gas() < price)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool ActorCriticForProductionStrategy::isMineRequireMeet(int price)
{
	if (price > 0
		&& BWAPI::Broodwar->self()->minerals() < price)
	{
		return false;
	}
	else
	{
		return true;
	}
}

string ActorCriticForProductionStrategy::getTargetUnit()
{
	//if (true)
	//{
	//exploreUnit = BWAPI::UnitTypes::Zerg_Hydralisk;
	//}

	featureValues.clear();
	//get current state features
	GetCurrentStateFeature();
	//Broodwar << "GetCurrentStateFeature" << endl;
	map<string, double> filteredActionResult;
	vector<double> nnInput(NNInputAction.size());
	for (size_t i = 0; i < NNInputAction.size(); i++)
	{
		if (featureValues.find(NNInputAction[i]) != featureValues.end())
		{
			nnInput[i] = featureValues[NNInputAction[i]];
			featureValues.erase(NNInputAction[i]);
		}
		else
		{
			nnInput[i] = -1;
		}
	}

	if (featureValues.size() != 0)
	{
		BWAPI::Broodwar->printf("feature error!!!");
	}

	/*
	//softmax output is raw data, not the probability
	vector<double> nnOutput = policyModel.feedForward(nnInput);
	double softmaxTotal = 0;
	for (auto s : nnOutput)
	{
	softmaxTotal += s;
	}
	for (auto& s : nnOutput)
	{
	s = s / softmaxTotal;
	}
	*/
	vector<double> nnOutput = policyModel.feedForward(nnInput);

	std::map<BWAPI::UnitType, std::set<BWAPI::Unit>> ourBuildings;
	for (size_t i = 0; i < nnOutput.size(); i++)
	{
		if (NNOutputAction[i].find("Building_") != std::string::npos)
		{
			if (!isGasRequireMeet(highLevelActions[NNOutputAction[i]].unitType.gasPrice()) ||
				!isMineRequireMeet(highLevelActions[NNOutputAction[i]].unitType.mineralPrice()))
			{
				continue;
			}

			//tech buildings just need one instance
			if (BWAPI::Broodwar->self()->allUnitCount(highLevelActions[NNOutputAction[i]].unitType) > 0)
			{
				continue;
			}
			if (BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Zerg_Hive) > 0
				&& highLevelActions[NNOutputAction[i]].unitType == BWAPI::UnitTypes::Terran_Covert_Ops)
			{
				continue;
			}
			if (BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Zerg_Greater_Spire) > 0
				&& highLevelActions[NNOutputAction[i]].unitType == BWAPI::UnitTypes::Terran_Starport)
			{
				continue;
			}
			bool isValid = true;

			if (isValid)
			{
				filteredActionResult[NNOutputAction[i]] = nnOutput[i];
			}
		}

		else if (NNOutputAction[i].find("Defense_") != std::string::npos)
		{
			if (!isGasRequireMeet(highLevelActions[NNOutputAction[i]].unitType.gasPrice()) ||
				!isMineRequireMeet(highLevelActions[NNOutputAction[i]].unitType.mineralPrice()))
			{
				continue;
			}

			int underBuildCount = 0;
			if (BWAPI::Broodwar->self()->allUnitCount(highLevelActions[NNOutputAction[i]].unitType) + underBuildCount >= 10
				|| BWAPI::Broodwar->getFrameCount() < nextDefenseBuildTime)
				continue;

			if (NNOutputAction[i] =="Defense_Barracks" && BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Barracks) > 0)
			{
				filteredActionResult[NNOutputAction[i]] = nnOutput[i];
			}
			else if (NNOutputAction[i] == "Defense_Armony" && BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Terran_Armory) > 0)
			{
				filteredActionResult[NNOutputAction[i]] = nnOutput[i];
			}
			else
			{
				continue;
			}
		}

		else if (NNOutputAction[i].find("Unit_") != std::string::npos)
		{
			//if (exploreUnit != BWAPI::UnitTypes::None && highLevelActions[NNOutputAction[i]].unitType != exploreUnit)
			//{
				//continue;
			//}
			if (highLevelActions[NNOutputAction[i]].unitType == BWAPI::UnitTypes::Terran_Marine
				&& BWAPI::Broodwar->self()->allUnitCount(BWAPI::UnitTypes::Zerg_Scourge) > 12)
			{
				continue;
			}

			if (!isGasRequireMeet(highLevelActions[NNOutputAction[i]].unitType.gasPrice()) ||
				!isMineRequireMeet(highLevelActions[NNOutputAction[i]].unitType.mineralPrice()))
			{
				continue;
			}

			if (BWAPI::Broodwar->self()->gas() < highLevelActions[NNOutputAction[i]].unitType.gasPrice() * 3
				&& BWAPI::Broodwar->self()->minerals() > 5000 )
			{
				continue;
			}

			int supplyRequired = highLevelActions[NNOutputAction[i]].unitType.supplyRequired();
			if (highLevelActions[NNOutputAction[i]].unitType == BWAPI::UnitTypes::Terran_Ghost
				|| highLevelActions[NNOutputAction[i]].unitType == BWAPI::UnitTypes::Terran_Vulture)
			{
				supplyRequired = supplyRequired * 2;
			}
			if (BWAPI::Broodwar->self()->supplyUsed() + supplyRequired > 200 * 2)
			{
				continue;
			}

			std::map<BWAPI::UnitType, int> requireUnits = highLevelActions[NNOutputAction[i]].unitType.requiredUnits();
			bool isValid = true;
			for (auto u : requireUnits)
			{
				if (u.first != BWAPI::UnitTypes::Zerg_Larva && BWAPI::Broodwar->self()->completedUnitCount(u.first) == 0)
				{
					if (u.first == BWAPI::UnitTypes::Terran_Goliath && BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Goliath) > 0)
					{
						continue;
					}
					if (u.first == BWAPI::UnitTypes::Terran_Dropship && BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Dropship) > 0)
					{
						continue;
					}
					isValid = false;
					break;
				}
			}

			if (isValid)
			{
				if (NNOutputAction[i] == "Unit_Terran_Bunker")
				{
					if (BWAPI::Broodwar->self()->hasResearched(BWAPI::TechTypes::Tank_Siege_Mode))
					{
						filteredActionResult[NNOutputAction[i]] = nnOutput[i];
					}
				}
				else
				{
					filteredActionResult[NNOutputAction[i]] = nnOutput[i];
				}
			}
		}

		//only the lurker tech
		else if (NNOutputAction[i].find("Tech_") != std::string::npos)
		{
			if (!isGasRequireMeet(highLevelActions[NNOutputAction[i]].techType.gasPrice()) ||
				!isMineRequireMeet(highLevelActions[NNOutputAction[i]].techType.mineralPrice()))
			{
				continue;
			}


			if (BWAPI::Broodwar->self()->hasResearched(BWAPI::TechTypes::Lurker_Aspect) == false &&
				BWAPI::Broodwar->self()->isResearching(BWAPI::TechTypes::Lurker_Aspect) == false &&
				BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Zerg_Hydralisk_Den) > 0 &&
				(BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Zerg_Lair) > 0 || 
				BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Zerg_Hive) > 0))
			{
				BWAPI::Unit hydraliskDen = *ourBuildings[BWAPI::UnitTypes::Zerg_Hydralisk_Den].begin();
				if (!hydraliskDen->isResearching() && !hydraliskDen->isUpgrading())
				{
					filteredActionResult[NNOutputAction[i]] = nnOutput[i];
				}
			}
		}

		else if (NNOutputAction[i].find("Upgrade_") != std::string::npos)
		{
			if (!isGasRequireMeet(highLevelActions[NNOutputAction[i]].upgradeType.gasPrice()) ||
				!isMineRequireMeet(highLevelActions[NNOutputAction[i]].upgradeType.mineralPrice()))
			{
				continue;
			}

			BWAPI::UnitType researchingUnit = highLevelActions[NNOutputAction[i]].upgradeType.whatUpgrades();
			BWAPI::UnitType requireUnit = highLevelActions[NNOutputAction[i]].upgradeType.whatsRequired();
		}
		else if (NNOutputAction[i].find("Expand_") != std::string::npos)
		{
			if (!isGasRequireMeet(highLevelActions[NNOutputAction[i]].unitType.gasPrice()) ||
				!isMineRequireMeet(highLevelActions[NNOutputAction[i]].unitType.mineralPrice()))
			{
				continue;
			}
			filteredActionResult[NNOutputAction[i]] = nnOutput[i];
		}
		
		else if (NNOutputAction[i].find("Wait_") != std::string::npos)
		{
			if (BWAPI::Broodwar->getFrameCount() > 24 * 60 * 15)
			{
				filteredActionResult[NNOutputAction[i]] = nnOutput[i];
			}
		}
		else
		{
			BWAPI::Broodwar->printf("error type filter!!");
		}
	}

	////no valid action available
	if (filteredActionResult.size() == 0)
	{
		return "Wait_doNothing";
	}
	//Broodwar << "filteedActtion" << endl;

	/*
	double filteredTotalProb = 0.0f;
	for (auto f : filteredActionResult)
	{
	filteredTotalProb += f.second;
	}

	LARGE_INTEGER cpuTime;
	QueryPerformanceCounter(&cpuTime);
	unsigned int startTimeInMicroSec = cpuTime.QuadPart % 1000000;

	std::srand(startTimeInMicroSec);
	int randomNum = std::rand() % 100;
	double accumulatedProb = 0.0;
	string chosedAction;
	double chosedActionProb;
	for (auto f : filteredActionResult)
	{
	accumulatedProb += f.second / filteredTotalProb;
	if (accumulatedProb * 100 > randomNum)
	{
	chosedAction = f.first;
	chosedActionProb = f.second / filteredTotalProb;
	break;
	}
	}

	vector<double> valueOutput = stateValueModel.feedForward(nnInput);

	//save data for training
	//schema: inputFeature, curStateValue, reward, curChosedActionIndex, curOutputChosen, fromStartIndex, nextStateInputFeature
	if (previousChosenAction != chosedAction)
	{
	std::vector<std::string> dataVector;
	std::stringstream dataString;
	dataString.precision(15);
	dataString << std::fixed;
	for (auto in : nnInput)
	{
	dataString << in << ":";
	}
	dataVector.push_back(dataString.str());
	dataVector.push_back(std::to_string(valueOutput[0]));
	dataVector.push_back(std::to_string(tmpReward));
	tmpReward = 0;
	dataVector.push_back(std::to_string(actionsOutputIndexMapping[chosedAction]));
	std::vector<int> filterOutput;
	filterOutput.reserve(filteredActionResult.size());
	for (auto f : filteredActionResult)
	{
	filterOutput.push_back(actionsOutputIndexMapping[f.first]);
	}
	dataString.str("");
	dataString.clear();
	for (auto f : filterOutput)
	{
	dataString << f << ":";
	}
	dataVector.push_back(dataString.str());
	dataVector.push_back(std::to_string(trainingData.size()));
	dataVector.push_back("");
	trainingData.push_back(dataVector);

	previousChosenAction = chosedAction;
	}

	*/

	double maxValue = -999999;
	string maxAction = "Wait_doNothing";
	double secondValue = -99999;
	string secondAction = "Wait_doNothing";
	for (auto f : filteredActionResult)
	{
		if (f.second > secondValue)
		{
			secondAction = f.first;
			secondValue = f.second;
			if (secondValue > maxValue)
			{
				string tmpAction = maxAction;
				double tmpValue = maxValue;
				maxValue = secondValue;
				maxAction = secondAction;
				secondAction = tmpAction;
				secondValue = tmpValue;
			}
		}
	}
	double diff = (maxValue - secondValue) / maxValue;

	int exploreRate = 0;
	exploreRate = 100 - int(((float(playMatchCount) / 50)) * 90)  < 10 ? 10 : 100 - int(((float(playMatchCount) / 50)) * 90);

	string chosedAction;
	std::random_device rand_dev;
	std::mt19937 generator(rand_dev());
	std::uniform_int_distribution<int>  distr(0, 100);
	int randomNum = distr(generator);
	bool isExplore = false;
	//do explore
	if (randomNum < exploreRate)
	{
		isExplore = true;
		std::uniform_int_distribution<int>  dist2(0, filteredActionResult.size() - 1);
		int randomActionNum = dist2(generator);
		int count = 0;
		for (auto f : filteredActionResult)
		{
			if (count == randomActionNum)
			{
				chosedAction = f.first;
				break;
			}
			count++;
		}

	}
	else
	{
		chosedAction = maxAction;
	}
	//Broodwar << "Maxaction" << endl;
	//BWAPI::Broodwar->printf("current:%s,max:%s,reward:%.2f,\n diff:%f, time: %d", chosedAction.c_str(), maxAction.c_str(), tmpReward, diff, BWAPI::Broodwar->getFrameCount());


	if (filteredActionResult.size() > 1)
	{
		//save data for training
		//schema: curState, action, reward, nextState, currentStateValidActions, nextStateValidActions
		if (trainingData.size() > 0)
		{
			trainingData.back()[2] = std::to_string(tmpReward);
		}
		tmpReward = 0;
		std::vector<std::string> dataVector;
		std::stringstream dataString;
		dataString.precision(15);
		dataString << std::fixed;
		for (auto in : nnInput)
		{
			dataString << in << ":";
		}
		dataVector.push_back(dataString.str());
		dataVector.push_back(std::to_string(actionsOutputIndexMapping[chosedAction]));
		dataVector.push_back("0");
		dataVector.push_back("");
		std::vector<int> filterOutput;
		filterOutput.reserve(filteredActionResult.size());
		for (auto f : filteredActionResult)
		{
			filterOutput.push_back(actionsOutputIndexMapping[f.first]);
		}
		dataString.str("");
		dataString.clear();
		for (auto f : filterOutput)
		{
			dataString << f << ":";
		}
		dataVector.push_back(dataString.str());
		dataVector.push_back("");
		trainingData.push_back(dataVector);
	}
	//Broodwar << "datasave" << endl;
	return chosedAction;
}


sampleParsedInfo ActorCriticForProductionStrategy::sampleParse(std::vector<std::string>& sample)
{
	sampleParsedInfo result;

	std::stringstream ss;
	ss.str("");
	ss.clear();
	ss << sample[0];
	string item;
	while (getline(ss, item, ':'))
	{
		if (item == "")
			continue;
		result.modelInputs.push_back(std::stod(item));
	}
	result.actualChosen = std::stoi(sample[1]);
	// 	ss.str("");
	// 	ss.clear();
	// 	ss << sample[0];
	// 	std::vector<double> ItemList;
	// 	while (getline(ss, item, ':'))
	// 	{
	// 	if (item == "")
	// 	continue;
	// 	ItemList.push_back(std::stod(item));
	// 	}
	// 	double currentStateValue = stateValueModel.feedForward(ItemList);//计算当前状态的价值
	double targetQValue = 0;
	//terminal state-- next state value is empty
	if (sample[3] == "")
	{
		targetQValue = std::stod(sample[2]);
	}
	else
	{
		ss.str("");
		ss.clear();
		ss << sample[3];
		std::vector<double> nextItemList;
		while (getline(ss, item, ':'))
		{
			if (item == "")
				continue;
			nextItemList.push_back(std::stod(item));
		}
		//double Q learning
		vector<double> nextStateValue = stateValueModel.feedForward(nextItemList);//计算下一个状态的状态价值
		/*
		ss.str("");
		ss.clear();
		ss << sample[5];
		std::set<int> outList;
		while (getline(ss, item, ':'))
		{
		if (item == "")
		continue;
		outList.insert(std::stoi(item));
		}
		double maxValue = -999999;
		int	maxActionIndex = 0;
		for (size_t i = 0; i < allActionValues.size(); i++)
		{
		if (outList.find(i) != outList.end() && allActionValues[i] > maxValue)
		{
		maxValue = allActionValues[i];
		maxActionIndex = i;
		}
		}

		double targetMaxValue = targetAllActionValues[maxActionIndex];
		*/
		targetQValue = std::stod(sample[2]) + std::pow(discountRate, tdStep) * nextStateValue[0];//R+r*Q(st+1)-Q(st)
	}
	result.targetQValue = targetQValue;

	return result;
}


double ActorCriticForProductionStrategy::getSamplePriority(std::vector<std::string>& sample)
{
	sampleParsedInfo result = sampleParse(sample);
	vector<double> allActionValues = QValueModel.feedForward(result.modelInputs);
	double TDError = allActionValues[result.actualChosen] - result.targetQValue;
	return  std::pow(std::abs(TDError) + 0.01, 0.6);
}


void ActorCriticForProductionStrategy::trainModels(int reward)
{
	//log_info file
	//fstream log_info;
	//std::string enemyName = BWAPI::Broodwar->enemy()->getName();
	//	std::string filePath_log = writeResourceFolder;
	//	filePath_log += enemyName + "_log_trainModels";
	//for each enemy, create a file
	//log_info.open(filePath_log.c_str(), ios::out);
	std::string ss = "1";
	//log_info << "std::string ss" << endl;
	//log_infofile
	if (trainingData.size() > 0)
	{
		//	log_info << "if (trainingData.size() > 0)" << endl;
		std::map<std::string, int> actionExecutionCount;
		trainingData[trainingData.size() - 1][2] = std::to_string(reward + tmpReward);
		for (int i = 0; i < int(trainingData.size()) - 1; i++)
		{
			int nStepIndex = i + tdStep;
			//schema: curState, action, reward, nextState, currentStateValidActions, nextStateValidActions
			if (nStepIndex <= int(trainingData.size()) - 1)
			{
				trainingData[i][3] = trainingData[nStepIndex][0];
				trainingData[i][5] = trainingData[nStepIndex][4];
			}

			double totalReward = 0;
			int step = 0;
			if (nStepIndex > int(trainingData.size()) - 1)
			{
				nStepIndex = int(trainingData.size());
			}
			for (int j = i; j < nStepIndex; j++)
			{
				totalReward += std::pow(discountRate, step) * std::stod(trainingData[j][2]);
				step++;
			}
			trainingData[i][2] = std::to_string(totalReward);
		}

		for (auto sample : trainingData)
		{
			//double priority = getSamplePriority(sample);
			double priority = 1;//更改优先级为统一方式
			replayData.add(priority, sample);
		}
	}

	if (replayData.getData().size() == 0){
		return;
	}

	int trainingRound = 200;//trainingData.size() < 200 ? 200 : trainingData.size();
	int miniBatch = 8;
	vector<double> loss;

	for (int round = 0; round < trainingRound; round++)
	{
		double curError = iterationTrain(miniBatch);
		//	log_info << "double curError = iterationTrain(miniBatch);" << endl;
		loss.push_back(curError);
	}

	/*
	fstream historyFile;
	string filePath = "./bwapi-data/write/loss_data";
	historyFile.open(filePath.c_str(), ios::app);
	double total = 0;
	historyFile << "round:" << playMatchCount <<endl;
	for (size_t i = 0; i < loss.size();)
	{
	total += loss[i];
	i++;
	if (i % 10 == 0 && i != 0)
	{
	historyFile << total / 10;
	historyFile << endl;
	total = 0;
	}
	}
	historyFile << endl;
	historyFile.close();
	*/

	//tesetSetPerformance();
	experienceDataSave();
	//log_info << "experienceDataSave();" << endl;
	string enemyName = BWAPI::Broodwar->enemy()->getName();
	policyModel.serialize("policy", writeResourceFolder);
	stateValueModel.serialize("stateValue", writeResourceFolder);
	//log_info << "experienceDataSave();" << endl;
	//QValueModel.serialize(enemyName + "_stateValue", writeResourceFolder);
	//log_info.close();
	/*
	int targetUpdateFrequence = 10;
	if (playMatchCount >= 50)
	{
	targetUpdateFrequence = 20;
	}
	else if (playMatchCount >= 100)
	{
	targetUpdateFrequence = 40;
	}
	*/
	//target network delay update
	//if (playMatchCount % targetUpdateFrequence == 0)
	//{
	//targetQValueModel.copy(QValueModel);
	//}
	//targetQValueModel.serialize(enemyName + "_targetQValueModel", writeResourceFolder);


	/*
	std::map<std::string, int> actionExecutionCount;
	for (int i = 0; i < int(trainingData.size()); i++)
	{
	double dReward = std::pow(discountRate, trainingData.size() - 1 - i) * reward;
	trainingData[i][2] = std::to_string(dReward);

	//std::size_t pos = NNOutputAction[std::stoi(trainingData[i][3])].find("_");
	//std::string categoty = NNOutputAction[std::stoi(trainingData[i][3])].substr(0, pos);
	std::string categoty = NNOutputAction[std::stoi(trainingData[i][3])];
	if (actionExecutionCount.find(categoty) == actionExecutionCount.end())
	{
	actionExecutionCount[categoty] = 0;
	}
	actionExecutionCount[categoty] += 1;
	}

	int maxCount = 0;
	for (auto e : actionExecutionCount)
	{
	if (e.second > maxCount)
	{
	maxCount = e.second;
	}
	}

	std::vector<std::vector<std::string>> tmpAdd;
	for (int i = 0; i < int(trainingData.size()); i++)
	{
	std::string categoty = NNOutputAction[std::stoi(trainingData[i][3])];
	if (categoty.find("Upgrade_") == std::string::npos)
	{
	int addCopyRatio = (int(maxCount / actionExecutionCount[categoty]) - 1);
	tmpAdd.insert(tmpAdd.end(), addCopyRatio, trainingData[i]);
	}
	}

	trainingData.insert(trainingData.end(), tmpAdd.begin(), tmpAdd.end());
	if (trainingData.size() != 0)
	{
	experienceData.push_back(trainingData);
	}
	}

	if (experienceData.size() == 0)
	return;

	vector<int> loseMatchIndex;
	vector<int> winMatchIndex;
	for (size_t i = 0; i < experienceData.size(); i++)
	{
	if (std::stod(experienceData[i][0][2]) < 0)
	{
	loseMatchIndex.push_back(i);
	}
	else
	{
	winMatchIndex.push_back(i);
	}
	}

	std::srand((unsigned int)std::time(0));
	int GDRound = 100;
	int eachGdRoundSampleMatch = 10;
	if (experienceData.size() < 10)
	{
	GDRound = experienceData.size();
	eachGdRoundSampleMatch = 3;
	}

	for (int i = 0; i < GDRound; i++)
	{
	std::vector<int> sampleMatches;
	for (int sample = 0; sample < eachGdRoundSampleMatch; sample++)
	{
	//int matchIndex = std::rand() % experienceData.size();
	int matchIndex = 0;
	if (loseMatchIndex.size() > 0 && winMatchIndex.size() > 0)
	{
	if (sample % 3 == 0)
	{
	matchIndex = loseMatchIndex[std::rand() % loseMatchIndex.size()];
	}
	else
	{
	matchIndex = winMatchIndex[std::rand() % winMatchIndex.size()];
	}
	}
	else
	{
	matchIndex = std::rand() % experienceData.size();
	}

	sampleMatches.push_back(matchIndex);
	}

	double curError = iterationTrain(sampleMatches);
	}

	//repeat training instance with largest error
	//iterationTrain(maxMatchIndex);

	experienceDataSave();
	policyModel.serialize("policy");
	stateValueModel.serialize("stateValue");
	stateValueTargetModel.serialize("stateValueTarget");
	*/
}


double ActorCriticForProductionStrategy::iterationTrain(int miniBatch)
{
	//log_info file
	//fstream log_info;
	//std::string enemyName = BWAPI::Broodwar->enemy()->getName();
	//std::string filePath_log = writeResourceFolder;
	//filePath_log += enemyName + "_iterationTrain";
	//for each enemy, create a file
	//log_info.open(filePath_log.c_str(), ios::out);
	//std::string ss = "1";
	//log_info << "std::string ss" << endl;
	//log_infofile
	map<int, vector<string>> sampleDatas = replayData.sample(miniBatch);

	vector<vector<double>> modelInputs;
	vector<int> actualChosen;
	vector<double> targetValues;
	for (auto instance : sampleDatas)
	{
		//double priority = getSamplePriority(instance.second);
		double priority = 1;//更改优先级为1，采用均衡采样，非优先采样模式
		replayData.updatePriority(instance.first, priority);

		//importance sampling weight

		sampleParsedInfo parseData = sampleParse(instance.second);
		modelInputs.push_back(parseData.modelInputs);
		actualChosen.push_back(parseData.actualChosen);
		targetValues.push_back(parseData.targetQValue);
	}

	vector<double> qValues(modelInputs.size());
	//log_info << "vector<double> qValues;" << endl;//log_infofile
	double curError_state = stateValueModel.train(modelInputs, MSE, targetValues, actualChosen);
	//log_info << "double curError_state" << endl;//log_infofile
	//std::string model_size = std::to_string(modelInputs.size());
	//log_info << model_size << endl;//log_infofile
	for (size_t instance = 0; instance <modelInputs.size(); instance++)
	{
		vector<double> vresults = stateValueModel.feedForward(modelInputs[instance]);
		//log_info << "vector<double> vresults " << endl;//log_infofile
		stringstream oss;
		for (auto resu : vresults)
		{
			oss << resu;
			//log_info << oss.str() << endl;//log_infofile
		}
		//log_info <<"end_vresults"<< endl;//log_infofile
		qValues[instance] = vresults[0];
		//log_info << "qValues[instance]" << endl;//log_infofile
	}
	//log_info << "qValues[instance] = vresults[0];" << endl;//log_infofile
	double curError_policy = policyModel.train(modelInputs, POLICY_GRADIENT, targetValues, actualChosen, qValues);
	//log_info << "policyModel.train(modelInputs, POLICY_GRADIENT, targetValues," << endl;//log_infofile
	double curError = curError_policy + curError_state;
	//log_info.close();
	return curError;


	/*
	vector<vector<double>> modelInputs;
	vector<double> expectValue;
	vector<int> actualChosen;
	vector<vector<int>> outputChosen;
	vector<double> qValues;
	vector<int> indexValue;
	std::stringstream ss;
	for (int i = 0; i < miniBatch; i++)
	{
	int index = std::rand() % experienceData.size();
	ss.str("");
	ss.clear();
	ss << experienceData[index][0];
	std::vector<double> itemList;
	string item;
	while (getline(ss, item, ':'))
	{
	if (item == "")
	continue;
	itemList.push_back(std::stod(item));
	}
	modelInputs.push_back(itemList);
	ss.str("");
	ss.clear();
	ss << experienceData[index][4];
	std::vector<int> outList;
	while (getline(ss, item, ':'))
	{
	if (item == "")
	continue;
	outList.push_back(std::stoi(item));
	}
	outputChosen.push_back(outList);
	//expectValue.push_back(std::stod(experienceData[matchIndex][index][2]));
	actualChosen.push_back(std::stoi(experienceData[index][3]));
	indexValue.push_back(std::stoi(experienceData[index][5]));

	double qValue = 0;
	double targetQValue = 0;
	//terminal state
	if (experienceData[index][6] == "")
	{
	qValue = targetQValue = std::stod(experienceData[index][2]);
	}
	else
	{
	ss.str("");
	ss.clear();
	ss << experienceData[index][6];
	std::vector<double> nextItemList;
	while (getline(ss, item, ':'))
	{
	if (item == "")
	continue;
	nextItemList.push_back(std::stod(item));
	}
	qValue = std::stod(experienceData[index][2]) + std::pow(discountRate, tdStep) * stateValueModel.feedForward(nextItemList)[0];
	targetQValue = std::stod(experienceData[index][2]) + std::pow(discountRate, tdStep) * stateValueTargetModel.feedForward(nextItemList)[0];
	}

	double baseline = stateValueModel.feedForward(itemList)[0];
	//reinforce with baseline
	qValues.push_back(qValue - baseline);
	expectValue.push_back(targetQValue);
	}

	double curError = policyModel.train(modelInputs, POLICY_GRADIENT, vector<double>(), actualChosen,
	outputChosen, qValues, indexValue, miniBatch);

	stateValueModel.train(modelInputs, MSE, expectValue);

	stateValueTargetModel.targetNetworkUpdate(stateValueModel);

	return curError;
	*/
}


void ActorCriticForProductionStrategy::tesetSetPerformance()
{
	vector<double> maxQValues;
	vector<vector<double>> modelInputs;
	vector<int> actualChosen;
	vector<vector<int>> outputChosen;
	vector<double> targetValues;
	std::stringstream ss;
	for (size_t index = 0; index < testSetData.size(); index++)
	{
		string item;
		ss.str("");
		ss.clear();
		ss << testSetData[index][0];
		std::vector<double> nextItemList;
		while (getline(ss, item, ':'))
		{
			if (item == "")
				continue;
			nextItemList.push_back(std::stod(item));
		}
		vector<double> allActionValues = QValueModel.feedForward(nextItemList);

		ss.str("");
		ss.clear();
		ss << testSetData[index][4];
		std::set<int> outList;
		while (getline(ss, item, ':'))
		{
			if (item == "")
				continue;
			outList.insert(std::stoi(item));
		}
		double maxValue = -999999;
		for (size_t i = 0; i < allActionValues.size(); i++)
		{
			if (outList.find(i) != outList.end() && allActionValues[i] > maxValue)
			{
				maxValue = allActionValues[i];
			}
		}
		maxQValues.push_back(maxValue);
	}

	double sumValue = 0;
	for (auto m : maxQValues)
	{
		sumValue += m;
	}
	testSetAvgQValue = sumValue / maxQValues.size();
}


void ActorCriticForProductionStrategy::update(int reward)
{
	trainModels(reward);
}

ActorCriticForProductionStrategy & ActorCriticForProductionStrategy::Instance()
{
	static ActorCriticForProductionStrategy instance;
	return instance;
}

int ActorCriticForProductionStrategy::getScore()
{
	int unitScore = BWAPI::Broodwar->self()->getUnitScore();
	int buildingScore = BWAPI::Broodwar->self()->getBuildingScore();
	int resourceScore = BWAPI::Broodwar->self()->gatheredMinerals() + BWAPI::Broodwar->self()->gatheredGas();
	return unitScore + buildingScore + resourceScore;
}


std::vector<ACMetaType> ActorCriticForProductionStrategy::getMetaVector(std::string buildString)
{
	std::stringstream ss;
	ss << buildString;
	std::vector<ACMetaType> meta;

	int action(0);
	while (ss >> action)
	{
		meta.push_back(actions[action]);
	}
	return meta;
}
