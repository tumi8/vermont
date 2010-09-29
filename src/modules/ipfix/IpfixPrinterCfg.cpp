/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2009 Vermont Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

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

