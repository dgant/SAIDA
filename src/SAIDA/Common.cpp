#include "Common.h"
#include "windows.h"
#include "direct.h"

using namespace MyBot;

void Logger::appendTextToFile(const string &logFile, const string &msg)
{
	ofstream logStream;
	logStream.open(logFile.c_str(), ofstream::app);
	logStream << msg;
	logStream.flush();
	logStream.close();
}

void Logger::appendTextToFile(const string &logFile, const char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	//vfprintf(log_file, fmt, arg);
	char buff[256];
	vsnprintf_s(buff, 256, fmt, arg);
	va_end(arg);

	ofstream logStream;
	logStream.open(logFile.c_str(), ofstream::app);
	logStream << buff;
	logStream.flush();
	logStream.close();
}

void Logger::overwriteToFile(const string &logFile, const string &msg)
{
	ofstream logStream(logFile.c_str());
	logStream << msg;
	logStream.flush();
	logStream.close();
}

void Logger::debugFrameStr(const char *fmt, ...) {
#ifdef _LOGGING

	//if (TIME < 12000 || TIME > 14000)
	//	return;

	va_list arg;

	va_start(arg, fmt);
	//vfprintf(log_file, fmt, arg);
	char buff[256];
	vsnprintf_s(buff, 256, fmt, arg);
	va_end(arg);

	ofstream logStream;
	logStream.open(Config::Files::WriteDirectory + Config::Files::LogFilename.c_str(), ofstream::app);
	logStream << "[" << TIME << "] ";
	logStream << buff;
	logStream.flush();
	logStream.close();
#endif
}

void Logger::debug(const char *fmt, ...) {
#ifdef _LOGGING

	//if (TIME < 12000 || TIME > 14000)
	//	return;

	va_list arg;

	va_start(arg, fmt);
	//vfprintf(log_file, fmt, arg);
	char buff[256];
	vsnprintf_s(buff, 256, fmt, arg);
	va_end(arg);

	ofstream logStream;
	logStream.open(Config::Files::WriteDirectory + Config::Files::LogFilename.c_str(), ofstream::app);
	logStream << buff;
	logStream.flush();
	logStream.close();
#endif
}

void Logger::info(const string fileName, const bool printTime, const char *fmt, ...) {
	va_list arg;

	va_start(arg, fmt);
	//vfprintf(log_file, fmt, arg);
	char buff[256];
	vsnprintf_s(buff, 256, fmt, arg);
	va_end(arg);

	ofstream logStream;
	logStream.open(Config::Files::WriteDirectory + fileName, ofstream::app);

	if (printTime)
		logStream << "[" << TIME << "] ";

	logStream << buff;
	logStream.flush();
	logStream.close();
}

void Logger::error(const char *fmt, ...) {
	va_list arg;

	va_start(arg, fmt);
	//vfprintf(log_file, fmt, arg);
	char buff[256];
	vsnprintf_s(buff, 256, fmt, arg);
	va_end(arg);

	ofstream logStream;
	logStream.open(Config::Files::WriteDirectory + Config::Files::ErrorLogFilename.c_str(), ofstream::app);
	logStream << "[" << TIME << "] ";
	logStream << buff;
	logStream.flush();
	logStream.close();
}

void FileUtil::MakeDirectory(const char *full_path)
{
	char temp[256], *sp;
	strcpy_s(temp, sizeof(temp), full_path);
	sp = temp; // 포인터를 문자열 처음으로

	while ((sp = strchr(sp, '\\'))) { // 디렉토리 구분자를 찾았으면
		if (sp > temp && *(sp - 1) != ':') { // 루트디렉토리가 아니면
			*sp = '\0'; // 잠시 문자열 끝으로 설정
			//mkdir(temp, S_IFDIR);
			CreateDirectory(temp, NULL);
			// 디렉토리를 만들고 (존재하지 않을 때)
			*sp = '\\'; // 문자열을 원래대로 복귀
		}

		sp++; // 포인터를 다음 문자로 이동
	}
}

string FileUtil::readFile(const string &filename)
{
	stringstream ss;

	FILE *file;
	errno_t err;

	if ((err = fopen_s(&file, filename.c_str(), "r")) != 0)
	{
		cout << "Could not open file: " << filename.c_str();
	}
	else
	{
		char line[4096]; /* or other suitable maximum line size */

		while (fgets(line, sizeof line, file) != nullptr) /* read a line */
		{
			ss << line;
		}

		fclose(file);
	}

	return ss.str();
}

void FileUtil::readResults()
{
	string enemyName = BWAPI::Broodwar->enemy()->getName();
	replace(enemyName.begin(), enemyName.end(), ' ', '_');

	string enemyResultsFile = Config::Files::ReadDirectory + enemyName + ".txt";

	//int wins = 0;
	//int losses = 0;

	FILE *file;
	errno_t err;

	if ((err = fopen_s(&file, enemyResultsFile.c_str(), "r")) != 0)
	{
		cout << "Could not open file: " << enemyResultsFile.c_str();
	}
	else
	{
		char line[4096]; /* or other suitable maximum line size */

		while (fgets(line, sizeof line, file) != nullptr) /* read a line */
		{
			//stringstream ss(line);
			//ss >> wins;
			//ss >> losses;
		}

		fclose(file);
	}
}

void FileUtil::writeResults()
{
	string enemyName = BWAPI::Broodwar->enemy()->getName();
	replace(enemyName.begin(), enemyName.end(), ' ', '_');

	string enemyResultsFile = Config::Files::WriteDirectory + enemyName + ".txt";

	stringstream ss;

	//int wins = 1;
	//int losses = 0;

	//ss << wins << " " << losses << "\n";

	Logger::overwriteToFile(enemyResultsFile, ss.str());
}


string CommonUtil::getYYYYMMDDHHMMSSOfNow()
{
	time_t timer;
	tm timeinfo;
	char buffer[80];
	timer = time(NULL);
	localtime_s(&timeinfo, &timer);
	strftime(buffer, 80, "%Y-%m-%d-%H-%M-%S", &timeinfo);
	return buffer;
}

void CommonUtil::pause(int milli) {
	Broodwar->pauseGame();
	Sleep(milli);
	Broodwar->resumeGame();
}