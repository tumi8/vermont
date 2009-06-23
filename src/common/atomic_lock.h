#if !defined(ATOMIC_LOCK_H)
#define ATOMIC_LOCK_H

#include "Mutex.h"

#include <stdint.h>


#if defined(__linux__) && (__GNUC__ >= 4)

typedef uint32_t alock_t;
#define atomic_lock(a) __sync_lock_test_and_set(a, 1)
#define atomic_release(a) __sync_lock_release(a)

#else

struct alock_t {
	uint32_t value;
	Mutex mutex;

	alock_t() : value(0) {}
	alock_t(uint32_t v) : value(v) {}
};

inline uint32_t atomic_lock(alock_t* a)
{
	a->mutex.lock();
	if (a->value==0) {
		a->value = 1;
		a->mutex.unlock();
		return 0;
	}
	a->mutex.unlock();
	return 1;
}

inline void atomic_release(alock_t* a)
{
	a->mutex.lock();
	a->value = 0;
	a->mutex.unlock();
}

#endif // __linux__


#endif
