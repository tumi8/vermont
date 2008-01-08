#include "SensorManager.h"

#include "cfg/CfgNode.h"
#include "cfg/Cfg.h"

#include <fcntl.h>
#include <sys/file.h>

SensorManager::SensorManager(uint32_t checkInterval, string outputfilename, GraphInstanceSupplier* gis)
	: graphIS(gis), 
	  thread(SensorManager::threadWrapper),
	  checkInterval(checkInterval),
	  outputFilename(outputfilename)
{
#if !defined(__linux__)
	msg(MSG_FATAL, "WARNING: this instance of vermont is *not* compiled for linux, support for CPU sensors is disabled");
	hertzValue = 0;
#else
	hertzValue = ThreadCPUInterface::getHertzValue();
	msg(MSG_DIALOG, "using value %u as hertz jiffy value", hertzValue);
	msg(MSG_FATAL, "writing sensor output data to '%s'", outputfilename.c_str());
#endif
}

SensorManager::~SensorManager()
{
}

void SensorManager::setGraphIS(GraphInstanceSupplier* gis)
{
	graphIS = gis;
}

void SensorManager::performStart()
{
	msg(MSG_DIALOG, "starting sensor check thread");
	
	thread.run(this);
}

void SensorManager::performShutdown()
{
	thread.join();
}

void SensorManager::collectDataWorker()
{
	timespec req;
	req.tv_sec = checkInterval;
	req.tv_nsec = 0;
	time_t lasttime = time(0);
	
	char* xmlpre = "<vermont>\n\t<sensorData>\n";
	char* xmlpost = "\t</sensorData>\n</vermont>\n";
	char* xmlglobals = "\t\t<%s>%s</%s>\n";
	
	
	string lockfile = outputFilename + ".lock";
	char hostname[100];
	if (gethostname(hostname, 100) != 0) 
		THROWEXCEPTION("failed to get hostname by gethostname()!");
		
	registerCurrentThread();
	
	msg(MSG_FATAL, "SensorManager: checking sensor values every %u seconds", checkInterval);
	while (!exitFlag) {
		// restart nanosleep with the remaining sleep time
		// if we got interrupted by a signal
		while (nanosleep(&req, &req) == -1 && errno == EINTR);
		
		graphIS->lockGraph();
		
		int fdlock = open(lockfile.c_str(), O_CREAT|O_RDONLY);
		if (fdlock == -1)
			THROWEXCEPTION("failed to open file %s, error code %d", lockfile.c_str(), errno);
		
		if (flock(fdlock, LOCK_EX)!=0)
			THROWEXCEPTION("failed to activate exclusive lock on file %s (flock())", lockfile.c_str());
		
		FILE* file = fopen(outputFilename.c_str(), "w");
		if (!file) {
			THROWEXCEPTION("failed to reopen file %s", outputFilename.c_str());
			perror("error:");
		}
		
		fprintf(file, xmlpre);
		fprintf(file, xmlglobals, "host", "test", "host");
		char text[100];
		snprintf(text, 100, "%u", static_cast<uint32_t>(getpid()));
		fprintf(file, xmlglobals, "pid", text, "pid");		
		fprintf(file, xmlglobals, "hostname", hostname, "hostname");
		time_t curtime = time(0);
		snprintf(text, 100, "%u", static_cast<uint32_t>(curtime));
		fprintf(file, xmlglobals, "time", text, "time");
		snprintf(text, 100, "%u", static_cast<uint32_t>(lasttime));
		fprintf(file, xmlglobals, "lastTime", text, "lastTime");
		
		msg(MSG_INFO, "statistics data at %u", static_cast<uint32_t>(time(0)));
		msg(MSG_INFO, "---------------------------------------");
		
		Graph* g = graphIS->getGraph();
		vector<CfgNode*> nodes = g->getNodes();
		vector<CfgNode*>::iterator iter = nodes.begin();
		while (iter != nodes.end()) {
			Cfg* cfg = (*iter)->getCfg();
			Module* m = cfg->getInstance();
			
			char* xmlmodpre = "\t\t<module id=\"%u\" name=\"%s\">\n";
			char* xmlmodpost = "\t\t</module>\n";
			char* xmlmodsimple = "\t\t\t<%s>%s</%s>\n";
			char* xmlmodthread = "\t\t\t<thread tid=\"%u\"><util type=\"system\">%.2f%%</util><util type=\"user\">%.2f%%</util></thread>\n";
			
			fprintf(file, xmlmodpre, cfg->getID(), cfg->getName().c_str());
			snprintf(text, 100, "%u", m->getCurrentMemUsage());
			fprintf(file, xmlmodsimple, "memUsage", text, "memUsage");
			
			msg(MSG_INFO, "module: %s, id: %u, mem usage: %u", cfg->getName().c_str(), cfg->getID(), m->getCurrentMemUsage());
			
#if defined(__linux__)
			list<ThreadCPUInterface::JiffyTime> jtimes;
			m->getJiffiesUsed(jtimes);
			list<ThreadCPUInterface::JiffyTime>::iterator jiter = jtimes.begin();
			while (jiter != jtimes.end()) {
				double sysutil = jiter->sysJiffies/(static_cast<double>(curtime)-lasttime)/hertzValue*100;
				double userutil = jiter->userJiffies/(static_cast<double>(curtime)-lasttime)/hertzValue*100;
				fprintf(file, xmlmodthread, static_cast<uint32_t>(jiter->tid), sysutil, userutil);
				msg(MSG_INFO, "   - thread (tid=%u): jiffies (sys/user): (%u/%u), utilization (sys/user): (%.2f%%/%.2f%%)", 
						static_cast<uint32_t>(jiter->tid), jiter->sysJiffies, jiter->userJiffies, sysutil, userutil);
				
				jiter++;
			}
#endif
			
			vector<uint32_t> nextids = cfg->getNext();
			vector<uint32_t>::const_iterator iditer = nextids.begin();
			while (iditer != nextids.end()) {
				snprintf(text, 100, "%u", *iditer);
				fprintf(file, xmlmodsimple, "next", text, "next");
				iditer++;
			}
			
			fprintf(file, xmlmodpost);
			
			iter++;
		}
		
		fprintf(file, xmlpost);
		fclose(file);
		close(fdlock);
		
		lasttime = time(0);
		
		graphIS->unlockGraph();
	}
	
	unregisterCurrentThread();
}

/**
 * small static wrapper function to start thread
 */
void* SensorManager::threadWrapper(void* instance)
{
	SensorManager* sm = reinterpret_cast<SensorManager*>(instance);
	sm->collectDataWorker();	
	return 0;
}
