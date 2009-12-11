#include "IpfixPrinterCfg.h"

IpfixPrinterCfg::IpfixPrinterCfg(XMLElement* elem)
	: CfgHelper<IpfixPrinter, IpfixPrinterCfg>(elem, "ipfixPrinter"),
	  outputType(IpfixPrinter::TREE), filename("")
{
	if (!elem)
		return;

	msg(MSG_INFO, "ParserCfg: Start reading ipfixPrinter section");
	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin(); it != set.end(); it++) {
		XMLElement* e = *it;

		if (e->matches("outputType")) {
			string type = e->getFirstText();
			if (type=="line") {
				outputType = IpfixPrinter::LINE;
			} else if (type=="tree") {
				outputType = IpfixPrinter::TREE;
			} else if (type=="table") {
				outputType = IpfixPrinter::TABLE;
			} else if (type=="none") {
				outputType = IpfixPrinter::NONE;
			} else {
				THROWEXCEPTION("Unknown IpfixPrinter output type %s", type.c_str());
			}
		} else if (e->matches("filename")) {
			filename = e->getFirstText();
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
	instance = new IpfixPrinter(outputType, filename);
	return instance;
}

bool IpfixPrinterCfg::deriveFrom(IpfixPrinterCfg* old)
{
	return true;
}

