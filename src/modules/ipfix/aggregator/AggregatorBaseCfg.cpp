#include "AggregatorBaseCfg.h"
#include "Rule.hpp"
#include "Rules.hpp"
#include "core/XMLElement.h"
#include "core/InfoElementCfg.h"

AggregatorBaseCfg::AggregatorBaseCfg(XMLElement* elem)
	: CfgBase(elem), pollInterval(0)
{
	if (!elem)
		return;

	rules = new Rules;
	htableBits = HT_DEFAULT_BITSIZE;

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;
		if (e->matches("rule")) {
			Rule* r = readRule(e);
			if (r)
				rules->rule[rules->count++] = r;
		} else if (e->matches("expiration")) {
			// get the time values or set them to '0' if they are not specified
			maxBufferTime = getTimeInUnit("activeTimeout", SEC, 0, e);
			minBufferTime = getTimeInUnit("inactiveTimeout", SEC, 0, e);
			if (!maxBufferTime) THROWEXCEPTION("active timeout not set in configuration for aggregator");
			if (!minBufferTime) THROWEXCEPTION("inactive timeout not set in configuration for aggregator");
		} else if (e->matches("pollInterval")) {
			pollInterval = getTimeInUnit("pollInterval", mSEC, AGG_DEFAULT_POLLING_TIME);
		} else if (e->matches("hashtableBits")) {
			htableBits = getInt("hashtableBits", HT_DEFAULT_BITSIZE);
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unkown Aggregator config entry %s\n", e->getName().c_str());
		}
	}
}

AggregatorBaseCfg::~AggregatorBaseCfg()
{
}

Rule* AggregatorBaseCfg::readRule(XMLElement* elem) {
	// nonflowkey -> aggregate
	// flowkey -> keep

	Rule* rule = new Rule();
	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("templateId")) {
			rule->id = getInt("templateId", -1, e);
		} else if (e->matches("biflowAggregation")) {
			rule->biflowAggregation = getInt("biflowAggregation", 0, e);
		} else if (e->matches("flowKey")) {
			Rule::Field* ruleField = readFlowKeyRule(e);
			if (ruleField)
				rule->field[rule->fieldCount++] = ruleField;
		} else if (e->matches("nonFlowKey")) {
			Rule::Field* ruleField = readNonFlowKeyRule(e);
			if (ruleField)
				rule->field[rule->fieldCount++] = ruleField;
		} else {
			THROWEXCEPTION("Unknown rule %s in Aggregator found", e->getName().c_str());
		}
	}

	// we found no rules, so do cleanup
	if (rule->fieldCount == 0) {
		delete rule;
		rule = NULL;
	}

	return rule;
}

Rule::Field* AggregatorBaseCfg::readNonFlowKeyRule(XMLElement* e)
{
	Rule::Field* ruleField = new Rule::Field();
	InfoElementCfg ie(e);
	ruleField->modifier = Rule::Field::AGGREGATE;

	// parse name
	if (ie.getIeName() != "") {
		if (0 == (ruleField->type.id = string2typeid(ie.getIeName().c_str()))) {
			msg(MSG_ERROR, "Bad field type \"%s\"", ie.getIeName().c_str());
			throw std::exception();
		}
	} else
		ruleField->type.id = ie.getIeId();

	// parse length
	if (ie.hasOptionalLength()) {
		ruleField->type.length = ie.getIeLength();
	} else {
		if (0 == (ruleField->type.length = string2typelength(ie.getIeName().c_str()))) {
			msg(MSG_ERROR, "Bad field type \"%s\", or length of field must be specified!", ie.getIeName().c_str());
			throw std::exception();
		}
	}

	if ((ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) || (ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address)) {
		ruleField->type.length++; // for additional mask field
	}

	if (ruleField->type.id==IPFIX_ETYPEID_frontPayload || ruleField->type.id==IPFIX_ETYPEID_revFrontPayload) {
		if (ruleField->type.length<5)
			THROWEXCEPTION("type %s must have at least size 5!", typeid2string(ruleField->type.id));
	}


	return ruleField;
}

