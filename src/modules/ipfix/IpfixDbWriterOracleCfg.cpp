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

#include "IpfixDbWriterOracleCfg.hpp"


IpfixDbWriterOracleCfg* IpfixDbWriterOracleCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixDbWriterOracleCfg(e);
}


IpfixDbWriterOracleCfg::IpfixDbWriterOracleCfg(XMLElement* elem)
    : CfgHelper<IpfixDbWriterOracle, IpfixDbWriterOracleCfg>(elem, "ipfixDbWriter"),
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
			msg(MSG_FATAL, "Unknown IpfixDbWriterOracle config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (hostname=="") THROWEXCEPTION("IpfixDbWriterOracleCfg: host not set in configuration!");
	if (port==0) THROWEXCEPTION("IpfixDbWriterOracleCfg: port not set in configuration!");
	if (dbname=="") THROWEXCEPTION("IpfixDbWriterOracleCfg: dbname not set in configuration!");
	if (user=="") THROWEXCEPTION("IpfixDbWriterOracleCfg: username not set in configuration!");
}

void IpfixDbWriterOracleCfg::readColumns(XMLElement* elem) {
	colNames.clear();
	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("name")) {
			colNames.push_back(e->getFirstText());
		} else {
			msg(MSG_FATAL, "Unknown IpfixDbWriterOracle config statement %s\n", e->getName().c_str());
			continue;
		}		
	}

}

IpfixDbWriterOracleCfg::~IpfixDbWriterOracleCfg()
{
}


IpfixDbWriter* IpfixDbWriterOracleCfg::createInstance()
{
    instance = new IpfixDbWriter(hostname, dbname, user, password, port, observationDomainId, bufferRecords, colNames);
    return instance;
}


bool IpfixDbWriterOracleCfg::deriveFrom(IpfixDbWriterOracleCfg* old)
{
    return false;
}

#endif /*DB_SUPPORT_ENABLED*/
