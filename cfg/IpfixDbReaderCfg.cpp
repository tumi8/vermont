#ifdef DB_SUPPORT_ENABLED

#include "IpfixDbReaderCfg.h"


IpfixDbReaderCfg* IpfixDbReaderCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixDbReaderCfg(e);
}


IpfixDbReaderCfg::IpfixDbReaderCfg(XMLElement* elem)
    : CfgHelper<IpfixDbReader, IpfixDbReaderCfg>(elem, "ipfixDbReader"),
      port(0)
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


IpfixDbReaderCfg::~IpfixDbReaderCfg()
{
}


IpfixDbReader* IpfixDbReaderCfg::createInstance()
{
    instance = new IpfixDbReader(hostname.c_str(), dbname.c_str(), user.c_str(), password.c_str(), port, 0);
    return instance;
}


bool IpfixDbReaderCfg::deriveFrom(IpfixDbReaderCfg* old)
{
    return false;
}

#endif /*DB_SUPPORT_ENABLED*/
