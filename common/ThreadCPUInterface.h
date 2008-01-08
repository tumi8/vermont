#ifndef THREADCPUINTERFACE_H_
#define THREADCPUINTERFACE_H_

#include <sys/types.h>
#include <stdint.h>

class ThreadCPUInterface
{
public:
#if defined(__linux__)
	struct JiffyTime {
		pid_t tid;
		uint32_t sysJiffies;
		uint32_t userJiffies;
		time_t lastAccess;
	};
	
	static JiffyTime getJiffies(pid_t tid);
	
	static unsigned long long getHertzValue();
#endif
	
private:
	ThreadCPUInterface();
	virtual ~ThreadCPUInterface();
};

#endif /*THREADCPUINTERFACE_H_*/
