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
void Sensor::getJiffiesUsed(std::list<ThreadCPUInterface::JiffyTime>& usedJiffies, std::list<ThreadCPUInterface::JiffyTime>& totalJiffies)
{
	wThreadsMutex.lock();
	std::list<ThreadCPUInterface::JiffyTime>::iterator iter = watchedThreads.begin();
	time_t curtime = time(0);
	while (iter != watchedThreads.end()) {
		if (iter->active) {
			ThreadCPUInterface::JiffyTime totaljt = ThreadCPUInterface::getThreadJiffies(iter->tid);
			ThreadCPUInterface::JiffyTime diffjt = totaljt;
			diffjt.userJiffies -= iter->userJiffies;
			diffjt.sysJiffies -= iter->sysJiffies;
			diffjt.lastAccess = iter->lastAccess;
			diffjt.volCtxtSwitches -= iter->volCtxtSwitches;
			diffjt.nonvolCtxtSwitches -= iter->nonvolCtxtSwitches;
			usedJiffies.push_back(diffjt);
			totalJiffies.push_back(totaljt);

			iter->userJiffies = totaljt.userJiffies;
			iter->sysJiffies = totaljt.sysJiffies;
			iter->volCtxtSwitches = totaljt.volCtxtSwitches;
			iter->nonvolCtxtSwitches = totaljt.nonvolCtxtSwitches;
			iter->lastAccess = curtime;
		} else {
			ThreadCPUInterface::JiffyTime diffjt = *iter;
			diffjt.userJiffies = 0;
			diffjt.sysJiffies = 0;
			usedJiffies.push_back(diffjt);
			totalJiffies.push_back(*iter);
		}
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
			if (iter->active) {
				wThreadsMutex.unlock();
				THROWEXCEPTION("thread id %u is already registered in this module", tid);
			}

			// reactivate thread
			iter->active = true;
			iter->sysJiffies = 0;
			iter->userJiffies = 0;

			wThreadsMutex.unlock();
			return;
		}
		iter++;
	}
	ThreadCPUInterface::JiffyTime jt = ThreadCPUInterface::getThreadJiffies(tid);
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
			if (!iter->active) {
				THROWEXCEPTION("given thread ID was already deactived");
			}
			// update thread statistics
			ThreadCPUInterface::JiffyTime jt = ThreadCPUInterface::getThreadJiffies(iter->tid);
			iter->userJiffies = jt.userJiffies;
			iter->sysJiffies = jt.sysJiffies;
			iter->lastAccess = time(0);
			iter->active = false;

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
