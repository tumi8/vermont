#ifndef INFOELEMENTCFG_H_
#define INFOELEMENTCFG_H_

#include "core/Cfg.h"
#include "common/ipfixlolib/ipfix_names.h"


class InfoElementCfg
	: public CfgBase {
public:
	InfoElementCfg(XMLElement* _elem)
		: CfgBase(_elem), ieLength(0), ieId(0)

	{
		if (!_elem)
			return;

		ieLength = getInt("ieLength", 0);
		ieId     = getInt("ieId", 0);
		enterpriseNumber = getUInt32("enterpriseNumber", 0);

		try {
			ieName = get("ieName");
			std::transform(ieName.begin(), ieName.end(), ieName.begin(), (int(*)(int))std::tolower);
		} catch (IllegalEntry ie) { /* ignore missing ieName */ }

		modifier         = getOptional("modifier");
		match            = getOptional("match");

		if (ieId>0 && ieName.size()>0) {
			// check given ieID with name
			const ipfix_identifier* ipfixid = ipfix_name_lookup(ieName.c_str());
			if (ipfixid) {
				if (ipfixid->id!=ieId || ipfixid->pen!=enterpriseNumber)
					THROWEXCEPTION("InfoElementCfg: given information element type id does not fit to given name (%hu, %s)", ieId, ieName.c_str());
			} else {
				msg(MSG_INFO, "InfoElementCfg: unknown information element %s", ieName.c_str());
			}
		}

		if (ieLength==0) {
			if (ieId>0) {
				const ipfix_identifier* ipfixid = ipfix_id_lookup(ieId, enterpriseNumber);
				if (ipfixid) ieLength = ipfixid->length;
			} else if (ieName.size()>0) {
				const ipfix_identifier* ipfixid = ipfix_name_lookup(ieName.c_str());
				if (ipfixid) ieLength = ipfixid->length;
			}
		}
	}

	std::string getName() { return "reportedIE"; }

	bool hasOptionalLength() const { return ieLength != -1; }

	std::string getIeName() {
		if (ieName.size()==0 && ieId>=0) {
			const ipfix_identifier* ipfixid = ipfix_name_lookup(ieName.c_str());
			ieName = ipfixid->name;
		}
		return ieName;
	}

	unsigned getIeLength() const { return ieLength; }

	uint16_t getIeId() const { return (ieId == -1) ? ipfix_name_lookup(ieName.c_str())->id : ieId; }

	uint32_t getEnterpriseNumber() { return enterpriseNumber; }

	std::string getMatch() { return match; }

	std::string getModifier() { return modifier; }
private:
	std::string ieName;
	uint16_t ieLength;
	int16_t ieId;

	uint32_t enterpriseNumber;
	std::string match;
	std::string modifier;
};

#endif /*INFOELEMENTCFG_H_*/
