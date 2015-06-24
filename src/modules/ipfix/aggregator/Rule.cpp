/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
 * Copyright (C) 2014 Oliver Gasser
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

/*
 * Internal representation of an IP address: up to 5 bytes: 192.168.0.201/24 ==> 192 168 0 201 8(!)
 * Internal representation of a Port range: 2n bytes: 80,443 ==> 80 80 443 443
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "Rule.hpp"
#include "PacketHashtable.h"
#include "common/ipfixlolib/ipfix.h"

#include "common/msg.h"

#define MAX_LINE_LEN 256

/* --- constants ------------*/

/* --- functions ------------*/

Rule::Rule()
	: id(0), preceding(0), fieldCount(0), biflowAggregation(0), hashtable(0), patternFields(0), patternFieldsLen(0)
{
}

/**
 * De-allocates memory used by the given rule.
 * This will NOT destroy hashtables associated with the rule
 */
Rule::~Rule() {
	int i;
	for (i = 0; i < fieldCount; i++) {
		delete field[i];
	}
	if (patternFields)
		delete [] patternFields;
}

/**
 * initialization function which builds some helper structures for optimization inside
 * the express aggregator
 */
void Rule::initialize()
{
	// determine which protocols are valid for this template
	validProtocols = Packet::NONE;
	for (int i=0; i<fieldCount; i++) {
		Rule::Field* f = field[i];
		if (f->type.enterprise == 0 && f->type.id == IPFIX_TYPEID_protocolIdentifier) {
			// small exception: if protocol id is inside the template, we assume that all types of protocols are valid
			validProtocols = Packet::ALL;
			msg(MSG_INFO, "IPFIX IE protocolIdentifier is contained in template %hu, accepting all protocol types for this template", id);
		}
		validProtocols = Packet::IPProtocolType(validProtocols | f->type.getValidProtocols());
	}
	if (validProtocols == Packet::NONE) {
		THROWEXCEPTION("received unknown field type, no valid protocol match");
	}

	DPRINTF("valid protocols for this template: %02X", validProtocols);

	// write all rules containing a pattern to be matched for in array
	patternFields = new Rule::Field*[fieldCount];
	patternFieldsLen = 0;
	for (int i=0; i<fieldCount; i++) {
		Rule::Field* f = field[i];
		if (f->pattern) patternFields[patternFieldsLen++] = f;
	}
}

const char* modifier2string(Rule::Field::Modifier i) {
	static char s[16];
	if (i == Rule::Field::DISCARD) return "discard";
	if (i == Rule::Field::KEEP) return "keep";
	if (i == Rule::Field::AGGREGATE) return "aggregate";
	if ((i >= Rule::Field::MASK_START) && (i <= Rule::Field::MASK_END)) {
		snprintf(s, ARRAY_SIZE(s), "mask/%d", i - Rule::Field::MASK_START);
		return s;
	}
	return "unknownModifier";
}

/**
 * Prints a textual representation of the rule to stdout
 */
void Rule::print() {
	int i;

	printf("Aggregate %d %d\n",id,preceding);
	for (i=0; i < fieldCount; i++) {
		printf("\t");
		const char* modifier = modifier2string(field[i]->modifier);
		if (modifier != NULL) {
			printf("%s ", modifier);
		} else {
			printf("unknownModifier ");
		}

		printf("%s ", field[i]->type.toString().c_str());

		if (field[i]->pattern != NULL) {
			printf("in ");
			printFieldData(field[i]->type, field[i]->pattern);
		}
		printf("\n");
	}
	printf("\n");
}

/**
 * @returns amount of bits used for host identification part of ip address
 * (in contrast to subnet identification part)
 */
uint8_t getIPv4IMask(const InformationElement::IeInfo* type, const IpfixRecord::Data* data)
{
	// sometimes there is a fifth byte after the ip address inside the ipfix flow which specifies
	// the amout of bits used for host identification part of ip address
	if (type->length > 4) return data[4];

	if (type->length == 4) return 0;
	if (type->length == 3) return 8;
	if (type->length == 2) return 16;
	if (type->length == 1) return 24;
	if (type->length == 0) return 32;

	msg(MSG_FATAL, "Invalid IPv4 address length: %d", type->length);
	return 0;
}

