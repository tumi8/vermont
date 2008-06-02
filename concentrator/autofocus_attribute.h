#ifndef __autofocus_af_attribute_h__ 
#define __autofocus_af_attribute_h__ 

#include "Connection.h"
#include "autofocus_iprecord.h"


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
		af_attribute() {};
		af_attribute(report*);
		virtual void aggregate(IPRecord*,Connection*) =0 ;
		virtual void collect(af_attribute*,af_attribute*)=0;
		virtual af_attribute* getCopy()=0;

};

class atr_payload_tcp : public af_attribute
{
	public:
		atr_payload_tcp(report*){};
		void collect(af_attribute*,af_attribute*);
		void aggregate(IPRecord*,Connection*);
		af_attribute* getCopy();
};

class atr_payload_udp : public af_attribute
{
	public:
		atr_payload_udp(report*){};
		void aggregate(IPRecord*,Connection*);
		void collect(af_attribute*,af_attribute*);
		af_attribute* getCopy();
};
class atr_fanouts : public af_attribute
{
	public:
		atr_fanouts(report*){};
		void aggregate(IPRecord*,Connection*);
		void collect(af_attribute*,af_attribute*);
		af_attribute* getCopy();
};

#endif
