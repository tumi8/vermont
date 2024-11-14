#ifndef _CONFIG_TESTER_H_
#define _CONFIG_TESTER_H_

#include "TestSuiteBase.h"

#include <vector>
#include <string>

class ConfigTester : public Test
{
public:
	ConfigTester(std::string config_dir);
	~ConfigTester();
	virtual TestResult execTest();

private:
	void test_Rules_parseProtoPattern();
	void testConfig(const std::string& configFile);
	std::string config_dir;
	std::vector<std::string> configFiles;
};

#endif
