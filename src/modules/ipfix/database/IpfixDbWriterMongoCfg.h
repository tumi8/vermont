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

#ifndef IPFIXDBWRITERMONGOCFG_H_
#define IPFIXDBWRITERMONGOCFG_H_

#ifdef MONGO_SUPPORT_ENABLED

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "IpfixDbWriterMongo.hpp"

#include <string>

using namespace std;


class IpfixDbWriterMongoCfg
	: public CfgHelper<IpfixDbWriterMongo, IpfixDbWriterMongoCfg>
{
public:
	friend class ConfigManager;
	
	virtual IpfixDbWriterMongoCfg* create(XMLElement* e);
	virtual ~IpfixDbWriterMongoCfg();
	
	virtual IpfixDbWriterMongo* createInstance();
	virtual bool deriveFrom(IpfixDbWriterMongoCfg* old);
	
protected:
	
	string hostname; /**< hostname of database host */
	uint16_t port;	/**< port of database */
	string database; /**< mongo database name */
	string user;	/**< user name for login to database */
	string password;	/**< password for login to database */
	uint16_t bufferObjects;	/**< amount of records to buffer until they are written to database */
	uint32_t observationDomainId;	/**< default observation domain id (overrides the one received in the records */
	vector<string> properties; /**< property names */
	bool beautifyProperties; /* whether to use beautified property names or raw ipfix number */
	bool allProperties; /* whether to get all properties or just a subset of it*/

	void readProperties(XMLElement* elem);
	IpfixDbWriterMongoCfg(XMLElement*);
};


#endif /*DB_SUPPORT_ENABLED*/

#endif /*IPFIXDBWRITERMONGOCFG_H_*/
