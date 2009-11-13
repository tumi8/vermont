#include "IpfixSamplerCfg.h"


IpfixSamplerCfg* IpfixSamplerCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixSamplerCfg(e);
}

IpfixSamplerCfg::IpfixSamplerCfg(XMLElement* elem)
    : CfgHelper<IpfixSampler, IpfixSamplerCfg>(elem, "ipfixSampler"),
    flowRate(1)
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("flowrate")) {
			flowRate = getDouble("flowrate");
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown IpfixSampler config statement %s\n", e->getName().c_str());
			continue;
		}
	}
}

IpfixSamplerCfg::~IpfixSamplerCfg()
{
}

IpfixSampler* IpfixSamplerCfg::createInstance()
{
    instance = new IpfixSampler(flowRate);
    return instance;
}

bool IpfixSamplerCfg::deriveFrom(IpfixSamplerCfg* old)
{
    return false;
}
