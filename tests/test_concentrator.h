#if !defined(TESTCONCENTRATOR_H)
#define TESTCONCENTRATOR_H

#include <boost/test/unit_test.hpp>

using boost::unit_test::test_suite;


class ConcentratorTestSuite : public test_suite
{
	public:
		ConcentratorTestSuite();
};

#endif
