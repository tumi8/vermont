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
	SensorManager(uint32_t checkInterval, string outputfilename, GraphInstanceSupplier* gis);
	virtual ~SensorManager();
	void setGraphIS(GraphInstanceSupplier* gis);
	
protected:
	virtual void performStart();
	virtual void performShutdown();
	
private:
	GraphInstanceSupplier* graphIS; /** module graph from config manager */
	Thread thread;
	uint64_t hertzValue;
	
	// config variables
	uint32_t checkInterval; /** check interval in seconds */
	string outputFilename;
	
	static void* threadWrapper(void* instance);
	void collectDataWorker();
};

#endif /*SENSORMANAGER_H_*/
