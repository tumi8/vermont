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

#include "common/ipfixlolib/ipfixlolib_config.h"
#ifdef SUPPORT_DTLS
#include "common/ipfixlolib/ipfixlolib_dtls.h"
#endif
#include "IpfixExporterCfg.h"

IpfixExporterCfg::IpfixExporterCfg(XMLElement* elem)
	: CfgHelper<IpfixSender, IpfixExporterCfg>(elem, "ipfixExporter"),
	templateRefreshTime(IS_DEFAULT_TEMPLATE_TIMEINTERVAL), /* templateRefreshRate(0), */
	sctpDataLifetime(0), sctpReconnectInterval(0), export_protocol(IPFIX_PROTOCOL),
	recordRateLimit(0), observationDomainId(0),
	dtlsMaxConnectionLifetime(0)
{

	if (!elem) {
		return;
	}
	
	recordRateLimit = getInt("maxRecordRate", IS_DEFAULT_MAXRECORDRATE);
	msg(LOG_NOTICE, "Exporter: using maximum rate of %d records/second", recordRateLimit);
	observationDomainId = getInt("observationDomainId", 0);
	sctpDataLifetime = getTimeInUnit("sctpDataLifetime", mSEC, IS_DEFAULT_SCTP_DATALIFETIME);
	sctpReconnectInterval = getTimeInUnit("sctpReconnectInterval", SEC, IS_DEFAULT_SCTP_RECONNECTINTERVAL);
	/* templateRefreshRate = getInt("templateRefreshRate", IS_DEFAULT_TEMPLATE_RECORDINTERVAL); */
	templateRefreshTime = getTimeInUnit("templateRefreshInterval", SEC, IS_DEFAULT_TEMPLATE_TIMEINTERVAL);
	// Config for DTLS
	certificateChainFile = getOptional("cert");
	privateKeyFile = getOptional("key");
	caFile = getOptional("CAfile");
	caPath = getOptional("CApath");
	dtlsMaxConnectionLifetime = getTimeInUnit("dtlsMaxConnectionLifetime", SEC, IS_DEFAULT_DTLS_CONNECTIONLIFETIME);
	

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("collector")) {
			CollectorCfg *c = new CollectorCfg(e, this->getID());
			if (c->getPeerFqdns().size() > 1) {
				delete c;
				THROWEXCEPTION("You specified more than one peerFqdn for an exporter.");
			}
			collectors.push_back(c);
		} else if (	e->matches("maxRecordRate") ||
				e->matches("sctpDataLifetime") ||
				e->matches("sctpReconnectInterval") ||
				/* e->matches("templateRefreshRate") || */
				e->matches("templateRefreshInterval") ||
				e->matches("observationDomainId") ||
				e->matches("cert") ||
				e->matches("key") ||
				e->matches("CAfile") ||
				e->matches("CApath") ||
				e->matches("dtlsMaxConnectionLifetime")
				) {
			// already done!
		} else if (e->matches("protocolVersion")) {
			std::string protVer = e->getContent();
			if (protVer=="IPFIX" || protVer=="10") {
				export_protocol = IPFIX_PROTOCOL;
			} else if (protVer=="NFV9" || protVer=="9") {
				export_protocol = NFV9_PROTOCOL;
			} else {
				THROWEXCEPTION("Invalid configuration parameter for protocolVersion (%s)", protVer.c_str());
			}
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
			sctpReconnectInterval, templateRefreshTime,
			certificateChainFile, privateKeyFile, caFile, caPath, export_protocol);

	std::vector<CollectorCfg*>::const_iterator it;
	for (it = collectors.begin(); it != collectors.end(); it++) {
		CollectorCfg *p = *it;
#ifdef DEBUG
		const char *protocol;
		switch (p->getProtocol()) {
			case SCTP:
				protocol = "SCTP"; break;
			case DTLS_OVER_UDP:
				protocol = "DTLS_OVER_UDP"; break;
			case DTLS_OVER_SCTP:
				protocol = "DTLS_OVER_SCTP"; break;
			case UDP:
				protocol = "UDP"; break;
			default:
				protocol = "unknown protocol"; break;
		}
		msg(LOG_INFO, "IpfixExporter: adding collector %s://%s:%d",
				protocol,
				p->getIpAddress().c_str(),
				p->getPort());
#endif
		void *aux_config = NULL;
		ipfix_aux_config_udp acu;
		ipfix_aux_config_udp *pacu = NULL;
#ifdef SUPPORT_DTLS
		ipfix_aux_config_dtls_over_udp acdou;
		ipfix_aux_config_dtls_over_sctp acdos;
		ipfix_aux_config_dtls *pacd = NULL;
#endif
		switch (p->getProtocol()) {
#ifdef SUPPORT_DTLS
			case DTLS_OVER_UDP:
				acdou.max_connection_lifetime = dtlsMaxConnectionLifetime;
				pacd = &acdou.dtls;
				pacu = &acdou.udp;
				aux_config = &acdou;
				break;
			case DTLS_OVER_SCTP:
				pacd = &acdos.dtls;
				aux_config = &acdos;
				break;
#endif
			case UDP:
				aux_config = &acu;
				pacu = &acu;
				break;
			default:
				break;
		}
#ifdef SUPPORT_DTLS
		if (pacd) {
			pacd->peer_fqdn = NULL;
			static const std::set<std::string> peerFqdns = p->getPeerFqdns();
			std::set<std::string>::const_iterator it = peerFqdns.begin();
			if (it != peerFqdns.end())
				pacd->peer_fqdn = it->c_str();
		}
#endif
		if (pacu) {
			pacu->mtu = p->getMtu();
		}
		instance->addCollector(
			p->getIpAddress().c_str(),
			p->getPort(), p->getProtocol(),
			aux_config,
			p->getVrfName().c_str());
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
	if (sctpDataLifetime != other->sctpDataLifetime) return false;
	if (sctpReconnectInterval != other->sctpReconnectInterval) return false;
	if (recordRateLimit != other->recordRateLimit) return false;
	if (observationDomainId != other->observationDomainId) return false;
	if (certificateChainFile != other->certificateChainFile) return false;
	if (privateKeyFile != other->privateKeyFile) return false;
	if (caFile != other->caFile) return false;
	if (caPath != other->caPath) return false;
	if (dtlsMaxConnectionLifetime != other->dtlsMaxConnectionLifetime) return false;
	if (templateRefreshTime != other->templateRefreshTime) return false;
	/* if (templateRefreshRate != other->templateRefreshRate) return false; */ /* TODO */
	if (export_protocol != other->export_protocol) return false;
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
