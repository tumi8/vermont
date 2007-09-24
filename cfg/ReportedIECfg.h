#ifndef REPORTEDIECFG_H_
#define REPORTEDIECFG_H_

#include <ipfix_configuration.h>
#include <string>

class Configuration::ReportedIE {
public:
	ReportedIE(xmlNodePtr p, const Configuration& m)
		: ieLength(-1), ieId(-1)

	{
		/*
		xmlNodePtr i = p->xmlChildrenNode;
		while (NULL != i) {
			if (m.tagMatches(i, "ieId")) {
				ieId = atoi(m.getContent(i).c_str());
				ieName = m.getContent(i);
			} else if (m.tagMatches(i, "ieLength")) {
				ieLength = atoi(m.getContent(i).c_str());
			} else if (m.tagMatches(i, "ieName")) {
				ieName = m.getContent(i);
			}
			i = i->next;
		}
		std::transform(ieName.begin(), ieName.end(), ieName.begin(), std::tolower);
		*/
	}

	bool hasOptionalLength() const { return ieLength != -1; }
	std::string getName() const { return ieName; }
	unsigned getLength() const { return ieLength; }
	unsigned getId() const { return (ieId == -1)?ipfix_name_lookup(ieName.c_str()):ieId; }

private:
	std::string getContent(xmlDocPtr doc, xmlNodePtr p);

	bool xmlCompare(const xmlNodePtr node, const std::string& tagName);

	std::string ieName;
	int ieLength;
	int ieId;
};


#endif /*REPORTEDIECFG_H_*/
