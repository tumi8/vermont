#include "IpfixPrinterCfg.h"

IpfixPrinterCfg::IpfixPrinterCfg(XMLElement* elem)
	: CfgHelper<IpfixPrinter, IpfixPrinterCfg>(elem, "ipfixPrinter")
{
	if (!elem)
		return;
		
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
	instance = new IpfixPrinter();
	return instance;
}

bool IpfixPrinterCfg::deriveFrom(IpfixPrinterCfg* old)
{
	return true;
}

