#ifdef HAVE_CONNECTION_FILTER

#ifndef _CONNECTION_FILTER_TEST_H_
#define _CONNECTION_FILTER_TEST_H_

#include <boost/test/unit_test.hpp>

using boost::unit_test::test_suite;


class ConnectionFilterTestSuite : public test_suite
{
	public:
		ConnectionFilterTestSuite();
};

#endif

#endif
