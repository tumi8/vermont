#ifndef PRINTERMODULE_H_
#define PRINTERMODULE_H_

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include <reconf/Module.h>
#include <reconf/Source.h>
#include <sampler/Packet.h>

class PrinterModule
	: public Source<Packet>, public Destination<Packet>, public Module
{
public:
	PrinterModule(bool end)	: isEndPoint(end) { } ;
	virtual ~PrinterModule();

	virtual void receive(Packet *);

private:
	bool isEndPoint;
};

#endif /*PRINTERMODULE_H_*/
