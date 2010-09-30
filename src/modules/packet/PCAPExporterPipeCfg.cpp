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

#include "PCAPExporterPipeCfg.h"

#include "common/defs.h"

#include <cassert>
#include <pcap.h>

PCAPExporterPipeCfg::PCAPExporterPipeCfg(XMLElement* elem)
	: CfgHelper<PCAPExporterPipe, PCAPExporterPipeCfg>(elem, "pcapExporterPipe"),
        link_type(DLT_EN10MB), snaplen(PCAP_MAX_CAPTURE_LENGTH), sigkilltimeout(1),
        logFileName(""),
        fifoReaderCmd(""),
        workingPath(""),
        appenddate(false),
        restart(false),
        restartInterval(0)
{
	if (!elem) return;

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("logfilebasename")) {
			logFileName = e->getFirstText();
		} else if (e->matches("linkType")) {
			int tmp =  pcap_datalink_name_to_val(e->getFirstText().c_str());
			if (tmp == -1) {
				msg(MSG_ERROR, "Found illegal link type");
			} else {
				link_type = tmp;
			}
		} else if (e->matches("snaplen")) {
			snaplen = getInt("snaplen", PCAP_MAX_CAPTURE_LENGTH, e);
		} else if (e->matches("sigkilltimeout")) {
            sigkilltimeout = getInt("sigkilltimeout", 1, e);
        } else if(e->matches("command")) {
            fifoReaderCmd = e->getFirstText();
		} else if(e->matches("workingpath")) {
            workingPath = e->getFirstText();
        } else if (e->matches("appenddate")) {
			appenddate = getBool("appenddate", false, e);
        } else if (e->matches("restartinterval")) {
			restartInterval = getInt("restartinterval", 0);
		} else if(e->matches("restartonsignal")) {
			restart = getBool("restartonsignal", false, e);
		}
	}
}

PCAPExporterPipeCfg* PCAPExporterPipeCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new PCAPExporterPipeCfg(elem);
}

PCAPExporterPipeCfg::~PCAPExporterPipeCfg()
{
}

PCAPExporterPipe* PCAPExporterPipeCfg::createInstance()
{
	instance = new PCAPExporterPipe(logFileName);
	instance->setDataLinkType(link_type);
	instance->setSnaplen(snaplen);
    instance->setSigKillTimeout(sigkilltimeout);
    instance->setPipeReaderCmd(fifoReaderCmd);
	instance->setAppendDate(appenddate);
	instance->setRestartOnSignal(restart);
	instance->setWorkingPath(workingPath);
	instance->setRestartInterval(restartInterval);
	return instance;
}

bool PCAPExporterPipeCfg::deriveFrom(PCAPExporterPipeCfg* old)
{
    if (logFileName != old->logFileName ||
        link_type != old->link_type ||
        snaplen != old->snaplen ||
        fifoReaderCmd != old->fifoReaderCmd ||
        sigkilltimeout != old->sigkilltimeout ||
		appenddate != old->appenddate ||
		restart != old->restart
        ) return false;
	return true; // FIXME: implement
}
