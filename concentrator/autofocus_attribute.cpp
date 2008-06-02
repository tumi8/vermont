#include "autofocus_attribute.h"

key_payload_tcp::aggregate(IPRecord* te,Connection* conn)
{

	uint64_t deltacount = ntohll(conn->srcOctets) + ntohll(conn->dstOctets);
	numCount += deltacount;

	m_report->aggregate(deltacount);
}

key_payload_udp::aggregate(IPRecord* te, Connection* con)
{

}

key_fanouts::aggregate(IPRecord* te,Connection* conn)
{

	if ((conn->srcTcpControlBits&Connection::SYN) && (ntohll(conn->srcTimeStart)<ntohll(conn->dstTimeStart)))
	{
		te->numCount++;
		m_report->aggregate(1);	
	}

}
