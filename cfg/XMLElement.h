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
	virtual ~XMLElement();
};

#endif /*XMLNODE_H_*/
