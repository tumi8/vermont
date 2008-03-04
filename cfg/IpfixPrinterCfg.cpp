#include "IpfixPrinterCfg.h"

IpfixPrinterCfg::IpfixPrinterCfg(XMLElement* elem)
	: CfgHelper<IpfixPrinter, IpfixPrinterCfg>(elem, "ipfixPrinter"),
	  lineOutput(false)
{
	if (!elem)
		return;
		
	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin(); it != set.end(); it++) {
		XMLElement* e = *it;

		if (e->matches("lineOutput")) {
			lineOutput = getInt("lineOutput")>0 ? true : false;
		} else {
			msg(MSG_FATAL, "Unknown IpfixPrinter config statement %s\n", e->getName().c_str());
			continue;
		}
	}
}

IpfixPrinterCfg::~IpfixPrinterCfg()
{
}

IpfixPrinterCfg* IpfixPrinterCfg::create(XMLElement* e)
{
	return new IpfixPrinterCfg(e);
}

IpfixPrinter* IpfixPrinterCfg::createInstance()
{
	instance = new IpfixPrinter(lineOutput);
	return instance;
}

bool IpfixPrinterCfg::deriveFrom(IpfixPrinterCfg* old)
{
	return true;
}

