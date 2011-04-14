#include "test_concentrator.h"

#include "modules/ipfix/IpfixParser.hpp"
#include "common/ipfixlolib/ipfixlolib.h"
#include "modules/ipfix/IpfixSender.hpp"
#include "modules/ipfix/IpfixRawdirReader.hpp"
#include "modules/ipfix/IpfixRawdirWriter.hpp"
#include "modules/ipfix/IpfixCollector.hpp"
#include "modules/ipfix/IpfixPrinter.hpp"
#include "core/ConnectionQueue.h"
#include "TestSuiteBase.h"

#include <boost/filesystem/operations.hpp>
#include <iostream>
#include <vector>
#include <stdlib.h>

class TestSink : public IpfixRecordDestination {
	public:
		int receivedRecords;
		bool checkSourceId;

		TestSink(bool checkSourceId = true) : receivedRecords(0), checkSourceId(checkSourceId) {
		}

		virtual void onDataRecord(IpfixDataRecord* record)
		{
			receivedRecords++;

			uint8_t inSourceID = record->sourceID->exporterAddress.ip[0];
			uint8_t inTemplateId = record->templateInfo->templateId - 256;
			uint8_t inTypeId = record->templateInfo->fieldInfo[0].type.id;
			uint8_t inData = record->data[0];
			if ((record->templateInfo->setId == TemplateInfo::IpfixDataTemplate) && (record->templateInfo->dataCount > 0)) {
				uint8_t inDataTemplateTypeId = record->templateInfo->dataInfo[0].type.id;
				uint8_t inDataTemplate = record->templateInfo->data[0];
				msg(MSG_DEBUG, "Received DataDataRecord: %d, %d, %d, %d, %d, %d", inSourceID, inTemplateId, inTypeId, inData, inDataTemplateTypeId, inDataTemplate);
				if (inData != inDataTemplateTypeId) ERROR("IpfixRecord got corrupted: inData != inDataTemplateTypeId");
				if (inData != inDataTemplate) ERROR("IpfixRecord got corrupted: inData != inDataTemplate");
			} else {
				msg(MSG_DEBUG, "Received DataRecord: %d, %d, %d, %d", inSourceID, inTemplateId, inTypeId, inData);
			}
			if (checkSourceId) if (inSourceID != inTemplateId) ERROR("SourceID or TemplateInfo got corrupted: inSourceID != inTemplateId");
			if (inTemplateId != inTypeId) ERROR("TemplateInfo got corrupted: inTemplateId != inTypeId");
			if (inData != inTemplateId) ERROR("IpfixRecord got corrupted: inData != inTemplateId");
			if (inData != inTypeId) ERROR("IpfixRecord got corrupted: inData != inTypeId");

			for (int i = 0; i < rand(); i++);
		}


		void onTemplateDestruction(IpfixTemplateDestructionRecord* record)
		{
		}

};

class DummyIpfixRecordSender : public IpfixRecordSender
{
public:
	virtual ~DummyIpfixRecordSender() {}
	virtual bool send(IpfixRecord* ipfixRecord) { return true; }
};


boost::shared_array<uint8_t> createTestData(uint8_t magic_number) {
		boost::shared_array<uint8_t> testData(new uint8_t[1]);
		testData.get()[0] = magic_number;
		return testData;
}

boost::shared_ptr<TemplateInfo> createTestTemplate(uint8_t magic_number) {
		boost::shared_ptr<TemplateInfo> testTemplate(new TemplateInfo);
		testTemplate->templateId = magic_number + 256;
		testTemplate->setId = TemplateInfo::IpfixTemplate;
		testTemplate->fieldCount = 1;
		testTemplate->fieldInfo = (TemplateInfo::FieldInfo*)malloc(testTemplate->fieldCount * sizeof(TemplateInfo::FieldInfo));
		testTemplate->fieldInfo[0].type.id = magic_number;
		testTemplate->fieldInfo[0].type.length = 1;
		testTemplate->fieldInfo[0].type.enterprise = 0;
		testTemplate->fieldInfo[0].offset = 0;

		return testTemplate;
}

