#include "XMLElement.h"

XMLElement::XMLElement(xmlNodePtr node)
	: XMLNode(node), xmlDocument(NULL)
{
}

XMLElement::XMLElement(xmlDocPtr xmlDocument, xmlNodePtr node)
	: XMLNode(node), xmlDocument(NULL)
{
}

XMLElement::~XMLElement()
{
}

const std::string XMLElement::getContent()
{
	xmlChar* content = xmlNodeListGetString(xmlDocument, cobj()->xmlChildrenNode, 1);
	std::string ret = std::string((const char*) content);
	xmlFree(content);
	return ret;
}
