#include "NetflowV9ConverterCfg.hpp"


NetflowV9ConverterCfg* NetflowV9ConverterCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new NetflowV9ConverterCfg(e);
}

NetflowV9ConverterCfg::NetflowV9ConverterCfg(XMLElement* elem)
    : CfgHelper<NetflowV9Converter, NetflowV9ConverterCfg>(elem, "netflowV9Converter"),
    copyMode(true)
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("copyMode")) {
			copyMode = getBool("copyMode");
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown NetflowV9Converter config statement %s\n", e->getName().c_str());
			continue;
		}
	}
}

NetflowV9ConverterCfg::~NetflowV9ConverterCfg()
{
}

NetflowV9Converter* NetflowV9ConverterCfg::createInstance()
{
    instance = new NetflowV9Converter(copyMode);
    return instance;
}

bool NetflowV9ConverterCfg::deriveFrom(NetflowV9ConverterCfg* old)
{
    return false;
}

