#ifndef _TEST_SUITE_BASE_H_
#define _TEST_SUITE_BASE_H_

#include "common/msg.h"

#include <vector>


#define ERROR(...) vermont_exception(__LINE__, __FILE__, __PRETTY_FUNCTION__, __func__, ##__VA_ARGS__)

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

class Test {
public:
	typedef enum {
		FAILED,
		PASSED
	} TestResult;
	virtual TestResult execTest() = 0;
	virtual ~Test();
};

class TestSuite {
public:
	TestSuite();
	~TestSuite();
	void add(Test* test);
	void run();

private:
	std::vector<Test*> tests; 
};


#endif
