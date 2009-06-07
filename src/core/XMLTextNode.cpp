#include "XMLNode.h"

XMLTextNode::XMLTextNode(xmlNodePtr ptr)
	: XMLNode(ptr)
{
}

XMLTextNode::~XMLTextNode()
{
}

const std::string XMLTextNode::getContent()
{
	const char* content =  (const char*)cobj()->content;
	if (!content)
		return "";

	return content;
}

bool XMLTextNode::isBlank() const
{
	return xmlIsBlankNode(cobj());
}
