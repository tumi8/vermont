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

#include "core/InfoElementCfg.h"
#include "modules/packet/Template.h"
#include "PacketReportingCfg.h"

#include <cassert>


PacketReportingCfg::PacketReportingCfg(XMLElement* elem)
	: CfgBase(elem), recordVLFields(0), recordLength(0), templateId(0), t(NULL)
{
	assert(elem);

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		if ((*it)->getName()=="templateId") {
			templateId = getInt("templateId", 0, elem);
		} else {
			exportedFields.push_back(new InfoElementCfg(*it));
		}
	}
}

PacketReportingCfg::~PacketReportingCfg()
{
	for (size_t i = 0; i < exportedFields.size(); i++)
		delete exportedFields[i];
	delete t;
}

Template* PacketReportingCfg::getTemplate()
{
        msg(MSG_DEBUG, "Creating template");
        if (t)
        	return t;
        t = new Template(templateId);

	InformationElement::IeInfo ie; 

        for (size_t i = 0; i != exportedFields.size(); ++i) {
                ie.id = exportedFields[i]->getIeId();
                ie.enterprise = exportedFields[i]->getEnterpriseNumber();
                ie.length = exportedFields[i]->getIeLength();

                if (!exportedFields[i]->isKnownIE()) {
                        msg(MSG_DIALOG, "IE %s will be ignored by PSAMP exporter.", ie.toString().c_str());
                        continue;
                }

                if (ie.length == 65535)
                	recordVLFields++;
                else
                	recordLength += ie.length;

                msg(MSG_INFO, "Template: Add field %s", ie.toString().c_str());

                t->addField(ie);
        }
        msg(MSG_DEBUG, "Template: got %d fields, record length is %u +%u * capture_len",
        		t->getFieldCount(), recordLength,recordVLFields);

        return t;
}

uint16_t PacketReportingCfg::getRecordLength()
{
	return recordLength;
}

uint16_t PacketReportingCfg::getRecordsVariableLen()
{
	return recordVLFields;
}
