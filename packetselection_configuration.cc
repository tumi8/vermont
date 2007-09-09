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
#include <sampler/stringFilter.h>
#include <sampler/regExFilter.h>


PacketSelectionConfiguration::PacketSelectionConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), dummySink(0)
{
	filter = new Filter();
}

PacketSelectionConfiguration::PacketSelectionConfiguration()
	: Configuration(0, 0), dummySink(0)
{
	filter = new Filter();
}

PacketSelectionConfiguration::~PacketSelectionConfiguration()
{
	delete filter;
	delete dummySink;
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
			msg(MSG_INFO, "PacketSelectionConfiguration: Creating count based sampler");
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
			msg(MSG_INFO, "PacketSelectionConfiguration: Creating time based sampler");
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
				msg(MSG_ERROR, "PacketSelectionConfiguration: filterMatch not yet implemented!");
				j = j->next;
			}
		} else if (tagMatches(i, "randOutOfN")) {
			xmlNodePtr j = i->xmlChildrenNode;
			int N, n;
			n = N = 0;
			msg(MSG_INFO, "PacketSelectionConfiguration: Creating random sampler");
			while (NULL != j) {
				if (tagMatches(j, "population")) {
					N = atoi(getContent(j).c_str());
				} else if (tagMatches(j, "sample")) {
					n = atoi(getContent(j).c_str());
				}
				j = j->next;
			}
			filter->addProcessor(new RandomSampler(n, N));
		} else if (tagMatches(i, "stringFilter")) {
			xmlNodePtr j = i->xmlChildrenNode;
			stringFilter* sfilter = new stringFilter();
			msg(MSG_INFO, "PacketSelectionConfiguration: Creating string filter");
			while (NULL != j) {
				if (tagMatches(j, "is")) {
					xmlChar *stype = xmlGetProp(j, (const xmlChar*)"type");
					std::string str = getContent(j);
					if(!xmlStrcmp(stype, (const xmlChar*)"HEX"))
						sfilter->addandFilter(sfilter->hexparser(str));
					else
						sfilter->addandFilter(str);
				} else if (tagMatches(j, "isnot")) {
					xmlChar *stype = xmlGetProp(j, (const xmlChar*)"type");
					if(!xmlStrcmp(stype, (const xmlChar*)"HEX"))
						sfilter->addnotFilter(sfilter->hexparser(getContent(j)));
					else
						sfilter->addnotFilter(getContent(j));
				}
				j = j->next;
			}
			filter->addProcessor(sfilter);
		} else if (tagMatches(i, "regExFilter")) {
			xmlNodePtr j = i->xmlChildrenNode;
			regExFilter* rfilter = new regExFilter();
			msg(MSG_INFO, "PacketSelectionConfiguration: Creating regular expression filter!");
			while (NULL != j) {
				if (tagMatches(j, "matchPattern")) {
					rfilter->match = getContent(j);
				}
				j = j->next;
			}

			rfilter->regcre();
			filter->addProcessor(rfilter);

		} else if (tagMatches(i, "uniProb")) {
			msg(MSG_ERROR, "PacketSelectionConfiguration: uniProb not yet implemented!");
		} else if (tagMatches(i, "nonUniProb")) {
			msg(MSG_ERROR, "PacketSelectionConfiguration: nonUniProb not yet implemented");
		} else if (tagMatches(i, "flowState")) {
			msg(MSG_ERROR, "PacketSelectionConfiguration: flowState not yet implemted");
		} else if (tagMatches(i, "filterHash")) {
			msg(MSG_ERROR, "PacketSelectionConfiguration: filterHash not yet implemented");
		} else if (tagMatches(i, "filterRState")) {
			msg(MSG_ERROR, "PacketSelectionConfiguration: filterRState not yet implemented");
		} else if (tagMatches(i, "rawFilter")) {
			xmlNodePtr j = i->xmlChildrenNode;
			int header, offset, size, comp, value;
			header = offset = size = comp = value = 0;
			msg(MSG_INFO, "PacketSelectionConfiguration: Creating raw filter");
			while (NULL != j) {
				if (tagMatches(j, "header")) {
					header =  atoi(getContent(j).c_str());
				} else if (tagMatches(j, "offset")) {
					offset = atoi(getContent(j).c_str());
				} else if (tagMatches(j, "size")) {
					size = atoi(getContent(j).c_str());
				} else if (tagMatches(j, "comp")) {
					comp = atoi(getContent(j).c_str());
				} else if (tagMatches(j, "value")) {
					value = atoi(getContent(j).c_str());
				}
				j = j->next;
			}
			filter->addProcessor(new IPHeaderFilter(header, offset, size, comp, value));
		}
		i = i->next;
	}
	msg(MSG_INFO, "PacketSelectionConfiguration: Successfully parsed packetSelection section");
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
		dummySink = new PacketSink();
		dummySink->runSink();
		filter->setReceiver(dummySink);
		msg(MSG_INFO, "PacketSelectionConfiguration: Added packet sink");
	}
	filter->startFilter();
}

void PacketSelectionConfiguration::stopSystem()
{
	filter->terminate();
	if (dummySink) {
		dummySink->terminateSink();
	}
}

