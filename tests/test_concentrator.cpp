#ifdef HAVE_BOOST_UNIT_TEST_FRAMEWORK

#include <iostream>
#include <vector>
#include <boost/test/minimal.hpp>

#include "concentrator/IpfixRecord.hpp"
#include "concentrator/FlowSource.hpp"
#include "concentrator/FlowSink.hpp"

class TestSink : public FlowSink {
	public:
		int onTemplate(IpfixRecord::SourceID* sourceID, const IpfixRecord::TemplateInfo* templateInfo) {
		}

		int onDataRecord(IpfixRecord::SourceID* sourceID, const IpfixRecord::TemplateInfo* templateInfo, uint16_t length, IpfixRecord::Data* data) {
			uint8_t inSourceID = sourceID->exporterAddress.ip[0];
			uint8_t inTemplateId = templateInfo->templateId;
			uint8_t inTypeId = templateInfo->fieldInfo[0].type.id;
			uint8_t inData = data[0];
			if (inSourceID != inTemplateId) BOOST_ERROR("SourceID or TemplateInfo got corrupted: inSourceID != inTemplateId");
			if (inTemplateId != inTypeId) BOOST_ERROR("TemplateInfo got corrupted: inTemplateId != inTypeId");
			if (inData != inTemplateId) BOOST_ERROR("IpfixRecord got corrupted: inData != inTemplateId");
			if (inData != inTypeId) BOOST_ERROR("IpfixRecord got corrupted: inData != inTypeId");
			for (int i = 0; i < rand(); i++);
		}

		int onTemplateDestruction(IpfixRecord::SourceID* sourceID, const IpfixRecord::TemplateInfo* templateInfo) {
		}

		int getQueueLength() {
			return ipfixRecords.getCount();
		}

};

boost::shared_ptr<IpfixRecord::TemplateInfo> createTestTemplate(uint8_t magic_number) {
		boost::shared_ptr<IpfixRecord::TemplateInfo> testTemplate(new IpfixRecord::TemplateInfo);
		testTemplate->templateId = magic_number;
		testTemplate->fieldCount = 1;
		testTemplate->fieldInfo = (IpfixRecord::FieldInfo*)malloc(testTemplate->fieldCount * sizeof(IpfixRecord::FieldInfo));
		testTemplate->fieldInfo[0].type.id = magic_number;
		testTemplate->fieldInfo[0].type.length = 1;
		testTemplate->fieldInfo[0].type.eid = 0;
		testTemplate->fieldInfo[0].offset = 0;
		testTemplate->userData = 0;

		return testTemplate;
}

boost::shared_ptr<IpfixRecord::SourceID> createTestSourceId(uint8_t magic_number) {
		boost::shared_ptr<IpfixRecord::SourceID> testSourceId(new IpfixRecord::SourceID);
		testSourceId->observationDomainId = 0;
		testSourceId->exporterAddress.ip[0] = magic_number;
		testSourceId->exporterAddress.len = 1;

		return testSourceId;
}
		
boost::shared_array<uint8_t> createTestData(uint8_t magic_number) {
		boost::shared_array<uint8_t> testData(new uint8_t[1]);
		testData.get()[0] = magic_number;
		return testData;
}

boost::shared_ptr<IpfixDataRecord> createTestRecord(uint8_t magic_number) {
		boost::shared_ptr<IpfixDataRecord> testRecord(new IpfixDataRecord);
		testRecord->sourceID = createTestSourceId(magic_number);
		testRecord->templateInfo = createTestTemplate(magic_number);
		testRecord->dataLength = 1;
		testRecord->message = createTestData(magic_number);
		testRecord->data = testRecord->message.get();

		return testRecord;
}

void test_module_coupling() {
	std::cout << "Testing: Concentrator module coupling..." << std::endl;

	// create testSink
	TestSink testSink;
	testSink.runSink();

	// create some test data
	std::vector<boost::shared_ptr<IpfixRecord> > testRecords;
	for (uint8_t magic_number = 0; magic_number < 255; magic_number++) {
		testRecords.push_back(createTestRecord(magic_number));	
	}

	// perform test
	for (uint8_t magic_number = 0; magic_number < 64; magic_number++) {
		testSink.push(testRecords[magic_number]);
	}
	for (int i = 0; i < 128; i++) {
		for (int i = 0; i < rand(); i++);
		testSink.push(testRecords[rand() % 64]);
	}
	for (int i = 0; i < 128; i++) {
		while (testSink.getQueueLength() > 5);
		int magic_number = rand() % 3;
		if ((i % 10) == 0) testRecords[magic_number] = createTestRecord(magic_number);
		testSink.push(testRecords[magic_number]);
	}


	// finish test
	sleep(1);
	TimeoutSemaphore::shutdown();
	testSink.terminateSink();
	sleep(1);
	testSink.push(testRecords[0]);



}

int test_main(int, char *[]) {
	
	// set Vermont messaging subsystem's debug level
	//msg_setlevel(MSG_DEFAULT+99);

	// seed random number generator to 0
	srand(0);	
	
	std::cout << "Testing: Concentrator" << std::endl;
	test_module_coupling();

	/*
	// six ways to detect and report the same error:
	BOOST_CHECK( add( 2,2 ) == 4 );        // #1 continues on error
	BOOST_REQUIRE( add( 2,2 ) == 4 );      // #2 throws on error
	if( add( 2,2 ) != 4 )
	BOOST_ERROR( "Ouch..." );            // #3 continues on error
	if( add( 2,2 ) != 4 )
	BOOST_FAIL( "Ouch..." );             // #4 throws on error
	if( add( 2,2 ) != 4 ) throw "Oops..."; // #5 throws on error

	return add( 2, 2 ) == 4 ? 0 : 1;       // #6 returns error code
	*/

	return 0;
}

#endif //HAVE_BOOST_UNIT_TEST_FRAMEWORK
#ifndef HAVE_BOOST_UNIT_TEST_FRAMEWORK
#include <iostream>
int main(int argc, char* argv[]) {
	std::cerr << "Not configured with HAVE_BOOST_UNIT_TEST_FRAMEWORK. No tests have been built." << std::endl;
}
#endif //HAVE_BOOST_UNIT_TEST_FRAMEWORK

