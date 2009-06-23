#ifndef FILTERMODULE_H
#define FILTERMODULE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/


#include "core/Module.h"
#include "core/Destination.h"
#include "core/Source.h"
#include "modules/packet/filter/PacketProcessor.h"

class FilterModule
	: public Module, public Source<Packet*>, public Destination<Packet*>
{
public:
	FilterModule();
	virtual ~FilterModule();

	virtual void receive(Packet *);

	void addProcessor(PacketProcessor *p);
	std::vector<PacketProcessor*> getProcessors();

	bool hasReceiver();
	virtual std::string getStatisticsXML(double interval);

private:
	std::vector<PacketProcessor* > processors;
};

#endif
