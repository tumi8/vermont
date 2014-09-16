#include "SensorManager.h"

#include "core/CfgNode.h"
#include "core/Cfg.h"
#include "common/msg.h"

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>



SensorManager::SensorManager()
	: graphIS(NULL),
	  thread(SensorManager::threadWrapper, "SensorManager"),
	  checkInterval(SM_DEFAULT_CHECK_INTERVAL),
	  outputFilename(SM_DEFAULT_OUTPUT_FNAME),
	  smExitFlag(false),
	  append(SM_DEFAULT_APPEND)
{
	usedBytes += sizeof(SensorManager);
}

SensorManager::~SensorManager()
{
}

void SensorManager::setParameters(uint32_t checkInterval = SM_DEFAULT_CHECK_INTERVAL,
							 string outputfilename = SM_DEFAULT_OUTPUT_FNAME,
							 bool append = SM_DEFAULT_APPEND,
							 GraphInstanceSupplier* gis = NULL)
{
#if !defined(__linux__)
	msg(MSG_DIALOG, "WARNING: this instance of vermont is *not* compiled for linux, support for CPU sensors is disabled");
	hertzValue = 0;
#else
	hertzValue = ThreadCPUInterface::getHertzValue();
	lastSystemInfo = ThreadCPUInterface::getSystemInfo();
#endif
	if (gethostname(hostname, 100) != 0)
		THROWEXCEPTION("failed to get hostname by gethostname()!");
	msg(MSG_INFO, "SensorManager: hertz jiffy value=%u, hostname=%s", hertzValue, hostname);

	msg(MSG_INFO, "SensorManager started with following parameters:");
	msg(MSG_INFO, "  - outputfilename=%s", outputfilename.c_str());
	msg(MSG_INFO, "  - checkInterval=%d seconds", checkInterval);
	msg(MSG_INFO, "  - append=%d", append);
	this->checkInterval = checkInterval;
	this->outputFilename = outputfilename;
	this->append = append;
	this->graphIS = gis;

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

void SensorManager::stopSMThread()
{
	smExitFlag = true;
	thread.join();
}

void SensorManager::performShutdown()
{
	stopSMThread();
}

/**
 * writes XML data about given sensor to file
 */
void SensorManager::writeSensorXML(FILE* file, Sensor* s, const char* name, uint32_t id, bool module,
								   time_t curtime, time_t lasttime, vector<uint32_t>* nextids)
{
	const char* xmlmodpre = "\t\t<sensor type=\"%s\" id=\"%u\" name=\"%s\">\n";
	const char* xmlmodpost = "\t\t</sensor>\n";
	const char* xmlmodsimple = "\t\t\t<%s>%s</%s>\n";

	char text[100];

	fprintf(file, xmlmodpre, (module ? "module" : "simple"), id, name);
	snprintf(text, 100, "%u", s->getCurrentMemUsage());
	fprintf(file, xmlmodsimple, "memUsage", text, "memUsage");

	//DPRINTF("module: %s, id: %u, mem usage: %u", name, id, s->getCurrentMemUsage());

#if defined(__linux__)
	const char* xmlmodthread = "\t\t\t<thread tid=\"%u\"><util type=\"system\">%.2f</util><util type=\"user\">%.2f</util>"
		"<totalJiffies type=\"system\">%u</totalJiffies><totalJiffies type=\"user\">%u</totalJiffies>"
		"<contextSwitches type=\"non-voluntary\">%llu</contextSwitches>"
		"<contextSwitches type=\"voluntary\">%llu</contextSwitches></thread>\n";
	list<ThreadCPUInterface::JiffyTime> jtimes;
	list<ThreadCPUInterface::JiffyTime> jtotal;
	s->getJiffiesUsed(jtimes, jtotal);
	list<ThreadCPUInterface::JiffyTime>::iterator jiter = jtimes.begin();
	list<ThreadCPUInterface::JiffyTime>::iterator jtiter = jtotal.begin();
	while (jiter != jtimes.end() && jtiter != jtotal.end()) {
		double sysutil = jiter->sysJiffies/(static_cast<double>(curtime)-lasttime)/hertzValue*100;
		double userutil = jiter->userJiffies/(static_cast<double>(curtime)-lasttime)/hertzValue*100;
		fprintf(file, xmlmodthread, static_cast<uint32_t>(jiter->tid), sysutil, userutil, jtiter->sysJiffies, jtiter->userJiffies,
				jtiter->nonvolCtxtSwitches, jtiter->volCtxtSwitches);
		//DPRINTF(" - thread (tid=%u): jiffies (sys/user): (%u/%u), util. (sys/user): (%.2f%%/%.2f%%)",
		//		static_cast<uint32_t>(jiter->tid), jiter->sysJiffies, jiter->userJiffies, sysutil, userutil);

		jiter++;
		jtiter++;
	}
#endif

	string addinfo = s->getStatisticsXML(curtime-lasttime);
	if (addinfo.size()>0) fprintf(file, xmlmodsimple, "addInfo", addinfo.c_str(), "addInfo");

	if (nextids) {
		vector<uint32_t>::const_iterator iditer = nextids->begin();
		while (iditer != nextids->end()) {
			snprintf(text, 100, "%u", *iditer);
			fprintf(file, xmlmodsimple, "next", text, "next");
			iditer++;
		}
	}

	fprintf(file, "%s", xmlmodpost);
}

void SensorManager::retrieveStatistics(bool ignoreshutdown)
{
	const char* xmlpre = "<vermont>\n\t<sensorData time=\"%s\" epochtime=\"%d.%03d\" host=\"%s\">\n";
	const char* xmlpost = "\t</sensorData>\n</vermont>\n";
	const char* xmlglobals = "\t\t<%s>%s</%s>\n";

	string lockfile = outputFilename + ".lock";
	bool haveGraphLock;

	// we must not wait for the graph lock, else there may be a race condition with
	// the ConfigManager
	while (! (haveGraphLock = graphIS->tryLockGraph())) {
		if (smExitFlag) break;
		timespec timeout = { 0, 200000 };
		nanosleep(&timeout, NULL);
	}

	if (!ignoreshutdown && smExitFlag) return;

	const char* openflags = (append ? "a" : "w");
	FILE* file = fopen(outputFilename.c_str(), openflags);
	if (!file) {
		THROWEXCEPTION("failed to reopen file %s", outputFilename.c_str());
		perror("error:");
	}

	timeval tvcurtime = unixtime();
	time_t curtime = tvcurtime.tv_sec;
	char curtimestr[100];
	ctime_r(&curtime, curtimestr);
	curtimestr[strlen(curtimestr)-1] = 0;
	fprintf(file, xmlpre, curtimestr, curtime, tvcurtime.tv_usec/1000, hostname);
	char text[100];
	snprintf(text, 100, "%u", static_cast<uint32_t>(getpid()));
	fprintf(file, xmlglobals, "pid", text, "pid");
	char lasttimestr[100];
	ctime_r(&lasttime, lasttimestr);
	lasttimestr[strlen(lasttimestr)-1] = 0;
	fprintf(file, xmlglobals, "lastTime", lasttimestr, "lastTime");

#if defined(__linux__)
	const char* xmlglobalsuint = "\t\t<%s>%u</%s>\n";
	ThreadCPUInterface::SystemInfo si = ThreadCPUInterface::getSystemInfo();

	fprintf(file, "\t\t<jiffyFrequency>%llu</jiffyFrequency>\n", (long long unsigned)hertzValue);
	fprintf(file, xmlglobalsuint, "processorAmount", si.noCPUs, "processorAmount");
	for (uint16_t i=0; i<si.sysJiffies.size(); i++) {
		double sysutil = (si.sysJiffies[i]-lastSystemInfo.sysJiffies[i])/(static_cast<double>(curtime)-lasttime)/hertzValue*100;
		double userutil = (si.userJiffies[i]-lastSystemInfo.userJiffies[i])/(static_cast<double>(curtime)-lasttime)/hertzValue*100;
		fprintf(file, "\t\t<processor id=\"%u\"><util type=\"system\">%.2f</util><util type=\"user\">%.2f</util></processor>\n",
				i, sysutil, userutil);
	}
	fprintf(file, "\t\t<memory><free type=\"bytes\">%llu</free><total type=\"bytes\">%llu</total></memory>\n",
			(long long unsigned)si.freeMemory, (long long unsigned)si.totalMemory);
	lastSystemInfo = si;
#endif

	//DPRINTF("*** sensor data at %s", ctime(&curtime));

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
        //DPRINTFL(MSG_ERROR, "non-module cfg->getName()=%s, s=%u", siter->name.c_str(), siter->sensor);
		writeSensorXML(file, siter->sensor, siter->name.c_str(), siter->id, false, curtime, lasttime, NULL);
		siter++;
	}
	mutex.unlock();

	fprintf(file, "%s", xmlpost);
	fclose(file);

	if (haveGraphLock) graphIS->unlockGraph();
}

void SensorManager::collectDataWorker()
{
	lasttime = time(0);

	if (!graphIS) {
		THROWEXCEPTION("GraphInstanceSupplier variable graphIS MUST be set when module is started!");
	}

	registerCurrentThread();

	msg(MSG_DIALOG, "SensorManager: checking sensor values every %u seconds", checkInterval);
	while (!smExitFlag) {
		uint32_t sleepcount = checkInterval*2;
		uint32_t i = 0;
		while (i<sleepcount && !smExitFlag) {
			// restart nanosleep with the remaining sleep time
			// if we got interrupted by a signal
			timespec req;
			req.tv_sec = 0;
			req.tv_nsec = 500000000;
			while (nanosleep(&req, &req) == -1 && errno == EINTR);
			i++;
		}

		if (smExitFlag) break;

		retrieveStatistics();

		lasttime = time(0);
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
