#ifndef INFOELEMENTCFG_H_
#define INFOELEMENTCFG_H_

#include "core/Cfg.h"
#include "common/ipfixlolib/ipfix_names.h"


class InfoElementCfg
	: public CfgBase {
public:
	InfoElementCfg(XMLElement* _elem)
		: CfgBase(_elem), ieLength(0), ieId(0), knownIE(false)

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

		if (ieId>0) {
			// check given ieID 
			const ipfix_identifier* ipfixid = ipfix_id_lookup(ieId, enterpriseNumber);
			if (ipfixid)
				knownIE = true;
			else
				msg(MSG_DIALOG, "InfoElementCfg: unknown information element id %u, try to continue anyway.", (unsigned) ieId);
		}

		if (ieId==0 && ieName.size()>0) {
			// get ieId and enterpriseNumber from name
			const ipfix_identifier* ipfixid = ipfix_name_lookup(ieName.c_str());
			if (ipfixid) {
				if (enterpriseNumber == 0 && ipfixid->pen != 0) {
					enterpriseNumber = ipfixid->pen; 
					msg(MSG_DIALOG, "InfoElementCfg: %s configured without enterprise number, assume enterprise number %lu.", ieName.c_str(), (unsigned long) enterpriseNumber);
				} else if (enterpriseNumber != ipfixid->pen) {
					THROWEXCEPTION("InfoElementCfg: given information element %s is configured with enterprise number %lu, but %lu is expected.", ieName.c_str(), (unsigned long) enterpriseNumber, (unsigned long) ipfixid->pen);
				}
				knownIE = true;
				ieId = ipfixid->id;
			}
			else
				THROWEXCEPTION("InfoElementCfg: unknown information element name %s", ieName.c_str());
		}

		if (ieId>0 && ieName.size()>0) {
			// compare given ieID with name
			const ipfix_identifier* ipfixid = ipfix_name_lookup(ieName.c_str());
			if (ipfixid->id!=ieId || ipfixid->pen!=enterpriseNumber)
				THROWEXCEPTION("InfoElementCfg: given information element type id does not fit to given name (%u, %s)", (unsigned) ieId, ieName.c_str());
		}

		if (ieLength==0 && knownIE) {
			const ipfix_identifier* ipfixid = ipfix_id_lookup(ieId, enterpriseNumber);
			if (ipfixid) ieLength = ipfixid->length;
		}
	}

	std::string getName() { return "IE"; }

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

	bool isKnownIE() { return knownIE; }

private:
	std::string ieName;
	uint16_t ieLength;
	int16_t ieId;

	uint32_t enterpriseNumber;
	std::string match;
	std::string modifier;

	bool knownIE;
};

#endif /*INFOELEMENTCFG_H_*/
