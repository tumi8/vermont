//
// C++ Interface: configfile
//
// Description:
//
//
// Author: Peter Baumann <siprbaum@users.berlios.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "Graph.h"

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/
class ConfigFile{
public:
	ConfigFile();
	~ConfigFile();

	Graph* parse(std::string fileName);

	static std::string getContent(xmlDocPtr doc, xmlNodePtr p);
	/*
	{
		xmlChar* v = xmlNodeListGetString(doc, p->xmlChildrenNode, 1);
		std::string ret = reinterpret_cast<char*>(v);
		xmlFree(v);
		return ret;
	}
	*/

private:
	xmlDocPtr document;
	xmlNodePtr current;
};


#endif
