#ifndef INFOELEMENTCFG_H_
#define INFOELEMENTCFG_H_

#include "Cfg.h"
#include "ipfix_names.h"

class InfoElementCfg
	: public Cfg {
public:
	InfoElementCfg(XMLElement* _elem)
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
		InfoElementCfg* cfg = dynamic_cast<InfoElementCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketCountFilter");
		return false;
	}

	virtual bool deriveFrom(InfoElementCfg* old)
	{
		return true; // FIXME: implement
	}
	
	bool hasOptionalLength() const { return ieLength != -1; }

	std::string getIeName() const { return ieName; }
	
	unsigned getIeLength() const { return ieLength; }

	unsigned getIeId() const { return (ieId == -1) ? ipfix_name_lookup(ieName.c_str()) : ieId; }

	unsigned get_enterpriseNumber() const { return 0; /* FIXME */ };
	
private:
	std::string ieName;
	int ieLength;
	int ieId;
};

#endif /*INFOELEMENTCFG_H_*/
