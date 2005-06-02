//FIXME: split source into internal and exported functions?

/** @file
 * Rule management sub-module.
 *
 * Reads in aggregation rules from a file (see @c parseRulesFromFile()),
 * checks if a flow matches a given rule (see @c templateDataMatchesRule(), @c dataTemplateDataMatchesRule())
 *
 */

/******************************************************************************

IPFIX Concentrator
Copyright (C) 2005 Christoph Sommer
http://www.deltadevelopment.de/users/christoph/ipfix

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

******************************************************************************/

/*
 * Internal representation of an IP address: up to 5 bytes: 192.168.0.201/24 ==> 192 168 0 201 8(!)
 * Internal representation of a Port range: 2n bytes: 80,443 ==> 80 80 443 443
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "rules.h"
#include "rcvIpfix.h"
#include "ipfix.h"

#include "msg.h"

#define MAX_LINE_LEN 256

/* --- constants ------------*/


/* --- functions ------------*/

static uint8_t getIPv4IMask(FieldType* type, FieldData* data);


RuleField* mallocRuleField() {
	RuleField* ruleField = (RuleField*)malloc(sizeof(RuleField));
	ruleField->type.id = 0;
	ruleField->type.length = 0;
	ruleField->type.eid = 0;
	ruleField->pattern = NULL;
	ruleField->modifier = 0;

	return ruleField;
}

void freeRuleField(RuleField* ruleField) {
	free(ruleField);
}

Rule* mallocRule() {
	Rule* rule = (Rule*)malloc(sizeof(Rule));
	rule->fieldCount = 0;

	return rule;
}

void freeRule(Rule* rule) {
	free(rule);
}

/**
 * Checks whether a given letter is found in an alphabet or not
 * @return 1 if @c letter is found
 */
static int is_in(char letter, char* alphabet) {
	int i;
	for (i = 0; i < strlen(alphabet); i++) {
		if (alphabet[i] == letter) return 1;
	}
	return 0;
}

#if 0
/* defined but not used */
/**
 * Removes whitespace at the end of a string by overwriting it with 0-bytes
 */
static void rtrim(char* text) {
	while ((*text != 0) && is_in(text[strlen(text)-1], " \n\t")) text[strlen(text)-1] = 0;
}

/**
 * Removes whitespace at the start of a string by returning a pointer with an added offset
 */
static char* ltrim(char* text) {
	while ((*text != 0) && is_in(*text, " \n\t")) ++text;
	return text;
}
#endif

/**
 * Successively tokenizes a string by inserting 0-bytes, returning pointers to the tokens one at a time.
 * Multiple delimiters between tokens are treated as one.
 * Modifies the source string!
 * @param text pointerpointer to the string to be tokenized. Its start-pointer will be shifted to reflect the current position while tokenizing
 * @param delim string of delimiters that seperate tokens
 * @return pointer to first token. Call again to retrieve next token
 */
static char* get_next_token(char** text, char* delim) {
	char* p = *text;

	if (**text == 0) return NULL;

	for (; **text != 0; ++*text) {
		if (is_in(**text, delim)) {
			**text = 0; ++*text;
			while ((**text != 0) && (is_in(**text, delim))) {
				++*text;
			}
			break;
		}
	}
	return p;
}

/**
 * parses the given string
 * @return 0 if successful
 */
static int parseModifier(char* s, FieldModifier* modifier) {
	if (strcmp(s, "discard") == 0) { *modifier = FIELD_MODIFIER_DISCARD; return 0; }
	if (strcmp(s, "keep") == 0) { *modifier = FIELD_MODIFIER_KEEP; return 0; }
	if (strcmp(s, "aggregate") == 0) { *modifier = FIELD_MODIFIER_AGGREGATE; return 0; }
	if (strncmp(s, "mask/", 5) == 0) { *modifier = (FIELD_MODIFIER_MASK_START + atoi(s+5)); return 0; }
	return -1;
}

/**
 * parses the given string
 * @return 0 if successful
 */
