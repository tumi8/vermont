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
