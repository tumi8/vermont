#include "autofocus_attribute.h"

attribute::attribute(attribute copy)
	{
	m_report = copy->m_report;
	numCount = copy->numCount;
	delta = copy->delta;
	data =  copy->data;
	}

attribute::attribute(report* rep)
	{
	m_report = rep;
	}

void key_payload_tcp::aggregate(IPRecord* te,Connection* conn)
	{
	uint64_t deltacount = ntohll(conn->srcOctets) + ntohll(conn->dstOctets);
	numCount += deltacount;

	m_report->aggregate(deltacount);
	}

void key_payload_udp::aggregate(IPRecord* te, Connection* con)
	{

	}

void key_fanouts::aggregate(IPRecord* te,Connection* conn)
	{

	if ((conn->srcTcpControlBits&Connection::SYN) && (ntohll(conn->srcTimeStart)<ntohll(conn->dstTimeStart)))
		{
		te->numCount++;
		m_report->aggregate(1);	
		}

	}