Rule::Field* AggregatorBaseCfg::readFlowKeyRule(XMLElement* e) {
	Rule::Field* ruleField = new Rule::Field();
	try {   // TODO: not sure why we don't abort here; just use the same code as before restructuring
		// I added a delete ruleField in the catch, perhaps thats why it was needed?

		InfoElementCfg ie(e);

		// parse modifier
		if (ie.getModifier().empty() || (ie.getModifier() == "keep")) {
			ruleField->modifier = Rule::Field::KEEP;
		} else if (ie.getModifier() == "discard") {
			ruleField->modifier = Rule::Field::DISCARD;
		} else {
			ruleField->modifier = (Rule::Field::Modifier)((int)Rule::Field::MASK_START + atoi(ie.getModifier().c_str() + 5));
		}

		// parse name
		if (ie.getIeName() != "") {
			if (0 == (ruleField->type.id = string2typeid(ie.getIeName().c_str()))) {
				msg(MSG_ERROR, "Bad field type \"%s\"", ie.getIeName().c_str());
				throw std::exception();
			}
		} else
			ruleField->type.id = ie.getIeId();

		// parse length
		if (ie.hasOptionalLength()) {
			ruleField->type.length = ie.getIeLength();
		} else {
			if (0 == (ruleField->type.length = string2typelength(ie.getIeName().c_str()))) {
				msg(MSG_ERROR, "Bad field type \"%s\", no length was specified!", ie.getIeName().c_str());
				throw std::exception();
			}
		}

		if ((ruleField->type.id == IPFIX_TYPEID_sourceIPv4Address) || (ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address)) {
			ruleField->type.length++; // for additional mask field
		}

		if (!ie.getMatch().empty()) {
			// TODO: we need to copy the string because parseProtoPattern and
			//  parseIPv4Pattern violate the original string
			char* tmp = new char[ie.getMatch().length() + 1];
			strcpy(tmp, ie.getMatch().c_str());
			ruleField->pattern = NULL;

			switch (ruleField->type.id) {
			case IPFIX_TYPEID_protocolIdentifier:
				if (parseProtoPattern(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
					msg(MSG_ERROR, "Bad protocol pattern \"%s\"", tmp);
					delete [] tmp;
					throw std::exception();
				}
				break;
			case IPFIX_TYPEID_sourceIPv4Address:
			case IPFIX_TYPEID_destinationIPv4Address:
				if (parseIPv4Pattern(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
					msg(MSG_ERROR, "Bad IPv4 pattern \"%s\"", tmp);
					delete [] tmp;
					throw std::exception();
				}
				break;
			case IPFIX_TYPEID_sourceTransportPort:
			case IPFIX_TYPEID_udpSourcePort:
			case IPFIX_TYPEID_tcpSourcePort:
			case IPFIX_TYPEID_destinationTransportPort:
			case IPFIX_TYPEID_udpDestinationPort:
			case IPFIX_TYPEID_tcpDestinationPort:
				if (parsePortPattern(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
					msg(MSG_ERROR, "Bad PortRanges pattern \"%s\"", tmp);
					delete [] tmp;
					throw std::exception();
				}
				break;
			case IPFIX_TYPEID_tcpControlBits:
				if (parseTcpFlags(tmp, &ruleField->pattern, &ruleField->type.length) != 0) {
					msg(MSG_ERROR, "Bad TCP flags pattern \"%s\"", tmp);
					delete [] tmp;
					throw std::exception();
				}
				break;

			default:
				msg(MSG_ERROR, "Fields of type \"%s\" cannot be matched against a pattern %s", "", tmp);
				delete tmp;
				throw std::exception();
				break;
			}
			delete [] tmp;
		}
	} catch (std::exception e) {
		delete ruleField;
		ruleField = NULL;
	}


	return ruleField;
}
