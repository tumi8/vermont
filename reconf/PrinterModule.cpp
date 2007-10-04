#include "PrinterModule.h"

PrinterModule::~PrinterModule()
{
	printf("PrinterModule received %u packets!\n", count);
}

void PrinterModule::receive(Packet* p)
{
	if (do_print)
		msg(MSG_FATAL, "packet %d received", p->totalPacketsReceived);
	count++;
	
	if (isEndPoint)
		p->removeReference();
	else
		send(p);
}
