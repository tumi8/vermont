/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2009-2012 Vermont Project
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

#include "common/msg.h"
#include "core/XMLElement.h"

#include "IpfixFileWriterCfg.hpp"

#include <string>
#include <vector>
#include <cassert>

IpfixFileWriterCfg* IpfixFileWriterCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new IpfixFileWriterCfg(e);
}

IpfixFileWriterCfg::IpfixFileWriterCfg(XMLElement* elem)
	: CfgHelper<IpfixFileWriter, IpfixFileWriterCfg>(elem, "ipfixFileWriter"),
	destinationPath("./"),
	filenamePrefix("ipfix.dump"),
	maximumFilesize(DEFAULTFILESIZE),
	observationDomainId(0)
{
	if (!elem) return;  // needed because of table inside ConfigManager

	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("maximumFilesize")) {
			maximumFilesize = getInt("maximumFilesize"); 
		}else if (e->matches("destinationPath")){
			destinationPath = e->getFirstText();
		}else if (e->matches("filenamePrefix")){
			filenamePrefix = e->getFirstText();
		} else if (e->matches("observationDomainId")) {
			observationDomainId = getInt("observationDomainId");
		}
		 else {
			msg(MSG_FATAL, "Unknown ipfixFileWriter config statement %s\n",
				 e->getName().c_str());
			continue;
		}
	}
}

IpfixFileWriterCfg::~IpfixFileWriterCfg()
{

}

IpfixFileWriter* IpfixFileWriterCfg::createInstance()
{
	instance = new IpfixFileWriter(observationDomainId, 
			filenamePrefix, destinationPath, maximumFilesize);
	return instance;
}

bool IpfixFileWriterCfg::deriveFrom(IpfixFileWriterCfg* old)
{
	if (maximumFilesize != old->maximumFilesize ||
	    destinationPath != old->destinationPath ||
	    filenamePrefix != old->filenamePrefix
	    ) return false;
		
	return true;
}

