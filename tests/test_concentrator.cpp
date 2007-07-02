#include "test_concentrator.h"

#include "concentrator/IpfixRecord.hpp"
#include "concentrator/FlowSource.hpp"
#include "concentrator/FlowSink.hpp"
#include "ipfixlolib/ipfixlolib.h"
#include "concentrator/IpfixSender.hpp"
#include "concentrator/IpfixRawdirReader.hpp"
#include "concentrator/IpfixRawdirWriter.hpp"
#include "concentrator/IpfixCollector.hpp"
#include "concentrator/IpfixPrinter.hpp"

#include <iostream>
#include <vector>

class TestSink : public FlowSink {
	public:
		int receivedRecords;
		bool checkSourceId;

		TestSink(bool checkSourceId = true) : receivedRecords(0), checkSourceId(checkSourceId) {
		}

		virtual int onTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo) 
		{
			return 0;
		}

		virtual int onDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo, uint16_t length, IpfixRecord::Data* data) {
			uint8_t inSourceID = sourceID->exporterAddress.ip[0];
			uint8_t inTemplateId = templateInfo->templateId - 256;
			uint8_t inTypeId = templateInfo->fieldInfo[0].type.id;
			uint8_t inData = data[0];

			msg(MSG_DEBUG, "Received DataRecord: %d, %d, %d, %d", inSourceID, inTemplateId, inTypeId, inData);
			if (checkSourceId) if (inSourceID != inTemplateId) BOOST_ERROR("SourceID or TemplateInfo got corrupted: inSourceID != inTemplateId");
			if (inTemplateId != inTypeId) BOOST_ERROR("TemplateInfo got corrupted: inTemplateId != inTypeId");
			if (inData != inTemplateId) BOOST_ERROR("IpfixRecord got corrupted: inData != inTemplateId");
			if (inData != inTypeId) BOOST_ERROR("IpfixRecord got corrupted: inData != inTypeId");

			for (int i = 0; i < rand(); i++);

			return 0;
		}

		int onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data) {
			receivedRecords++;

			uint8_t inSourceID = sourceID->exporterAddress.ip[0];
			uint8_t inTemplateId = dataTemplateInfo->templateId - 256;
			uint8_t inTypeId = dataTemplateInfo->fieldInfo[0].type.id;
			uint8_t inData = data[0];
			uint8_t inDataTemplateTypeId = dataTemplateInfo->dataInfo[0].type.id;
			uint8_t inDataTemplate = dataTemplateInfo->data[0];
			msg(MSG_DEBUG, "Received DataDataRecord: %d, %d, %d, %d, %d, %d", inSourceID, inTemplateId, inTypeId, inData, inDataTemplateTypeId, inDataTemplate);
			if (checkSourceId) if (inSourceID != inTemplateId) BOOST_ERROR("SourceID or TemplateInfo got corrupted: inSourceID != inTemplateId");
			if (inTemplateId != inTypeId) BOOST_ERROR("TemplateInfo got corrupted: inTemplateId != inTypeId");
			if (inData != inTemplateId) BOOST_ERROR("IpfixRecord got corrupted: inData != inTemplateId");
			if (inData != inTypeId) BOOST_ERROR("IpfixRecord got corrupted: inData != inTypeId");
			if (inData != inDataTemplateTypeId) BOOST_ERROR("IpfixRecord got corrupted: inData != inDataTemplateTypeId");
			if (inData != inDataTemplate) BOOST_ERROR("IpfixRecord got corrupted: inData != inDataTemplate");

			for (int i = 0; i < rand(); i++);

			return 0;
		}


		int onTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo) {
			return 0;
		}

		int getQueueLength() {
			return ipfixRecords.getCount();
		}

};


boost::shared_array<uint8_t> createTestData(uint8_t magic_number) {
		boost::shared_array<uint8_t> testData(new uint8_t[1]);
		testData.get()[0] = magic_number;
		return testData;
}

boost::shared_ptr<IpfixRecord::TemplateInfo> createTestTemplate(uint8_t magic_number) {
		boost::shared_ptr<IpfixRecord::TemplateInfo> testTemplate(new IpfixRecord::TemplateInfo);
		testTemplate->templateId = magic_number + 256;
		testTemplate->fieldCount = 1;
		testTemplate->fieldInfo = (IpfixRecord::FieldInfo*)malloc(testTemplate->fieldCount * sizeof(IpfixRecord::FieldInfo));
		testTemplate->fieldInfo[0].type.id = magic_number;
		testTemplate->fieldInfo[0].type.length = 1;
		testTemplate->fieldInfo[0].type.eid = 0;
		testTemplate->fieldInfo[0].offset = 0;
		testTemplate->userData = 0;

		return testTemplate;
}

