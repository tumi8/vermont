#ifndef __autofocus_report_h__
#define __autofocus_report_h__

#include <list>
#include "autofocus_iprecord.h"

class af_attribute;


struct treeNode;
struct treeRecord;

class report
{
	protected:

		uint32_t minSubbits;
	public:
		uint64_t numTotal;
		std::list<treeNode*> specNodes;
		std::string global;
		std::string local;
		report(uint32_t);
		virtual ~report() {};
		uint64_t getTotal();

		virtual af_attribute* createaf_attribute() =0;
		uint32_t distance(treeNode* a,treeNode* b);
		virtual void aggregate(uint64_t);
		virtual void post(vector<treeRecord*>& ,uint32_t)=0;
		virtual void checkNode(treeNode*,uint32_t);
		treeNode* getComparismValue(treeNode*,vector<treeRecord*>& ,uint32_t);
		void	f_post(vector<treeRecord*>& p_treeRecords,uint32_t index,report_enum attribute);
		virtual report_enum getID()=0;
};


class rep_payload_tcp : public report
{
	public:
		rep_payload_tcp(uint32_t bits):report(bits)

	{
		global = "Datavolume TCP";
		local = "Volume";
	}
		af_attribute* createaf_attribute();
		void post(vector<treeRecord*>& ,uint32_t);
		report_enum getID();
};

class rep_payload_udp :public report
{
	public:
		rep_payload_udp(uint32_t bits):report(bits)
	{
		global = "Datavolume UDP";
		local = "Volume";
	}
		af_attribute* createaf_attribute();
		void post(vector<treeRecord*>& ,uint32_t);
		report_enum getID();

};

class rep_fanouts :public report
{
	public:
		rep_fanouts(uint32_t bits):report(bits)
	{
		global = "Outgoing Connections";
		local = "Connections";
	}
		af_attribute* createaf_attribute();
		void post(vector<treeRecord*>& ,uint32_t);
		report_enum getID();

};

class rep_fanins :public report
{
	public:
		rep_fanins(uint32_t bits):report(bits)
	{
		global = "Incoming Connections";
		local = "Connections";
	}

		af_attribute* createaf_attribute();
		void post(vector<treeRecord*>& ,uint32_t);
		report_enum getID();

};
class rep_packets_tcp :public report
{
	public:
		rep_packets_tcp(uint32_t bits):report(bits)
	{
		global = "Packetcount TCP";
		local = "Packets";

	}
		af_attribute* createaf_attribute();
		void post(vector<treeRecord*>& ,uint32_t);
		report_enum getID();

};
class rep_packets_udp :public report
{
	public:
		rep_packets_udp(uint32_t bits):report(bits)
	{
		global = "Packetcount UDP";
		local = "Packets";
	}
		af_attribute* createaf_attribute();
		void post(vector<treeRecord*>& ,uint32_t);
		report_enum getID();

};
class rep_failed :public report
{
	public:
		rep_failed(uint32_t bits):report(bits)
	{
		global = "Failed Connectons";
		local = "Failed";
	}
		af_attribute* createaf_attribute();
		void post(vector<treeRecord*>& ,uint32_t);
		report_enum getID();

};
class rep_simult :public report
{
	public:
		rep_simult(uint32_t bits):report(bits)
	{
		global = "Simultanous Connections";
		local = "Connections";
	}
		af_attribute* createaf_attribute();
		void post(vector<treeRecord*>& ,uint32_t);
		report_enum getID();

};
#endif
