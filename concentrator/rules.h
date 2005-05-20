#ifndef RULES_H
#define RULES_H

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "rcvIpfix.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FIELD_MODIFIER_DISCARD     0
#define FIELD_MODIFIER_KEEP        1
#define FIELD_MODIFIER_AGGREGATE   2
#define FIELD_MODIFIER_MASK_START  126
#define FIELD_MODIFIER_MASK_END    254

#define MAX_RULE_FIELDS  255
#define MAX_RULES        255

typedef uint8_t FieldModifier;

/**
 * Single field of an aggregation rule.
 */
typedef struct {
	FieldType type;         /**< field type this RuleField refers to */
	FieldData* pattern;     /**< pattern to match fields against to determine applicability of a rule. A pattern of NULL means no pattern needs to be matched for this field */
	FieldModifier modifier; /**< modifier to apply to the corresponding field if this rule is matched */
	} RuleField;

/**
 * Single aggregation rule
 */
typedef struct {
	int fieldCount;
	RuleField* field[MAX_RULE_FIELDS];
	
	void* hashtable;
	} Rule;

/**
 * Set of aggregation rules
 */
typedef struct {
	int count;
	Rule* rule[MAX_RULES];
	} Rules;

Rules* parseRulesFromFile(char* fname);
void destroyRules(Rules* rules);
void printRule(Rule* rule);
int templateDataMatchesRule(TemplateInfo* info, FieldData* data, Rule* rule);
int dataTemplateDataMatchesRule(DataTemplateInfo* info, FieldData* data, Rule* rule);


#ifdef __cplusplus
}
#endif

#endif
