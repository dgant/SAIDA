#include "network.h"

//Initialize network with one layer and 0 input/outputs
Network::Network() 
{
	setLearningRate(0.2);
}

void Network::copy(Network n)
{
	network = n.network;
}


void Network::targetNetworkUpdate(Network& n)
{
	for (size_t layer = 0; layer < n.network.size(); layer++)
	{
		for (size_t neuron = 0; neuron < n.network[layer].size(); neuron++)
		{
			for (size_t weightIndex = 0; weightIndex < n.network[layer][neuron].weights.size(); weightIndex++)
			{
				network[layer][neuron].weights[weightIndex] = 0.001 * n.network[layer][neuron].weights[weightIndex] + 0.999 * network[layer][neuron].weights[weightIndex];
			}
			network[layer][neuron].biasWeight = 0.001 * n.network[layer][neuron].biasWeight + 0.999 * network[layer][neuron].biasWeight;
		}
	}
}


//Set the networks learning rate
void Network::setLearningRate(double rate) 
{
	for (size_t i = 0; i < network.size(); i++) 
	{
		for (size_t j = 0; j < network[i].size(); j++) 
		{
			network[i][j].setLearningRate(rate);
		}
	}
}


//input layer is not shown in the network
void Network::setNumInputs(int ninp) 
{
	for (size_t i = 0; i < network[0].size(); i++) 
	{
		network[0][i].setNumInputs(ninp);
	}
}

//Set the depth of the network
void Network::setDepth(int depth) 
{
	network.resize(depth);
}

//Set the height of the network at a given layer
void Network::setNodeCountAtLevel(int count, int level, activeFunctionType af)
{
	network[level].resize(count);
	for (auto& n : network[level])
	{
		n.setActiveFunction(af);
	}
}

//This function sets the number of inputs in each layer to the number of neuron in the previous layer
void Network::linkNeurons() 
{
	for (size_t i = 1; i < network.size(); i++) 
	{
		for (size_t j = 0; j < network[i].size(); j++) 
		{
			network[i][j].setNumInputs(network[i-1].size());
		}
	}
}

//Prints each neuron in each layer and the weights of each
void Network::printNetwork() 
{
	for (size_t i = 0; i < network.size(); i++) 
	{
		cout << "Layer " << i << "\n";
		for (size_t j = 0; j < network[i].size(); j++) 
		{
			network[i][j].print();
		}
	}
}

//Randomly sets the weights for each connection
void Network::initNetwork()
{
	for (size_t i = 0; i < network.size(); i++) 
	{
		for (size_t j = 0; j < network[i].size(); j++) 
		{
			network[i][j].resetWeights(i, j);
		}
	}
}

//Applies the inputs to the network and returns the output
vector<double> Network::feedForward(vector<double> inputs)
{
	vector<int> noneZeroIndex;
	for (size_t i = 0; i < inputs.size(); i++)
	{
		if (inputs[i] != 0.0)
		{
			noneZeroIndex.push_back(i);
		}
	}

	for (size_t i = 0; i < network.size(); i++) 
	{
		vector<double> lInp(network[i].size());
		for (size_t j = 0; j < network[i].size(); j++)
		{
			//input is sparse
			if (i == 0)
			{
				lInp[j] = network[i][j].activation(inputs, noneZeroIndex);
			}
			else
			{
				lInp[j] = network[i][j].activation(inputs);
			}
		}
		inputs = lInp;
	}
	return inputs;
}


