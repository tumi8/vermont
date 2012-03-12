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

#ifdef ORACLE_SUPPORT_ENABLED

#include "IpfixDbReaderOracleCfg.h"


IpfixDbReaderOracleCfg* IpfixDbReaderCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixDbReaderOracleCfg(e);
}


IpfixDbReaderOracleCfg::IpfixDbReaderOracleCfg(XMLElement* elem)
    : CfgHelper<IpfixDbReaderOracle, IpfixDbReaderOracleCfg>(elem, "ipfixDbReaderOracle"),
      port(0), timeshift(false), fullspeed(false), observationDomainId(0)
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("host")) {
			hostname = e->getFirstText();
		} else if (e->matches("port")) {
			port = getInt("port");
		} else if (e->matches("dbname")) {
			dbname = e->getFirstText();
		} else if (e->matches("username")) {
			user = e->getFirstText();
		} else if (e->matches("password")) {
			password = e->getFirstText();
		} else if (e->matches("timeshift")) {
			timeshift = getBool("timeshift", timeshift);
		} else if (e->matches("fullspeed")) {
			fullspeed = getBool("fullspeed", fullspeed);
		} else if (e->matches("observationDomainId")) {
			observationDomainId = getInt("observationDomainId");
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown IpfixDbReaderOracle config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (hostname=="") THROWEXCEPTION("IpfixDbReaderOracleCfg: host not set in configuration!");
	if (port==0) THROWEXCEPTION("IpfixDbReaderOracleCfg: port not set in configuration!");
	if (dbname=="") THROWEXCEPTION("IpfixDbReaderOracleCfg: dbname not set in configuration!");
	if (user=="") THROWEXCEPTION("IpfixDbReaderOracleCfg: username not set in configuration!");
	if (password=="") THROWEXCEPTION("IpfixDbReaderOracleCfg: password not set in configuration!");
}


IpfixDbReaderOracleCfg::~IpfixDbReaderOracleCfg()
{
}


IpfixDbReaderOracle* IpfixDbReaderOracleCfg::createInstance()
{
    instance = new IpfixDbReaderOracle(hostname, dbname, user, password, port, observationDomainId, timeshift, fullspeed);
    return instance;
}


bool IpfixDbReaderOracleCfg::deriveFrom(IpfixDbReaderOracleCfg* old)
{
    return false;
}

#endif /* ORACLE_SUPPORT_ENABLED */
