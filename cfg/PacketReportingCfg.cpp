#include "InfoElementCfg.h"
#include "Template.h"
#include "PacketReportingCfg.h"

#include <cassert>


PacketReportingCfg::PacketReportingCfg(XMLElement* elem)
	: Cfg(elem), recordVLFields(0), recordLength(0), templateId(0), t(NULL)
{
	assert(elem);

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		exportedFields.push_back(new InfoElementCfg(*it));
	}
}

PacketReportingCfg::~PacketReportingCfg()
{
	for (size_t i = 0; i < exportedFields.size(); i++)
		delete exportedFields[i];
	delete t;
}

bool PacketReportingCfg::deriveFrom(Cfg* old)
{
	return false;
	PacketReportingCfg* cfg = dynamic_cast<PacketReportingCfg*>(old);
	if (cfg)
		return deriveFrom(cfg);

	THROWEXCEPTION("Can't derive from %s", getName().c_str());
	return false;
}

bool PacketReportingCfg::deriveFrom(PacketReportingCfg* old)
{
	return false; // FIXME: implement
}

Template* PacketReportingCfg::getTemplate()
{
        msg(MSG_DEBUG, "Creating template");
        if (t)
        	return t;
        t = new Template(templateId);
        for (size_t i = 0; i != exportedFields.size(); ++i) {
                int tmpId = exportedFields[i]->getIeId();
                if (!ipfix_id_rangecheck(tmpId)) {
                        msg(MSG_ERROR, "Template: ignoring template field %s -> %d - rangecheck not ok",
                        		exportedFields[i]->getName().c_str(), tmpId);
                        continue;
                }
                
                const ipfix_identifier *id = ipfix_id_lookup(tmpId);
                if ((tmpId == -1) || (id == NULL)) {
                        msg(MSG_ERROR, "Template: ignoring unknown template field %s",
                        		exportedFields[i]->getName().c_str());
                        continue;
                }
                
                uint16_t fieldLength = id->length;
                if (exportedFields[i]->hasOptionalLength()) {
                        // field length 65535 indicates variable length encoding
                        // we allow configuring a fixed length for IEs with variabel length (and length=0)
                        if ((fieldLength == 0) || (fieldLength == 65535)) {
                                fieldLength = exportedFields[i]->getIeLength();
                        } else {
                                msg(MSG_ERROR, "Template: this is not a variable length field, ignoring optional length");
                        }
                }
                if (fieldLength == 65535)
                	recordVLFields++;
                else
                	recordLength += fieldLength;

                msg(MSG_FATAL, "Template: adding %s -> ID %d with size %d",
                		exportedFields[i]->getName().c_str(), id->id, fieldLength);
                
                t->addField((uint16_t)id->id, fieldLength);
        }
        msg(MSG_FATAL, "Template: got %d fields, record length is %u+%u*capture_len",
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
