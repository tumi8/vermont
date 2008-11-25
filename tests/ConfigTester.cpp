#include "ConfigTester.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstdlib>
#include <cstring>

ConfigTester::ConfigTester()
{
	DIR* dir = opendir("test_configs/");
	if (!dir) {
		ERROR("Could not open config dir");
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
	for (unsigned i = 0; i != configFiles.size(); ++i) {
		testConfig(configFiles[i]);
	}
	return PASSED;
}

void ConfigTester::testConfig(const std::string& configFile)
{
	std::string vermontCommand = "../vermont -ddddd -f test_configs/" + configFile;

	std::string generatedOutput = "gen_output/" + configFile;
	std::string expectedOutput = "exp_output/" + configFile;

	std::string diffCommand = "diff " + generatedOutput + " " + expectedOutput;

	REQUIRE(system(vermontCommand.c_str()) == 0);
	REQUIRE(system(diffCommand.c_str()) == 0);
}
