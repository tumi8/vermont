#include "Module.h"


Module::Module() 
	: exitFlag(false), running(false)
{ 
};


Module::~Module()
{
	if (running) {
		THROWEXCEPTION("you must shutdown the module first before destroying it!");
	}
}

void Module::start(bool fail_if_already_running)
{
	if (running && !fail_if_already_running)
		return;

	ASSERT(!running, "module must not be in state 'running' when started");
	running = true;
	exitFlag = false;
	performStart();
}

void Module::notifyShutdown()
{
	exitFlag = true;
}

void Module::shutdown(bool fail_if_not_running)
{
	if (!running && !fail_if_not_running)
		return;

	ASSERT(running, "module must be in state running when it is shut down");
	
	notifyShutdown();
	performShutdown();
	running = false;
}

bool Module::getExitFlag() const
{
	return exitFlag;
}



