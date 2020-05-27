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

#include "IpfixSamplerCfg.h"


IpfixSamplerCfg* IpfixSamplerCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new IpfixSamplerCfg(e);
}

IpfixSamplerCfg::IpfixSamplerCfg(XMLElement* elem)
    : CfgHelper<IpfixSampler, IpfixSamplerCfg>(elem, "ipfixSampler"),
    flowRate(1)
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("flowrate")) {
			flowRate = getDouble("flowrate");
			if (flowRate <= 0 || flowRate > 1){
				THROWEXCEPTION("Illegal value for flowRate. Must be greater 0 and smaller or equal to 1, received %lf", flowRate);
			}
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(LOG_CRIT, "Unknown IpfixSampler config statement %s\n", e->getName().c_str());
			continue;
		}
	}
}

IpfixSamplerCfg::~IpfixSamplerCfg()
{
}

IpfixSampler* IpfixSamplerCfg::createInstance()
{
    instance = new IpfixSampler(flowRate);
    return instance;
}

bool IpfixSamplerCfg::deriveFrom(IpfixSamplerCfg* old)
{
    return false;
}
