#ifdef HAVE_CONNECTION_FILTER

#ifndef _BLOOMFILTER_TEST_H_
#define _BLOOMFILTER_TEST_H_

#include "TestSuiteBase.h"

class BloomFilterTestSuite : public Test
{
	public:
		BloomFilterTestSuite();
		virtual TestResult execTest();
};


#endif

#endif
