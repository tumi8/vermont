#include "StatisticsManager.h"

#include <stdio.h>
#include <string>
#include <unistd.h>


StatisticsManager::StatisticsManager()
	: Thread(threadWrapper), interval(10000)
{
}

StatisticsManager::~StatisticsManager()
{
	while (!statModules.empty()) {
		StatisticsModule* sm = statModules.front();
		statModules.pop_front();
		delete sm;
	}
}

StatisticsManager& StatisticsManager::getInstance()
{
	static StatisticsManager sm;
	return sm;
}

void StatisticsManager::addModule(StatisticsModule* statmodule)
{
	mutex.lock();
	statModules.push_back(statmodule);
	mutex.unlock();
}

void StatisticsManager::removeModule(StatisticsModule* statmodule)
{
	mutex.lock();
	statModules.remove(statmodule);
	mutex.unlock();
}

void* StatisticsManager::threadWrapper(void* sm)
{
	reinterpret_cast<StatisticsManager*>(sm)->runStats();
	return 0;
}

void StatisticsManager::setInterval(long milliseconds)
{
	interval = milliseconds;
}

void StatisticsManager::setOutput(const string& output)
{
	outputFile = output;
}

void StatisticsManager::runStats()
{
	// truncate output file
	FILE* f = fopen(outputFile.c_str(), "w");
	if (f == 0) THROWEXCEPTION("failed to open file %s", outputFile.c_str());
	fclose(f);

	while (!exitFlag) {
		if (usleep(interval*1000) != 0) THROWEXCEPTION("usleep failed");

		FILE* f = fopen(outputFile.c_str(), "a");
		if (f == 0) THROWEXCEPTION("failed to open file %s", outputFile.c_str());
		fprintf(f, "statistics dump at %lu\n", time(0));

		mutex.lock();
		list<StatisticsModule*>::const_iterator iter = statModules.begin();
		while (iter != statModules.end()) {
			string text = (*iter)->getStatistics();
			fprintf(f, "%s\n", text.c_str());
			iter++;
		}
		mutex.unlock();

		fclose(f);
	}
}

void StatisticsManager::start()
{
	run(this);
}

void StatisticsManager::stop()
{
	join();
}
