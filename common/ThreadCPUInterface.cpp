#include "ThreadCPUInterface.h"

#include "common/msg.h"
#include "osdep/linux/sysinfo.h"

#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

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

#endif // __linux__
