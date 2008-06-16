#ifdef HAVE_CONNECTION_FILTER

#include "BloomFilterTest.h"
#include <common/BloomFilter.h>
#include <common/AgeBloomFilter.h>
#include <common/CountBloomFilter.h>
#include <common/msg.h>

#include <iostream>

#include <ctime>

static void startTests();

static QuintupleKey key1;
static QuintupleKey key2;

BloomFilterTestSuite::BloomFilterTestSuite()
        : test_suite("BloomFilterTest")
{
        add(BOOST_TEST_CASE(&startTests));
}

static void setupGlobalKey()
{
	key1.getQuintuple()->srcIp = 12345678; //
	key1.getQuintuple()->dstIp = 87654321; //
	key1.getQuintuple()->proto = 17;
	key1.getQuintuple()->srcPort = 11234;
	key1.getQuintuple()->dstPort = 80;

	key2.getQuintuple()->srcIp = 134567832;
	key2.getQuintuple()->dstIp = 123543688;
	key2.getQuintuple()->proto = 17;
	key1.getQuintuple()->srcPort = 12323;
	key1.getQuintuple()->dstPort = 32432;

}

static void testBloomFilter()
{
	HashParams hashParams(10);
	BloomFilter* bf = new BloomFilter(&hashParams, 1000);

	BOOST_REQUIRE(bf->get(key1.data, key1.len) == false);
	BOOST_REQUIRE(bf->get(key1.data, key1.len) == false);

	bf->set(key1.data, key1.len);
	BOOST_REQUIRE(bf->get(key1.data, key1.len) == true);
	
	BOOST_REQUIRE(bf->get(key2.data, key2.len) == false);
	bf->set(key2.data, key2.len);
	BOOST_REQUIRE(bf->get(key2.data, key2.len) == true);
	BOOST_REQUIRE(bf->get(key1.data, key1.len) == true);

	delete bf;
}

static void testCountBloomFilter()
{
	HashParams hashParams(10);
	CountBloomFilter* bf = new CountBloomFilter(&hashParams, 1000);

	std::cout << "bf(key1) == " << bf->get(key1.data, key1.len) << std::endl;
        BOOST_REQUIRE(bf->get(key1.data, key1.len) == 0);
        std::cout << "bf(key2) == " << bf->get(key2.data, key2.len) << std::endl;
        BOOST_REQUIRE(bf->get(key2.data, key2.len) == 0);
	
        bf->set(key1.data, key1.len, 100);
	std::cout << "bf(key1) == " << bf->get(key1.data, key1.len) << std::endl;
	std::cout << "bf(key2) == " << bf->get(key2.data, key2.len) << std::endl;
        BOOST_REQUIRE(bf->get(key1.data, key1.len) == 100);
        BOOST_REQUIRE(bf->get(key2.data, key2.len) == 0);
	
        bf->set(key2.data, key2.len, 1000);
	std::cout << "bf(key1) == " << bf->get(key1.data, key1.len) << std::endl;
	std::cout << "bf(key2) == " << bf->get(key2.data, key2.len) << std::endl;
        BOOST_REQUIRE(bf->get(key2.data, key2.len) == 1000);
        BOOST_REQUIRE(bf->get(key1.data, key1.len) == 100);

	bf->set(key1.data, key1.len, 100);
	std::cout << "bf(key1) == " << bf->get(key1.data, key1.len) << std::endl;
	BOOST_REQUIRE(bf->get(key1.data, key1.len) == 200);

	bf->set(key1.data, key1.len, -200);
	std::cout << "bf(key1) == " << bf->get(key1.data, key1.len) << std::endl;
	BOOST_REQUIRE(bf->get(key1.data, key1.len) == 0);	

	bf->set(key2.data, key1.len, -1000);
	std::cout << "bf(key2) == " << bf->get(key2.data, key2.len) << std::endl;
	BOOST_REQUIRE(bf->get(key2.data, key2.len) == 0);

	delete bf;
}

static void testAgeBloomFilter()
{
	HashParams hashParams(10);
	AgeBloomFilter* bf = new AgeBloomFilter(&hashParams, 1000);

	time_t now = time(NULL);
	time_t later = now + 10;

        BOOST_REQUIRE(bf->get(key1.data, key1.len) == 0);
        BOOST_REQUIRE(bf->get(key1.data, key1.len) == 0);

        bf->set(key1.data, key1.len, now);
        BOOST_REQUIRE(bf->get(key1.data, key1.len) == now);
        BOOST_REQUIRE(bf->get(key2.data, key2.len) == 0);

        bf->set(key2.data, key2.len, later);
        BOOST_REQUIRE(bf->get(key2.data, key2.len) == later);
        BOOST_REQUIRE(bf->get(key1.data, key1.len) == now);	

	BOOST_REQUIRE(bf->get(key2.data, key2.len) > bf->get(key1.data, key1.len));

	delete bf;
}

static void startTests()
{
	std::cout << "Running tests on BloomFilter classes" << std::endl;
	msg_init();
	msg_setlevel(100);
	setupGlobalKey();

	std::cout << "Testing BloomFilter..." << std::endl;
	testBloomFilter();
	
	std::cout << "Testing AgeBloomFilter..." << std::endl;
	testAgeBloomFilter();

	std::cout << "Testing CountBloomFilter..." << std::endl;
	testCountBloomFilter();

	std::cout << "All tests on all BloomFilter classes passed" << std::endl;
}

#endif
