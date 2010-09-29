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
		pid_t pid;
		pid_t tid;
		uint32_t sysJiffies;
		uint32_t userJiffies;
		uint64_t volCtxtSwitches;	  /**< voluntary context switches, only available with Linux CFS kernel */
		uint64_t nonvolCtxtSwitches;  /**< non-voluntary context switches, only available with Linux CFS kernel */
		time_t lastAccess;
		/**
		 * threads are marked as active when they are registered. Inactive
		 * threads are not polled for new sensor data. This variable purely exists to
		 * retain thread data during Vermont shutdown.
		 */
		bool active;
	};


	struct SystemInfo
	{
		uint16_t noCPUs;
		std::vector<uint32_t> userJiffies;
		std::vector<uint32_t> sysJiffies;
		uint64_t totalMemory; // in bytes
		uint64_t freeMemory; // in bytes
	};

	static JiffyTime getJiffies(pid_t pid, pid_t tid);
	static JiffyTime getThreadJiffies(pid_t tid);
	static JiffyTime getProcessJiffies(pid_t pid);
	static JiffyTime extractJiffies(char* statfile, char* schedfile);
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