void Network::numericGradientCalculate(vector<vector<double>> multiInputs, vector<int> actualChosen, 
	vector<vector<int>> outputChosen, bool isMSE, vector<double> expectValue)
{
	for (size_t layer = 0; layer < network.size(); layer++)
	{
		for (size_t neuronIndex = 0; neuronIndex < network[layer].size(); neuronIndex++)
		{
			for (int weightIndex = 0; weightIndex < network[layer][neuronIndex].getInputNum(); weightIndex++)
			{



				double difference = 0.0f;
				double addLossFunctionValue = 0;
				double subtrackLoss = 0;
				double regulation_term = 0;
				double regulation_term_sub = 0;
				for (size_t inputIndex = 0; inputIndex < multiInputs.size(); inputIndex++)
				{
					vector<double> testInput = multiInputs[inputIndex];
					int actualChosenIndex = actualChosen[inputIndex];
					regulation_term_sub = 0;
					regulation_term = 0;
					for (size_t i = 0; i < network.size(); i++)
					{
						vector<double> lInp(network[i].size());
						for (size_t j = 0; j < network[i].size(); j++)
						{
							vector<double>& allWeights = network[i][j].getAllWeight();
							for (size_t w_r = 0; w_r < allWeights.size(); w_r++)
							{
								if (w_r == weightIndex && i == layer && j == neuronIndex)
									regulation_term += 0.05 * pow((allWeights[w_r] + 0.0001), 2);
								else
									regulation_term += 0.05 * pow(allWeights[w_r], 2);
							}

							if (i == layer && j == neuronIndex)
							{
								lInp[j] = network[i][j].activationChange(testInput, weightIndex, true);
								
							}
							else
							{
								lInp[j] = network[i][j].activation(testInput);

							}
						}
						testInput = lInp;
					}
					
					if (isMSE)
					{
						addLossFunctionValue += 0.5 * std::pow((testInput[actualChosenIndex] - expectValue[inputIndex]), 2) / multiInputs.size();
					}
					else
					{
						double tmpAdd = 0;
						for (auto o : outputChosen[inputIndex])
						{
							tmpAdd += testInput[o];
						}

						addLossFunctionValue = testInput[actualChosenIndex] / tmpAdd;
						addLossFunctionValue = addLossFunctionValue - 0.1 * regulation_term;
					}
					
					
					testInput = multiInputs[inputIndex];
					for (size_t i = 0; i < network.size(); i++)
					{
						vector<double> lInp(network[i].size());
						for (size_t j = 0; j < network[i].size(); j++)
						{
							vector<double>& allWeights = network[i][j].getAllWeight();
							for (size_t w_r = 0; w_r < allWeights.size(); w_r++)
							{
								if (w_r == weightIndex && i == layer && j == neuronIndex)
									regulation_term_sub += 0.05 * pow((allWeights[w_r] - 0.0001), 2);
								else
									regulation_term_sub += 0.05 * pow(allWeights[w_r], 2);
							}

							if (i == layer && j == neuronIndex)
							{
								lInp[j] = network[i][j].activationChange(testInput, weightIndex, false);
							}
							else
							{
								lInp[j] = network[i][j].activation(testInput);
							}
						}
						testInput = lInp;
					}
					
					if (isMSE)
					{
						subtrackLoss += 0.5 * std::pow((testInput[actualChosenIndex] - expectValue[inputIndex]), 2) / multiInputs.size();
					}
					else
					{
						double tmpAdd = 0;
						for (auto o : outputChosen[inputIndex])
						{
							tmpAdd += testInput[o];
						}
						subtrackLoss = testInput[actualChosenIndex] / tmpAdd;
						subtrackLoss = subtrackLoss - 0.1 * regulation_term;
					}

				}
				difference = (addLossFunctionValue + regulation_term - subtrackLoss - regulation_term_sub) / 0.0002;
				printf("numeric check: layer: %d, neuron: %d, weightIndex: %d gradient: %f \n",
					layer, neuronIndex, weightIndex, difference);

			}
		}
	}
}


