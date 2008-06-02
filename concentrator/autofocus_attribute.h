#ifndef __autofochs_attribute.h__
#define __autofochs_attribute.h__

#include "Connection.h"
#include "autofocus_report.h"

class report;

class attribute
{

	protected:
		uint64_t numCount;
		report* m_report;
		uint64_t delta;
		uint64_t data;

	public:

		virtual attribute(report*);
		virtual attribute(attribute*);
		virtual void aggregate(Connection) =0 ;

}

class atr_payload_tcp : public attribute
{

};

class atr_payloard_udp : public attribute
{

};
class atr_fanouts : public attribute
{

};
#endif
