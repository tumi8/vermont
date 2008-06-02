#ifndef __autofocus_report.h__
#define __autofocus_report.h__

#include <list>

class attribute;

struct treeNode;

class report 
{
	protected:

		uint64_t numTotal;
		std::list<treeNode*> reportNodes;
	public:
		enum report_enum { payload_tcp, payload_udp, fanouts, NUM_REPORTS };
		report();
		~report();
		uint64_t getTotal();
		virtual attribute createAttribute() =0;
		virtual aggregate(uint64_t);
		virtual post()=0;
		virtual checkNode(treeNode*);
		virtual report_enum getID()=0;
};


class rep_payload_tcp : public report
{

};

class rep_payload_udp :public report
{

};

class rep_fanouts :public report
{

};
#endif
