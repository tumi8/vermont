#include "IpfixDbCommon.hpp"

IpfixDbReaderCommonCfg::IpfixDbReaderCommonCfg()
    :  port(0), timeshift(false), fullspeed(false), observationDomainId(0)
{
}


void IpfixDbReaderCommonCfg::readConfigSection(XMLElement* elem)
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
			msg(MSG_FATAL, "Unknown IpfixDbReader config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (hostname=="") THROWEXCEPTION("IpfixDbReaderCfg: host not set in configuration!");
	if (port==0) THROWEXCEPTION("IpfixDbReaderCfg: port not set in configuration!");
	if (dbname=="") THROWEXCEPTION("IpfixDbReaderCfg: dbname not set in configuration!");
	if (user=="") THROWEXCEPTION("IpfixDbReaderCfg: username not set in configuration!");
	if (password=="") THROWEXCEPTION("IpfixDbReaderCfg: password not set in configuration!");
}



