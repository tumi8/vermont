#include "ObserverCfg.h"
#include "common/msg.h"
#include "cfg/XMLElement.h"

#include "sampler/Observer.h"

#include <string>
#include <vector>
#include <cassert>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

ObserverCfg* ObserverCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new ObserverCfg(e);
}

ObserverCfg::ObserverCfg(XMLElement* elem)
	: Cfg(elem), observer(NULL), interface(), pcap_filter(), capture_len(0)
{

}

ObserverCfg::~ObserverCfg()
{
}

Observer* ObserverCfg::getInstance()
{
	if (observer)
		return observer;


	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("interface")) {
			interface = e->getFirstText();
		} else if (e->matches("capture_len")) {
			capture_len = atoi(e->getFirstText().c_str());
		} else if (e->matches("pcap_filter")) {
			pcap_filter = e->getFirstText();
		} else if (e->matches("timeBased")) {

		} else {
			msg(MSG_FATAL, "Unkown observer config statement %s\n", e->getName().c_str());
			continue;
		}
	}

	observer = new Observer(interface, &packetManager);

	if (capture_len) {
		if(!observer->setCaptureLen(capture_len)) {
			msg(MSG_FATAL, "Observer: wrong snaplen specified - using %d",
					observer->getCaptureLen());
		}
	}

	if (!pcap_filter.empty()) {
		if (!observer->prepare(pcap_filter.c_str())) {
			msg(MSG_FATAL, "Observer: preparing failed");
			THROWEXCEPTION("Observer setup failed!");
		}
	}

	return observer;
}

bool ObserverCfg::deriveFrom(ObserverCfg* old)
{
	if (interface != old->interface)
		return false;
	if (capture_len != old->capture_len)
		return false;
	if (pcap_filter != old->pcap_filter)
		return false;

	observer = old->getInstance();
	return true;
}
