#ifndef __autofocus_iprecord_h
#define __autofocus_iprecord_h

#include <map>
#include <list>
#include "autofocus_attribute.h"

#define THRESHOLD 40

class report;
class af_attribute;

/*
 * IPRecord Container
 *
 */
enum report_enum { payload_tcp, payload_udp, fanouts, fanins, packets_tcp, packets_udp, failed, simult, NUM_REPORTS };


typedef struct IPRecord {
	uint32_t subnetIP;
	uint32_t subnetBits;
	std::map<report_enum,af_attribute*> m_attributes;
}IPRecord;

typedef	struct treeNode {
	IPRecord data;
	uint32_t prio;
	treeNode* left;
	treeNode* right;
}treeNode;

typedef struct treeRecord {
	treeNode* root;
	std::list<report*> reports;
}treeRecord;
#endif
