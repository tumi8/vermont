/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/

#ifndef   	PACKETSELECTION_CONFIGURATION_H_
#define   	PACKETSELECTION_CONFIGURATION_H_

#include "ipfix_configuration.h"

#include <vector>


class FilterModule;
class PacketProcessor;
class MeteringConfiguration;
class PacketSink;

class PacketSelectionConfiguration : public Configuration {
public:
	PacketSelectionConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	PacketSelectionConfiguration();
	~PacketSelectionConfiguration();

	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();
	virtual void stopSystem();

	FilterModule* getFilters() { return filter; }

protected:
	FilterModule * filter;

	PacketSink* dummySink; /**< contains a pointer if we created a DummySink, i.e. we are responsible for cleaning it up */

	friend class MeteringConfiguration;
};

#endif 	    /* !PACKETSELECTION_CONFIGURATION_H_ */