//Train using back propagation
double Network::train(vector<vector<double>>& multiInputs, lossFunctionType loss,
	const vector<double>& expectValue, const vector<int>& actualChosen, const vector<double>& qValues, const vector<vector<int>>& outputChosen,
	const vector<int>& indexValue, double totalMatchCount)//outputChosen:当前状态可用得动作列表，actualChosen：当前状态选择得动作，qvalues:当前状态价值,expectValue:当前状态所选动作价值
{
	//log_info file
	//for each enemy, create a file
	std::string ss;
	if (loss == MSE)
	{
		ss = "MSE";
	}
	else
	{
		ss = "POLICY";
	}
	//fstream log_info;
	//std::string filePath_log = "D:/software/StarCraft/StarCraft/bwapi-data/write/network_train_"+ss;
	//log_info.open(filePath_log.c_str(), ios::out);
	//log_info <<"log_info.open"<< endl;//log_infofile
	double totalError = 0.0;
	for (size_t instance = 0; instance < multiInputs.size(); instance++)
	{
		//Feed forward for results
		vector<double> vresults = feedForward(multiInputs[instance]);
		double multiplyValue = 1.0f;
		if (loss == POLICY_GRADIENT)//策略网络参数更新
		{
			vector<double> results(vresults);//e指数激活
			vector<double> softmaxresults(results.size(),0) ;//动作概率分布
			//log_info << "vector<double> softmaxresults ;//动作概率分布" << endl;
			double softmaxTotal = 0;
			for (size_t i = 0; i < results.size(); i++)
			{
				softmaxTotal += results[i];

			}
			for (size_t i = 0; i < results.size(); i++)
			{
				softmaxresults[i] = results[i] / softmaxTotal;
				//log_info << std::to_string(softmaxresults[i]) << endl;
			}//概率归一化,
			int actualChosenIndex = actualChosen[instance];
			//multiplyValue = (expectValue[instance]-qValues[instance]) * (1 / results[actualChosenIndex]) * std::pow(0.99, indexValue[instance]) * (1 / totalMatchCount);//tidu p(a,s)*Q(s,a)=1/p*Q(s,a)
			multiplyValue = (expectValue[instance] - qValues[instance]) * (1 / softmaxresults[actualChosenIndex]);//tidu logp(a,s)*Q(s,a)=1/p*Q(s,a)
			//Calculate output error.
			for (size_t i = 0; i < results.size(); i++)
			{
				if (i == actualChosenIndex)
				{
					network[network.size() - 1][i].setErrorSignal(std::pow(softmaxTotal,-2.0)*(softmaxTotal-results[i])*multiplyValue);
					totalError += (expectValue[instance] - qValues[instance]) *  softmaxresults[actualChosenIndex];//sigmoid层梯度*目标函数梯度
				}
				else
				{
					network[network.size() - 1][i].setErrorSignal(std::pow(softmaxTotal, -2.0)*( - results[i])*multiplyValue);
				}
			}
            		
		}
		//MSE loss
		else//价值网络参数更新
		{
			//log_info << "价值网络参数更新" << endl;//log_infofile
			double results = vresults[0];
			double gradient = 0;
			double tmp = (results - expectValue[instance]) / multiInputs.size();
			gradient = tmp; //> 1 ? 1 : (tmp < -1 ? -1 : tmp);
			totalError += std::pow(results - expectValue[instance], 2);
			network[network.size() - 1][0].setErrorSignal(gradient);
		//	log_info << "network[network.size() - 1][0]" << endl;//log_infofile
		}

		//back propagate
		//log_info << "//back propagate" << endl;//log_infofile
		for (int i = (int)network.size() - 2; i >= 0; i--)
		{
			for (size_t hid = 0; hid < network[i].size(); hid++)
			{
				//log_info << "i,hid" << endl;//log_infofile
				double backPropVal = 0;
				//Summation of: (W_i * Err_i)
				for (size_t bpvI = 0; bpvI < network[i + 1].size(); bpvI++)
				{
					backPropVal += (network[i + 1][bpvI].getWeight(hid) * network[i + 1][bpvI].getErrorSignal());
				}

				//Error = O * (1 - O) * sum(W_i * Err_i) * optional_value
				if (loss == POLICY_GRADIENT)
					network[i][hid].setErrorSignal(0.0);
				else
					network[i][hid].setErrorSignal(network[i][hid].getBpDerivative() * backPropVal);
			}
		}
	}

	//update weight
	//log_info << "//update weight" << endl;//log_infofile
 	for (size_t i = 0; i < network.size(); i++)
	{
		for (size_t hid = 0; hid < network[i].size(); hid++)
		{
			//network[i][hid].debugPrintGradient(i, hid);
			//log_info << "i,hid" << endl;//log_infofile
			if (loss == POLICY_GRADIENT)
				network[i][hid].adjustForError(false);
			else
				network[i][hid].adjustForError(true);//true沿着负梯度方向使得目标函数减小,false沿着梯度方向使得目标函数增加

		}
	}
	//log_info.close();
	return totalError;
}


void Network::serialize(string modelName, string pathName)
{
	string filePath;
	filePath = pathName + "DLmodel_" + modelName;
	fstream NNModel;

	NNModel << std::fixed << std::setprecision(30);
	NNModel.open(filePath.c_str(), ios::out);

	for (auto row : network)
	{
		for (auto n : row)
		{
			string neuronString = n.serialize();
			NNModel << neuronString << "$";
		}
		NNModel << "#";
	}
	NNModel.close();
}


int Network::deserialize(string modelName, string pathName)
{
	string filePath;
	filePath = pathName + "DLmodel_" + modelName;

	fstream NNModel;
	NNModel.open(filePath.c_str(), ios::in);

	if (NNModel.is_open())
	{
		string content;
		while (getline(NNModel, content, '#'))
		{
			if (content == "")
				continue;
			
			std::stringstream ss(content);
			std::vector<Neuron> itemList;
			string item;
			while (getline(ss, item, '$'))
			{
				if (item == "")
					continue;
				Neuron n;
				n.deserialize(item);
				itemList.push_back(n);
			}
			network.push_back(itemList);
		}
		NNModel.close();
		return 0;
	}
	else
	{
		return -1;
	}
}

