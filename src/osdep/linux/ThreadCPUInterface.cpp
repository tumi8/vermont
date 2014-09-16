#include "ThreadCPUInterface.h"

#include "common/msg.h"
#include "common/defs.h"

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
 * @param tid thread id
 */
ThreadCPUInterface::JiffyTime ThreadCPUInterface::getThreadJiffies(pid_t tid)
{
	pid_t pid = getpid();
	return getJiffies(pid, tid);
}

/**
 * determines current amount of jiffies used by given process id
 * accesses proc filesystem to get info
 * @param pid process id
 */
ThreadCPUInterface::JiffyTime ThreadCPUInterface::getProcessJiffies(pid_t pid)
{
	char statfile[1024];
	char schedfile[1024];
	snprintf(statfile, ARRAY_SIZE(statfile), "/proc/%u/stat", static_cast<uint32_t>(pid));
	snprintf(schedfile, ARRAY_SIZE(schedfile), "/proc/%u/sched", static_cast<uint32_t>(pid));
	JiffyTime jt = extractJiffies(statfile, schedfile);
	jt.pid = pid;
	jt.tid = 0;
	return jt;
}

/**
 * determines current amount of jiffies used by given thread id
 * accesses proc filesystem to get info
 * @param pid process id
 * @param tid thread id
 */
ThreadCPUInterface::JiffyTime ThreadCPUInterface::getJiffies(pid_t pid, pid_t tid)
{
	char statfile[1024];
	char schedfile[1024];
	snprintf(statfile, ARRAY_SIZE(statfile), "/proc/%u/task/%u/stat", static_cast<uint32_t>(pid), static_cast<uint32_t>(tid));
	snprintf(schedfile, ARRAY_SIZE(schedfile), "/proc/%u/task/%u/sched", static_cast<uint32_t>(pid), static_cast<uint32_t>(tid));
	JiffyTime jt = extractJiffies(statfile, schedfile);
	jt.pid = pid;
	jt.tid = tid;
	return jt;
}

/**
 * extracts jiffy information from given statfile in proc filesystem
 * @param statfile absolut path to process or thread statfile in proc-fs
 * @param schedfile absolut path to process or thread schedule file in proc-fs
 */
ThreadCPUInterface::JiffyTime ThreadCPUInterface::extractJiffies(char* statfile, char* schedfile)
{
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


	uint64_t vcswitch = 0;
	uint64_t nvcswitch = 0;
	char line[1024];
	// only report scheduler statistics if new kernel CFS (completely fair scheduler) is used
	if (stat(schedfile, &buf) == 0) {
		f = fopen(schedfile, "r");
		if (f == NULL) {
			THROWEXCEPTION("failed to open CFS scheduler file '%s' (fopen)", schedfile);
		}

		while (fgets(line, ARRAY_SIZE(line), f) != NULL) {
			long long unsigned tmp;
			sscanf(line, "nr_voluntary_switches%*[ :]%llu", &tmp);
			vcswitch = (uint64_t)tmp;
			sscanf(line, "nr_involuntary_switches%*[ :]%llu", &tmp);
			nvcswitch = (uint64_t)tmp;
		}

		fclose(f);
	}


	JiffyTime jt = { 0, 0, stime, utime, vcswitch, nvcswitch, 0, true };
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
	std::string procfile = "/proc/meminfo";
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
	if (fscanf(f, "Buffers: %u kB\n", &mem) != 1) {
		// Linux 3.14 introduces an additional line in the /proc/meminfo output
		fscanf(f, "MemAvailable: %*u kB\n");
		if (fscanf(f, "Buffers: %u kB\n", &mem) != 1)
			THROWEXCEPTION("failed to parse file '%s' 3", procfile.c_str());
	}
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
	while ((g = getc(f))) if (g=='\n' || g==EOF) break;
	uint32_t sys, user;
	while (fscanf(f, "cpu%*d %u %*u %u %*u %*u %*u %*u %*u", &user, &sys) == 2) {
		while ((g = getc(f))) if (g=='\n' || g==EOF) break;
		si.sysJiffies.push_back(sys);
		si.userJiffies.push_back(user);
	}
	si.noCPUs = si.sysJiffies.size();
	fclose(f);

	return si;
}


#endif // __linux__