boost::shared_ptr<IpfixRecord::DataTemplateInfo> createTestDataTemplate(uint8_t magic_number) {
		boost::shared_ptr<IpfixRecord::DataTemplateInfo> testTemplate(new IpfixRecord::DataTemplateInfo);
		testTemplate->templateId = magic_number + 256;
		testTemplate->preceding = 0;
		testTemplate->fieldCount = 1;
		testTemplate->fieldInfo = (IpfixRecord::FieldInfo*)malloc(testTemplate->fieldCount * sizeof(IpfixRecord::FieldInfo));
		testTemplate->dataCount = 1;
		testTemplate->dataInfo = (IpfixRecord::FieldInfo*)malloc(testTemplate->fieldCount * sizeof(IpfixRecord::FieldInfo));
		testTemplate->data = (uint8_t*)malloc(1); testTemplate->data[0] = magic_number;
		testTemplate->fieldInfo[0].type.id = magic_number;
		testTemplate->fieldInfo[0].type.length = 1;
		testTemplate->fieldInfo[0].type.eid = 0;
		testTemplate->fieldInfo[0].offset = 0;
		testTemplate->dataInfo[0].type.id = magic_number;
		testTemplate->dataInfo[0].type.length = 1;
		testTemplate->dataInfo[0].type.eid = 0;
		testTemplate->dataInfo[0].offset = 0;
		testTemplate->userData = 0;

		return testTemplate;
}

boost::shared_ptr<IpfixRecord::SourceID> createTestSourceId(uint8_t magic_number) {
		boost::shared_ptr<IpfixRecord::SourceID> testSourceId(new IpfixRecord::SourceID);
		testSourceId->observationDomainId = 0xdeadbeef;
		testSourceId->exporterAddress.ip[0] = magic_number;
		testSourceId->exporterAddress.ip[1] = magic_number;
		testSourceId->exporterAddress.ip[2] = magic_number;
		testSourceId->exporterAddress.ip[3] = magic_number;
		testSourceId->exporterAddress.len = 4;

		return testSourceId;
}
		
boost::shared_ptr<IpfixDataRecord> createTestDataRecord(uint8_t magic_number, boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_ptr<IpfixRecord::TemplateInfo> templateInfo) {
		boost::shared_ptr<IpfixDataRecord> testRecord(new IpfixDataRecord);
		testRecord->sourceID = sourceId;
		testRecord->templateInfo = templateInfo;
		testRecord->dataLength = 1;
		testRecord->message = createTestData(magic_number);
		testRecord->data = testRecord->message.get();

		return testRecord;
}

boost::shared_ptr<IpfixTemplateDestructionRecord> createTestTemplateDestructionRecord(uint8_t magic_number, boost::shared_ptr<IpfixRecord::TemplateInfo> templateInfo) {
		boost::shared_ptr<IpfixTemplateDestructionRecord> testRecord(new IpfixTemplateDestructionRecord);
		testRecord->templateInfo = templateInfo;

		return testRecord;
}

boost::shared_ptr<IpfixDataDataRecord> createTestDataDataRecord(uint8_t magic_number, boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo) {
		boost::shared_ptr<IpfixDataDataRecord> testRecord(new IpfixDataDataRecord);
		testRecord->sourceID = sourceId;
		testRecord->dataTemplateInfo = dataTemplateInfo;
		testRecord->dataLength = 1;
		testRecord->message = createTestData(magic_number);
		testRecord->data = testRecord->message.get();

		return testRecord;
}

boost::shared_ptr<IpfixTemplateRecord> createTestTemplateRecord(uint8_t magic_number, boost::shared_ptr<IpfixRecord::TemplateInfo> templateInfo) {
		boost::shared_ptr<IpfixTemplateRecord> testRecord(new IpfixTemplateRecord);
		testRecord->templateInfo = templateInfo; 

		return testRecord;
}

boost::shared_ptr<IpfixDataTemplateRecord> createTestDataTemplateRecord(uint8_t magic_number, boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo) {
		boost::shared_ptr<IpfixDataTemplateRecord> testRecord(new IpfixDataTemplateRecord);
		testRecord->dataTemplateInfo = dataTemplateInfo; 

		return testRecord;
}

boost::shared_ptr<IpfixDataTemplateDestructionRecord> createTestDataTemplateDestructionRecord(uint8_t magic_number, boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo) {
		boost::shared_ptr<IpfixDataTemplateDestructionRecord> testRecord(new IpfixDataTemplateDestructionRecord);
		testRecord->dataTemplateInfo = dataTemplateInfo;

		return testRecord;
}

