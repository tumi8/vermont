#include "SensorManager.h"

#include "cfg/CfgNode.h"
#include "cfg/Cfg.h"

#include <fcntl.h>
#include <sys/file.h>



SensorManager::SensorManager(uint32_t checkInterval = SM_DEFAULT_CHECK_INTERVAL, 
							 string outputfilename = SM_DEFAULT_OUTPUT_FNAME, 
							 GraphInstanceSupplier* gis = NULL)
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
	lastSystemInfo = ThreadCPUInterface::getSystemInfo();
#endif
	usedBytes += sizeof(SensorManager);
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

/**
 * writes XML data about given sensor to file
 */
void SensorManager::writeSensorXML(FILE* file, Sensor* s, const char* name, uint32_t id, bool module, 
								   time_t curtime, time_t lasttime, vector<uint32_t>* nextids)
{
	char* xmlmodpre = "\t\t<sensor type=\"%s\" id=\"%u\" name=\"%s\">\n";
	char* xmlmodpost = "\t\t</sensor>\n";
	char* xmlmodsimple = "\t\t\t<%s>%s</%s>\n";
	char* xmlmodthread = "\t\t\t<thread tid=\"%u\"><util type=\"system\">%.2f%%</util><util type=\"user\">%.2f%%</util></thread>\n";
	
	char text[100];
	
	fprintf(file, xmlmodpre, (module ? "module" : "simple"), id, name);
	snprintf(text, 100, "%u", s->getCurrentMemUsage());
	fprintf(file, xmlmodsimple, "memUsage", text, "memUsage");
	
	msg(MSG_INFO, "module: %s, id: %u, mem usage: %u", name, id, s->getCurrentMemUsage());
	
#if defined(__linux__)
	list<ThreadCPUInterface::JiffyTime> jtimes;
	s->getJiffiesUsed(jtimes);
	list<ThreadCPUInterface::JiffyTime>::iterator jiter = jtimes.begin();
	while (jiter != jtimes.end()) {
		double sysutil = jiter->sysJiffies/(static_cast<double>(curtime)-lasttime)/hertzValue*100;
		double userutil = jiter->userJiffies/(static_cast<double>(curtime)-lasttime)/hertzValue*100;
		fprintf(file, xmlmodthread, static_cast<uint32_t>(jiter->tid), sysutil, userutil);
		msg(MSG_INFO, " - thread (tid=%u): jiffies (sys/user): (%u/%u), util. (sys/user): (%.2f%%/%.2f%%)", 
				static_cast<uint32_t>(jiter->tid), jiter->sysJiffies, jiter->userJiffies, sysutil, userutil);
		
		jiter++;
	}
#endif
	
	string addinfo = s->getStatisticsXML();
	if (addinfo.size()>0) fprintf(file, xmlmodsimple, "addInfo", addinfo.c_str(), "addInfo");
	
	if (nextids) {
		vector<uint32_t>::const_iterator iditer = nextids->begin();
		while (iditer != nextids->end()) {
			snprintf(text, 100, "%u", *iditer);
			fprintf(file, xmlmodsimple, "next", text, "next");
			iditer++;
		}
	}
	
	fprintf(file, xmlmodpost);
}

