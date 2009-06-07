#ifndef INFOELEMENTCFG_H_
#define INFOELEMENTCFG_H_

#include "Cfg.h"
#include "common/ipfixlolib/ipfix_names.h"

class InfoElementCfg
	: public CfgBase {
public:
	InfoElementCfg(XMLElement* _elem)
		: CfgBase(_elem), ieLength(-1), ieId(-1)

	{
		if (!_elem)
			return;

		ieLength = getInt("ieLength", -1);
		ieId     = getInt("ieId", -1);

		try {
			ieName = get("ieName");
			std::transform(ieName.begin(), ieName.end(), ieName.begin(), (int(*)(int))std::tolower);
		} catch (IllegalEntry ie) { /* ingore missing ieName */ }

		modifier         = getOptional("modifier");
		enterpriseNumber = getOptional("enterpriseNumber");
		match            = getOptional("match");
	}

	std::string getName() { return "reportedIE"; }

	bool hasOptionalLength() const { return ieLength != -1; }

	std::string getIeName() const { return ieName; }

	unsigned getIeLength() const { return ieLength; }

	unsigned getIeId() const { return (ieId == -1) ? ipfix_name_lookup(ieName.c_str()) : ieId; }

	std::string getEnterpriseNumber() { return enterpriseNumber; }

	std::string getMatch() { return match; }

	std::string getModifier() { return modifier; }
private:
	std::string ieName;
	int ieLength;
	int ieId;

	std::string enterpriseNumber;
	std::string match;
	std::string modifier;
};

#endif /*INFOELEMENTCFG_H_*/
