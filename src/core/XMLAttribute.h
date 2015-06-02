#ifndef XMLATTRIBUTE_H_
#define XMLATTRIBUTE_H_

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "XMLNode.h"

#include <vector>

class XMLAttribute : public XMLNode
{
public:
	XMLAttribute(xmlNodePtr ptr);
	virtual ~XMLAttribute();

	std::string getName() const;
	std::string getValue() const;

protected:
	inline xmlAttrPtr cobj()
	{
		return reinterpret_cast<xmlAttrPtr>(XMLNode::cobj());
	}

	inline xmlAttrPtr cobj() const
	{
		return reinterpret_cast<const xmlAttrPtr>(XMLNode::cobj());
	}
};

#endif /*XMLATTRIBUTE_H_*/
