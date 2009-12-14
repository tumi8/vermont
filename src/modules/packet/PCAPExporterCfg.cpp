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

#include "PCAPExporterCfg.h"

#include "common/defs.h"

#include <cassert>
#include <pcap.h>

PCAPExporterCfg::PCAPExporterCfg(XMLElement* elem) 
	: CfgHelper<PCAPExporterModule, PCAPExporterCfg>(elem, "pcapExporter"), link_type(DLT_EN10MB), snaplen(PCAP_MAX_CAPTURE_LENGTH)
{ 
	if (!elem) return;

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("filename")) {
			fileName = e->getFirstText();
		} else if (e->matches("linkType")) {
			int tmp =  pcap_datalink_name_to_val(e->getFirstText().c_str());
			if (tmp == -1) {
				msg(MSG_ERROR, "Found illegal link type");
			} else {
				link_type = tmp;
			}
		} else if (e->matches("snaplen")) {
			snaplen = getInt("snaplen", PCAP_MAX_CAPTURE_LENGTH, e);
		}
	}
} 

PCAPExporterCfg* PCAPExporterCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new PCAPExporterCfg(elem);
}

PCAPExporterCfg::~PCAPExporterCfg()
{
}

PCAPExporterModule* PCAPExporterCfg::createInstance()
{
	instance = new PCAPExporterModule(fileName);
	instance->setDataLinkType(link_type);
	instance->setSnaplen(snaplen);
	return instance;
}

bool PCAPExporterCfg::deriveFrom(PCAPExporterCfg* old)
{
	return false; // FIXME: implement
}
