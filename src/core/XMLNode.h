#ifndef XNODE_H_
#define XNODE_H_

#include <libxml/parser.h>

#include <string>
#include <vector>

class XMLTextNode;
class XMLElement;

class XMLNode
{
public:
	typedef std::vector<XMLNode*> XMLNodeSet;

	template <class T>
	class XMLSet : public std::vector<T> { };


	XMLNode(xmlNodePtr ptr);
	virtual ~XMLNode();

	/** Get the name of this node. */
	std::string getName () const;

	/** Get the content of this node. */
	virtual std::string getContent () const;

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

	/** return the first child with a given name */
	XMLNode* getFirstChild(const std::string& str);
	
	/** Return all children */
	inline XMLNodeSet getChildren()
	{
		return findChildren("");
	}

	XMLSet<XMLElement*> getElementChildren();

	/** Returns all non whitespace TextNodes
	 *
	 *  This is a conveniance function to make life easier for
	 *  callers which are only interested in values.
	 */
	XMLSet<XMLTextNode*> getTextChildren();

	/** Returns the value of the first TextNode
	 *
	 *  This is a conveniance function to make life easier for
	 *  callers which are only interested in values.
	 */
	std::string getFirstText();

	/** Return all children */
	inline const XMLNodeSet getChildren() const
	{
		return findChildren("");
	}

protected:
	inline xmlNodePtr cobj() {
		return xmlNode;
	}

	inline xmlNodePtr cobj() const {
		return xmlNode;
	}

private:
	XMLNode(const XMLNode&);
	XMLNode& operator=(const XMLNode&);

	xmlNodePtr xmlNode;
};


class XMLTextNode
	: public XMLNode
{
public:
	XMLTextNode(xmlNodePtr ptr);
	virtual ~XMLTextNode();

	/** Get the value of this node. */
	const std::string getContent();

	/** checks if the node consists of only whitespace */
	bool isBlank() const;
};

#endif /*XNODE_H_*/
