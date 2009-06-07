#if !defined(TESTCONCENTRATOR_H)
#define TESTCONCENTRATOR_H

#include "TestSuiteBase.h"

class ConcentratorTestSuite : public Test
{
	public:
		ConcentratorTestSuite();
		
		virtual TestResult execTest();
};

#endif
