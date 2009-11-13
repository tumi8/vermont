#ifndef XMLDOCUMENT_H_
#define XMLDOCUMENT_H_

#include "core/XMLElement.h"

#include <string>

class XMLDocument
{
public:
	virtual ~XMLDocument();
	
	/** create a XMLDocument
	 * @return A pointer the XMLDocument representing the parsed file 
	 */
	static XMLDocument* parse_file(const std::string& fileName);
	
	/** Return the root node.
	 * @return A pointer to the root node if it exists, NULL otherwise.
	 */
	XMLElement* getRootNode() const;

private:
	// hide the c'tor to create instances of this object only with parse_file
	XMLDocument(xmlDocPtr doc);
	
	xmlDocPtr xmlDocument;
	xmlNodePtr xmlRootNode;
	
	struct Initializer {
		Initializer();
	};

	static Initializer init;
};

#endif /*XMLDOCUMENT_H_*/
