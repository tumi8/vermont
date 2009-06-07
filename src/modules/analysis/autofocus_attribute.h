#ifndef __autofocus_af_attribute_h__ 
#define __autofocus_af_attribute_h__ 

#include "modules/ipfix/Connection.h"
#include "autofocus_iprecord.h"
#include <list>

class report;
class AutoFocus;
struct IPRecord;

class af_attribute
{

	protected:
		report* m_report;

	public:
		uint64_t numCount;
		uint64_t delta;
		virtual ~af_attribute() {};
		af_attribute() {};
		af_attribute(report*);
		virtual void test()=0;
		virtual void aggregate(IPRecord*,Connection*) =0 ;
		virtual void collect(af_attribute*,af_attribute*)=0;

};

class atr_payload_tcp : public af_attribute
{
	public:
		atr_payload_tcp(report* r):af_attribute(r){};
		void collect(af_attribute*,af_attribute*);
		void aggregate(IPRecord*,Connection*);
		void test();
};

class atr_payload_udp : public af_attribute
{
	public:
		atr_payload_udp(report* r):af_attribute(r){};
		void aggregate(IPRecord*,Connection*);
		void collect(af_attribute*,af_attribute*);
		void test();
};
class atr_fanouts : public af_attribute
{
	public:
		atr_fanouts(report* r):af_attribute(r){};
		void aggregate(IPRecord*,Connection*);
		void collect(af_attribute*,af_attribute*);
		void test();
};
class atr_fanins : public af_attribute
{
	public:
		atr_fanins(report* r):af_attribute(r){};
		void aggregate(IPRecord*,Connection*);
		void collect(af_attribute*,af_attribute*);
		void test();
};
class atr_packets_tcp : public af_attribute
{
	public:
		atr_packets_tcp(report* r):af_attribute(r){};
		void aggregate(IPRecord*,Connection*);
		void collect(af_attribute*,af_attribute*);
		void test();
};
class atr_packets_udp : public af_attribute
{
	public:
		atr_packets_udp(report* r):af_attribute(r){};
		void aggregate(IPRecord*,Connection*);
		void collect(af_attribute*,af_attribute*);
		void test();
};
class atr_failed : public af_attribute
{
	public:
		atr_failed(report* r):af_attribute(r){};
		void aggregate(IPRecord*,Connection*);
		void collect(af_attribute*,af_attribute*);
		void test();
};
class atr_simult : public af_attribute
{
	private:
		list<uint32_t> accessedHosts;
	public:	
		~atr_simult();
		atr_simult(report* r):af_attribute(r){};
		void aggregate(IPRecord*,Connection*);
		void collect(af_attribute*,af_attribute*);
		void test();
};

#endif
