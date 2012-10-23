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

#ifdef PG_SUPPORT_ENABLED

#include "IpfixDbWriterPgCfg.h"


IpfixDbWriterPgCfg* IpfixDbWriterPgCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixDbWriterPgCfg(e);
}


IpfixDbWriterPgCfg::IpfixDbWriterPgCfg(XMLElement* elem)
    : CfgHelper<IpfixDbWriterPg, IpfixDbWriterPgCfg>(elem, "ipfixDbWriterPg"),
      port(0),
      bufferRecords(30)
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
		} else if (e->matches("bufferrecords")) {
			bufferRecords = getInt("bufferrecords");
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown IpfixDbWriterPg config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (hostname=="") THROWEXCEPTION("IpfixDbWriterPgCfg: host not set in configuration!");
	if (port==0) THROWEXCEPTION("IpfixDbWriterPgCfg: port not set in configuration!");
	if (dbname=="") THROWEXCEPTION("IpfixDbWriterPgCfg: dbname not set in configuration!");
	if (user=="") THROWEXCEPTION("IpfixDbWriterPgCfg: username not set in configuration!");
	if (password=="") THROWEXCEPTION("IpfixDbWriterPgCfg: password not set in configuration!");
}


IpfixDbWriterPgCfg::~IpfixDbWriterPgCfg()
{
}


IpfixDbWriterPg* IpfixDbWriterPgCfg::createInstance()
{
    instance = new IpfixDbWriterPg(hostname.c_str(), dbname.c_str(), user.c_str(), password.c_str(), port, 0, bufferRecords);
    return instance;
}


bool IpfixDbWriterPgCfg::deriveFrom(IpfixDbWriterPgCfg* old)
{
    return false;
}

#endif /*DB_SUPPORT_ENABLED*/
