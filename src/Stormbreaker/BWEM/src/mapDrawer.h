//////////////////////////////////////////////////////////////////////////
//
// This file is part of the BWEM Library.
// BWEM is free software, licensed under the MIT/X11 License.
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2015, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#ifndef BWEM_MAP_DRAWER_H
#define BWEM_MAP_DRAWER_H

#include <BWAPI.h>
#include <memory>
#include "utils.h"
#include "defs.h"


class BMP;
namespace BWEM {
	class Map;
	namespace utils {



		//////////////////////////////////////////////////////////////////////////////////////////////
		//                                                                                          //
		//                                  class MapDrawer
		//                                                                                          //
		//////////////////////////////////////////////////////////////////////////////////////////////
		//


		class MapDrawer
		{
		public:

			static bool ProcessCommand(const std::string &command);

			static bool		showSeas;
			static bool		showLakes;
			static bool		showUnbuildable;
			static bool		showGroundHeight;
			static bool		showMinerals;
			static bool		showGeysers;
			static bool		showStaticBuildings;
			static bool		showBases;
			static bool		showAreas;
			static bool		showSmallAreas;
			static bool		showAssignedRessources;
			static bool		showFrontier;
			static bool		showCP;
			static bool		showSupply;
			static bool		showReserved;
			static bool		showId;
			static bool		showHide;
			static bool		showStatus;

			struct Color
			{
				static const BWAPI::Color	sea;
				static const BWAPI::Color	lakes;
				static const BWAPI::Color	unbuildable;
				static const BWAPI::Color	highGround;
				static const BWAPI::Color	veryHighGround;
				static const BWAPI::Color	minerals;
				static const BWAPI::Color	geysers;
				static const BWAPI::Color	staticBuildings;
				static const BWAPI::Color	bases;
				static const BWAPI::Color	areas;
				static const BWAPI::Color	smallAreas;
				static const BWAPI::Color	assignedRessources;
				static const BWAPI::Color	frontier;
				static const BWAPI::Color	cp;
				static const BWAPI::Color	supply;
				static const BWAPI::Color	reserved;
				static const BWAPI::Color	id;
				static const BWAPI::Color	status;
			};

		private:
			static bool ProcessCommandVariants(const std::string &command, const std::string &attributName, bool &attribut);
		};


	}
} // namespace BWEM::detail::detail


#endif

