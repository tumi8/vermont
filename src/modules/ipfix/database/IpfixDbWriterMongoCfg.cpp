/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2011 Vermont Project
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

#ifdef MONGO_SUPPORT_ENABLED

#include "IpfixDbWriterMongoCfg.h"
#include <cassert>


IpfixDbWriterMongoCfg* IpfixDbWriterMongoCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixDbWriterMongoCfg(e);
}


IpfixDbWriterMongoCfg::IpfixDbWriterMongoCfg(XMLElement* elem)
    : CfgHelper<IpfixDbWriterMongo, IpfixDbWriterMongoCfg>(elem, "ipfixDbWriterMongo"),
      port(27017), bufferObjects(30), observationDomainId(0)
{
  if (!elem) return;

  XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
  	beautifyProperties = false;
  	allProperties = false;
	for ( XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
        it != set.end();
	      it++) {
	  XMLElement* e = *it;

		if (e->matches("host")) {
			hostname = e->getFirstText();
		} else if (e->matches("port")) {
			port = getInt("port");
		} else if (e->matches("database")) {
			database = e->getFirstText();
		} else if (e->matches("username")) {
			user = e->getFirstText();
		} else if (e->matches("password")) {
			password = e->getFirstText();
		} else if (e->matches("bufferobjects")) {
			bufferObjects = getInt("bufferobjects");
		} else if (e->matches("properties")) {
			readProperties(e);
		} else if (e->matches("observationDomainId")) {
			observationDomainId = getInt("observationDomainId");
		} else if (e->matches("beautifyProperties")) {
			beautifyProperties = true;
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown IpfixDbWriterMongo config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (hostname=="") THROWEXCEPTION("IpfixDbWriterMongoCfg: host not set in configuration!");
	if (database=="") THROWEXCEPTION("IpfixDbWriterMongoCfg: dbname not set in configuration!");
}

void IpfixDbWriterMongoCfg::readProperties(XMLElement* elem) {
	properties.clear();
	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("name") && !allProperties) {
			properties.push_back(e->getFirstText());
		} else if (e->matches("all")) {
			properties.clear();
			allProperties = true;
		} else {
			msg(MSG_FATAL, "Unknown IpfixDbWriterMongo config statement %s\n", e->getName().c_str());
			continue;
		}		
	}

}

IpfixDbWriterMongoCfg::~IpfixDbWriterMongoCfg()
{
}


IpfixDbWriterMongo* IpfixDbWriterMongoCfg::createInstance()
{
  instance = new IpfixDbWriterMongo(hostname, database, user, password, port, observationDomainId, bufferObjects, properties, beautifyProperties, allProperties);
	msg(MSG_DEBUG, "IpfixDbWriterMongo configuration host %s collection %s user %s password %s port %i observationDomainId %i bufferRecords %i\n", 
	  hostname.c_str(), database.c_str(), user.c_str(), password.c_str(), port, observationDomainId, bufferObjects);
  return instance;
}


bool IpfixDbWriterMongoCfg::deriveFrom(IpfixDbWriterMongoCfg* old)
{
  return false;
}

#endif /*MONGO_SUPPORT_ENABLED*/
