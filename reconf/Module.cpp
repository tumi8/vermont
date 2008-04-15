#include "Module.h"

#include "common/VermontControl.h"

#include <signal.h>

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

void Module::notifyShutdown(bool shutdownProperly)
{
	this->shutdownProperly = shutdownProperly; 
	exitFlag = true;
}

void Module::shutdown(bool fail_if_not_running, bool shutdownProperly)
{
	if (!running && !fail_if_not_running)
		return;

	ASSERT(running, "module must be in state running when it is shut down");
	
	notifyShutdown(shutdownProperly);
	performShutdown();
	running = false;
}

bool Module::getExitFlag() const
{
	return exitFlag;
}

bool Module::getShutdownProperly() const
{
	return shutdownProperly;
}

void Module::shutdownVermont()
{
	DPRINTF("initiating Vermont shutdown");
	::initiateShutdown();
}


