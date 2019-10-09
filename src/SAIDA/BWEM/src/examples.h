//////////////////////////////////////////////////////////////////////////
//
// This file is part of the BWEM Library.
// BWEM is free software, licensed under the MIT/X11 License.
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2015, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#ifndef BWEM_EXAMPLES_H
#define BWEM_EXAMPLES_H

#include <BWAPI.h>
#include "exampleWall.h"
#include "defs.h"

namespace BWEM
{

	class Map;

	namespace utils
	{

		void drawMap(const Map &theMap);

#if BWEM_USE_MAP_PRINTER

		void printMap(const Map &theMap);


		void pathExample(const Map &theMap);

#endif


		void gridMapExample(const Map &theMap);





	}
} 


#endif

