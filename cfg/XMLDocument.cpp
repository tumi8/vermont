#include "XMLDocument.h"
#include "XMLAttribute.h"

#include "common/msg.h"

#include <assert.h>
#include <iostream>

#include <libxml/parserInternals.h>
#include <libxml/xmlIO.h>

#define DO_VALIDATION 0

// static variable initializing
XMLDocument::Initializer XMLDocument::init;


XMLDocument::XMLDocument(xmlDocPtr doc) : xmlDocument(doc)
{
	xmlDocument->_private = this;
}

XMLDocument::~XMLDocument()
{
	xmlFreeDoc(xmlDocument);
}

XMLElement* XMLDocument::getRootNode() const
{

	xmlNodePtr root = xmlDocGetRootElement(xmlDocument);
	if(root)
		return reinterpret_cast<XMLElement*>(root->_private);
	return NULL;
}

#if DO_VALIDATION
static void validate_error(void* ctx, const char* msg, ...)
{
	xmlParserCtxtPtr context = (xmlParserCtxtPtr)ctx;
	if(context) {

		va_list arg;
		char buf[1024]; // big enough?

		va_start(arg, msg);
		vsnprintf(buf, sizeof(buf)/sizeof(buf[0]), msg, arg);
		va_end(arg);

		THROWEXCEPTION(buf);
	}
}
#endif

XMLDocument* XMLDocument::parse_file(const std::string& fileName)
{
	_xmlParserCtxt* ctx;
	XMLDocument* doc = NULL;
	std::string exceptionMsg;

	ctx = xmlCreateFileParserCtxt(fileName.c_str());
	if(!ctx) {
		exceptionMsg = "Couldn't create parsing context";
		goto free;
	}

	if(!ctx->directory) {
		char* directory = xmlParserGetDirectory(fileName.c_str());
		ctx->directory = (char*) xmlStrdup((xmlChar*) directory);
	}

#if DO_VALIDATION
	/* turn on/off validation; this is off because because activating it
	 * errors out because of the mssing DTD
	 */
	ctx->validate = 1;

	// callback functions for validity checking
	ctx->vctxt.error   = &validate_error;
	ctx->vctxt.warning = &validate_error;
#endif

	xmlParseDocument(ctx);

	if(!ctx->wellFormed) {
		exceptionMsg = "Document not well-formed.";
		goto free;
	}

	if(ctx->errNo) {
		exceptionMsg = "Can't parse the file; libxml inidicates error "
				+  ctx->errNo;
		goto free;
	}

	doc = new XMLDocument(ctx->myDoc);
	ctx->myDoc = NULL;

free:	// jump here if you need to free/delete your mess; 
	// e.g before returning or before throwing an excpetion 
	if(ctx) {
		ctx->_private = NULL;
		if(ctx->myDoc)
			xmlFreeDoc(ctx->myDoc);

		xmlFreeParserCtxt(ctx);
		ctx = NULL;
	}

	if (!exceptionMsg.empty())
		THROWEXCEPTION(exceptionMsg.c_str());

	return doc;
}

#define PRINT_TYPE(x) \
	if(false) { \
		fprintf(stderr, "%s\n", x); \
	}

void on_construct(xmlNodePtr node)
{
	switch (node->type) {
	case XML_ELEMENT_NODE:
		PRINT_TYPE("XML_ELEMENT_NODE");
		node->_private = new XMLElement(node);
		break;
	case XML_ATTRIBUTE_NODE:
		PRINT_TYPE("XML_ATTRIBUTE_NODE");
		node->_private = new XMLAttribute(node);
		break;
	case XML_TEXT_NODE:
		PRINT_TYPE("XML_TEXT_NODE");
		node->_private = new XMLTextNode(node);;
		break;
	case XML_COMMENT_NODE:
		PRINT_TYPE("XML_COMMENT_NODE");
		break;
	case XML_CDATA_SECTION_NODE:
		PRINT_TYPE("XML_CDATA_SECTION_NODE");
		break;
	case XML_PI_NODE:
		PRINT_TYPE("XML_PI_NODE");
		break;
	case XML_DTD_NODE:
		PRINT_TYPE("XML_DTD_NODE");
		break;
	case XML_ENTITY_REF_NODE:
		PRINT_TYPE("XML_ENTITY_REF_NODE");
		break;
	case XML_DOCUMENT_NODE:
		PRINT_TYPE("XML_DOCUMENT_NODE");
		// we are the document !
		break;
	default:
		PRINT_TYPE("DEFAULT TYPE\n");
		break;
	}
}

void on_destruct(xmlNode* n)
{
	// protection: don't delete myself
	if (n->type == XML_DOCUMENT_NODE)
		return;

	XMLNode* node =  static_cast<XMLNode*>(n->_private);
	if(node)
		delete node;
	n->_private = NULL;
}

XMLDocument::Initializer::Initializer()
{
	xmlInitParser();
	xmlRegisterNodeDefault(on_construct);
	xmlDeregisterNodeDefault(on_destruct);
	xmlThrDefRegisterNodeDefault(on_construct);
	xmlThrDefDeregisterNodeDefault(on_destruct);
}

