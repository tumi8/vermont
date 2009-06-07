#ifndef REPORTEDIECFG_H_
#define REPORTEDIECFG_H_

#include "core/Cfg.h"

#include <string>

// forward declaration
class InfoElementCfg;
class Template;

class PacketReportingCfg
	: public CfgBase
{
public:
	PacketReportingCfg(XMLElement* elem);
	~PacketReportingCfg();

	Template* getTemplate();

	uint16_t getRecordLength();

	uint16_t getRecordsVariableLen();

private:
        uint16_t recordVLFields;
        uint16_t recordLength;

	unsigned templateId;
	std::vector<InfoElementCfg*> exportedFields;
	Template* t;
};

#endif /*REPORTEDIECFG_H_*/
