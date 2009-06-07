/*
 * VERMONT
 * Copyright (C) 2009 Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
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
 */

#include "FrontPayloadSigMatcherCfg.h"


FrontPayloadSigMatcherCfg* FrontPayloadSigMatcherCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new FrontPayloadSigMatcherCfg(e);
}

FrontPayloadSigMatcherCfg::FrontPayloadSigMatcherCfg(XMLElement* elem)
    : CfgHelper<FrontPayloadSigMatcher, FrontPayloadSigMatcherCfg>(elem, "frontPayloadSigMatcher")
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("signaturedir")) {
			signatureDir = e->getFirstText();
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown FrontPayloadSigMatcher config statement %s\n", e->getName().c_str());
			continue;
		}
	}

	if (signatureDir=="") THROWEXCEPTION("FrontPayloadSigMatcher: did not find required configuration parameter signaturedir");
}

FrontPayloadSigMatcherCfg::~FrontPayloadSigMatcherCfg()
{
}

FrontPayloadSigMatcher* FrontPayloadSigMatcherCfg::createInstance()
{
    instance = new FrontPayloadSigMatcher(signatureDir);
    return instance;
}

bool FrontPayloadSigMatcherCfg::deriveFrom(FrontPayloadSigMatcherCfg* old)
{
    return false;
}