static int parseProtoPattern(char* s, FieldData** fdata, FieldLength* length) {
	int proto = -1;
	if (strcmp(s, "ICMP") == 0) proto = IPFIX_protocolIdentifier_ICMP;
	if (strcmp(s, "TCP") == 0) proto = IPFIX_protocolIdentifier_TCP;
	if (strcmp(s, "UDP") == 0) proto = IPFIX_protocolIdentifier_UDP;
	if (strcmp(s, "RAW") == 0) proto = IPFIX_protocolIdentifier_RAW;

	if (proto == -1) return -1;

	*length = 1;
	FieldData* fd = (FieldData*)malloc(*length);
	fd[0] = proto;
	*fdata = fd;

	return 0;
}

/**
 * parses the given string
 * @return 0 if successful
 */
static int parseIPv4Pattern(char* s, FieldData** fdata, FieldLength* length) {
	char* p = s;
	char* octet1 = get_next_token(&p, ".");
	char* octet2 = get_next_token(&p, ".");
	char* octet3 = get_next_token(&p, ".");
	char* octet4 = get_next_token(&p, "/");
	char* imask = get_next_token(&p, " ");
	if (!octet4) return -1;
	/*
	 if (imask) {
	 *length = 5;
	 } else {
	 *length = 4;
	 }
	 */
	*length = 5;

	FieldData* fd = (FieldData*)malloc(*length);
	fd[0]=atoi(octet1);
	fd[1]=atoi(octet2);
	fd[2]=atoi(octet3);
	fd[3]=atoi(octet4);
	fd[4]=0;
	if (imask) fd[4]=32-atoi(imask);
	*fdata = fd;
	return 0;
}

/**
 * parses the given string
 * @return 0 if successful
 */
static int parsePortPattern(char* s, FieldData** fdata, FieldLength* length) {
	char buf[256];

	char* p = s;
	*length = 0;
	*fdata = 0;
	char* pair;

	while ((pair = get_next_token(&p, ","))) {
		strncpy(buf, pair, sizeof(buf));
		char* p2 = buf;
		char* start = get_next_token(&p2, ":");
		char* end = get_next_token(&p2, ",");
		*length = *length + 4;
		*fdata = realloc(*fdata, *length);
		uint16_t starti = atoi(start);
		uint16_t endi = starti;
		if (end) endi = atoi(end);

		(*fdata)[*length-4] = ((starti >> 8) & 0xFF);
		(*fdata)[*length-3] = ((starti >> 0) & 0xFF);
		(*fdata)[*length-2] = ((endi >> 8) & 0xFF);
		(*fdata)[*length-1] = ((endi >> 0) & 0xFF);
	}

	return 0;
}

static char* modifier2string(FieldModifier i) {
	static char s[16];
	if (i == FIELD_MODIFIER_DISCARD) return "discard";
	if (i == FIELD_MODIFIER_KEEP) return "keep";
	if (i == FIELD_MODIFIER_AGGREGATE) return "aggregate";
	if ((i >= FIELD_MODIFIER_MASK_START) && (i <= FIELD_MODIFIER_MASK_END)) {
		sprintf(s, "mask/%d", i - FIELD_MODIFIER_MASK_START);
		return s;
	}
	return "unknownModifier";
}

/**
 * Prints a textual representation of the rule to stdout
 */
void printRule(Rule* rule) {
	int i;

	printf("Aggregate\n");
	for (i=0; i < rule->fieldCount; i++) {
		printf("\t");
		char* modifier = modifier2string(rule->field[i]->modifier);
		if (modifier != NULL) {
			printf("%s ", modifier);
		} else {
			printf("unknownModifier ");
		}

		char* type = typeid2string(rule->field[i]->type.id);
		if (type != NULL) {
			printf("%s ", type);
		} else {
			printf("unknownType ");
		}

		if (rule->field[i]->pattern != NULL) {
			printf("in ");
			printFieldData(rule->field[i]->type, rule->field[i]->pattern);
		}
		printf("\n");
	}
	printf("\n");
}

/**
 * De-allocates memory used by the given ruleField.
 */
void destroyRuleField(RuleField* ruleField) {
	free(ruleField->pattern);
	free(ruleField);
}