uint32_t getIPv4Address(const InformationElement::IeInfo* type, const IpfixRecord::Data* data) {
	uint32_t addr = 0;
	if (type->length >= 1) addr |= data[0] << 24;
	if (type->length >= 2) addr |= data[1] << 16;
	if (type->length >= 3) addr |= data[2] << 8;
	if (type->length >= 4) addr |= data[3] << 0;
	return addr;
}

/**
 * Checks if a given "PortRanges" Field matches a "PortRanges" Pattern
 * @c return 1 if field matches
 */
int matchesPortPattern(const InformationElement::IeInfo* dataType, const IpfixRecord::Data* data, const InformationElement::IeInfo* patternType, const IpfixRecord::Data* pattern) {
	int i;
	int j;

	if ((dataType->length == 2) && (patternType->length == 2)) {
		return ((data[0] == pattern[0]) && (data[1] == pattern[1]));
	}
	if ((dataType->length == 2) && ((patternType->length % 4) == 0)) {
		int dport = (data[0] << 8) + data[1];
		int foundMatch = 0;
		for (j = 0; j < patternType->length; j+=4) {
			int pports = (pattern[j+0] << 8) + pattern[j+1];
			int pporte = (pattern[j+2] << 8) + pattern[j+3];
			if ((dport >= pports) && (dport <= pporte)) {
				foundMatch = 1;
				break;
			}
		}
		return foundMatch;
	}
	if (((dataType->length % 4) == 0) && (patternType->length == 2)) {
		for (i = 0; i < dataType->length; i+=4) {
			int dports = (data[i+0] << 8) + data[i+1];
			int dporte = (data[i+2] << 8) + data[i+3];
			int pport = (pattern[0] << 8) + pattern[1];
			if ((dports < pport) && (dporte > pport)) return 0;
		}
		return 1;
	}
	if (((dataType->length % 4) == 0) && ((patternType->length % 4) == 0)) {
		for (i = 0; i < dataType->length; i+=4) {
			int dports = (data[i+0] << 8) + data[i+1];
			int dporte = (data[i+2] << 8) + data[i+3];
			int foundMatch = 0;
			for (j = 0; j < patternType->length; j+=4) {
				int pports = (pattern[j+0] << 8) + pattern[j+1];
				int pporte = (pattern[j+2] << 8) + pattern[j+3];
				if ((dports >= pports) && (dporte <= pporte)) {
					foundMatch = 1;
					break;
				}
			}
			if (!foundMatch) return 0;
		}
		return 1;
	}

	msg(MSG_FATAL, "matching port of length %d with pattern of length %d not supported",
	    dataType->length, patternType->length);
	return 0;
}

/**
 * Checks if a given IPv4 Field matches a IPv4 Pattern
 * @c return 1 if field matches
 */
int matchesIPv4Pattern(const InformationElement::IeInfo* dataType, const IpfixRecord::Data* data, const InformationElement::IeInfo* patternType, const IpfixRecord::Data* pattern) {
	/* Get (inverse!) Network Masks */
	int dmaski = getIPv4IMask(dataType, data);
	int pmaski = getIPv4IMask(patternType, pattern);

	/* If the pattern is more specific than our data we return 0 */
	if (dmaski > pmaski) return 0;

	uint32_t daddr = getIPv4Address(dataType, data);
	uint32_t paddr = getIPv4Address(patternType, pattern);

	return ((daddr >> pmaski) == (paddr >> pmaski));
}

/**
 * Checks if a given Field matches a Pattern when compared byte for byte
 * @c return 1 if field matches
 */
