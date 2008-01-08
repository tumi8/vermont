#include "Module.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

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

void Module::getJiffiesUsed(list<ThreadCPUInterface::JiffyTime>& usedJiffies)
{
	wThreadsMutex.lock();
	list<ThreadCPUInterface::JiffyTime>::iterator iter = watchedThreads.begin();
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

void Module::registerThreadID(pid_t tid)
{
	wThreadsMutex.lock();
	list<ThreadCPUInterface::JiffyTime>::iterator iter = watchedThreads.begin();
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
}

void Module::registerCurrentThread()
{ 
#if defined(__linux__)
	pid_t tid;
	tid = (long)syscall(SYS_gettid);
	registerThreadID(tid);
#else
	registerThreadID(0);
#endif
}

void Module::unregisterThreadID(pid_t tid)
{
	wThreadsMutex.lock();
	list<ThreadCPUInterface::JiffyTime>::iterator iter = watchedThreads.begin();
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
}

void Module::unregisterCurrentThread()
{
	pid_t tid;
	tid = (long)syscall(SYS_gettid);
	unregisterThreadID(tid);
}


