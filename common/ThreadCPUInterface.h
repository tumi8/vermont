#ifndef THREADCPUINTERFACE_H_
#define THREADCPUINTERFACE_H_

#include <sys/types.h>
#include <stdint.h>

#include <vector>

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
	

	struct SystemInfo 
	{
		uint16_t noCPUs;
		std::vector<uint32_t> userJiffies;
		std::vector<uint32_t> sysJiffies;
		uint64_t totalMemory; // in bytes
		uint64_t freeMemory; // in bytes
	};
	
	static JiffyTime getJiffies(pid_t tid);
	static SystemInfo getSystemInfo();
	
	static unsigned long long getHertzValue();
#else
	struct SystemInfo
	{
	};
#endif // __linux__
	
private:
	ThreadCPUInterface();
	virtual ~ThreadCPUInterface();
};

#endif /*THREADCPUINTERFACE_H_*/
