#ifndef __autofocus_attribute_h__ 
#define __autofocus_attribute_h__ 

#include "Connection.h"
#include "autofocus_iprecord.h"


class report;
class AutoFocus;
struct IPRecord;

class attribute
{

	protected:
		report* m_report;

	public:
		uint64_t numCount;
		uint64_t delta;
		attribute();
		attribute(report*);
		virtual void aggregate(IPRecord*,Connection*) =0 ;
		virtual void collect(attribute*,attribute*)=0;
		virtual attribute* getCopy()=0;

};

class atr_payload_tcp : public attribute
{
	public:
		atr_payload_tcp(report*){};
		void collect(attribute*,attribute*);
		void aggregate(IPRecord*,Connection*);
		attribute* getCopy();
};

class atr_payload_udp : public attribute
{
	public:
		atr_payload_udp(report*){};
		void aggregate(IPRecord*,Connection*);
		void collect(attribute*,attribute*);
		attribute* getCopy();
};
class atr_fanouts : public attribute
{
	public:
		atr_fanouts(report*){};
		void aggregate(IPRecord*,Connection*);
		void collect(attribute*,attribute*);
		attribute* getCopy();
};

#endif
