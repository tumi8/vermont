#ifndef MODULE_H
#define MODULE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "Source.h"

class Module : public virtual BaseSource, public virtual BaseDestination {
public:	
	Module() : exitFlag(false) { };

	void shutdown()
	{
		exitFlag = true;
	}

protected:
	bool exitFlag;
};

#endif

