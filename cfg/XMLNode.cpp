#include "XMLNode.h"

XMLNode::XMLNode(xmlNodePtr doc)
	: xmlNode(doc)
{
	xmlNode->_private = this;
}

XMLNode::~XMLNode()
{
}

std::string XMLNode::getName() const
{
	if (!xmlNode->name)
		return "";

	return  std::string((const char*)xmlNode->name);
}

XMLNode::XMLNodeSet XMLNode::findChildren(const std::string& name)
{
	XMLNode::XMLNodeSet children;

	for (xmlNodePtr child = xmlNode->children; child; child = child->next) {
		if(child->_private &&
		   (name.empty() || name == (const char*)child->name)) {
			children.push_back(reinterpret_cast<XMLNode*>(child->_private));
		}
	}

	return children;
}

