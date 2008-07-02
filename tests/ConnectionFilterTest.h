#ifdef HAVE_CONNECTION_FILTER

#ifndef _CONNECTION_FILTER_TEST_H_
#define _CONNECTION_FILTER_TEST_H_

#include "TestSuiteBase.h"

class ConnectionFilterTestSuite : public Test
{
public:
	ConnectionFilterTestSuite();
	virtual TestResult execTest();
};

#endif

#endif
