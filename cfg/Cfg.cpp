#include "Cfg.h"

#include <cassert>

std::string Cfg::get(const std::string& name)
{
	XMLNode* n = _elem->getFirstChild(name);
	assert(n);
	return n->getFirstText();
}

int Cfg::getInt(const std::string& name, int def)
{
	std::string str = get(name);
	if(str.empty())
		return def;
	return atoi(str.c_str());
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
