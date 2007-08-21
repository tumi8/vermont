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

XMLNode::XMLSet<XMLTextNode*> XMLNode::getTextChildren()
{
	XMLNode::XMLSet<XMLTextNode*> children;

	for (xmlNodePtr child = xmlNode->children; child; child = child->next) {
		// check if name and/or type matches
		if (child->_private &&  child->type == XML_TEXT_NODE) {
			XMLTextNode* tn = reinterpret_cast<XMLTextNode*>(child->_private);
			if (!tn->isBlank())
				children.push_back(tn);
		}
	}
	return children;
}

std::string XMLNode::getFirstText()
{
	XMLNode::XMLSet<XMLTextNode*> set = getTextChildren();
	if (set.size() <= 0)
		return "";

	return set.front()->getContent();
}
