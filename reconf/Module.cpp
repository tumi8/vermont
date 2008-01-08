#include "Module.h"

#include <sys/stat.h>

Module::Module() 
	: usedBytes(0), exitFlag(false), running(false)
{ 
};


Module::~Module()
{
	if (running) {
		THROWEXCEPTION("you must shutdown the module first before destroying it!");
	}
}

void Module::start(bool fail_if_already_running)
{
	if (running && !fail_if_already_running)
		return;

	ASSERT(!running, "module must not be in state 'running' when started");
	running = true;
	exitFlag = false;
	performStart();
}

void Module::notifyShutdown()
{
	exitFlag = true;
}

void Module::shutdown(bool fail_if_not_running)
{
	if (!running && !fail_if_not_running)
		return;

	ASSERT(running, "module must be in state running when it is shut down");
	
	notifyShutdown();
	performShutdown();
	running = false;
}

bool Module::getExitFlag() const
{
	return exitFlag;
}

uint32_t Module::getCurrentMemUsage()
{
	return usedBytes;
}

void Module::getJiffiesUsed(list<JiffyTime>& usedJiffies)
{
	wThreadsMutex.lock();
	list<JiffyTime>::iterator iter = watchedThreads.begin();
	time_t curtime = time(0);
	while (iter != watchedThreads.end()) {
		JiffyTime jt = getJiffies(iter->thread);		
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

void Module::registerThreadID(pthread_t thread)
{
	wThreadsMutex.lock();
	list<JiffyTime>::iterator iter = watchedThreads.begin();
	while (iter != watchedThreads.end()) {
		if (iter->thread == thread) {
			wThreadsMutex.unlock();
			THROWEXCEPTION("thread id %d is already registered in this module", thread);
		}
		iter++;
	}
	JiffyTime jt = getJiffies(thread);
	jt.lastAccess = time(0);
	
	watchedThreads.push_back(jt);
}

void Module::unregisterThreadID(pthread_t thread)
{
	wThreadsMutex.lock();
	list<JiffyTime>::iterator iter = watchedThreads.begin();
	while (iter != watchedThreads.end()) {
		if (iter->thread == thread) {
			watchedThreads.erase(iter);
			wThreadsMutex.unlock();
			return;
		}
		iter++;
	}
	wThreadsMutex.unlock();
	THROWEXCEPTION("thread id %d was not found in list", thread);
}

/**
 * determines current amount of jiffies used by given thread id
 * accesses proc filesystem to get info
 */
Module::JiffyTime Module::getJiffies(pthread_t thread)
{
	pid_t pid = getpid();
	char statfile[1024];
	
	snprintf(statfile, 1024, "/proc/%u/task/%u/stat", static_cast<uint32_t>(pid), static_cast<uint32_t>(thread));
	
	struct stat buf;
	if (stat(statfile, &buf) != 0) {
		THROWEXCEPTION("failed to access stat file '%s' (stat)", statfile);
	}
	
	FILE* f = fopen(statfile, "r");
	if (f == NULL) {
		THROWEXCEPTION("failed to open stat file '%s' (fopen)", statfile);
	}
	
	uint32_t utime, stime, cutime, cstime;
	// 14. position: utime, then: stime, cutime, cstime
	if (fscanf(f, "%*d %*s %*s %*d %*d %*d %*d %*d %*u %*d %*d %*u %*u %u %u %u %u %*u",
			&utime, &stime, &cutime, &cstime) != 18) {
		THROWEXCEPTION("failed to parse stat file '%s'", statfile);
	}

	fclose(f);
	
	JiffyTime jt = { thread, stime, utime, 0 };
	
	return jt;
}
