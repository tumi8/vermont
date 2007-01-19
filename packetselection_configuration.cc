/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#include "packetselection_configuration.h"
#include "exporter_configuration.h"

#include <sampler/Filter.h>
#include <sampler/IPHeaderFilter.h>
#include <sampler/PacketProcessor.h>
#include <sampler/HookingFilter.h>
#include <sampler/PacketSink.h>


PacketSelectionConfiguration::PacketSelectionConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint)
{
	filter = new Filter();
}

PacketSelectionConfiguration::PacketSelectionConfiguration()
	: Configuration(0, 0)
{
	filter = new Filter();
}

PacketSelectionConfiguration::~PacketSelectionConfiguration()
{
	delete filter;
}

void PacketSelectionConfiguration::configure()
{
	msg(MSG_INFO, "PacketSelectionConfiguration: Start reading packetSelection section");
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "countBased")) {
			xmlNodePtr j = i->xmlChildrenNode;
			int interval = 0;
			int spacing = 0;
			msg(MSG_INFO, "packetSelection: Creating count based filter");
			while (NULL != j) { 
				if (tagMatches(j, "interval")) {
					interval = atoi(getContent(j).c_str());
				} else if (tagMatches(j, "spacing")) {
					spacing = atoi(getContent(j).c_str());
				}
				j = j->next;
			}
			filter->addProcessor(new SystematicSampler(SYSTEMATIC_SAMPLER_COUNT_BASED,
								   interval, spacing));
		} else if (tagMatches(i, "timeBased")) {
			xmlNodePtr j = i->xmlChildrenNode;
			int interval = 0;
			int spacing = 0;
			msg(MSG_INFO, "packetSelection: Creating time based filter");
			while (NULL != j) {
				if (tagMatches(j, "interval")) {
					interval = atoi(getContent(j).c_str());
				} else if (tagMatches(j, "spacing")) {
					spacing = atoi(getContent(j).c_str());
				}
				j = j->next;
			}
			filter->addProcessor(new SystematicSampler(SYSTEMATIC_SAMPLER_TIME_BASED,
								   interval, spacing));
		} else if (tagMatches(i, "filterMatch")) {
			xmlNodePtr j = i->xmlChildrenNode;
			while (NULL != j) {
				// TODO: construct filter ...
				msg(MSG_ERROR, "packetSelection: filterMatch not yet implemented!");
				j = j->next;
			}
		} else if (tagMatches(i, "randOutOfN")) {
			xmlNodePtr j = i->xmlChildrenNode;
			int N, n;
			n = N = 0;
			msg(MSG_INFO, "packetSelection: Creating radom filter");
			while (NULL != j) {
				if (tagMatches(j, "population")) {
					N = atoi(getContent(j).c_str());
				} else if (tagMatches(j, "sample")) {
					n = atoi(getContent(j).c_str());
				}
				j = j->next;
			}
			filter->addProcessor(new RandomSampler(n, N));
		} else if (tagMatches(i, "uniProb")) {
			msg(MSG_ERROR, "packetSelection: uniProb not yet implemented!");
		} else if (tagMatches(i, "nonUniProb")) {
			msg(MSG_ERROR, "packetSelection: nonUniProb not yet implemented");
		} else if (tagMatches(i, "flowState")) {
			msg(MSG_ERROR, "packetSelection: flowState not yet implemted");
		} else if (tagMatches(i, "filterHash")) {
			msg(MSG_ERROR, "packetSelection: filterHash not yet implemented");
		} else if (tagMatches(i, "filterRState")) {
			msg(MSG_ERROR, "packetSelection: filterRState not yet implemented");
		} else if (tagMatches(i, "rawFilter")) {
			// TODO: remove the rawfilter ...
			std::string settings;
			xmlNodePtr j = i->xmlChildrenNode;
			msg(MSG_INFO, "packetSelection: Creating raw filter");
			while (NULL != j) {
				if (tagMatches(j, "settings")) {
					settings = getContent(j);
				}
				j = j->next;
			}
			if (!settings.empty()) {
				filter->addProcessor(makeFilterProcessor("raw-filter", settings.c_str()));
			}
		}
		i = i->next;
	}
	msg(MSG_INFO, "PacketSelectionConfiguration: Successfully parsed packetSelection section");
}

// TODO: remove this method
PacketProcessor* PacketSelectionConfiguration::makeFilterProcessor(const char *name, const char *setting)
{
#define PROCESSOR_MAX_PARAM 6
#define PROCESSOR_INTERNAL 0
#define PROCESSOR_SYSTEMATIC 1
#define PROCESSOR_RANDOM 2
#define PROCESSOR_IPHEADER 3
	static char *PP_TAB[]={"Internal", "Systematic", "Random", "IPHeader", NULL };

	PacketProcessor *n;
	char *l, *token;
	int id;

	/*
	  the following is helluva dirty
	  keywords for grep: FUCK, DIRTY, MORON, SUCKS, SUCKZ

	  we don't accept filters with more than PROCESSOR_MAX_PARAM parameters
	*/
	int p_conf[PROCESSOR_MAX_PARAM];
	int p_conf_nr=0, i=0;

	/* really do not violate original string */
	if(!(l=strdup(setting))) {
		return NULL;
	}

	/* processor type is the first number */
	token=strsep(&l, ",");
	id=atoi(token);

	msg(MSG_DEBUG, "Filter: new PacketProcessor %s - type %d (%s), full params %s", name, id, PP_TAB[id], setting);
	while((token=strsep(&l, ",")) && p_conf_nr < PROCESSOR_MAX_PARAM) {
		p_conf[p_conf_nr]=atoi(token);
		/* include hardcode debuggin in case of */
		DPRINTF("PacketProcessor: token %s parsed as %d\n", token, p_conf[p_conf_nr]);
		p_conf_nr++;
	}

	/* just dump the settings if one is interested */
	while(i < p_conf_nr) {
		msg(MSG_INFO, "Processor %s param #%d: %d", name, i, p_conf[i]);
		i++;
	}

	/*
	  the following is helluva dirty, too
	  keywords for grep: FUCK, DIRTY, MORON, SUCKS, SUCKZ

	  PROCESSOR_* are defined in config_sampler.h
	  hardcoded parameters!
	*/
	switch(id) {
	case PROCESSOR_INTERNAL:
		n=NULL;
		break;
	case PROCESSOR_SYSTEMATIC:
		n=new SystematicSampler(p_conf[0], p_conf[1], p_conf[2]);
		break;
	case PROCESSOR_RANDOM:
		n=new RandomSampler(p_conf[0], p_conf[1]);
		break;
	case PROCESSOR_IPHEADER:
		n=new IPHeaderFilter(p_conf[0], p_conf[1], p_conf[2], p_conf[3], p_conf[4]);
		break;
	default:
		msg(MSG_FATAL, "Filter: cannot make PacketProcessor with ID %d, settings %s", id, setting);
		n=NULL;
	}
 //out:
	free(l);

	return n;
#undef PROCESSOR_MAX_PARAM
#undef PROCESSOR_INTERNAL
#undef PROCESSOR_SYSTEMATIC
#undef PROCESSOR_RANDOM
#undef PROCESSOR_IPHEADER
}


void PacketSelectionConfiguration::connect(Configuration* c)
{
}

void PacketSelectionConfiguration::startSystem()
{
	if (!filter->hasReceiver()) {
		// we need at least one data sink. if we don't have
		// any, there will be a memory leak (no packets will
		// be freed within the sampler)
		filter->setReceiver(new PacketSink());
		msg(MSG_INFO, "Added packet sink");
	}
	filter->startFilter();
}
