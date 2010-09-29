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

#include "common/msg.h"
#include "core/XMLElement.h"

#include "IpfixCsExporterCfg.hpp"

#include <string>
#include <vector>
#include <cassert>

IpfixCsExporterCfg* IpfixCsExporterCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new IpfixCsExporterCfg(e);
}

//TODO: check default values
IpfixCsExporterCfg::IpfixCsExporterCfg(XMLElement* elem)
	: CfgHelper<IpfixCsExporter, IpfixCsExporterCfg>(elem, "ipfixCsExporter"),
	filenamePrefix("carmentis_"),
	destinationPath("./"),
	maxFileSize(DEFAULTFILESIZE),
	maxChunkBufferTime(300),
	maxChunkBufferRecords(50000),
	maxFileCreationInterval(1500),
	exportMode(0)
{
	if (!elem) return;  // needed because of table inside ConfigManager

	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("maxFileSize")) {
			maxFileSize = getInt("maxFileSize"); 
		} else if (e->matches("destinationPath")){
			destinationPath = e->getFirstText();
		} else if (e->matches("filenamePrefix")){
			filenamePrefix = e->getFirstText();
		} else if (e->matches("maxChunkBufferTime")){
			maxChunkBufferTime = atoi(e->getFirstText().c_str());
		} else if (e->matches("maxChunkBufferRecords")){
			maxChunkBufferRecords = atoi(e->getFirstText().c_str());
		} else if (e->matches("maxFileCreationInterval")){
			maxFileCreationInterval = atoi(e->getFirstText().c_str());
		} else if (e->matches("exportMode")){
			exportMode = atoi(e->getFirstText().c_str());
			if(exportMode != 0 && exportMode != 1 && exportMode != 2) {
				msg(MSG_FATAL, "Unknown ipfixCsExporter-exportMode config value %i\n",exportMode);
	                        continue;
			}
		} else {
			msg(MSG_FATAL, "Unknown ipfixCsExporter config statement %s\n",
				 e->getName().c_str());
			continue;
		}
	}
}

IpfixCsExporterCfg::~IpfixCsExporterCfg()
{

}

IpfixCsExporter* IpfixCsExporterCfg::createInstance()
{
	instance = new IpfixCsExporter(filenamePrefix, destinationPath, maxFileSize,
					maxChunkBufferTime, maxChunkBufferRecords,
					maxFileCreationInterval, exportMode);
	return instance;
}

bool IpfixCsExporterCfg::deriveFrom(IpfixCsExporterCfg* old)
{
	if (maxFileSize != old->maxFileSize ||
	    destinationPath != old->destinationPath ||
	    filenamePrefix != old->filenamePrefix ||
	    maxChunkBufferTime != old-> maxChunkBufferTime ||
            maxChunkBufferRecords != old-> maxChunkBufferRecords ||
            maxFileCreationInterval != old-> maxFileCreationInterval ||
            exportMode != old->exportMode
      	    ) return false;
		
	return true;
}