void SensorManager::collectDataWorker()
{
	time_t lasttime = time(0);
	
	char* xmlpre = "<vermont>\n\t<sensorData time=\"%s\" host=\"%s\">\n";
	char* xmlpost = "\t</sensorData>\n</vermont>\n";
	char* xmlglobals = "\t\t<%s>%s</%s>\n";
	char* xmlglobalsuint = "\t\t<%s>%u</%s>\n";
	
	
	if (!graphIS) {
		THROWEXCEPTION("GraphInstanceSupplier variable graphIS MUST be set when module is started!");
	}
	
	string lockfile = outputFilename + ".lock";
	char hostname[100];
	if (gethostname(hostname, 100) != 0) 
		THROWEXCEPTION("failed to get hostname by gethostname()!");
		
	registerCurrentThread();
	
	msg(MSG_FATAL, "SensorManager: checking sensor values every %u seconds", checkInterval);
	while (!exitFlag) {
		timespec req;
		req.tv_sec = checkInterval;
		req.tv_nsec = 0;
		// restart nanosleep with the remaining sleep time
		// if we got interrupted by a signal
		while (nanosleep(&req, &req) == -1 && errno == EINTR);
		
		// we must not wait for the graph lock, else there may be a race condition with
		// the ConfigManager
		while (!graphIS->tryLockGraph()) {
			if (exitFlag) break;
			timespec timeout = { 0, 200000 };
			nanosleep(&timeout, NULL);
		}
		if (exitFlag) break;
		
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
		
		time_t curtime = time(0);
		char curtimestr[100];
		ctime_r(&curtime, curtimestr);
		curtimestr[strlen(curtimestr)-1] = 0;
		fprintf(file, xmlpre, curtimestr, hostname);
		char text[100];
		snprintf(text, 100, "%u", static_cast<uint32_t>(getpid()));
		fprintf(file, xmlglobals, "pid", text, "pid");		
		char lasttimestr[100];
		ctime_r(&lasttime, lasttimestr);
		lasttimestr[strlen(lasttimestr)-1] = 0;
		fprintf(file, xmlglobals, "lastTime", lasttimestr, "lastTime");
		
#if defined(__linux__)		
		ThreadCPUInterface::SystemInfo si = ThreadCPUInterface::getSystemInfo();
		
		fprintf(file, xmlglobalsuint, "processorAmount", si.noCPUs, "processorAmount");
		for (uint16_t i=0; i<si.sysJiffies.size(); i++) {
			double sysutil = (si.sysJiffies[i]-lastSystemInfo.sysJiffies[i])/(static_cast<double>(curtime)-lasttime)/hertzValue*100;
			double userutil = (si.userJiffies[i]-lastSystemInfo.userJiffies[i])/(static_cast<double>(curtime)-lasttime)/hertzValue*100;
			fprintf(file, "\t\t<processor id=\"%u\"><util type=\"system\">%.2f%%</util><util type=\"user\">%.2f%%</util></processor>\n",
					i, sysutil, userutil);			
		}
		fprintf(file, "\t\t<memory><free type=\"bytes\">%llu</free><total type=\"bytes\">%llu</total></memory>\n",
				si.freeMemory, si.totalMemory);
		lastSystemInfo = si;
#endif
	
		msg(MSG_INFO, "*** sensor data at %s", ctime(&curtime));
		
		Graph* g = graphIS->getGraph();
		vector<CfgNode*> nodes = g->getNodes();
		vector<CfgNode*>::iterator iter = nodes.begin();
		while (iter != nodes.end()) {
			Cfg* cfg = (*iter)->getCfg();
			Sensor* s = cfg->getInstance();		
			vector<uint32_t> nextids = cfg->getNext();
			writeSensorXML(file, s, cfg->getName().c_str(), cfg->getID(), true, curtime, lasttime, &nextids);
			
			iter++;
		}
		
		// iterate through all non-module sensors
		mutex.lock();
		list<SensorEntry>::const_iterator siter = sensors.begin();
		while (siter != sensors.end()) {
			writeSensorXML(file, siter->sensor, siter->name.c_str(), siter->id, false, curtime, lasttime, NULL);
			siter++;
		}
		mutex.unlock();
		
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

SensorManager& SensorManager::getInstance()
{
	static SensorManager smInstance;
	return smInstance;
}

void SensorManager::setCheckInterval(uint32_t checkInterval)
{
	this->checkInterval = checkInterval;
}

void SensorManager::setOutputFilename(string name)
{
	outputFilename = name;
}

void SensorManager::addSensor(Sensor* sensor, string name, uint32_t id)
{
	mutex.lock();
	SensorEntry se = {name, id, sensor};
	sensors.push_back(se);
	usedBytes += sizeof(se);
	mutex.unlock();
}

void SensorManager::removeSensor(Sensor* sensor)
{
	mutex.lock();
	list<SensorEntry>::iterator iter = sensors.begin();
	bool found = false;
	while (iter != sensors.end()) {
		if (iter->sensor == sensor) {
			sensors.erase(iter);
			usedBytes -= sizeof(SensorEntry);
			found = true;
			break;
		}
		iter++;
	}
	if (!found) {
		THROWEXCEPTION("did not find specified sensor in SensorManager");
	}
	mutex.unlock();
}
