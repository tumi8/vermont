#include "autofocus_af_attribute.h"
#include "autofocus_report.h"
#include "autofocus_iprecord.h"

af_attribute::af_attribute(report* rep)
	{
	m_report = rep;
	numCount = 0;
	delta = 0;
	}

void atr_payload_tcp::aggregate(IPRecord* te,Connection* conn)
	{
	uint64_t deltacount = ntohll(conn->srcOctets) + ntohll(conn->dstOctets);
	numCount += deltacount;

	m_report->aggregate(deltacount);
	}

void atr_payload_udp::aggregate(IPRecord* te, Connection* con)
	{

	}

void atr_fanouts::aggregate(IPRecord* te,Connection* conn)
	{

	if ((conn->srcTcpControlBits&Connection::SYN) && (ntohll(conn->srcTimeStart)<ntohll(conn->dstTimeStart)))
		{
		numCount++;
		m_report->aggregate(1);	
		}

	}

void atr_fanouts::collect(af_attribute* a,af_attribute* b) 
{
	numCount = a->numCount + b->numCount;
	delta = a->delta + b->delta;
	
}
void atr_payload_tcp::collect(af_attribute* a,af_attribute* b) 
{
	numCount = a->numCount + b->numCount;
	delta = a->delta + b->delta;
	
}
void atr_payload_udp::collect(af_attribute* a,af_attribute* b) 
{
	numCount = a->numCount + b->numCount;
	delta = a->delta + b->delta;
	
}

af_attribute* atr_payload_tcp::getCopy()
{
	atr_payload_tcp* ret =  new atr_payload_tcp(m_report);
	ret->numCount = numCount;
	ret->delta = delta;
	return ret;
}
af_attribute* atr_payload_udp::getCopy()
{
	atr_payload_udp* ret =  new atr_payload_udp(m_report);
	ret->numCount = numCount;
	ret->delta = delta;
	return ret;
}
af_attribute* atr_fanouts::getCopy()
{
	atr_fanouts* ret =  new atr_fanouts(m_report);
	ret->numCount = numCount;
	ret->delta = delta;
	return ret;
}
