#include "autofocus_attribute.h"
#include "autofocus_report.h"
#include "autofocus_iprecord.h"
#include <iostream>

af_attribute::af_attribute(report* rep)
{
	m_report = rep;
	numCount = 0;
	delta = 0;
}

//------------------------------------------------------------


atr_simult::~atr_simult() 
{

}


//------------------------------------------------------------

void atr_payload_tcp::aggregate(IPRecord* te,Connection* conn)
{
	if (conn->protocol != 6) return;
	uint64_t deltacount = ntohll(conn->srcOctets) + ntohll(conn->dstOctets);
	numCount += deltacount;
	m_report->aggregate(deltacount);

}

void atr_payload_udp::aggregate(IPRecord* te, Connection* conn)
{
	if (conn->protocol != 17) return;
	uint64_t deltacount = ntohll(conn->srcOctets) + ntohll(conn->dstOctets);
	numCount += deltacount;
	m_report->aggregate(deltacount);

}

void atr_fanouts::aggregate(IPRecord* te,Connection* conn)
{

	if ((conn->srcTcpControlBits&Connection::SYN) && (ntohll(conn->srcTimeStart)<ntohll(conn->dstTimeStart)))
	{
		numCount++;
		m_report->aggregate(1);	
	}

}
void atr_fanins::aggregate(IPRecord* te,Connection* conn)
{

	if ((conn->srcTcpControlBits&Connection::SYN) && (ntohll(conn->srcTimeStart)>ntohll(conn->dstTimeStart)))
	{
		numCount++;
		m_report->aggregate(1);	
	}

}
void atr_packets_tcp::aggregate(IPRecord* te,Connection* conn)
{

	if (conn->protocol != 6) return;
	uint64_t deltacount = ntohll(conn->srcPackets) + ntohll(conn->dstPackets);
	numCount += deltacount;
	m_report->aggregate(deltacount);

}
void atr_packets_udp::aggregate(IPRecord* te,Connection* conn)
{
	if (conn->protocol != 17) return;
	uint64_t deltacount = ntohll(conn->srcPackets) + ntohll(conn->dstPackets);
	numCount += deltacount;
	m_report->aggregate(deltacount);

}

void atr_failed::aggregate(IPRecord* te,Connection* conn)
{

	if ((conn->srcTcpControlBits&Connection::SYN) && ((ntohll(conn->dstPackets) + ntohll(conn->srcPackets) < 5)))
	{
		numCount++;
		m_report->aggregate(1);	
	}

}

void atr_simult::aggregate(IPRecord* te,Connection* conn)
{

	if (find(accessedHosts.begin(), accessedHosts.end(), conn->dstIP) != accessedHosts.end()) return;

	accessedHosts.push_back(conn->dstIP);
	numCount++;
	m_report->aggregate(1);	

}

//------------------------------------------------------------
void atr_payload_udp::test() 
{
	std::cerr << "UDP " << numCount << std::endl;
}
void atr_payload_tcp::test() 
{
	std::cerr << "TCP  " << numCount << std::endl;
}
void atr_fanouts::test() 
{
	std::cerr << "FANOUTS " << numCount << std::endl;
}
void atr_fanins::test() 
{
	std::cerr << "FANINS " << numCount << std::endl;
}
void atr_packets_tcp::test() 
{
	std::cerr << "PACKETS TCP " << numCount << std::endl;
}
void atr_packets_udp::test() 
{
	std::cerr << "PACKETS UDP" << numCount << std::endl;
}
void atr_failed::test() 
{
	std::cerr << "PACKETS FAILED" << numCount << std::endl;
}
void atr_simult::test() 
{
	std::cerr << "SIMULT " << numCount << std::endl;
}

//------------------------------------------------------------

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
void atr_fanouts::collect(af_attribute* a,af_attribute* b) 
{
	numCount = a->numCount + b->numCount;
	delta = a->delta + b->delta;

}
void atr_fanins::collect(af_attribute* a,af_attribute* b) 
{
	numCount = a->numCount + b->numCount;
	delta = a->delta + b->delta;

}
void atr_packets_tcp::collect(af_attribute* a,af_attribute* b) 
{
	numCount = a->numCount + b->numCount;
	delta = a->delta + b->delta;

}
void atr_packets_udp::collect(af_attribute* a,af_attribute* b) 
{
	numCount = a->numCount + b->numCount;
	delta = a->delta + b->delta;

}
void atr_failed::collect(af_attribute* a,af_attribute* b) 
{
	numCount = a->numCount + b->numCount;
	delta = a->delta + b->delta;

}
void atr_simult::collect(af_attribute* a,af_attribute* b) 
{
	numCount = a->numCount + b->numCount;
	delta = a->delta + b->delta;

}

//------------------------------------------------------------

