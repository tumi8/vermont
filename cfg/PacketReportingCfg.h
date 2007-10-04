#ifndef REPORTEDIECFG_H_
#define REPORTEDIECFG_H_

#include "Cfg.h"
#include "ipfix_names.h"

#include <string>

// forward declaration
class Template;

class ReportedIECfg
	: public Cfg {
public:
	ReportedIECfg(XMLElement* _elem)
		: Cfg(_elem), ieLength(-1), ieId(-1)

	{
		if (!_elem)
			return;
		
		ieLength = getInt("ieLength", -1);
		ieId     = getInt("ieId", -1);
	
		try {
			ieName = get("ieName");
			std::transform(ieName.begin(), ieName.end(), ieName.begin(), std::tolower);
		} catch(IllegalEntry ie) { /* ingore missing ieName */ }
	}
	
	std::string getName() { return "reportedIE"; }
	
	virtual bool deriveFrom(Cfg* old)	{
		ReportedIECfg* cfg = dynamic_cast<ReportedIECfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketCountFilter");
		return false;
	}

	virtual bool deriveFrom(ReportedIECfg* old)
	{
		return true; // FIXME: implement
	}
	
	bool hasOptionalLength() const { return ieLength != -1; }

	std::string get_ieName() const { return ieName; }
	
	unsigned getLength() const { return ieLength; }

	unsigned get_ieId() const { return (ieId == -1) ? ipfix_name_lookup(ieName.c_str()) : ieId; }

private:
	std::string ieName;
	int ieLength;
	int ieId;
};

class PacketReportingCfg
	: public Cfg
{
public:
	PacketReportingCfg(XMLElement* elem);
	~PacketReportingCfg();

	std::string getName() { return "packetReporting"; }

	virtual bool deriveFrom(Cfg* old);

	virtual bool deriveFrom(PacketReportingCfg* old);

	Template* getTemplate();

	uint16_t getRecordLength();

	uint16_t getRecordsVariableLen();

private:
        uint16_t recordVLFields;
        uint16_t recordLength;

	unsigned templateId;
	std::vector<ReportedIECfg*> exportedFields;
	Template* t;
};

#endif /*REPORTEDIECFG_H_*/