void test_module_coupling() {
	std::cout << "Testing: Concentrator module coupling..." << std::endl;

	// create testSink
	TestSink testSink(true);
	testSink.runSink();

	// create some test data
	std::vector<boost::shared_ptr<IpfixDataRecord> > testRecords;
	for (uint8_t magic_number = 0; magic_number < 255; magic_number++) {
		boost::shared_ptr<IpfixRecord::SourceID> testSourceId = createTestSourceId(magic_number);
		boost::shared_ptr<IpfixRecord::TemplateInfo> templateInfo = createTestTemplate(magic_number);
		boost::shared_ptr<IpfixTemplateRecord> dtr = createTestTemplateRecord(magic_number, templateInfo);
		testSink.push(dtr);
		testRecords.push_back(createTestDataRecord(magic_number, testSourceId, templateInfo));	
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
		if ((i % 10) == 0) testRecords[magic_number] = createTestDataRecord(magic_number, testRecords[magic_number]->sourceID, testRecords[magic_number]->templateInfo);
		testSink.push(testRecords[magic_number]);
	}


	// finish test
	sleep(1);

	//TimeoutSemaphore::shutdown();
	// make sure that pop semaphore doesn't hang. This shouldn't be needed, but let's play it safe
	for (uint8_t magic_number = 0; magic_number < 255; magic_number++) {
		testSink.push(createTestTemplateDestructionRecord(magic_number, testRecords[magic_number]->templateInfo));
	}
	testSink.terminateSink();
	sleep(1);

}

void test_ipfixlolib_rawdir() {
#ifndef IPFIXLOLIB_RAWDIR_SUPPORT
	std::cout << "Not Testing: Ipfixlolib Rawdir reading/writing" << std::endl;
#endif
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT

	std::cout << "Testing: Ipfixlolib Rawdir writing..." << std::endl;

	{
		// create IpfixRawdirWriter
		IpfixRawdirWriter ipfixRawdirWriter(0xbeef, "/tmp/pack2");
		ipfixRawdirWriter.runSink();

		// create some test data
		std::vector<boost::shared_ptr<IpfixDataDataRecord> > testDataRecords;
		for (uint8_t magic_number = 0; magic_number < 16; magic_number++) {
			boost::shared_ptr<IpfixRecord::SourceID> testSourceId = createTestSourceId(magic_number);
			boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo = createTestDataTemplate(magic_number);
			boost::shared_ptr<IpfixDataTemplateRecord> dtr = createTestDataTemplateRecord(magic_number, dataTemplateInfo);
			ipfixRawdirWriter.push(dtr);
			ipfixRawdirWriter.onIdle();
			testDataRecords.push_back(createTestDataDataRecord(magic_number, testSourceId, dataTemplateInfo));
		}

		// perform test	
		for (uint8_t magic_number = 0; magic_number < 16; magic_number++) {
			ipfixRawdirWriter.push(testDataRecords[magic_number]);
			ipfixRawdirWriter.onIdle();
		}

		// finish test
		sleep(1);

		//TimeoutSemaphore::shutdown();
		// make sure that pop semaphore doesn't hang. This shouldn't be needed, but let's play it safe
		for (uint8_t magic_number = 0; magic_number < 16; magic_number++) {
			ipfixRawdirWriter.push(createTestDataTemplateDestructionRecord(magic_number, testDataRecords[magic_number]->dataTemplateInfo));
		}
		ipfixRawdirWriter.terminateSink();
		sleep(1);

	}

	std::cout << "Testing: Ipfixlolib Rawdir reading..." << std::endl;
	{
		// create testSink
		TestSink testSink(false);
		testSink.runSink();

		IpfixRawdirReader ipfixRawdirReader("/tmp/pack2");

		IpfixParser ipfixParser;
		ipfixParser.addFlowSink(&testSink);

		IpfixCollector ipfixCollector;
		ipfixCollector.addIpfixReceiver(&ipfixRawdirReader);
		ipfixCollector.addIpfixPacketProcessor(&ipfixParser);
		ipfixCollector.start();

		// finish test
		sleep(1);

		//TimeoutSemaphore::shutdown();
		// make sure that pop semaphore doesn't hang. This shouldn't be needed, but let's play it safe
		testSink.push(createTestDataTemplateDestructionRecord(0, createTestDataTemplate(0)));
		testSink.terminateSink();
		sleep(1);

		if (testSink.receivedRecords != 16) {
			char s[256];
			snprintf(s, 255, "IpfixRawdirReader should have read 16 records, but read %d", testSink.receivedRecords);
			BOOST_ERROR(s);
		}
	}

#endif
}

void test_parser_stability() {

	boost::shared_ptr<IpfixRecord::SourceID> testSourceId = createTestSourceId(42);

	IpfixParser ipfixParser;

	for (int iteration = 0; iteration < 65000; iteration++) {
		for (int len = 32; len < 64; len++) {
			boost::shared_array<uint8_t> message(new uint8_t[len]);
			for (int i = 0; i < len; i++) message[i]=rand();
			*((uint16_t*)message.get())=htons(0x000a);
			*((uint16_t*)(message.get()+2))=htons(len);

			ipfixParser.processPacket(message, len, testSourceId); 
		}
	}
}


void start_test()
{
	// set Vermont messaging subsystem's debug level
	//msg_setlevel(MSG_DEFAULT+99);

	// seed random number generator to 0
	srand(0);	

	std::cout << "Testing: Concentrator" << std::endl;

	//test_module_coupling();

	test_ipfixlolib_rawdir();

	//test_parser_stability();

	test_module_coupling();
}

ConcentratorTestSuite::ConcentratorTestSuite()
	: test_suite("ConcentratorTest")
{
	add(BOOST_TEST_CASE(&start_test));
}

