#include "XMLNode.h"

#include <cassert>

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

std::string XMLNode::getContent() const
{
	return std::string((const char*)xmlNodeGetContent(xmlNode));
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

XMLNode* XMLNode::getFirstChild(const std::string& name)
{
	for (xmlNodePtr child = xmlNode->children; child; child = child->next) {
		if(child->_private &&
				(name.empty() || name == (const char*)child->name)) {
			return reinterpret_cast<XMLNode*>(child->_private);
		}
	}
	return NULL;
}

XMLNode::XMLSet<XMLTextNode*> XMLNode::getTextChildren()
{
	XMLNode::XMLSet<XMLTextNode*> children;
	assert(xmlNode != NULL);

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

XMLNode::XMLSet<XMLElement*> XMLNode::getElementChildren()
{
	XMLNode::XMLSet<XMLElement*> children;

	for (xmlNodePtr child = xmlNode->children; child; child = child->next) {
		// check if name and/or type matches
		if (child->_private &&  child->type == XML_ELEMENT_NODE) {
			XMLElement* e = reinterpret_cast<XMLElement*>(child->_private);
			children.push_back(e);
		}
	}
	return children;
}
