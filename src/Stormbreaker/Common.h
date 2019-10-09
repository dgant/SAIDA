#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>
#include <cstdio>
#include <cstdlib>

#include <stdarg.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <deque>
#include <list>
#include <set>
#include <map>
#include <array>
#include <ctime>
#include <iomanip>
#include <winsock2.h>
#include <windows.h>
#include <BWAPI.h>
#include "CommandUtil.h"
#include "BuildingStrategy/BuildType.h"
#include "BWEM/src/bwem.h"      

using namespace BWAPI;
using namespace BWAPI::UnitTypes;
using namespace BWAPI::InitialBuildTypes;
using namespace BWAPI::MainBuildTypes;
using namespace BWAPI::MainStrategyTypes;
using namespace std;
using namespace BWEM;
using namespace BWEM::BWAPI_ext;
using namespace BWEM::utils;

typedef unsigned int word;

namespace {
	auto &theMap = BWEM::Map::Instance();
	auto &bw = Broodwar;
}
#define S bw->self()
#define E bw->enemy()
#define TIME bw->getFrameCount()
/*
const char yellow = '\x03';
const char white = '\x04';
const char darkRed = '\x06';   // dim
const char green = '\x07';
const char red = '\x08';
const char purple = '\x10';   // dim
const char orange = '\x11';
const char gray = '\x1E';   // dim
const char cyan = '\x1F';

*/

struct double2
{
	double x, y;

	double2() : x(0), y(0) {}
	double2(const double2& p) : x(p.x), y(p.y) {}
	double2(double x, double y) : x(x), y(y) {}
	double2(const BWAPI::Position & p) : x(p.x), y(p.y) {}

	operator BWAPI::Position()				const { return BWAPI::Position(static_cast<int>(x), static_cast<int>(y)); }

	double2 operator + (const double2 & v)	const { return double2(x + v.x, y + v.y); }
	double2 operator - (const double2 & v)	const { return double2(x - v.x, y - v.y); }
	double2 operator * (double s)			const { return double2(x*s, y*s); }
	double2 operator / (double s)			const { return double2(x / s, y / s); }

	double dot(const double2 & v)			const { return x*v.x + y*v.y; }
	double lenSq()							const { return x*x + y*y; }
	double len()							const { return sqrt(lenSq()); }
	double2 normal()						const
	{
		if (len() == 0)
			return *this;
		else
			return *this / len();
	}

	void normalise() { double s(len()); x /= s; y /= s; }
	void rotate(double angle)
	{
		angle = angle*M_PI / 180.0;
		*this = double2(x * cos(angle) - y * sin(angle), y * cos(angle) + x * sin(angle));
	}

	double2 rotateReturn(double angle)
	{
		angle = angle*M_PI / 180.0;
		return double2(x * cos(angle) - y * sin(angle), y * cos(angle) + x * sin(angle));
	}
};

