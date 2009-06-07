#ifndef XMLELEMENT_H_
#define XMLELEMENT_H_

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "core/XMLNode.h"
#include "core/XMLAttribute.h"

#include <string>
#include <vector>

class XMLAttribute;

class XMLElement: public XMLNode
{
public:
	typedef std::vector<XMLAttribute*> XMLAttributeSet;

	XMLElement(xmlNodePtr ptr);
	virtual ~XMLElement();

	XMLAttributeSet getAttributes();
	XMLAttribute* getAttribute(const std::string& name);

private:
	XMLAttributeSet getAttribHelper(const std::string&);
};

#endif /*XMLNODE_H_*/
