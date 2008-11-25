#include "Sensor.h"
#include "msg.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>


Sensor::Sensor()
	: usedBytes(0)
{
}

Sensor::~Sensor()
{
}

uint32_t Sensor::getCurrentMemUsage()
{
	return usedBytes;
}

#ifdef __linux__
void Sensor::getJiffiesUsed(std::list<ThreadCPUInterface::JiffyTime>& usedJiffies)
{
	wThreadsMutex.lock();
	std::list<ThreadCPUInterface::JiffyTime>::iterator iter = watchedThreads.begin();
	time_t curtime = time(0);
	while (iter != watchedThreads.end()) {
		ThreadCPUInterface::JiffyTime jt = ThreadCPUInterface::getJiffies(iter->tid);		
		jt.userJiffies -= iter->userJiffies;
		jt.sysJiffies -= iter->sysJiffies;
		jt.lastAccess = iter->lastAccess;
		usedJiffies.push_back(jt);
		
		iter->userJiffies += jt.userJiffies;
		iter->sysJiffies += jt.sysJiffies;
		iter->lastAccess = curtime;
		iter++;
	}
	wThreadsMutex.unlock();
}
#endif

void Sensor::registerThreadID(pid_t tid)
{
#ifdef __linux__
	wThreadsMutex.lock();
	std::list<ThreadCPUInterface::JiffyTime>::iterator iter = watchedThreads.begin();
	while (iter != watchedThreads.end()) {
		if (iter->tid == tid) {
			wThreadsMutex.unlock();
			THROWEXCEPTION("thread id %u is already registered in this module", tid);
		}
		iter++;
	}
	ThreadCPUInterface::JiffyTime jt = ThreadCPUInterface::getJiffies(tid);
	jt.lastAccess = time(0);
	
	watchedThreads.push_back(jt);
	wThreadsMutex.unlock();
#else
	// do nothing in non-linux oses
#endif
}

void Sensor::registerCurrentThread()
{ 
#if defined(__linux__)
	pid_t tid;
	tid = (long)syscall(SYS_gettid);
	registerThreadID(tid);
#else
	// do nothing in non-linux oses
#endif
}

void Sensor::unregisterThreadID(pid_t tid)
{
#ifdef __linux__
	wThreadsMutex.lock();
	std::list<ThreadCPUInterface::JiffyTime>::iterator iter = watchedThreads.begin();
	while (iter != watchedThreads.end()) {
		if (iter->tid == tid) {
			watchedThreads.erase(iter);
			wThreadsMutex.unlock();
			return;
		}
		iter++;
	}
	wThreadsMutex.unlock();
	THROWEXCEPTION("thread id %u was not found in list", tid);
#else
	// do nothing in non-linux oses
#endif
}

void Sensor::unregisterCurrentThread()
{
#ifdef __linux__
	pid_t tid;
	tid = (long)syscall(SYS_gettid);
	unregisterThreadID(tid);
#else
	// do nothing in non-linux oses
#endif
}
