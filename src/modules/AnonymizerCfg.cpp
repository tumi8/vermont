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

#include "AnonymizerCfg.h"
#include "core/InfoElementCfg.h"

#include <common/ipfixlolib/ipfix_names.h>
#include <common/anon/CrpytoPanInfoElements.h>

AnonymizerCfg* AnonymizerCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new AnonymizerCfg(e);
}

AnonymizerCfg::AnonymizerCfg(XMLElement* elem)
	: CfgHelper<IpfixRecordAnonymizer, AnonymizerCfg>(elem, "anonRecord")
{
}

AnonymizerCfg::~AnonymizerCfg()
{

}

IpfixRecordAnonymizer* AnonymizerCfg::createInstance()
{
	if (!instance) {
		instance = new IpfixRecordAnonymizer();
	}

	initInstance(this, instance, _elem->getElementChildren());
	instance->setCopyMode(getBool("copyMode", false));

	return instance;
}

bool AnonymizerCfg::deriveFrom(AnonymizerCfg* old)
{
	return true;
}

void AnonymizerCfg::initInstance(CfgBase* c, AnonModule* module, XMLNode::XMLSet<XMLElement*> set)
{
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("anonField")) {
			InfoElementCfg* cfg = NULL;
			std::string method;
			std::string method_parameter;
            std::vector<map_info> mapping;

			XMLNode::XMLSet<XMLElement*> set = e->getElementChildren();
			for (XMLNode::XMLSet<XMLElement*>::iterator jt = set.begin();
			     jt != set.end();
			     ++jt) {
				XMLElement* e = *jt;
				if (e->matches("anonIE")) {
					if(cfg) {
						THROWEXCEPTION("Only on anonIE tag per anonField tag allowed");
					}
					cfg = new InfoElementCfg(*jt);
				} else if (e->matches("anonMethod")) {
					method = c->get("anonMethod", e);
				} else if (e->matches("anonParam")) {
					method_parameter = c->get("anonParam", e);
				} else if (e->matches("mappingList")) {
                    XMLNode::XMLSet<XMLElement*> set = e->getElementChildren();
                    std::vector<std::string> from;
                    std::vector<std::string> to;
                    std::vector<std::string> cidr;
                    for (XMLNode::XMLSet<XMLElement*>::iterator kt = set.begin();
                            kt != set.end();
                            ++kt) {
                        XMLElement* e = *kt;
                        if (e->matches("subnet")) {
                            XMLAttribute* a = e->getAttribute("cidr");
                            if (!a)
                                THROWEXCEPTION("No CIDR specified!");
                            cidr.push_back(a->getValue());

                            XMLNode::XMLSet<XMLElement*> set = e->getElementChildren();
                            bool have_from, have_to;
                            have_from = have_to = false;
                            int __count = 0;
                            for (XMLNode::XMLSet<XMLElement*>::iterator lt = set.begin();
                                    lt != set.end();
                                    ++lt) {
                                XMLElement* e = *lt;
                                if(e->matches("from")){
                                    from.push_back(c->get("from", e));
                                    have_from = true;
                                }else if(e->matches("to")){
                                    to.push_back(c->get("to", e));
                                    have_to = true;
                                }
                                __count++;

                            }
                            if(__count != 2 || !have_from || ! have_to)
                                THROWEXCEPTION("Invalid subnet configuration");
                            
                        }
                    }
                    if (from.size() != to.size() || to.size() != cidr.size())
                        THROWEXCEPTION("Invalid Configuration for cryptoPanMapping");
                    for (uint32_t i=0; i<to.size(); i++) {
                        map_info tmp;
                        tmp.fromNet = from[i];
                        tmp.toNet = to[i];
                        tmp.cidr = cidr[i];
                        mapping.push_back(tmp);
                    }

                } else {
					msg(MSG_ERROR, "Unknown field in anonField");
					continue;
				}
			}
			if (!cfg) {
				THROWEXCEPTION("Missing information element in anonField");
			}
			if (method.empty()) {
				THROWEXCEPTION("Missing anonymization method in anonField");
			}
			if (cfg->getIeLength()==0) THROWEXCEPTION("Information element specified in anonField, but length==0");
			module->addAnonymization(InformationElement::IeInfo(cfg->getIeId(), cfg->getEnterpriseNumber()), cfg->getIeLength(), AnonMethod::stringToMethod(method), mapping, method_parameter);
			const ipfix_identifier* id = ipfix_id_lookup(cfg->getIeId(), cfg->getEnterpriseNumber());
			msg(MSG_INFO, "Added anonymization %s for field %i (%s) with length %i", method.c_str(), cfg->getIeId(), id->name, cfg->getIeLength());
			delete cfg;
		} else if (e->matches("next") || e->matches("copyMode")) {
			// ignore next and copyMode (see createInstance)
		} else {
			msg(MSG_FATAL, "Unkown anonymization field %s\n", e->getName().c_str());
			continue;
		}
	}


}

