#include "Cfg.h"

#include <cassert>

std::string Cfg::get(const std::string& name, XMLElement* elem) throw(IllegalEntry)
{
	if (!elem)
		elem = _elem;

	XMLNode* n = elem->getFirstChild(name);
	if (!n)
		throw new IllegalEntry();

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
