#include "XMLAttribute.h"

XMLAttribute::XMLAttribute(xmlNodePtr ptr)
	:XMLNode(ptr)
{
}

XMLAttribute::~XMLAttribute()
{
}

std::string XMLAttribute::getName() const
{
	if (cobj()->name)
		return (const char*)cobj()->name;

	return "";
}

std::string XMLAttribute::getValue() const
{
	std::string ret;
	xmlChar *v = xmlGetProp(cobj()->parent, cobj()->name);
	ret = (v ? (const char*)v : "");
	xmlFree(v);
	return ret;
	
}


