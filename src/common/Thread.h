/*
 * PSAMP Reference Implementation
 *
 * Thread.h
 *
 * Implementation of a base class for a thread
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef THREAD_H
#define THREAD_H

#include "msg.h"
#include <string.h>
#include <pthread.h>

#define MAX_THREAD_NAME_LEN 15

typedef void* (*ThreadFunc)(void *);

class Thread
{
	public:
		Thread(ThreadFunc threadFunction, const char *threadName="") : exitFlag(false), thread_created(false), f(threadFunction)
		{
			size_t threadNameLen = strlen(threadName);
			// truncate to 15 chars + NULL byte
			if (threadNameLen > MAX_THREAD_NAME_LEN) {
				msg(MSG_ERROR, "truncating thread name %s to %d characters", threadName, MAX_THREAD_NAME_LEN);
				threadNameLen = MAX_THREAD_NAME_LEN;
			}
			memcpy(name, threadName, threadNameLen);
			name[threadNameLen] = '\0';
		};

		void run(void *threadData)
		{
			// remember that run() has been called
			thread_created = true;

			//data = threadData;
			msg(MSG_DEBUG, "creating new thread: %s", name);
			if (pthread_create(&thread, NULL, f, threadData) != 0) {
				THROWEXCEPTION("failed to create new thread");
			}

			// Set thread name if specified
			if (strlen(name) > 0) {
				if (pthread_setname_np(thread, name) != 0) {
					THROWEXCEPTION("failed to set thread name");
				}
			}
		};

		void *join()
		{
			// do not attempt to join if run() had not yet been called
			if (!thread_created) return 0;

			void *result=NULL;
			if(!thread || pthread_join(thread, &result)) {
				msg(MSG_ERROR, "joining failed");
			}
			thread_created = false;

			return result;
		};

		bool detach()
		{
			// do not attempt to detach if run() had not yet been called
			if (!thread_created) return 0;

			bool result = (pthread_detach(thread) == 0);
			thread_created = false;

			return result;
		}

		~Thread()
		{
		};

		bool exitFlag;
		//void *data;

	private:
		pthread_t thread;
		bool thread_created; /**< true after Thread::run() was called */
		ThreadFunc f;
		char name[MAX_THREAD_NAME_LEN+1];
};

#endif
