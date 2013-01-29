/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2009-2013 Vermont Project
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

#if defined(DB_SUPPORT_ENABLED) || defined(MONGO_SUPPORT_ENABLED) || defined(PG_SUPPORT_ENABLED) || defined(ORACLE_SUPPORT_ENABLED) || defined(REDIS_SUPPORT_ENABLED)

#include "IpfixDbReaderCfg.h"

#include "IpfixDbReaderMySQL.hpp"
#include "IpfixDbReaderOracle.hpp"

IpfixDbReaderCfg* IpfixDbReaderCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixDbReaderCfg(e);
}


IpfixDbReaderCfg::IpfixDbReaderCfg(XMLElement* elem)
    : CfgHelper<IpfixDbReader, IpfixDbReaderCfg>(elem, "ipfixDbReader"),
      port(0), observationDomainId(0)
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("dbType")) {
			databaseType = e->getFirstText();
		} else if (e->matches("host")) {
			hostname = e->getFirstText();
		} else if (e->matches("port")) {
			port = getInt("port");
		} else if (e->matches("dbname")) {
			dbname = e->getFirstText();
		} else if (e->matches("username")) {
			user = e->getFirstText();
		} else if (e->matches("password")) {
			password = e->getFirstText();
		} else if (e->matches("observationDomainId")) {
			observationDomainId = getInt("observationDomainId");
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown IpfixDbReader config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (databaseType != "mysql" && databaseType != "postgres" && databaseType != "oracle") THROWEXCEPTION("IpfixDbReaderCfg: Incorrect value for dbType: \"%s\"", databaseType.c_str());
	if (hostname=="") THROWEXCEPTION("IpfixDbReaderCfg: host not set in configuration!");
	if (port==0) THROWEXCEPTION("IpfixDbReaderCfg: port not set in configuration!");
	if (dbname=="") THROWEXCEPTION("IpfixDbReaderCfg: dbname not set in configuration!");
	if (user=="") THROWEXCEPTION("IpfixDbReaderCfg: username not set in configuration!");
	if (password=="") THROWEXCEPTION("IpfixDbReaderCfg: password not set in configuration!");
}


IpfixDbReaderCfg::~IpfixDbReaderCfg()
{
}


IpfixDbReader* IpfixDbReaderCfg::createInstance()
{
	if (databaseType == "mysql") {
#if defined(DB_SUPPORT_ENABLED)
		instance = new IpfixDbReaderMySQL(databaseType, hostname, dbname, user, password, port, observationDomainId);
#else
		goto except;
#endif
	} else if  (databaseType == "postgres") {

#if defined(PG_SUPPORT_ENABLED)
		goto except;
#else
		goto except;
#endif
	} else if (databaseType == "oracle") {
#if defined(ORACLE_SUPPORT_ENABLED)
		instance = new IpfixDbReaderOracle(databaseType, hostname, dbname, user, password, port, observationDomainId);
#else
		goto except;
#endif
	} else {
		goto except;
	}
	return instance;
except:
	THROWEXCEPTION("IpfixDbWriterCfg: Database type \"%s\" not yet implemented or support in vermont is not compiled in ...", databaseType.c_str());
	// this is only to surpress compiler warnings. we should never get here ...
	return 0;
    return instance;
}


bool IpfixDbReaderCfg::deriveFrom(IpfixDbReaderCfg* old)
{
    return false;
}

#endif /*DB_SUPPORT_ENABLED*/
