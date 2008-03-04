#include "IpfixExporterCfg.h"

IpfixExporterCfg::IpfixExporterCfg(XMLElement* elem)
	: CfgHelper<IpfixSender, IpfixExporterCfg>(elem, "ipfixExporter"),
	templateRefreshTime(0), templateRefreshRate(0),
	maxPacketSize(0), exportDelay(0)
{

	if (!elem) {
		return;
	}

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("ipfixPacketRestrictions")) {
			msg(MSG_ERROR, "Exporter: maxPacketSize and/or exportDelay not yet supported by IpfixSender. Ignored.");
#if 0			// not supported yet
			maxPacketSize = (uint16_t)getInt("maxPacketSize", 0, e);
			try {
				exportDelay = getTimeInUnit("maxExportDelay", mSEC, 0, e);
			} catch (IllegalEntry ie) { /* ignore if not set */ }
#endif
		} else if (e->matches("udpTemplateManagement")) {
			msg(MSG_ERROR, "Exporter: Configuration of templateRefreshRate/Time not yet supported..");
#if 0			// not supported yet
			templateRefreshTime = getInt("templateRefreshTimeout", 0, e);
			templateRefreshRate = getInt("templateRefreshRate", 0, e);
#endif
		} else if (e->matches("collector")) {
			collectors.push_back(new CollectorCfg(e));
		} else {
			THROWEXCEPTION("Illegal PSAMPExporter config entry \"%s\" found",
					e->getName().c_str());
		}
	}
}

IpfixExporterCfg::~IpfixExporterCfg()
{
	for(size_t i = 0; i < collectors.size(); i++)
		delete collectors[i];
}

IpfixSender* IpfixExporterCfg::createInstance()
{
	instance = new IpfixSender(0); // FIXME: observationDomainId

	for (unsigned i = 0; i != collectors.size(); ++i) {
		msg(MSG_DEBUG, "IpfixExporter: adding collector %s:%d",
				collectors[i]->getIpAddress().c_str(),
				collectors[i]->getPort());
		instance->addCollector(collectors[i]->getIpAddress().c_str(),
				       collectors[i]->getPort());
	}

	return instance;
}

IpfixExporterCfg* IpfixExporterCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new IpfixExporterCfg(elem);
}

bool IpfixExporterCfg::deriveFrom(IpfixExporterCfg* other)
{
	return equalTo(other);
}

bool IpfixExporterCfg::equalTo(IpfixExporterCfg* other)
{
	if (maxPacketSize != other->maxPacketSize) return false;
	if (exportDelay != other->exportDelay) return false;
	if (templateRefreshTime != other->templateRefreshTime) return false;
	if (templateRefreshRate != other->templateRefreshRate) return false;
	if (collectors.size() != other->collectors.size()) return false;
	std::vector<CollectorCfg*>::const_iterator iter = collectors.begin();
	while (iter != collectors.end()) {
		std::vector<CollectorCfg*>::const_iterator biter = other->collectors.begin();
		bool found = false;
		while (biter != collectors.end()) {
			if ((*iter)->equalTo(*biter)) {
				found = true;
				break;
			}
			biter++;
		}
		if (!found) return false;
		iter++;
	}
	
	return true;
}
