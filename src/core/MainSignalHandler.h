#ifndef _MAIN_SIGNAL_HANDLER
#define _MAIN_SIGNAL_HANDLER

#include "common/SignalInterface.h"
#include "common/SignalHandler.h"

#include "common/VermontControl.h"
#include "modules/ConfigManager.hpp"

class MainSignalHandler : public SignalInterface 
{

	public:
		MainSignalHandler();
		virtual void handleSigUsr1(int x);
		virtual void handleSigInt(int x);
		virtual void handleSigTerm(int x);
		virtual void handleSigHup(int x);
		virtual void handleSigUsr2(int x);
};
#endif
