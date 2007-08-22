#include "PrinterModule.h"

PrinterModule::~PrinterModule()
{
}

void PrinterModule::receive(Packet* p)
{
	msg(MSG_INFO, "packet %d received", p->totalPacketsReceived);
	
	if (isEndPoint)
		p->removeReference();
	else
		send(p);
}
