#ifndef FILTERMODULE_H
#define FILTERMODULE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/


#include "reconf/Module.h"
#include "reconf/Destination.h"
#include "reconf/Source.h"
#include "sampler/PacketProcessor.h"

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

private:
	std::vector<PacketProcessor* > processors;
};

#endif