/**
 * De-allocates memory used by the given rule.
 * This will NOT destroy hashtables associated with the rule
 */
void destroyRule(Rule* rule) {
	int i;
	for (i = 0; i < rule->fieldCount; i++) {
		destroyRuleField(rule->field[i]);
	}
	free(rule);
}

/**
 * De-allocates memory used by rules.
 * This will NOT destroy hashtables associated with individual rules
 */
void destroyRules(Rules* rules) {
	int i;
	for (i = 0; i < rules->count; i++) {
		destroyRule(rules->rule[i]);
	}
	free(rules);
}

/**
 * Reads in a ruleset from the specified file
 */
Rules* parseRulesFromFile(char* fname) {
	
	FILE* f;
	char buf[MAX_LINE_LEN];
	char* p;
	int lineNo = 0;
	
	Rules* rules = (Rules*)malloc(sizeof(Rules));
	rules->count = 0;

	Rule* currentRule = mallocRule();

	RuleField* ruleField = mallocRuleField();

	f = fopen(fname, "r");
	assert(f);

	while (fgets(buf, sizeof(buf), f)) {
		lineNo++;
		if (strlen(buf) < 3) continue;
		if (*buf == '#') continue;
		p = buf;
		char* col1 = get_next_token(&p, " \t\n");
		char* modifier = get_next_token(&p, " \t\n");
		char* field = get_next_token(&p, " \t\n");
		get_next_token(&p, " \t\n");
		char* pattern = get_next_token(&p, " \t\n");

		if (!col1) {
			msg(MSG_ERROR, "Unparseable line in %s, l.%d", fname, lineNo);			
			continue;
		}

		if (*col1 != 0) {
			/* Start of new Rule */
			if (currentRule->fieldCount > 0) {
				rules->rule[rules->count++] = currentRule;
				currentRule = mallocRule();
			}
			continue;
		}

		if (!field) {
			msg(MSG_ERROR, "Incomplete line in %s, l.%d", fname, lineNo);
			continue;
		}

		if (parseModifier(modifier, &ruleField->modifier) != 0) {
			msg(MSG_ERROR, "Bad modifier \"%s\" in %s, l.%d", modifier, fname, lineNo);
			continue;
		}

		if ((ruleField->type.id = string2typeid(field)) == 0) {
			msg(MSG_ERROR, "Bad field type \"%s\" in %s, l.%d", field, fname, lineNo);
			continue;
		}

		if ((ruleField->type.length = string2typelength(field)) == 0) {
			msg(MSG_ERROR, "Bad field type \"%s\" in %s, l.%d", field, fname, lineNo);
			continue;
		}

		ruleField->pattern = NULL;
		if (pattern)
		switch (ruleField->type.id) {
		case IPFIX_TYPEID_protocolIdentifier:
			if (parseProtoPattern(pattern, &ruleField->pattern, &ruleField->type.length) != 0) {
				msg(MSG_ERROR, "Bad protocol pattern \"%s\" in %s, l.%d", pattern, fname, lineNo);
				continue;
			}
			break;
		case IPFIX_TYPEID_sourceIPv4Address:
		case IPFIX_TYPEID_destinationIPv4Address:
			if (parseIPv4Pattern(pattern, &ruleField->pattern, &ruleField->type.length) != 0) {
				msg(MSG_ERROR, "Bad IPv4 pattern \"%s\" in %s, l.%d", pattern, fname, lineNo);
				continue;
			}
			break;
		case IPFIX_TYPEID_sourceTransportPort:
		case IPFIX_TYPEID_destinationtransportPort:
			if (parsePortPattern(pattern, &ruleField->pattern, &ruleField->type.length) != 0) {
				msg(MSG_ERROR, "Bad PortRanges pattern \"%s\" in %s, l.%d", pattern, fname, lineNo);
				continue;
			}
			break;
		default:
			msg(MSG_ERROR, "Fields of type \"%s\" cannot be matched against a pattern %s, l.%d", field, fname, lineNo);
			continue;
			break;

		}

		currentRule->field[currentRule->fieldCount++] = ruleField;
		ruleField = mallocRuleField();
	}
	if (currentRule->fieldCount > 0) {
		rules->rule[rules->count++] = currentRule;
		currentRule = mallocRule();
	}

	fclose(f);

	/* tidy up */
	destroyRuleField(ruleField);
	destroyRule(currentRule);

	return rules;
}

