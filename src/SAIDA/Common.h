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

#include "Config.h"
#include "CommandUtil.h"
#include "BuildType.h"
#include "BWEM/src/bwem.h"      // update the path if necessary

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

namespace MyBot
{

	struct double2
	{
		double x, y;

		double2() {}
		double2(double x, double y) : x(x), y(y) {}
		double2(const Position &p) : x(p.x), y(p.y) {}

		operator Position()				const {
			return Position(static_cast<int>(x), static_cast<int>(y));
		}

		double2 operator + (const double2 &v)	const {
			return double2(x + v.x, y + v.y);
		}
		double2 operator - (const double2 &v)	const {
			return double2(x - v.x, y - v.y);
		}
		double2 operator * (double s)			const {
			return double2(x * s, y * s);
		}
		double2 operator / (double s)			const {
			return double2(x / s, y / s);
		}

		double dot(const double2 &v)			const {
			return x * v.x + y * v.y;
		}
		double lenSq()							const {
			return x * x + y * y;
		}
		double len()							const {
			return sqrt(lenSq());
		}
		double2 normal()						const {
			return *this / len();
		}

		void normalise() {
			double s(len());
			x /= s;
			y /= s;
		}
		void rotate(double angle)
		{
			angle = angle * M_PI / 180.0;
			*this = double2(x * cos(angle) - y * sin(angle), y * cos(angle) + x * sin(angle));
		}
	};

	/// 로그 유틸
	namespace Logger
	{
		void appendTextToFile(const string &logFile, const string &msg);
		void appendTextToFile(const string &logFile, const char *fmt, ...);
		void overwriteToFile(const string &logFile, const string &msg);
		void debugFrameStr(const char *fmt, ...);
		void debug(const char *fmt, ...);
		void info(const string fileName, const bool printTime, const char *fmt, ...);
		void error(const char *fmt, ...);
	};

	class SAIDA_Exception
	{
	private:
		unsigned int nSE;
		PEXCEPTION_POINTERS     m_pException;
	public:
		SAIDA_Exception(unsigned int errCode, PEXCEPTION_POINTERS pException) : nSE(errCode), m_pException(pException) {}
		unsigned int getSeNumber() {
			return nSE;
		}
		PEXCEPTION_POINTERS getExceptionPointers() {
			return m_pException;
		}
	};

	/// 파일 유틸
	namespace FileUtil {
		/// 디렉토리 생성
		void MakeDirectory(const char *full_path);
		/// 파일 유틸 - 텍스트 파일을 읽어들인다
		string readFile(const string &filename);

		/// 파일 유틸 - 경기 결과를 텍스트 파일로부터 읽어들인다
		void readResults();

		/// 파일 유틸 - 경기 결과를 텍스트 파일에 저장한다
		void writeResults();
	}

	namespace CommonUtil {
		string getYYYYMMDDHHMMSSOfNow();
		void pause(int milli);
	}
}