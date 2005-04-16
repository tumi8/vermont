#ifndef VERMONT_H
#define VERMONT_H

#include <vector>

#include "iniparser.h"
#include <sampler/Template.h>
#include <sampler/Observer.h>
#include <sampler/Filter.h>
#include <sampler/PacketProcessor.h>
#include <sampler/ExporterSink.h>

/* holding all objects/handles/... for the subsystems like sampler and collector */
struct v_objects {

	/* main vermont: */
        /* the configuration struct, from iniparser */
	dictionary *v_config;
	/* initialized subsystems */
	unsigned int v_subsystems;

	/* for sampler: */
	Template *templ;
	Observer *observer;
        Filter *filter;
        /* exporter is either a normal sink or a real exporter process */
        PacketReceiver *sink;
		
	/* for concentrator: */
	//IpfixReceiver *concentrator;
};

#endif
