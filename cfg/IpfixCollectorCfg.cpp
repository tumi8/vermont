#include "IpfixCollectorCfg.h"
#include <concentrator/IpfixReceiverUdpIpV4.hpp>

IpfixCollectorCfg::IpfixCollectorCfg(XMLElement* elem)
	: CfgHelper<IpfixCollector, IpfixCollectorCfg>(elem, "ipfixCollector"),
	listener(NULL),
	ipfixCollector(NULL),
        observationDomainId(0)
{

	if (!elem)
		return;
	
	msg(MSG_INFO, "CollectorConfiguration: Start reading packetReporting section");
	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("listener")) {
			listener = new CollectorCfg(e);
		} else if (e->matches("udpTemplateLifetime")) {
			msg(MSG_DEBUG, "Don't know how to handle udpTemplateLifetime! Ignored.");
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unkown observer config statement %s\n", e->getName().c_str());
			continue;
		}
	}

	observationDomainId = getInt("observationDomainId", 0);
	
	if (listener == NULL)
		THROWEXCEPTION("collectingProcess has to listen on one address!");
	if (listener->getProtocolType() != "UDP")
		THROWEXCEPTION("collectingProcess can handle only UPD!");
	
	msg(MSG_INFO, "CollectorConfiguration: Successfully parsed collectingProcess section");
}

IpfixCollectorCfg::~IpfixCollectorCfg()
{
}

IpfixCollectorCfg* IpfixCollectorCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new IpfixCollectorCfg(elem);
}

IpfixCollector* IpfixCollectorCfg::createInstance()
{
	IpfixReceiver* ipfixReceiver = new IpfixReceiverUdpIpV4(listener->getPort());	
	if (!ipfixReceiver) {
		THROWEXCEPTION("Could not create IpfixReceiver");
	}

	instance = new IpfixCollector(ipfixReceiver);
	return instance;
}

bool IpfixCollectorCfg::deriveFrom(IpfixCollectorCfg* old)
{
	/*
	 * The template handling in IpfixCollector/IpfixParser must be reworked before implementing
	 * this function, because the templates must all be invalidated on preReConfigure1().
	 * Invalid templates must be removed in preReconfigure2() and the new templates
	 * must be transmited on preConnect()
	 */
	return false;   // FIXME: implement it, to gain performance increase in reconnect
}
