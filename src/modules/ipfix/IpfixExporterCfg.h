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

#ifndef IPFIXEXPORTERCFG_H_
#define IPFIXEXPORTERCFG_H_

#include "core/Cfg.h"
#include "modules/ipfix/CollectorCfg.h"
#include "modules/ipfix/IpfixSender.hpp"

class IpfixExporterCfg
	: public CfgHelper<IpfixSender, IpfixExporterCfg>
{
	friend class ConfigManager;
public:
	virtual ~IpfixExporterCfg();

	virtual IpfixExporterCfg* create(XMLElement* elem);

	virtual IpfixSender* createInstance();

	bool deriveFrom(IpfixExporterCfg* old);
	
	bool equalTo(IpfixExporterCfg* other);

protected:
	IpfixExporterCfg(XMLElement* elem);

private:
	/** this holds the info for all the hosts we want to export to */
	std::vector<CollectorCfg*> collectors;

	/** template management */
	unsigned templateRefreshTime;
	/* unsigned templateRefreshRate; */ /* TODO */
	
	/** sctp parameters */
	uint32_t sctpDataLifetime;
	uint32_t sctpReconnectInterval;

	export_protocol_version export_protocol;
	uint32_t recordRateLimit;
	uint32_t observationDomainId;
	
	/** DTLS parameters */
	std::string certificateChainFile;
	std::string privateKeyFile;
	std::string caFile;
	std::string caPath;

	unsigned dtlsMaxConnectionLifetime;
};

#endif /*IPFIXEXPORTERCFG_H_*/
