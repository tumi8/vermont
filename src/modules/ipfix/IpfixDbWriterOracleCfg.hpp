/*
 * IPFIX Database Reader/Writer Oracle Connector Configuration
 * Copyright (C) 2011 Philipp Fehre <philipp.fehre@googlemail.com>
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

#ifdef ORACLE_SUPPORT_ENABLED

#ifdef IPFIXDBWRITERORACLECFG_H
#define IPFIXDBWRITERORACLECFG_H

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/ipfix/IpfixDbWriterOracle.hpp"

#include <string>

using namespace std;


class IpfixDbWriterOracleCfg
	: public CfgHelper<IpfixDbWriterOracle, IpfixDbWriterOracleCfg>
{
public:
	friend class ConfigManager;
	
	virtual IpfixDbWriterOracleCfg* create(XMLElement* e);
	virtual ~IpfixDbWriterOracleCfg();
	
	virtual IpfixDbWriterOracle* createInstance();
	virtual bool deriveFrom(IpfixDbWriteOracleCfg* old);
	
protected:
	
	string hostname; /**< hostname of database host */
	uint16_t port;	/**< port of database */
	string dbname; /**< database name */
	string user;	/**< user name for login to database */
	string password;	/**< password for login to database */
	uint16_t bufferRecords;	/**< amount of records to buffer until they are written to database */
	uint32_t observationDomainId;	/**< default observation domain id (overrides the one received in the records */
	vector<string> colNames; /**< column names */

	void readColumns(XMLElement* elem);
	IpfixDbWriterOracleCfg(XMLElement*);
};


#endif /* IPFIXDBWRITERORACLECFG_H */

#endif /* ORACLE_SUPPORT_ENABLED */
