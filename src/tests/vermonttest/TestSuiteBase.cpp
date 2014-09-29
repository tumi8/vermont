#include "TestSuiteBase.h"


Test::~Test()
{
}

TestSuite::TestSuite()
{

}

TestSuite::~TestSuite()
{
	for (unsigned i = 0; i != tests.size(); ++i) {
		delete tests[i];
	}
}

void TestSuite::add(Test* t)
{
	tests.push_back(t);
}

void TestSuite::run()
{
	for (unsigned i = 0; i != tests.size(); ++i) {
		tests[i]->execTest();
	}
}
