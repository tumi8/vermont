#ifndef _CONFIG_TESTER_H_
#define _CONFIG_TESTER_H_

#include "TestSuiteBase.h"

#include <vector>
#include <string>

class ConfigTester : public Test
{
public:
	ConfigTester();
	~ConfigTester();
	virtual TestResult execTest();

private:
	void testConfig(const std::string& configFile);
	std::vector<std::string> configFiles;
};

#endif
