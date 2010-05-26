/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2009 Vermont Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "IpfixExporterCfg.h"

IpfixExporterCfg::IpfixExporterCfg(XMLElement* elem)
	: CfgHelper<IpfixSender, IpfixExporterCfg>(elem, "ipfixExporter"),
	templateRefreshTime(IS_DEFAULT_TEMPLATE_TIMEINTERVAL), templateRefreshRate(0),	
	sctpDataLifetime(0), sctpReconnectInterval(0),
	maxPacketSize(0), exportDelay(0),
	recordRateLimit(0), observationDomainId(0)
{

	if (!elem) {
		return;
	}
	
	recordRateLimit = getInt("maxRecordRate", IS_DEFAULT_MAXRECORDRATE);
	observationDomainId = getInt("observationDomainId", 0);
	msg(MSG_INFO, "Exporter: using maximum rate of %d records/second", recordRateLimit);
	sctpDataLifetime = getTimeInUnit("sctpDataLifetime", mSEC, IS_DEFAULT_SCTP_DATALIFETIME);
	sctpReconnectInterval = getTimeInUnit("sctpReconnectInterval", SEC, IS_DEFAULT_SCTP_RECONNECTINTERVAL);
	templateRefreshRate = getInt("templateRefreshRate", IS_DEFAULT_TEMPLATE_RECORDINTERVAL);
	templateRefreshTime = getTimeInUnit("templateRefreshInterval", SEC, IS_DEFAULT_TEMPLATE_TIMEINTERVAL);
	

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("collector")) {
			collectors.push_back(new CollectorCfg(e));
		} else if (e->matches("maxRecordRate") || e->matches("sctpDataLifetime") || e->matches("sctpReconnectInterval")
				|| e->matches("templateRefreshRate")|| e->matches("templateRefreshInterval") || e->matches("observationDomainId")) {		
			// already done!
		} else {
			THROWEXCEPTION("Illegal Exporter config entry \"%s\" found",
					e->getName().c_str());
		}
	}
}

IpfixExporterCfg::~IpfixExporterCfg()
{
	for(size_t i = 0; i < collectors.size(); i++)
		delete collectors[i];
}

IpfixSender* IpfixExporterCfg::createInstance()
{
	instance = new IpfixSender(observationDomainId, recordRateLimit, sctpDataLifetime, 
			sctpReconnectInterval, templateRefreshTime, templateRefreshRate);

	for (unsigned i = 0; i != collectors.size(); ++i) {
		msg(MSG_DEBUG, "IpfixExporter: adding collector %s://%s:%d",
				collectors[i]->getProtocol()==132?"SCTP":"UDP",
				collectors[i]->getIpAddress().c_str(),
				collectors[i]->getPort());
		instance->addCollector(collectors[i]->getIpAddress().c_str(), collectors[i]->getPort(), collectors[i]->getProtocol());
	}

	return instance;
}

IpfixExporterCfg* IpfixExporterCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new IpfixExporterCfg(elem);
}

bool IpfixExporterCfg::deriveFrom(IpfixExporterCfg* other)
{
	return equalTo(other);
}

bool IpfixExporterCfg::equalTo(IpfixExporterCfg* other)
{
	if (maxPacketSize != other->maxPacketSize) return false;
	if (exportDelay != other->exportDelay) return false;
	if (templateRefreshTime != other->templateRefreshTime) return false;
	if (templateRefreshRate != other->templateRefreshRate) return false;
	if (collectors.size() != other->collectors.size()) return false;
	std::vector<CollectorCfg*>::const_iterator iter = collectors.begin();
	while (iter != collectors.end()) {
		std::vector<CollectorCfg*>::const_iterator biter = other->collectors.begin();
		bool found = false;
		while (biter != other->collectors.end()) {
			if ((*iter)->equalTo(*biter)) {
				found = true;
				break;
			}
			biter++;
		}
		if (!found) return false;
		iter++;
	}
	
	return true;
}
