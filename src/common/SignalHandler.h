#include <map>
#include <list>
#include <signal.h>
#include <utility>
#include <pthread.h>

#include "common/msg.h"
#include "SignalInterface.h"

/**
 * Provides Signal handling for Vermont modules
 * If a module needs to respond to certain signals,
 * it must be derived from SignalInterface a overwrite
 * the appropriate methods for the signals to be handled
 */ 
class SignalHandler {
	private: 
		SignalHandler();
		virtual ~SignalHandler() {} 
		SignalHandler(const SignalHandler &);             /**< intentionally undefined */
		SignalHandler & operator=(const SignalHandler &); /**< intentionally undefined */
		int setupSignal(int signal);

		static void handleSignalWrapper(int sig);

		/**
		 * this map holds a list for every registered signal
		 * each list element points to a SignalInterface object
		 */
		std::map<int, std::list<SignalInterface* > > signalList;

		pthread_mutex_t mutex;


	public:
		static SignalHandler &getInstance();
		int registerSignalHandler(int signal, SignalInterface *t);
		int unregisterSignalHandler(int signal, SignalInterface *t);
		void handleSignal(int sig);
};

