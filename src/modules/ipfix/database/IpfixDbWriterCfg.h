/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2009 - 2012 Vermont Project
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

#ifndef IPFIXDBWRITERCFG_H_
#define IPFIXDBWRITERCFG_H_

#if defined(DB_SUPPORT_ENABLED) || defined(MONGO_SUPPORT_ENABLED) || defined(PG_SUPPORT_ENABLED) || defined(ORACLE_SUPPORT_ENABLED) || defined(REDIS_SUPPORT_ENABLED)

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "IpfixDbWriterSQL.hpp"

#include <string>

using namespace std;


class IpfixDbWriterCfg
	: public CfgHelper<IpfixDbWriterSQL, IpfixDbWriterCfg>
{
public:
	friend class ConfigManager;
	
	virtual IpfixDbWriterCfg* create(XMLElement* e);
	virtual ~IpfixDbWriterCfg();
	
	virtual IpfixDbWriterSQL* createInstance();
	virtual bool deriveFrom(IpfixDbWriterCfg* old);
	
protected:
	string databaseType; /**< Type of database (mysql, psgl, oracle, ...) */	
	string hostname; /**< hostname of database host */
	uint16_t port;	/**< port of database */
	string dbname; /**< database name */
	string user;	/**< user name for login to database */
	string password;	/**< password for login to database */
	uint16_t bufferRecords;	/**< amount of records to buffer until they are written to database */
	uint32_t observationDomainId;	/**< default observation domain id (overrides the one received in the records */
	string tablePrefix; /**< prefix for database table names */
	vector<string> colNames; /**< column names */
	bool useLegacyNames;

	void readColumns(XMLElement* elem);
	IpfixDbWriterCfg(XMLElement*);
};


#endif /*DB_SUPPORT_ENABLED*/

#endif /*IPFIXDBWRITERCFG_H_*/
