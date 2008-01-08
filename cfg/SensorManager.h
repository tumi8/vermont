#ifndef SENSORMANAGER_H_
#define SENSORMANAGER_H_

#include "reconf/Module.h"
#include "GraphInstanceSupplier.h"

#include "common/Thread.h"


class SensorManager
	: public Module,
	  public Destination<NullEmitable*>, 
	  public Source<NullEmitable*>
{
public:
	virtual ~SensorManager();
	
	static SensorManager& getInstance();
	void setGraphIS(GraphInstanceSupplier* gis);
	void setCheckInterval(uint32_t checkInterval);
	void setOutputFilename(string name);
	void addSensor(Sensor* sensor, const string name, uint32_t id);
	void removeSensor(Sensor* sensor);
	
	
	
protected:
	virtual void performStart();
	virtual void performShutdown();
	
private:
	struct SensorEntry {
		string name;
		uint32_t id;
		Sensor* sensor;
	};
	
	GraphInstanceSupplier* graphIS; /** module graph from config manager */
	Thread thread;
	uint64_t hertzValue;
	list<SensorEntry> sensors;
	Mutex mutex; /** protects variable sensors */
	
	// config variables
	uint32_t checkInterval; /** check interval in seconds */
	string outputFilename;

	SensorManager(uint32_t checkInterval, string outputfilename, GraphInstanceSupplier* gis);
	
	static void* threadWrapper(void* instance);
	void collectDataWorker();
	void writeSensorXML(FILE* file, Sensor* s, const char* name, uint32_t id, bool module, 
			   			time_t curtime, time_t lasttime, vector<uint32_t>* nextids);
};

#endif /*SENSORMANAGER_H_*/
