#ifndef PRINTERMODULE_H_
#define PRINTERMODULE_H_

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include <core/Module.h>
#include <core/Source.h>
#include <modules/packet/Packet.h>

class PrinterModule
	: public Source<Packet*>, public Destination<Packet*>, public Module
{
public:
	PrinterModule(bool end = true)	: isEndPoint(end), do_print(true), count(0) { } ;
	virtual ~PrinterModule();

	virtual void receive(Packet *);

	void doPrint(bool b)
	{
		do_print = b;
	}

	void setPrefix(std::string s)
	{
		prefix = s;
	}
	
	unsigned int getCount()
	{
		return count;
	}

	void reset()
	{
		count = 0;
	}
private:
	bool isEndPoint;
	bool do_print;
	unsigned int count;
	std::string prefix;
};

#endif /*PRINTERMODULE_H_*/
