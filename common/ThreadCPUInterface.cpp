#include "ThreadCPUInterface.h"

#include "common/msg.h"

#ifdef __linux__
#include "osdep/linux/sysinfo.h"
#endif

#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string>



ThreadCPUInterface::ThreadCPUInterface()
{
}

ThreadCPUInterface::~ThreadCPUInterface()
{
}

#if defined(__linux__)
/**
 * determines current amount of jiffies used by given thread id
 * accesses proc filesystem to get info
 */
ThreadCPUInterface::JiffyTime ThreadCPUInterface::getJiffies(pid_t tid)
{
	pid_t pid = getpid();
	char statfile[1024];
	
	snprintf(statfile, 1024, "/proc/%u/task/%u/stat", static_cast<uint32_t>(pid), static_cast<uint32_t>(tid));
	
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
			&utime, &stime, &cutime, &cstime) != 4) {
		THROWEXCEPTION("failed to parse stat file '%s'", statfile);
	}

	fclose(f);
	
	JiffyTime jt = { tid, stime, utime, 0 };
	return jt;
}

/**
 * determines length of one jiffy
 */
unsigned long long ThreadCPUInterface::getHertzValue()
{
	return ::getHertzValue();
}

/**
 * returns global system statistics by accessing the /proc filesystem
 */
ThreadCPUInterface::SystemInfo ThreadCPUInterface::getSystemInfo()
{
	string procfile = "/proc/meminfo";
	FILE* f = fopen(procfile.c_str(), "r");
	if (f == NULL) {
		THROWEXCEPTION("failed to open file '%s' (fopen)", procfile.c_str());
	}
	SystemInfo si;
	uint32_t mem;
	if (fscanf(f, "MemTotal: %u kB\n", &mem) != 1)
		THROWEXCEPTION("failed to parse file '%s' 1", procfile.c_str());
	si.totalMemory = mem*1024;
	if (fscanf(f, "MemFree: %u kB\n", &mem) != 1)
		THROWEXCEPTION("failed to parse file '%s' 2", procfile.c_str());
	si.freeMemory = mem;
	if (fscanf(f, "Buffers: %u kB\n", &mem) != 1)
		THROWEXCEPTION("failed to parse file '%s' 3", procfile.c_str());
	si.freeMemory += mem;
	if (fscanf(f, "Cached: %u kB\n", &mem) != 1)
		THROWEXCEPTION("failed to parse file '%s' 4", procfile.c_str());
	si.freeMemory += mem;
	si.freeMemory *= 1024;
	fclose(f);
	
	procfile = "/proc/stat";
	f = fopen(procfile.c_str(), "r");
	if (f == NULL) {
		THROWEXCEPTION("failed to open file '%s' (fopen)", procfile.c_str());
	}
	if (fscanf(f, "cpu %*u %*u %*u %*u %*u %*u %*u %*u") != 0)
		THROWEXCEPTION("failed to parse file '%s'", procfile.c_str());
	int g;
	while (g = getc(f)) if (g=='\n' || g==EOF) break;
	uint32_t sys, user;
	while (fscanf(f, "cpu%*d %u %*u %u %*u %*u %*u %*u %*u", &user, &sys) == 2) {
		while (g = getc(f)) if (g=='\n' || g==EOF) break;
		si.sysJiffies.push_back(sys);
		si.userJiffies.push_back(user);
	}
	si.noCPUs = si.sysJiffies.size();
	fclose(f);
	
	return si;
}


#endif // __linux__
