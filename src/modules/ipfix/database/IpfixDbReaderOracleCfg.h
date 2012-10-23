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

#ifndef IPFIXDBREADERORACLECFG_H_
#define IPFIXDBREADERORACLECFG_H_

#ifdef ORACLE_SUPPORT_ENABLED

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/ipfix/IpfixDbReaderOracle.hpp"

#include <string>

class IpfixDbReaderOracleCfg
	: public CfgHelper<IpfixDbReaderOracle, IpfixDbReaderOracleCfg>
{
public:
	friend class ConfigManager;
	
	virtual IpfixDbReaderOracleCfg* create(XMLElement* e);
	virtual ~IpfixDbReaderOracleCfg();
	
	virtual IpfixDbReaderOracle* createInstance();
	virtual bool deriveFrom(IpfixDbReaderOracleCfg* old);
	
protected:
	
	string hostname; /**< hostname of database host */
	uint16_t port;	/**< port of database */
	string dbname; /**< database name */
	string user;	/**< user name for login to database */
	string password;	/**< password for login to database */
	bool timeshift; /**< shift time stamps */
	bool fullspeed;  /**< reading in full speed */
	uint32_t observationDomainId;	/**< observation domain id */
	uint32_t startTime; /** FirstSwitchedTime of first flow that should be read from the DB */
	uint32_t endTime; /** FirstSwitchedTime of last flow that should be read form the DB */
	
	IpfixDbReaderOracleCfg(XMLElement*);
};


#endif /*ORACLE_SUPPORT_ENABLED*/

#endif /*IPFIXDBREADERORACLECFG_H_*/
