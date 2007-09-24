#include "Cfg.h"

#include <cassert>

std::string Cfg::get(const std::string& name, XMLElement* elem) throw(IllegalEntry)
{
	if (!elem)
		elem = _elem;

	XMLNode* n = elem->getFirstChild(name);
	if (!n)
		throw IllegalEntry();

	return n->getFirstText();
}

int Cfg::getInt(const std::string& name, int def, XMLElement* elem)
{
	std::string str;
	try {
		str = get(name, elem);
		return atoi(str.c_str());
	} catch (IllegalEntry ie) { }

	// return default value
	return def;
}

unsigned int Cfg::getTimeInUnit(const std::string& name, timeUnit unit,
				unsigned def, XMLElement* elem)
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
		} catch (IllegalEntry ie) {

		}

		time = atoi(e->getFirstText().c_str());

		XMLAttribute* a = e->getAttribute("unit");
		if (!a)
			continue;

		if (a->getValue() == "sec")
			return time * (uSEC/unit);
		else if (a->getValue() == "msec")
			return time * (mSEC/unit);
		else if (a->getValue() == "usec")
			return time * (uSEC/unit);
		else
			THROWEXCEPTION("Unkown time unit '%s'", a->getValue().c_str());
	}

	// we didn't find the element
	throw IllegalEntry();
	return -1;
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
	assert(attr != NULL);
	return atoi(attr->getValue().c_str());
}
