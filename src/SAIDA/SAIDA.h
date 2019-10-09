#pragma once

#include <BWAPI.h>
#include <BWAPI/Client.h>

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <string>

#include "Common.h"
#include "CommandUtil.h"
#include "GameCommander.h"
#include "ActorCriticForProductionStrategy.h"
namespace MyBot
{
	class Stormbreaker : public BWAPI::AIModule
	{
		GameCommander   gameCommander;

		/// 解析处理文本
		void ParseTextCommand(const string &commandLine);
		_se_translator_function original;

	public:
		Stormbreaker();
		~Stormbreaker();
		void Broodwar_set();
		void onStart();
		void onEnd(bool isWinner);
		void onFrame();
		void drawgameinfo();
		void drawUnitStatus();
		void drawBullets();
		void drawVisibilityData();
		void showPlayers();
		void showForces();
		void onUnitCreate(BWAPI::Unit unit);
		void onUnitDestroy(BWAPI::Unit unit);
		void onUnitMorph(BWAPI::Unit unit); //处理当单位（建筑物 / 地面单位 / 空中单位）的玩家改变
		void onUnitRenegade(BWAPI::Unit unit);//单位叛变
		void onUnitComplete(BWAPI::Unit unit);//单位完成
		void onUnitDiscover(BWAPI::Unit unit);
		//单位发现物体时处理
		void onUnitEvade(BWAPI::Unit unit);//单位逃走
		void onUnitShow(BWAPI::Unit unit);
		void onUnitHide(BWAPI::Unit unit);
		void onNukeDetect(BWAPI::Position target);

	private:
		string mapFileName;

		// BasicBot 1.1 Patch End //////////////////////////////////////////////////

	};
}