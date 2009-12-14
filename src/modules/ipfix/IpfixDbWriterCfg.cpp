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

#ifdef DB_SUPPORT_ENABLED

#include "IpfixDbWriterCfg.h"


IpfixDbWriterCfg* IpfixDbWriterCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixDbWriterCfg(e);
}


IpfixDbWriterCfg::IpfixDbWriterCfg(XMLElement* elem)
    : CfgHelper<IpfixDbWriter, IpfixDbWriterCfg>(elem, "ipfixDbWriter"),
      port(0), bufferRecords(30), observationDomainId(0)
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
		} else if (e->matches("columns")) {
			readColumns(e);
		} else if (e->matches("observationDomainId")) {
			observationDomainId = getInt("observationDomainId");
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown IpfixDbWriter config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (hostname=="") THROWEXCEPTION("IpfixDbWriterCfg: host not set in configuration!");
	if (port==0) THROWEXCEPTION("IpfixDbWriterCfg: port not set in configuration!");
	if (dbname=="") THROWEXCEPTION("IpfixDbWriterCfg: dbname not set in configuration!");
	if (user=="") THROWEXCEPTION("IpfixDbWriterCfg: username not set in configuration!");
	if (password=="") THROWEXCEPTION("IpfixDbWriterCfg: password not set in configuration!");
}

void IpfixDbWriterCfg::readColumns(XMLElement* elem) {
	colNames.clear();
	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("name")) {
			colNames.push_back(e->getFirstText());
		} else {
			msg(MSG_FATAL, "Unknown IpfixDbWriter config statement %s\n", e->getName().c_str());
			continue;
		}		
	}

}

IpfixDbWriterCfg::~IpfixDbWriterCfg()
{
}


IpfixDbWriter* IpfixDbWriterCfg::createInstance()
{
    instance = new IpfixDbWriter(hostname, dbname, user, password, port, observationDomainId, bufferRecords, colNames);
    return instance;
}


bool IpfixDbWriterCfg::deriveFrom(IpfixDbWriterCfg* old)
{
    return false;
}

#endif /*DB_SUPPORT_ENABLED*/
