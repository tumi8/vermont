#include "PrinterModule.h"

PrinterModule::~PrinterModule()
{
	printf("PrinterModule received %u packets!\n", count);
}

void PrinterModule::receive(Packet* p)
{
	if (do_print)
		msg(MSG_FATAL, "%s: packet %d received", prefix.c_str(), p->totalPacketsReceived);
	count++;
	
	if (isEndPoint) {
		p->removeReference();
	} else
		send(p);
}
