#include "Cfg.h"

#include <cassert>
#include <stdlib.h>
#include <limits.h>


/**
 * internally used function
 */
std::string CfgBase::_get(const std::string& name, XMLElement* elem)
{
	if (!elem)
		elem = _elem;

	if (elem->matches(name))
		return elem->getFirstText();

	XMLNode* n = elem->getFirstChild(name);
	if (!n)
		throw IllegalEntry();

	return n->getFirstText();
}

std::string CfgBase::get(const std::string& name, XMLElement* elem)
{
	if (!elem) {
		if (!_elem) {
			THROWEXCEPTION("Error: accessing null XML element");
		} else {
			elem = _elem;
		}
	}

	std::string result;
	try {
		result = _get(name, elem);
	} catch (IllegalEntry& ie) {

		THROWEXCEPTION("Error: Element '%s' not found in node '%s' in configuration", name.c_str(), elem->getName().c_str());
	}

	return result;
}

std::string CfgBase::getOptional(const std::string& name, XMLElement* elem)
{
	std::string result;
	try {
		result = _get(name, elem);
	} catch (IllegalEntry& ie) { }

	return result;
}

double CfgBase::getDouble(const std::string& name, double def, XMLElement* elem)
{
	std::string str;
	try {
		str = _get(name, elem);
		return atof(str.c_str());
	} catch (IllegalEntry& ie) { }

	// return default value
	return def;
}

int CfgBase::getInt(const std::string& name, int def, XMLElement* elem)
{
	std::string str;
	try {
		str = _get(name, elem);
		long res = strtol(str.c_str(), NULL, 10);
		if (res==LONG_MIN || res==LONG_MAX) {
			THROWEXCEPTION("failed to read integer %s in element %s in configuration (is it too large? or invalid?)", str.c_str(), name.c_str());
		}
		return res;
	} catch (IllegalEntry& ie) { }

	// return default value
	return def;
}

uint32_t CfgBase::getUInt32(const std::string& name, uint32_t def, XMLElement* elem)
{
	std::string str;
	try {
		str = _get(name, elem);
		return strtoul(str.c_str(), NULL, 10);
	} catch (IllegalEntry& ie) { }

	// return default value
	return def;
}

int64_t CfgBase::getInt64(const std::string& name, int64_t def, XMLElement* elem)
{
	std::string str;
	try {
		str = _get(name, elem);
		return atoll(str.c_str());
	} catch (IllegalEntry& ie) { }

	// return default value
	return def;
}

bool CfgBase::getBool(const std::string& name, bool def, XMLElement* elem)
{
	std::string str;
	try {
		str = _get(name, elem);
		// make lower case
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return ((str == "true") || (str == "1"));
	} catch (IllegalEntry& ie) { }

	// return default value
	return def;
}

unsigned int CfgBase::getTimeInUnit(const std::string& name, timeUnit unit, uint32_t def, XMLElement* elem)
{
	unsigned int time;
	if (!elem)
		elem = _elem;

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	XMLNode::XMLSet<XMLElement*>::const_iterator it = set.begin();
	for (; it != set.end(); it++) {
		XMLElement* e = *it;

		try {
			if (e->getName() != name)
				continue;
		} catch (IllegalEntry& ie) {

		}

		time = atoi(e->getFirstText().c_str());

		XMLAttribute* a = e->getAttribute("unit");
		if (!a)
			continue;

		if (a->getValue() == "sec")
			return time*unit/SEC;
		else if (a->getValue() == "msec")
			return time*unit/mSEC;
		else if (a->getValue() == "usec")
			return time*unit/uSEC;
		else
			THROWEXCEPTION("Unkown time unit '%s'", a->getValue().c_str());
	}

	// we didn't find the element, return default
	return def;
}

std::vector<unsigned int> Cfg::getNext()
{
	std::vector<unsigned int> next;
	XMLNode::XMLNodeSet childs = _elem->findChildren("next");
	for (XMLNode::XMLNodeSet::iterator it = childs.begin();
	     it != childs.end();
	     it++) {
		XMLNode* e = *it;
		next.push_back(atoi(e->getFirstText().c_str()));
	}
	return next;
}

unsigned int Cfg::getID()
{
	XMLAttribute* attr = _elem->getAttribute("id");
	if (attr == NULL) {
		THROWEXCEPTION("Error: Configuration Element '%s' does not have an 'id' field. 'id' is required!", _elem->getName().c_str());
	}
	return atoi(attr->getValue().c_str());
}
