#ifdef HAVE_CONNECTION_FILTER

#ifndef _BLOOMFILTER_TEST_H_
#define _BLOOMFILTER_TEST_H_

#include <boost/test/unit_test.hpp>

using boost::unit_test::test_suite;


class BloomFilterTestSuite : public test_suite
{
	public:
		BloomFilterTestSuite();
};


#endif

#endif
