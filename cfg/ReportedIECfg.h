#ifndef REPORTEDIECFG_H_
#define REPORTEDIECFG_H_

#include "Cfg.h"
#include <string>

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
		} catch(IllegalEntry ie) {
			THROWEXCEPTION("Missing 'ieName' in the config");
		}
	}
	
	std::string getName() const { return "reportedIE"; }
	
	virtual bool deriveFrom(Cfg* old)	{
		ReportedIECfg* cfg = dynamic_cast<ReportedIECfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketCountFilter");
		return false;
	}

	virtual bool deriveFrom(ReportedIECfg* old)
	{
		return true;
	}
	
	bool hasOptionalLength() const { return ieLength != -1; }

	std::string get_ieName() const { return ieName; }
	
	unsigned getLength() const { return ieLength; }

//	unsigned get_ieId() const { return (ieId == -1) ? ipfix_name_lookup(ieName.c_str()) : ieId; }

private:
	std::string ieName;
	int ieLength;
	int ieId;
};


#endif /*REPORTEDIECFG_H_*/
