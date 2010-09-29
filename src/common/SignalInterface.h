#ifndef _SIGNALINTERFACE_
#define _SIGNALINTERFACE_


class SignalInterface 
{
	public:
		SignalInterface() {}
		virtual void handleSigAlrm(int sig) {}
		virtual void handleSigChld(int sig) {}
		virtual void handleSigHup(int sig) {}
		virtual void handleSigInt(int sig) {}
		virtual void handleSigPipe(int sig) {}
		virtual void handleSigQuit(int sig) {}
		virtual void handleSigTerm(int sig) {}
		virtual void handleSigUsr1(int sig) {}
		virtual void handleSigUsr2(int sig) {}
};
#endif
