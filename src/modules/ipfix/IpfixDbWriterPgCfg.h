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

#ifndef IPFIXDBWRITERPGCFG_H_
#define IPFIXDBWRITERPGCFG_H_

#ifdef PG_SUPPORT_ENABLED

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "IpfixDbWriterPg.hpp"

#include <string>

using namespace std;


class IpfixDbWriterPgCfg
	: public CfgHelper<IpfixDbWriterPg, IpfixDbWriterPgCfg>
{
public:
	friend class ConfigManager;
	
	virtual IpfixDbWriterPgCfg* create(XMLElement* e);
	virtual ~IpfixDbWriterPgCfg();
	
	virtual IpfixDbWriterPg* createInstance();
	virtual bool deriveFrom(IpfixDbWriterPgCfg* old);
	
protected:
	
	string hostname; /**< hostname of database host */
	uint16_t port;	/**< port of database */
	string dbname; /**< database name */
	string user;	/**< user name for login to database */
	string password;	/**< password for login to database */
	uint16_t bufferRecords;	/**< amount of records to buffer until they are written to database */

	
	IpfixDbWriterPgCfg(XMLElement*);
};


#endif /*DB_SUPPORT_ENABLED*/

#endif /*IPFIXDBWRITERCFG_H_*/
