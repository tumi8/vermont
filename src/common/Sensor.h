#ifndef SENSOR_H_
#define SENSOR_H_

//#TODO: plattformunabhaengig machen
#include "osdep/linux/ThreadCPUInterface.h"
#include "Mutex.h"

#include <string>
#include <list>

class Sensor
{
public:
	Sensor();
	virtual ~Sensor();

	/**
	 * @param time in seconds when this function was called last time (to be used for calculating average)
	 * @returns a string containing statistics about the current module in XML format
	 */
	virtual std::string getStatisticsXML(double interval) { return ""; /* override this in the modules you need */ }

	/**
	 * returns memory usage of this module
	 * ATTENTION: module needs to count memory usage on its own, see variable usedBytes!
	 */
	uint32_t getCurrentMemUsage();

	/**
	 * returns number of jiffies used by all threads since last call of this function
	 * @param empty list which will be filled with data
	 */
#ifdef __linux__
	void getJiffiesUsed(std::list<ThreadCPUInterface::JiffyTime>& usedJiffies, std::list<ThreadCPUInterface::JiffyTime>& totalJiffies);
#endif

	/**
	 * registers given thread id as thread belonging to this module
	 * module code should do this once for each thread that is used by it
	 * @param thread thread id to be registered
	 */
	void registerThreadID(pid_t tid);

	/**
	 * registers current thread as thread belonging to this module
	 * module code should do this once for each thread that is used by it
	 */
	void registerCurrentThread();

	/**
	 * unregisters given thread id as thread belonging to this module
	 * @param thread thread id to be unregistered
	 */
	void unregisterThreadID(pid_t tid);

	/**
	 * unregisters current thread as thread belonging to this module
	 */
	void unregisterCurrentThread();

protected:
	/**
	 * modules must store the amount of used memory in this variable
	 * ATTENTION: this variable is not safely accessed in multi-threading context!
	 * do not store temporary data there, only final values!
	 */
	uint32_t usedBytes;

private:
#ifdef __linux__
	std::list<ThreadCPUInterface::JiffyTime> watchedThreads; /** all threads that are used by module */
#endif
	Mutex wThreadsMutex; /** mutex for locking watchedThreads */
};

#endif /*SENSOR_H_*/
