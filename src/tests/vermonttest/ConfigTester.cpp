#include "ConfigTester.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include "modules/ipfix/aggregator/Rules.hpp"

ConfigTester::ConfigTester(std::string config_dir)
	: config_dir{config_dir}
{
	DIR* dir = opendir(config_dir.c_str());
	if (!dir) {
		ERROR("Could not open config dir %s", config_dir.c_str());
	}

	struct dirent* d;
	while ((d = readdir(dir))) {
		if (!strncmp(".", d->d_name, 1) || !strncmp("..", d->d_name, 2))
			continue;
		configFiles.push_back(d->d_name);
	}
	closedir(dir);
	mkdir("gen_output", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

ConfigTester::~ConfigTester()
{

}

Test::TestResult ConfigTester::execTest()
{
	test_Rules_parseProtoPattern();
	// TODO: The following tests need to be fixed before we can reenable them.
	/*
	for (unsigned i = 0; i != configFiles.size(); ++i) {
		testConfig(configFiles[i]);
	}
	*/
	return PASSED;
}

void ConfigTester::testConfig(const std::string& configFile)
{
	std::string vermontCommand = "../../../vermont -l debug -f " + config_dir + "/" + configFile;

	std::string generatedOutput = "gen_output/" + configFile;
	std::string expectedOutput = "exp_output/" + configFile;

	std::string diffCommand = "diff " + generatedOutput + " " + expectedOutput;

	REQUIRE(system(vermontCommand.c_str()) == 0);
	REQUIRE(system(diffCommand.c_str()) == 0);
}

void ConfigTester::test_Rules_parseProtoPattern()
{
	std::map<std::string, int> testPatterns;
	testPatterns["ICMP"] = 0;
	testPatterns["TCP" ] = 0;
	testPatterns["UDP"] = 0;
	testPatterns["RAW"] = 0;
	testPatterns["17"] = 0;
	testPatterns["-1"] = -1;
	testPatterns["256"] = -1;
	testPatterns["SOMETHING_INVALID"] = -1;
	testPatterns[""] = -1;
	testPatterns["155SOMETHING_INVALID"] = -1;
	for (std::map<std::string, int>::iterator i = testPatterns.begin(); i != testPatterns.end(); ++i) {
		IpfixRecord::Data* d;
		InformationElement::IeLength l;
		int ret =  parseProtoPattern(i->first.c_str(), &d, &l);
		if (ret != -1)
			free(d);
		REQUIRE(ret == i->second);
	}
}	