static uint8_t getIPv4IMask(FieldType* type, FieldData* data) {
	if (type->length > 4) return data[4];
	if (type->length == 4) return 0;
	if (type->length == 3) return 8;
	if (type->length == 2) return 16;
	if (type->length == 1) return 24;
	if (type->length == 0) return 32;

	msg(MSG_FATAL, "Invalid IPv4 address length: %d", type->length);
	return 0;
}

static uint32_t getIPv4Address(FieldType* type, FieldData* data) {
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
static int matchesPortPattern(FieldType* dataType, FieldData* data, FieldType* patternType, FieldData* pattern) {
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
static int matchesIPv4Pattern(FieldType* dataType, FieldData* data, FieldType* patternType, FieldData* pattern) {
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
static int matchesRawPattern(FieldType* dataType, FieldData* data, FieldType* patternType, FieldData* pattern) {
	int i;

	/* Byte-wise comparison, so lengths must be equal */
	if (dataType->length != patternType->length) return 0;

	for (i = 0; i < dataType->length; i++) if (data[i] != pattern[i]) return 0;

	return 1;
}

/**
 * Checks if a data block matches a given pattern
 * @return 1 if pattern is matched
 */
static int matchesPattern(FieldType* dataType, FieldData* data, FieldType* patternType, FieldData* pattern) {
	/* an inexistent pattern is always matched */
	if (pattern == NULL) return 1;

	if ((dataType->id != patternType->id) || (dataType->eid != patternType->eid)) return 0;

	switch (patternType->id) {
	case IPFIX_TYPEID_sourceIPv4Address:
	case IPFIX_TYPEID_destinationIPv4Address: {
		return matchesIPv4Pattern(dataType, data, patternType, pattern);
		break;
	}
	case IPFIX_TYPEID_sourceTransportPort:
	case IPFIX_TYPEID_destinationtransportPort:
		return matchesPortPattern(dataType, data, patternType, pattern);
		break;
	default:
		return matchesRawPattern(dataType, data, patternType, pattern);
		break;
	}
}

/**
 * templateDataMatchesRule helper.
 * If a flow's IP matched a ruleField's IP address + mask,
 * we will also have to check if the flow's mask is no broader than the ruleField's
 * @return 0 if the field had an associated mask that did not match
 */
static int checkAssociatedMask(TemplateInfo* info, FieldData* data, RuleField* ruleField) {
	if ((ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getTemplateFieldInfo(info, &(FieldType){.id = IPFIX_TYPEID_sourceIPv4Mask, .eid = 0});
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	if ((ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getTemplateFieldInfo(info, &(FieldType){.id = IPFIX_TYPEID_destinationIPv4Mask, .eid = 0});
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	return 1;
}

/**
 * templateDataMatchesRule helper.
 * If a flow's IP matched a ruleField's IP address + mask,
 * we will also have to check if the flow's mask is no broader than the ruleField's
 * @return 0 if the field had an associated mask that did not match
 */
static int checkAssociatedMask2(DataTemplateInfo* info, FieldData* data, RuleField* ruleField) {
	if ((ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getDataTemplateFieldInfo(info, &(FieldType){.id = IPFIX_TYPEID_sourceIPv4Mask, .eid = 0});
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	if ((ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getDataTemplateFieldInfo(info, &(FieldType){.id = IPFIX_TYPEID_destinationIPv4Mask, .eid = 0});
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	return 1;
}

/**
 * templateDataMatchesRule helper.
 * If a flow's IP matched a ruleField's IP address + mask,
 * we will also have to check if the flow's mask is no broader than the ruleField's
 * @return 0 if the field had an associated mask that did not match
 */
static int checkAssociatedMask3(DataTemplateInfo* info, FieldData* data, RuleField* ruleField) {
	if ((ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getDataTemplateDataInfo(info, &(FieldType){.id = IPFIX_TYPEID_sourceIPv4Mask, .eid = 0});
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	if ((ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address) && (ruleField->pattern) && (ruleField->type.length == 5)) {
		FieldInfo* maskInfo = getDataTemplateDataInfo(info, &(FieldType){.id = IPFIX_TYPEID_destinationIPv4Mask, .eid = 0});
		if (!maskInfo) return 1;

		uint8_t pmask = 32 - getIPv4IMask(&ruleField->type, ruleField->pattern);
		uint8_t dmask = *(data + maskInfo->offset);
		return (dmask >= pmask);
	}
	return 1;
}

/**
 * Checks if a given flow matches a rule
 * @return 1 if rule is matched, 0 otherwise
 */
int templateDataMatchesRule(TemplateInfo* info, FieldData* data, Rule* rule) {
	int i;
	FieldInfo* fieldInfo;

	for (i = 0; i < rule->fieldCount; i++) {
		RuleField* ruleField = rule->field[i];

		/* for all patterns of this rule, check if they are matched */
		if (rule->field[i]->pattern) {
			fieldInfo = getTemplateFieldInfo(info, &ruleField->type);
			if (fieldInfo) {
				/* corresponding data field found, check if it matches. If it doesn't the whole rule cannot be matched */
				if (!matchesPattern(&fieldInfo->type, (data + fieldInfo->offset), &ruleField->type, ruleField->pattern)) return 0;
				if (!checkAssociatedMask(info, data, ruleField)) return 0;
				continue;
			}

			/* no corresponding data field found, this flow cannot match */
			msg(MSG_DEBUG, "No corresponding DataRecord field for RuleField of type %s", typeid2string(ruleField->type.id));
			return 0;
		}
		/* if a non-discarding rule field specifies no pattern, check at least if the data field exists */
		else if (rule->field[i]->modifier != FIELD_MODIFIER_DISCARD) {
			fieldInfo = getTemplateFieldInfo(info, &ruleField->type);
			if (fieldInfo) continue;
			msg(MSG_DEBUG, "No corresponding DataRecord field for RuleField of type %s", typeid2string(ruleField->type.id));
			return 0;
		}
	}

	/* all rule fields were matched */
	return 1;
}

/**
 * Checks if a given flow matches a rule
 * @return 1 if rule is matched, 0 otherwise
 */
int dataTemplateDataMatchesRule(DataTemplateInfo* info, FieldData* data, Rule* rule) {
	int i;
	FieldInfo* fieldInfo;

	/* for all patterns of this rule, check if they are matched */
	for (i = 0; i < rule->fieldCount; i++) if (rule->field[i]->pattern) {
		RuleField* ruleField = rule->field[i];

		fieldInfo = getDataTemplateFieldInfo(info, &ruleField->type);
		if (fieldInfo) {
			/* corresponding data field found, check if it matches. If it doesn't the whole rule cannot be matched */
			if (!matchesPattern(&fieldInfo->type, (data + fieldInfo->offset), &ruleField->type, ruleField->pattern)) return 0;
			if (!checkAssociatedMask2(info, data, ruleField)) return 0;
			continue;
		}

		/* no corresponding data field found, see if we find a corresponding fixed data field */

		fieldInfo = getDataTemplateDataInfo(info, &ruleField->type);
		if (fieldInfo) {
			/* corresponding fixed data field found, check if it matches. If it doesn't the whole rule cannot be matched */
			if (!matchesPattern(&fieldInfo->type, (info->data + fieldInfo->offset), &ruleField->type, ruleField->pattern)) return 0;
			if (!checkAssociatedMask3(info, info->data, ruleField)) return 0;
			continue;
		}

		/* FIXME: if a non-discarding rule field specifies no pattern, check at least if the data field exists? */

		/* no corresponding data field or fixed data field found, this flow cannot match */
		msg(MSG_DEBUG, "No corresponding DataDataRecord field for RuleField of type %s", typeid2string(ruleField->type.id));
		return 0;
	}

	/* all rule fields were matched */
	return 1;
}
