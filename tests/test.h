
#include "common/msg.h"


#define ERROR(fmt, args...) vermont_exception(__LINE__, __FILE__, __PRETTY_FUNCTION__, __func__, fmt, ##args)

// redefine assert, so that ASSERT is always properly defined in unit tests
#undef ASSERT
#define ASSERT(cond, description)																	\
	{																								\
		if (!(cond)) {																				\
			vermont_assert(#cond, (description), __LINE__, __FILE__, __PRETTY_FUNCTION__, __func__);\
		}																							\
	}

#define REQUIRE(cond)																				\
	{																								\
		if (!(cond)) {																				\
			vermont_assert(#cond, "", __LINE__, __FILE__, __PRETTY_FUNCTION__, __func__);			\
		}																							\
	}
