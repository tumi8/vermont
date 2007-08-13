#ifndef XMLELEMENT_H_
#define XMLELEMENT_H_

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "cfg/XMLNode.h"

#include <string>
#include <vector>

class XMLElement: public XMLNode
{
public:
	XMLElement(xmlNodePtr ptr);
	XMLElement(xmlDocPtr xmlDocument, xmlNodePtr ptr);
	virtual ~XMLElement();

	/** Get the value of this node. */
	const std::string getContent();

private:
	xmlDocPtr xmlDocument;
};

#endif /*XMLNODE_H_*/
