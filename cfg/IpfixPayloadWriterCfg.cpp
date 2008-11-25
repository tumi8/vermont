#include "IpfixPayloadWriterCfg.h"

#include <sys/stat.h>

IpfixPayloadWriterCfg* IpfixPayloadWriterCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixPayloadWriterCfg(e);
}

IpfixPayloadWriterCfg::IpfixPayloadWriterCfg(XMLElement* elem)
    : CfgHelper<IpfixPayloadWriter, IpfixPayloadWriterCfg>(elem, "ipfixPayloadWriter"),
      noConnections(0),
      ignoreEmptyPayload(false),
      ignoreIncompleteTCP(false)
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("destPath")) {
			path = e->getFirstText();
		} else if (e->matches("filenamePrefix")) {
			filenamePrefix = e->getFirstText();
		} else if (e->matches("connNumber")) {
			noConnections = getInt("connNumber");
		} else if (e->matches("ignoreEmptyPayload")) {
			ignoreEmptyPayload = getBool("ignoreEmptyPayload");
		} else if (e->matches("ignoreIncompleteTCP")) {
			ignoreIncompleteTCP = getInt("ignoreIncompleteTCP");
		} else if (e->matches("ignoreIncompleteTCP")) {
			ignoreIncompleteTCP = getInt("ignoreIncompleteTCP");
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown IpfixPayloadWriter config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (path=="") THROWEXCEPTION("IpfixPayloadWriterCfg: destPath not set in configuration!");
	if (filenamePrefix=="") THROWEXCEPTION("IpfixPayloadWriterCfg: filenamePrefix not set in configuration!");
	if (noConnections==0) THROWEXCEPTION("IpfixPayloadWriterCfg: connNumber not set in configuration, or value not > 0!");

	struct stat s;
	if (stat(path.c_str(), &s) != 0)
		THROWEXCEPTION("IpfixPayloadWriterCfg: failed to access destination path '%s', error: %s", path.c_str(), strerror(errno));
}

IpfixPayloadWriterCfg::~IpfixPayloadWriterCfg()
{
}

IpfixPayloadWriter* IpfixPayloadWriterCfg::createInstance()
{
    instance = new IpfixPayloadWriter(path, filenamePrefix, noConnections, ignoreEmptyPayload, ignoreIncompleteTCP);
    return instance;
}

bool IpfixPayloadWriterCfg::deriveFrom(IpfixPayloadWriterCfg* old)
{
    return false;
}