int matchesRawPattern(const InformationElement::IeInfo* dataType, const IpfixRecord::Data* data, const InformationElement::IeInfo* patternType, const IpfixRecord::Data* pattern) {
	//int i;

	/* Byte-wise comparison, so lengths must be equal */
	if (dataType->length != patternType->length) return 0;

	//for (i = 0; i < dataType->length; i++) if (data[i] != pattern[i]) return 0;
	//return 1;

	return (memcmp(data, pattern, dataType->length) == 0);

}

/**
 * Checks if the IPv4Prefix fits the IP mask of the rule
 * @return true if the IPv4Prefix does not exists or if it is larger than 32-mask
 */
bool checkMask(TemplateInfo::FieldInfo* prefixInfo, IpfixRecord::Data* prefixData, Rule::Field* ruleField) {
	if (!prefixInfo) return true;
	uint8_t rulePrefix = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
	uint8_t recordPrefix = *(prefixData + prefixInfo->offset);
	return (recordPrefix >= rulePrefix);
}

/**
 * Checks if a data block matches a given pattern
 * @return 1 if pattern is matched
 */
int matchesPattern(const InformationElement::IeInfo* dataType, const IpfixRecord::Data* data, const InformationElement::IeInfo* patternType, const IpfixRecord::Data* pattern) {
	/* an inexistent pattern is always matched */
	if (pattern == NULL) return 1;

	if ((dataType->id != patternType->id) || (dataType->enterprise != patternType->enterprise)) return 0;

	switch (patternType->id) {
	case IPFIX_TYPEID_sourceIPv4Address:
	case IPFIX_TYPEID_destinationIPv4Address: {
		return matchesIPv4Pattern(dataType, data, patternType, pattern);
		break;
	}
	case IPFIX_TYPEID_sourceTransportPort:
	case IPFIX_TYPEID_destinationTransportPort:
		return matchesPortPattern(dataType, data, patternType, pattern);
		break;
	default:
		return matchesRawPattern(dataType, data, patternType, pattern);
		break;
	}
}

/**
 * Checks if a given flow matches a rule
 * only for Express version of concentrator
 * @return 1 if rule is matched, 0 otherwise
 */
bool Rule::ExptemplateDataMatches(const Packet* p)
{
#if defined (DEBUG)
	if (!patternFields) THROWEXCEPTION("patternFields not initialized yet!");
#endif

	// check if packet has correct protocol
	if ((p->ipProtocolType & validProtocols) == 0) return false;

	// check all fields containing patterns
	for (int i = 0; i<patternFieldsLen; i++) {
		Rule::Field* ruleField = patternFields[i];
		const IpfixRecord::Data* field_data = p->data.netHeader + PacketHashtable::getRawPacketFieldOffset(ruleField->type, p);

		switch (ruleField->type.id) {
			case IPFIX_TYPEID_sourceIPv4Address:
				{
					TemplateInfo::FieldInfo fi;
					fi.type.id = IPFIX_TYPEID_sourceIPv4Address;
					fi.type.length = 4;
					fi.offset = 12;

					uint8_t dmaski = 0; // no host identification part in IP adress
					int pmaski = getIPv4IMask(&ruleField->type, ruleField->pattern);

					if (dmaski > pmaski) return false;

					uint32_t daddr = getIPv4Address(&fi.type, field_data);
					uint32_t paddr = getIPv4Address(&ruleField->type, ruleField->pattern);

					if ((daddr >> pmaski) != (paddr >> pmaski))
						return false;
					break;
				}
			case IPFIX_TYPEID_destinationIPv4Address:
				{
					TemplateInfo::FieldInfo fi;
					fi.type.id = IPFIX_TYPEID_destinationIPv4Address;
					fi.type.length = 4;
					fi.offset = 16;

					int dmaski = getIPv4IMask(&fi.type, field_data);
					int pmaski = getIPv4IMask(&ruleField->type, ruleField->pattern);


					if (dmaski > pmaski) return false;

					uint32_t daddr = getIPv4Address( &fi.type, field_data);
					uint32_t paddr = getIPv4Address(&ruleField->type, ruleField->pattern);

					if ((daddr >> pmaski) != (paddr >> pmaski))
						return false;
					break;
				}
			case IPFIX_TYPEID_sourceTransportPort:
				{
					TemplateInfo::FieldInfo fi;
					fi.type.id = IPFIX_TYPEID_sourceTransportPort;
					fi.type.length = 2;
					fi.offset = 0;
					if (!matchesPortPattern(&fi.type, field_data, &ruleField->type, ruleField->pattern))
						return false;
					break;
				}
			case IPFIX_TYPEID_destinationTransportPort:
				{
					TemplateInfo::FieldInfo fi;
					fi.type.id = IPFIX_TYPEID_destinationTransportPort;
					fi.type.length = 2;
					fi.offset = 2;
					if (!matchesPortPattern(&fi.type, field_data, &ruleField->type, ruleField->pattern))
						return false;
					break;
				}
			default:
				if (!matchesRawPattern(&ruleField->type, field_data, &ruleField->type, ruleField->pattern))
					return false;
				break;
		}

	}

	/* all rule fields were matched */
	return true;
}


