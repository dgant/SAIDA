//////////////////////////////////////////////////////////////////////////
//
// This file is part of the BWEM Library.
// BWEM is free software, licensed under the MIT/X11 License.
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2015, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "mapDrawer.h"
#include "map.h"

using namespace BWAPI;
using namespace BWAPI::UnitTypes::Enum;
namespace {
	auto &bw = Broodwar;
}

using namespace std;


namespace BWEM {

	using namespace BWAPI_ext;

	namespace utils {


		//////////////////////////////////////////////////////////////////////////////////////////////
		//                                                                                          //
		//                                  class MapDrawer
		//                                                                                          //
		//////////////////////////////////////////////////////////////////////////////////////////////

		bool MapDrawer::showSeas						= false;
		bool MapDrawer::showLakes						= false;
		bool MapDrawer::showUnbuildable					= false;
		bool MapDrawer::showGroundHeight				= false;
		bool MapDrawer::showMinerals					= false;
		bool MapDrawer::showGeysers						= false;
		bool MapDrawer::showStaticBuildings				= false;
		bool MapDrawer::showBases						= true;
		bool MapDrawer::showAreas						= false;
		bool MapDrawer::showSmallAreas					= false;
		bool MapDrawer::showAssignedRessources			= false;
		bool MapDrawer::showFrontier					= false;
		bool MapDrawer::showCP							= true;
		bool MapDrawer::showSupply						= false;
		bool MapDrawer::showReserved					= false;
		bool MapDrawer::showId							= true;
		bool MapDrawer::showHide						= true;
		bool MapDrawer::showStatus						= true;

		const Color MapDrawer::Color::sea					= BWAPI::Colors::Blue;
		const Color MapDrawer::Color::lakes					= BWAPI::Colors::Blue;
		const Color MapDrawer::Color::unbuildable			= BWAPI::Colors::Brown;
		const Color MapDrawer::Color::highGround			= BWAPI::Colors::Brown;
		const Color MapDrawer::Color::veryHighGround		= BWAPI::Colors::Red;
		const Color MapDrawer::Color::minerals				= BWAPI::Colors::Cyan;
		const Color MapDrawer::Color::geysers				= BWAPI::Colors::Green;
		const Color MapDrawer::Color::staticBuildings		= BWAPI::Colors::Purple;
		const Color MapDrawer::Color::bases					= BWAPI::Colors::Blue;
		const Color MapDrawer::Color::areas					= BWAPI::Colors::Yellow;
		const Color MapDrawer::Color::smallAreas			= BWAPI::Colors::Purple;
		const Color MapDrawer::Color::assignedRessources	= BWAPI::Colors::Blue;
		const Color MapDrawer::Color::frontier				= BWAPI::Colors::Grey;
		const Color MapDrawer::Color::cp					= BWAPI::Colors::White;
		const Color MapDrawer::Color::supply				= BWAPI::Colors::Blue;
		const Color MapDrawer::Color::reserved				= BWAPI::Colors::Grey;
		const Color MapDrawer::Color::id					= BWAPI::Colors::Grey;
		const Color MapDrawer::Color::status				= BWAPI::Colors::Grey;


		bool MapDrawer::ProcessCommandVariants(const string &command, const string &attributName, bool &attribut)
		{
			bool isChange = false;

			if (command == "show " + attributName) {
				attribut = true;
				isChange = true;
			}
			else if (command == "hide " + attributName) {
				attribut = false;
				isChange = true;
			}

			if (command == attributName) {
				attribut = !attribut;
				isChange = true;
			}

			if (attribut && isChange) {
				if (attributName == "seas") {
					MapDrawer::showUnbuildable = false;
				}
				else if (attributName == "unbuildable") {
					MapDrawer::showSeas = false;
				}

				return true;
			}

			return isChange;
		}


		bool MapDrawer::ProcessCommand(const string &command)
		{
			if (ProcessCommandVariants(command, "seas", showSeas))								return true;

			if (ProcessCommandVariants(command, "lakes", showLakes))							return true;

			if (ProcessCommandVariants(command, "unbuildable", showUnbuildable))				return true;

			if (ProcessCommandVariants(command, "gh", showGroundHeight))						return true;

			if (ProcessCommandVariants(command, "minerals", showMinerals))						return true;

			if (ProcessCommandVariants(command, "geysers", showGeysers))						return true;

			if (ProcessCommandVariants(command, "static buildings", showStaticBuildings))		return true;

			if (ProcessCommandVariants(command, "bases", showBases))							return true;

			if (ProcessCommandVariants(command, "areas", showAreas))							return true;

			if (ProcessCommandVariants(command, "small areas", showSmallAreas))						return true;

			if (ProcessCommandVariants(command, "assigned ressources", showAssignedRessources))	return true;

			if (ProcessCommandVariants(command, "frontier", showFrontier))						return true;

			if (ProcessCommandVariants(command, "cp", showCP))									return true;

			if (ProcessCommandVariants(command, "supply", showSupply))							return true;

			if (ProcessCommandVariants(command, "reserved", showReserved))						return true;

			if (ProcessCommandVariants(command, "id", showId))						return true;

			if (ProcessCommandVariants(command, "hide", showHide))						return true;

			if (ProcessCommandVariants(command, "status", showStatus))						return true;

			if (ProcessCommandVariants(command, "all", showSeas))
				if (ProcessCommandVariants(command, "all", showLakes))
					if (ProcessCommandVariants(command, "all", showUnbuildable)) {
						if (showSeas && showUnbuildable) showSeas = false;

						if (ProcessCommandVariants(command, "all", showGroundHeight))
							if (ProcessCommandVariants(command, "all", showMinerals))
								if (ProcessCommandVariants(command, "all", showGeysers))
									if (ProcessCommandVariants(command, "all", showStaticBuildings))
										if (ProcessCommandVariants(command, "all", showBases))
											if (ProcessCommandVariants(command, "all", showAreas))
												if (ProcessCommandVariants(command, "all", showSmallAreas))
													if (ProcessCommandVariants(command, "all", showAssignedRessources))
														if (ProcessCommandVariants(command, "all", showFrontier))
															if (ProcessCommandVariants(command, "all", showCP))
																if (ProcessCommandVariants(command, "all", showSupply))
																	if (ProcessCommandVariants(command, "all", showReserved))
																		if (ProcessCommandVariants(command, "all", showId))
																			if (ProcessCommandVariants(command, "all", showHide))
																				if (ProcessCommandVariants(command, "all", showStatus))
																					return true;
					}

			return false;
		}

	}
} // namespace BWEM::utils



