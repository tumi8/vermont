#ifndef XNODE_H_
#define XNODE_H_

#include <libxml/parser.h>

#include <string>
#include <vector>

class XMLNode
{
public:
	typedef std::vector<XMLNode*> XMLNodeSet;

	XMLNode(xmlNodePtr ptr);
	virtual ~XMLNode();

	/** Get the name of this node. */
	std::string getName () const;

	/** compares the node name with the supplied string */
	inline bool matches(const char* str) const
	{
		return getName() == std::string(str);
	}

	/** compares the node name with the supplied string */
	inline bool matches(const std::string& str) const
	{
		return getName() == str;
	}

	/** find all children matching a given name */
	XMLNodeSet findChildren(const std::string&);

	/** find all children matching a given name */
	const XMLNodeSet findChildren(const std::string& str) const
	{
		return const_cast<XMLNode*>(this)->findChildren(str);
	}

	/** Return all children */
	inline XMLNodeSet getChildren()
	{
		return findChildren("");
	}

	/** Return all children */
	inline const XMLNodeSet getChildren() const
	{
		return findChildren("");
	}

	inline xmlNodePtr cobj() {
		return xmlNode;
	}

private:
	xmlDocPtr xmlDocument;
	xmlNodePtr xmlNode;
};

#endif /*XNODE_H_*/