/**
 * Checks if a given flow matches a rule
 * @return 1 if rule is matched, 0 otherwise
 */
int Rule::dataRecordMatches(IpfixDataRecord* record) {
	int i;
	TemplateInfo::FieldInfo* recordField;
	IpfixRecord::Data* recordData = record->data;
	boost::shared_ptr<TemplateInfo> dataTemplateInfo = record->templateInfo;

	/* for all patterns of this rule, check if they are matched */
	for(i = 0; i < fieldCount; i++) {
		Rule::Field* ruleField = field[i];

		/* no check for biflow fields */
		if (ruleField->type.isReverseField())
			continue;

		if(ruleField->pattern) {

			recordField = dataTemplateInfo->getFieldInfo(ruleField->type);
			if (recordField) {
				/* corresponding data field found, check if it matches. If it doesn't the whole rule cannot be matched */
				if (!matchesPattern(&recordField->type, (recordData + recordField->offset), &ruleField->type, ruleField->pattern)) return 0;
				if ((ruleField->type.enterprise == 0) && (ruleField->type.length == 5)) {
					if (ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) {
						if(!checkMask(dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_sourceIPv4PrefixLength, 0), recordData, ruleField)) return 0;
					} else if (ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address) {
						if(!checkMask(dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_destinationIPv4PrefixLength, 0), recordData, ruleField)) return 0;
					}
				}
				continue;
			}

			/* Probably, this does not lead to the desired result:
			if (biflowAggregation) {
				// check if the rule field as a corresponding type in opposite direction
				InformationElement::IeId oppDirIeId = InformationElement::oppositeDirectionIeId(ruleField->type);
				if(oppDirIeId) {
					recordField = dataTemplateInfo->getFieldInfo(oppDirIeId, ruleField->type.enterprise);
					if (recordField) {
						// corresponding data field found, check if it matches. If it doesn't the whole rule cannot be matched
						if (!matchesPattern(&recordField->type, (recordData + recordField->offset), &ruleField->type, ruleField->pattern)) return 0;
						if ((ruleField->type.enterprise == 0) && (ruleField->type.length == 5)) {
							if (oppDirIeId == IPFIX_TYPEID_sourceIPv4Address) {
								if(!checkMask(dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_sourceIPv4PrefixLength, 0), recordData, ruleField)) return 0;
							} else if (oppDirIeId == IPFIX_TYPEID_destinationIPv4Address) {
								if(!checkMask(dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_destinationIPv4PrefixLength, 0), recordData, ruleField)) return 0;
							}
						}
						continue;
					}
				}
			}
			*/

			/*
			  in the case of Data Template, see if we find a corresponding fixed data field
			*/
			if ((dataTemplateInfo->setId == TemplateInfo::IpfixDataTemplate) && (dataTemplateInfo->dataCount > 0)) {
				recordField = dataTemplateInfo->getDataInfo(ruleField->type);
				if (recordField) {
					/* corresponding fixed data field found, check if it matches. If it doesn't the whole rule cannot be matched */
					if (!matchesPattern(&recordField->type, (dataTemplateInfo->data + recordField->offset), &ruleField->type, ruleField->pattern)) return 0;
					if ((ruleField->type.enterprise == 0) && (ruleField->type.length == 5)) {
						if (ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) {
							if(!checkMask(dataTemplateInfo->getDataInfo(IPFIX_TYPEID_sourceIPv4PrefixLength, 0), dataTemplateInfo->data, ruleField)) return 0;
						} else if (ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address) {
							if(!checkMask(dataTemplateInfo->getDataInfo(IPFIX_TYPEID_destinationIPv4PrefixLength, 0), dataTemplateInfo->data, ruleField)) return 0;
						}
					}
					continue;
				}
				/* Probably, this does not lead to the desired result:
				if (biflowAggregation) {
					// check if the rule field as a corresponding type in opposite direction
					InformationElement::IeId oppDirIeId = InformationElement::oppositeDirectionIeId(ruleField->type);
					if(oppDirIeId) {
						recordField = dataTemplateInfo->getDataInfo(oppDirIeId, ruleField->type.enterprise);
						if (recordField) {
							// corresponding data field found, check if it matches. If it doesn't the whole rule cannot be matched
							if (!matchesPattern(&recordField->type, (dataTemplateInfo->data + recordField->offset), &ruleField->type, ruleField->pattern)) return 0;
							if ((ruleField->type.enterprise == 0) && (ruleField->type.length == 5)) {
								if (oppDirIeId == IPFIX_TYPEID_sourceIPv4Address) {
									if(!checkMask(dataTemplateInfo->getDataInfo(IPFIX_TYPEID_sourceIPv4PrefixLength, 0), dataTemplateInfo->data, ruleField)) return 0;
								} else if (oppDirIeId == IPFIX_TYPEID_destinationIPv4Address) {
									if(!checkMask(dataTemplateInfo->getDataInfo(IPFIX_TYPEID_destinationIPv4PrefixLength, 0), dataTemplateInfo->data, ruleField)) return 0;
								}
							}
							continue;
						}
					}
				} */
			}

			/* no corresponding data field or fixed data field found, this flow cannot match */
			msg(MSG_VDEBUG, "No corresponding DataDataRecord field for RuleField of type %s", ruleField->type.toString().c_str());
			return 0;
		}
		/* if a non-discarding rule field specifies no pattern, check at least if the data field exists */
		else if (ruleField->modifier != Rule::Field::DISCARD) {
			recordField = dataTemplateInfo->getFieldInfo(ruleField->type);
			if (recordField) continue;

			/* in the case of biflow, we also check the reverse direction */
			if (biflowAggregation && ruleField->type.existsReverseDirection()) {
				InformationElement::IeInfo revie = ruleField->type.getReverseDirection();
				recordField = dataTemplateInfo->getFieldInfo(revie);
				if (recordField) continue;
			}

			/*
			  in the case of Data Template, see if we find a corresponding fixed data field
			*/
			if ((dataTemplateInfo->setId == TemplateInfo::IpfixDataTemplate) && (dataTemplateInfo->dataCount > 0)) {
				recordField = dataTemplateInfo->getDataInfo(ruleField->type);
				if (recordField) continue;

				/* in the case of biflow, we also check the reverse direction */
				if (biflowAggregation && ruleField->type.existsReverseDirection()) {
					InformationElement::IeInfo revie = ruleField->type.getReverseDirection();
					recordField = dataTemplateInfo->getDataInfo(revie);
					if (recordField) continue;
				}
			}

			// anonymisationType is filled by the anonymizer, so we ignore it quietly
			if (ruleField->type==InformationElement::IeInfo(IPFIX_ETYPEID_anonymisationType, IPFIX_PEN_vermont))
				continue;

			msg(MSG_INFO, "No corresponding DataRecord field for RuleField of type %s", ruleField->type.toString().c_str());
			return 0;
		}
	}

	/* all rule fields were matched */
	return 1;
}
