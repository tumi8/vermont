#ifndef __autofocus_report.h__
#define __autofocus_report.h__


class report 
{

	protected:

		uint64_t numTotal;
		std::list<treeNode*> reportNodes;
	public:
	
		report();
		~report();
		uint64_t getTotal();
		virtual attribute createAttribute() =0;
		virtual aggregate(uint64_t);
		virtual post()=0;
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
