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

#ifndef IPFIXCOLLECTORCFG_H_
#define IPFIXCOLLECTORCFG_H_

#include "core/Cfg.h"
#include <modules/ipfix/IpfixCollector.hpp>

class CollectorCfg;

class IpfixCollectorCfg
	: public CfgHelper<IpfixCollector, IpfixCollectorCfg>
{
public:
	IpfixCollectorCfg(XMLElement* elem);
	virtual ~IpfixCollectorCfg();
	
	virtual IpfixCollectorCfg* create(XMLElement* elem);
	
	virtual IpfixCollector* createInstance();
	
	bool deriveFrom(IpfixCollectorCfg* old);
	
private:
	CollectorCfg* listener;

	friend class CollectorCfg;

	/** DTLS parameters */
	std::string certificateChainFile;
	std::string privateKeyFile;
	std::string caFile;
	std::string caPath;

	
        IpfixCollector* ipfixCollector;

        int32_t udpTemplateLifetime;
};

#endif /*IPFIXCOLLECTORCFG_H_*/