boost::shared_ptr<TemplateInfo> createTestDataTemplate(uint8_t magic_number) {
		boost::shared_ptr<TemplateInfo> testTemplate(new TemplateInfo);
		testTemplate->templateId = magic_number + 256;
		testTemplate->setId = TemplateInfo::IpfixDataTemplate;
		testTemplate->preceding = 0;
		testTemplate->fieldCount = 1;
		testTemplate->fieldInfo = (TemplateInfo::FieldInfo*)malloc(testTemplate->fieldCount * sizeof(TemplateInfo::FieldInfo));
		testTemplate->dataCount = 1;
		testTemplate->dataInfo = (TemplateInfo::FieldInfo*)malloc(testTemplate->fieldCount * sizeof(TemplateInfo::FieldInfo));
		testTemplate->data = (uint8_t*)malloc(1); testTemplate->data[0] = magic_number;
		testTemplate->fieldInfo[0].type.id = magic_number;
		testTemplate->fieldInfo[0].type.length = 1;
		testTemplate->fieldInfo[0].type.enterprise = 0;
		testTemplate->fieldInfo[0].offset = 0;
		testTemplate->dataInfo[0].type.id = magic_number;
		testTemplate->dataInfo[0].type.length = 1;
		testTemplate->dataInfo[0].type.enterprise = 0;
		testTemplate->dataInfo[0].offset = 0;

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
		

IpfixDataRecord* createTestDataRecord(uint8_t magic_number, boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_ptr<TemplateInfo> templateInfo) {
		static InstanceManager<IpfixDataRecord> im("IpfixDataRecord");
		IpfixDataRecord* testRecord = im.getNewInstance();
		testRecord->sourceID = sourceId;
		testRecord->templateInfo = templateInfo;
		testRecord->dataLength = 1;
		testRecord->message = createTestData(magic_number);
		testRecord->data = testRecord->message.get();

		return testRecord;
}

IpfixTemplateDestructionRecord* createTestTemplateDestructionRecord(uint8_t magic_number, boost::shared_ptr<TemplateInfo> templateInfo) {
		static InstanceManager<IpfixTemplateDestructionRecord> im("IpfixTemplateDestructionRecord");
		IpfixTemplateDestructionRecord* testRecord = im.getNewInstance();
		testRecord->templateInfo = templateInfo;

		return testRecord;
}

IpfixDataRecord* createTestDataDataRecord(uint8_t magic_number, boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_ptr<TemplateInfo> dataTemplateInfo) {
		static InstanceManager<IpfixDataRecord> im("IpfixDataDataRecord");
		IpfixDataRecord* testRecord = im.getNewInstance();
		testRecord->sourceID = sourceId;
		testRecord->templateInfo = dataTemplateInfo;
		testRecord->dataLength = 1;
		testRecord->message = createTestData(magic_number);
		testRecord->data = testRecord->message.get();

		return testRecord;
}

IpfixTemplateRecord* createTestTemplateRecord(uint8_t magic_number, boost::shared_ptr<TemplateInfo> templateInfo) {
		static InstanceManager<IpfixTemplateRecord> im("IpfixTemplateRecord");
		IpfixTemplateRecord* testRecord = im.getNewInstance();
		testRecord->templateInfo = templateInfo; 

		return testRecord;
}

IpfixTemplateRecord* createTestDataTemplateRecord(uint8_t magic_number, boost::shared_ptr<TemplateInfo> dataTemplateInfo) {
		static InstanceManager<IpfixTemplateRecord> im("IpfixDataTemplateRecord");
		IpfixTemplateRecord* testRecord = im.getNewInstance();	
		testRecord->templateInfo = dataTemplateInfo; 

		return testRecord;
}

IpfixTemplateDestructionRecord* createTestDataTemplateDestructionRecord(uint8_t magic_number, boost::shared_ptr<TemplateInfo> dataTemplateInfo) {
		static InstanceManager<IpfixTemplateDestructionRecord> im("IpfixDataTemplateDestructionRecord");
		IpfixTemplateDestructionRecord* testRecord = im.getNewInstance();	
		testRecord->templateInfo = dataTemplateInfo;

		return testRecord;
}

void test_module_coupling() {
	std::cout << "Testing: Concentrator module coupling..." << std::endl;

	ConnectionQueue<IpfixRecord*> queue1(100);
	TestSink testSink(true);
	
	queue1.connectTo(&testSink);	
	queue1.start();

	// create some test data
	std::vector<IpfixDataRecord*> testRecords;
	for (uint8_t magic_number = 0; magic_number < 255; magic_number++) {
		boost::shared_ptr<IpfixRecord::SourceID> testSourceId = createTestSourceId(magic_number);
		boost::shared_ptr<TemplateInfo> templateInfo = createTestTemplate(magic_number);
		IpfixTemplateRecord* dtr = createTestTemplateRecord(magic_number, templateInfo);
		testSink.receive(dtr);
		testRecords.push_back(createTestDataRecord(magic_number, testSourceId, templateInfo));	
	}

	// perform test
	for (uint8_t magic_number = 0; magic_number < 64; magic_number++) {
		queue1.receive(testRecords[magic_number]);
	}
	for (int i = 0; i < 128; i++) {
		for (int i = 0; i < rand(); i++);
		queue1.receive(testRecords[rand() % 64]);
	}
	for (int i = 0; i < 128; i++) {
		while (queue1.getCount() > 5);
		int magic_number = rand() % 3;
		if ((i % 10) == 0) testRecords[magic_number] = createTestDataRecord(magic_number, testRecords[magic_number]->sourceID, testRecords[magic_number]->templateInfo);
		queue1.receive(testRecords[magic_number]);
	}

	// be nice
	for (uint8_t magic_number = 0; magic_number < 255; magic_number++) {
		queue1.receive(createTestTemplateDestructionRecord(magic_number, testRecords[magic_number]->templateInfo));
	}

	// give modules a chance to process their queues
	sleep(1);

	queue1.shutdown();
}

void test_ipfixlolib_rawdir() {
#ifndef IPFIXLOLIB_RAWDIR_SUPPORT
	std::cout << "Not Testing: Ipfixlolib Rawdir reading/writing" << std::endl;
#endif
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT

	std::cout << "Testing: Ipfixlolib Rawdir writing..." << std::endl;

	// create temporary directory
	char* tmpdirname = strdup("/tmp/vermont-tests-concentrator-rawdir-XXXXXX");
	if (mkdtemp(tmpdirname) == 0) {
		ERROR("Unable to create temporary directory. Cannot continue.");
		free(tmpdirname);
		return;
	}
	std::string tmppath(tmpdirname);
	free(tmpdirname);

	{
		// create IpfixRawdirWriter
		IpfixRawdirWriter ipfixRawdirWriter(0xbeef, tmppath);
		ipfixRawdirWriter.start();

		// create some test data
		std::vector<IpfixDataRecord*> testDataRecords;
		for (uint8_t magic_number = 0; magic_number < 16; magic_number++) {
			boost::shared_ptr<IpfixRecord::SourceID> testSourceId = createTestSourceId(magic_number);
			boost::shared_ptr<TemplateInfo> dataTemplateInfo = createTestDataTemplate(magic_number);
			IpfixTemplateRecord* dtr = createTestDataTemplateRecord(magic_number, dataTemplateInfo);
			ipfixRawdirWriter.receive(dtr);
			
			testDataRecords.push_back(createTestDataRecord(magic_number, testSourceId, dataTemplateInfo));
		}

		// perform test	
		for (uint8_t magic_number = 0; magic_number < 16; magic_number++) {
			ipfixRawdirWriter.receive(testDataRecords[magic_number]);
			//ipfixRawdirWriter.flushPacket();
		}

		// be nice
		for (uint8_t magic_number = 0; magic_number < 16; magic_number++) {
			ipfixRawdirWriter.receive(createTestDataTemplateDestructionRecord(magic_number, testDataRecords[magic_number]->templateInfo));
		}

		// give modules a chance to process their queues
		sleep(1);

		// shut down modules
		ipfixRawdirWriter.shutdown();

	}

	std::cout << "Testing: Ipfixlolib Rawdir reading..." << std::endl;
	{
		// create testSink
		TestSink testSink(false);

		IpfixRawdirReader ipfixRawdirReader(tmppath);

		IpfixCollector ipfixCollector(&ipfixRawdirReader);
		ipfixCollector.connectTo(&testSink);

		ipfixCollector.start();

		// give modules a chance to process their queues
		sleep(1);

		// shut down modules
		//ipfixCollector.shutdown();

		// check results
		if (testSink.receivedRecords != 16) {
			char s[256];
			snprintf(s, 255, "IpfixRawdirReader should have read 16 records, but read %d", testSink.receivedRecords);
			ERROR(s);
		}
	}

#endif
}

void test_parser_stability() {

	boost::shared_ptr<IpfixRecord::SourceID> testSourceId = createTestSourceId(42);
	DummyIpfixRecordSender recordSender;

	IpfixParser ipfixParser(&recordSender);

	for (int iteration = 0; iteration < 1000; iteration++) {
		for (int len = 32; len < 64; len++) {
			boost::shared_array<uint8_t> message(new uint8_t[len]);
			for (int i = 0; i < len; i++) message[i]=rand();
			*((uint16_t*)message.get())=htons(0x000a);
			*((uint16_t*)(message.get()+2))=htons(len);

			ipfixParser.processPacket(message, len, testSourceId); 
		}
	}
}



ConcentratorTestSuite::ConcentratorTestSuite()
{
}

Test::TestResult ConcentratorTestSuite::execTest()
{
	// set Vermont messaging subsystem's debug level
	//msg_setlevel(MSG_DEFAULT+99);

	// seed random number generator to 0
	srand(0);	

	std::cout << "Testing: Concentrator" << std::endl;

	test_module_coupling();

	test_ipfixlolib_rawdir();

	//test_parser_stability();
	
	return Test::FAILED;
}
