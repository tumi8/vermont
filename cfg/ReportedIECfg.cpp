#include "ReportedIECfg.h"

std::string getContent(xmlDocPtr doc, xmlNodePtr p)
{
	xmlChar* v = xmlNodeListGetString(doc, p->xmlChildrenNode, 1);
	std::string ret = (const char*) v;
	xmlFree(v);
	return ret;
}

bool xmlCompare(const xmlNodePtr node, const std::string& tagName)
{
	return !xmlStrcmp(node->name, (const xmlChar*)tagName.c_str());
}
