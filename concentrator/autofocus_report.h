#ifndef __autofocus_report_h__
#define __autofocus_report_h__

#include <list>
#include "autofocus_iprecord.h"

class attribute;


struct treeNode;
struct treeRecord;

class report 
{
	protected:

		uint64_t numTotal;
		std::list<treeNode*> specNodes;
	public:
		report();
		~report();
		uint64_t getTotal();

		virtual attribute* createAttribute() =0;

		virtual void aggregate(uint64_t);
		virtual void post(treeRecord*,treeRecord*,uint32_t)=0;
		virtual void checkNode(treeNode*,uint32_t);
		virtual report_enum getID()=0;
};


class rep_payload_tcp : public report
{
		attribute* createAttribute();
		void post(treeRecord*,treeRecord*,uint32_t);
		report_enum getID();
};

class rep_payload_udp :public report
{
		attribute* createAttribute();
		void post(treeRecord*,treeRecord*,uint32_t);
		report_enum getID();

};

class rep_fanouts :public report
{
		attribute* createAttribute();
		void post(treeRecord*,treeRecord*,uint32_t);
		report_enum getID();

};
#endif
