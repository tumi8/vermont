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

#include "IpfixPayloadWriterCfg.h"

#include <sys/stat.h>

IpfixPayloadWriterCfg* IpfixPayloadWriterCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixPayloadWriterCfg(e);
}

IpfixPayloadWriterCfg::IpfixPayloadWriterCfg(XMLElement* elem)
    : CfgHelper<IpfixPayloadWriter, IpfixPayloadWriterCfg>(elem, "ipfixPayloadWriter"),
      noConnections(0),
      ignoreEmptyPayload(false),
      ignoreIncompleteTCP(false),
      startIdx(0)
{
    if (!elem) return;

    bool noconnseen = false;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("destPath")) {
			path = e->getFirstText();
		} else if (e->matches("filenamePrefix")) {
			filenamePrefix = e->getFirstText();
		} else if (e->matches("connNumber")) {
			noConnections = getInt("connNumber");
			noconnseen = true;
		} else if (e->matches("ignoreEmptyPayload")) {
			ignoreEmptyPayload = getBool("ignoreEmptyPayload");
		} else if (e->matches("ignoreIncompleteTCP")) {
			ignoreIncompleteTCP = getInt("ignoreIncompleteTCP");
		} else if (e->matches("ignoreIncompleteTCP")) {
			ignoreIncompleteTCP = getInt("ignoreIncompleteTCP");
		} else if (e->matches("startIndex")) {
			startIdx = getInt64("startIndex");
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown IpfixPayloadWriter config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (path=="") THROWEXCEPTION("IpfixPayloadWriterCfg: destPath not set in configuration!");
	if (filenamePrefix=="") THROWEXCEPTION("IpfixPayloadWriterCfg: filenamePrefix not set in configuration!");
	if (!noconnseen) THROWEXCEPTION("IpfixPayloadWriterCfg: connNumber not set in configuration!");

	struct stat s;
	if (stat(path.c_str(), &s) != 0)
		THROWEXCEPTION("IpfixPayloadWriterCfg: failed to access destination path '%s', error: %s", path.c_str(), strerror(errno));
}

IpfixPayloadWriterCfg::~IpfixPayloadWriterCfg()
{
}

IpfixPayloadWriter* IpfixPayloadWriterCfg::createInstance()
{
    instance = new IpfixPayloadWriter(path, filenamePrefix, noConnections, ignoreEmptyPayload, ignoreIncompleteTCP, startIdx);
    return instance;
}

bool IpfixPayloadWriterCfg::deriveFrom(IpfixPayloadWriterCfg* old)
{
    return false;
}
