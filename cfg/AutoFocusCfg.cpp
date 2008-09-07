#include "AutoFocusCfg.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

AutoFocusCfg* AutoFocusCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new AutoFocusCfg(e);
}

AutoFocusCfg::AutoFocusCfg(XMLElement* elem)
    : CfgHelper<AutoFocus, AutoFocusCfg>(elem,"AutoFocus"),
      hashBits(20),
      timeTreeInterval(3600),
      numMaxResults(20)
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("hashbits")) {
			hashBits = getInt("hashbits");
		} else if (e->matches("timetreeinterval")) {
			timeTreeInterval = getInt("timetreeinterval");
		} else if (e->matches("numtrees")) {
			numTrees = getInt("numtrees");
		} else if (e->matches("nummaxresults")) {
			numMaxResults = getInt("nummaxresults");
		} else if (e->matches("minSubbits")) {
			minSubbits = getInt("minSubbits");
		} else if (e->matches("analyzerid")) {
			analyzerId = e->getFirstText();
		} else if (e->matches("reportfile")) {
			reportfile = e->getFirstText();
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown AutoFocus config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (analyzerId=="") THROWEXCEPTION("AutoFocus: analyzerid not set in configuration!");
}

AutoFocusCfg::~AutoFocusCfg()
{
}

AutoFocus* AutoFocusCfg::createInstance()
{
    instance = new AutoFocus(hashBits, timeTreeInterval, numMaxResults, numTrees, minSubbits, analyzerId, reportfile);
    return instance;
}

bool AutoFocusCfg::deriveFrom(AutoFocusCfg* old)
{
    return false;
}
