#include "FlowSigMatcherCfg.h"


FlowSigMatcherCfg* FlowSigMatcherCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new FlowSigMatcherCfg(e);
}

FlowSigMatcherCfg::FlowSigMatcherCfg(XMLElement* elem)
    : CfgHelper<FlowSigMatcher, FlowSigMatcherCfg>(elem, "flowSigMatcher")
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;
                if (e->matches("analyzerid")) {
			analyzerId = e->getFirstText();
		} else if (e->matches("idmeftemplate")) {
			idmefTemplate = e->getFirstText();
                } else if (e->matches("homenet")) {
			homenet = e->getFirstText();
                } else if (e->matches("rulesfile")) {
			rulesfile = e->getFirstText();
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown FlowSigMatcher config statement %s\n", e->getName().c_str());
			continue;
		}
	}
        if (rulesfile=="") THROWEXCEPTION("FlowSigMatcherCfg: rulesfilename not set in configuration!");
        if (homenet=="") THROWEXCEPTION("FlowSigMatcherCfg: homenet not set in configuration!");
	if (analyzerId=="") THROWEXCEPTION("FlowSigMatcherCfg: analyzerid not set in configuration!");
	if (idmefTemplate=="") THROWEXCEPTION("FlowSigMatcherCfg: idmeftemplate not set in configuration!");
}

FlowSigMatcherCfg::~FlowSigMatcherCfg()
{
}

FlowSigMatcher* FlowSigMatcherCfg::createInstance()
{
    instance = new FlowSigMatcher(homenet, rulesfile, analyzerId, idmefTemplate);
    return instance;
}

bool FlowSigMatcherCfg::deriveFrom(FlowSigMatcherCfg* old)
{
    return false;
}
