#include "XMLElement.h"

#include <cassert>

XMLElement::XMLElement(xmlNodePtr node)
	: XMLNode(node)
{
}

XMLElement::~XMLElement()
{
}

XMLElement::XMLAttributeSet XMLElement::getAttributes()
{
	return getAttribHelper("");
}

XMLAttribute* XMLElement::getAttribute(const std::string &name)
{
	XMLAttributeSet set = getAttribHelper(name);
	if(set.size() == 0)
		return NULL;

	assert(set.size() >= 1);
	return set.front();
}

XMLElement::XMLAttributeSet XMLElement::getAttribHelper(const std::string &name)
{
	XMLAttributeSet attributes;

	for (xmlAttrPtr a = cobj()->properties; a; a = a->next) {
		if(a->_private &&
		   (name.empty() || name == (const char*)a->name)) {
			attributes.push_back(reinterpret_cast<XMLAttribute*>(a->_private));
		}
	}

	return attributes;
}

