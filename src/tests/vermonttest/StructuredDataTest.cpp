#include "StructuredDataTest.h"
#include <fstream>

StructuredDataTest::StructuredDataTest() :
	recordSender(NULL), parser(NULL) {

}

Test::TestResult StructuredDataTest::execTest() {
	setup();
	run();
	cleanup();
	return Test::PASSED;
}

void StructuredDataTest::setup() {
	if (recordSender != NULL)
		delete recordSender;
	if (parser != NULL)
		delete parser;

	recordSender = new IpfixRecordSenderStub();
	parser = new IpfixParser(recordSender);
	sourceId = boost::shared_ptr<IpfixRecord::SourceID>(new IpfixRecord::SourceID());

	addTemplate("testpackets/structured_data/basicListTemplate.ipfix");
	addTemplate("testpackets/structured_data/basicListFixedLengthTemplate.ipfix");
	addTemplate("testpackets/structured_data/subTemplateListTemplate.ipfix");
	addTemplate("testpackets/structured_data/subTemplateMultiListTemplate.ipfix");
}

void StructuredDataTest::addTemplate(const std::string &filename) {
	uint16_t size;
	boost::shared_array<uint8_t> buffer = readFile(filename, size);

	parser->processPacket(buffer, size, sourceId);
}

boost::shared_array<uint8_t> StructuredDataTest::readFile(const std::string &filename, uint16_t &size) const {
	std::ifstream input;

	input.open(filename.c_str(), ios::in|ios::binary|ios::ate);

	REQUIRE(!input.fail());

	size = input.tellg();
	boost::shared_array<uint8_t> buffer(new uint8_t[size]);

	input.seekg(0, ios::beg);
	input.read((char *) buffer.get(), size);

	return buffer;
}

void StructuredDataTest::cleanup() {
	delete recordSender;
	recordSender = NULL;

	delete parser;
	parser = NULL;
}

void BasicListTest::run() {
	uint16_t size;
	boost::shared_array<uint8_t> buffer = readFile("testpackets/structured_data/basicList.ipfix", size);

	parser->processPacket(buffer, size, sourceId);

	IpfixRecord *record = recordSender->records.back();
	IpfixDataRecord *dataRecord = dynamic_cast<IpfixDataRecord *>(record);

	REQUIRE(dataRecord != NULL);

	// Verify that template information matches
	TemplateInfo *templateInfo = dataRecord->templateInfo.get();

	ASSERT(templateInfo->templateId == BASIC_LIST_TEMPLATE_ID, "");
	ASSERT(templateInfo->fieldCount == 4u, "");

	// Verify that the static field values match
	TemplateInfo::FieldInfo *fieldInfo = templateInfo->fieldInfo;
	IpfixRecord::Data *data = dataRecord->data;

	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[0].offset))) == 9u, ""); // ingressInterface = 9
	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[1].offset))) == 0xffffffff, ""); // sourceIPv4Address
	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[2].offset))) == 0x00000000u, ""); // DestinationIPv4Address

	// Test generic basicList
	ASSERT(fieldInfo[3].rowCount == 3u, "");
	REQUIRE(fieldInfo[3].rows != NULL);
	ASSERT(fieldInfo[3].semantic == 3u, "");

	// Test list content
	uint32_t sampleData[] = { 1, 4, 8 };

	for (int i = 0; i < 3; i++) {
		ASSERT(fieldInfo[3].rows[i].fieldCount == 1u, "");
		ASSERT(fieldInfo[3].rows[i].fields[0].type.id == 14u, "");

		ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[3].rows[i].fields[0].offset))) == sampleData[i], "");
	}
}

void BasicListFixedLengthTest::run() {
	uint16_t size;
	boost::shared_array<uint8_t> buffer = readFile("testpackets/structured_data/basicListFixedLength.ipfix", size);

	parser->processPacket(buffer, size, sourceId);

	IpfixRecord *record = recordSender->records.back();
	IpfixDataRecord *dataRecord = dynamic_cast<IpfixDataRecord *>(record);

	REQUIRE(dataRecord != NULL);

	// Verify that template information matches
	TemplateInfo *templateInfo = dataRecord->templateInfo.get();

	ASSERT(templateInfo->templateId == BASIC_LIST_FIXED_LENGTH_TEMPLATE_ID, "");
	ASSERT(templateInfo->fieldCount == 4u, "");

	// Verify that the static field values match
	TemplateInfo::FieldInfo *fieldInfo = templateInfo->fieldInfo;
	IpfixRecord::Data *data = dataRecord->data;

	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[0].offset))) == 9u, ""); // ingressInterface = 9
	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[1].offset))) == 0xffffffff, ""); // sourceIPv4Address
	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[2].offset))) == 0x00000000u, ""); // destinationIPv4Address

	// Test generic basicList
	ASSERT(fieldInfo[3].rowCount == 3u, "");
	REQUIRE(fieldInfo[3].rows != NULL);
	ASSERT(fieldInfo[3].semantic == 3u, "");

	// Test list content
	uint32_t sampleData[] = { 1, 4, 8 };

	for (int i = 0; i < 3; i++) {
		ASSERT(fieldInfo[3].rows[i].fieldCount == 1u, "");
		ASSERT(fieldInfo[3].rows[i].fields[0].type.id == 14u, "");

		ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[3].rows[i].fields[0].offset))) == sampleData[i], "");
	}
}

void BasicListVariableFieldLengthTest::run() {
	uint16_t size;
	boost::shared_array<uint8_t> buffer = readFile("testpackets/structured_data/basicListVariableLengthContent.ipfix", size);

	parser->processPacket(buffer, size, sourceId);

	IpfixRecord *record = recordSender->records.back();
	IpfixDataRecord *dataRecord = dynamic_cast<IpfixDataRecord *>(record);

	REQUIRE(dataRecord != NULL);

	// Verify that template information matches
	TemplateInfo *templateInfo = dataRecord->templateInfo.get();

	ASSERT(templateInfo->templateId == BASIC_LIST_TEMPLATE_ID, "");
	ASSERT(templateInfo->fieldCount == 4u, "");

	// Verify that the static field values match
	TemplateInfo::FieldInfo *fieldInfo = templateInfo->fieldInfo;
	IpfixRecord::Data *data = dataRecord->data;

	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[0].offset))) == 9u, ""); // ingressInterface = 9
	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[1].offset))) == 0xffffffff, ""); // sourceIPv4Address
	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[2].offset))) == 0x00000000u, ""); // destinationIPv4Address

	// Test generic basicList
	ASSERT(fieldInfo[3].rowCount == 3u, "");
	REQUIRE(fieldInfo[3].rows != NULL);
	ASSERT(fieldInfo[3].semantic == 3u, "");

	// Test list content
	const char *sampleData[] = { "FE0/0", "FE10/10", "FE2/2" };

	for (int i = 0; i < 3; i++) {
		ASSERT(fieldInfo[3].rows[i].fieldCount == 1u, "");
		ASSERT(fieldInfo[3].rows[i].fields[0].type.id == 82u, "");

		char fieldContent[8];
		memcpy(fieldContent, data + fieldInfo[3].rows[i].fields[0].offset, fieldInfo[3].rows[i].fields[0].type.length);
		fieldContent[fieldInfo[3].rows[i].fields[0].type.length] = 0;
		ASSERT(std::string(fieldContent) == std::string(sampleData[i]), "");
	}
}

void SubTemplateListTest::run() {
	uint16_t size;
	boost::shared_array<uint8_t> buffer = readFile("testpackets/structured_data/subTemplateList.ipfix", size);

	parser->processPacket(buffer, size, sourceId);

	IpfixRecord *record = recordSender->records.back();
	IpfixDataRecord *dataRecord = dynamic_cast<IpfixDataRecord *>(record);

	REQUIRE(dataRecord != NULL);

	// Verify that template information matches
	TemplateInfo *templateInfo = dataRecord->templateInfo.get();

	ASSERT(templateInfo->templateId == SUB_TEMPLATE_LIST_TEMPLATE_ID, "");
	ASSERT(templateInfo->fieldCount == 3u, "");

	// Verify that the static field values match
	TemplateInfo::FieldInfo *fieldInfo = templateInfo->fieldInfo;
	IpfixRecord::Data *data = dataRecord->data;

	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[0].offset))) == 9u, ""); // ingressInterface = 9
	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[1].offset))) == 0xffffffff, ""); // sourceIPv4Address

	// Test generic subTemplateList
	ASSERT(fieldInfo[2].rowCount == 2u, "");
	REQUIRE(fieldInfo[2].rows != NULL);
	ASSERT(fieldInfo[2].semantic == 3u, "");

	// Test row 1 content
	TemplateInfo::StructuredDataRow *rowInfo = &templateInfo->fieldInfo[2].rows[0];
	ASSERT(rowInfo->fieldCount == 4, "");
	ASSERT(rowInfo->templateId == BASIC_LIST_TEMPLATE_ID, "");

	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[0].offset))) == 10u, ""); // ingressInterface = 10
	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[1].offset))) == 0u, ""); // sourceIPv4Address = 0.0.0.0
	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[2].offset))) == 1u, ""); // destinationIPv4Address = 0.0.0.1
	// Last entry should be an empty basicList
	ASSERT(rowInfo->fields[3].semantic == 3, "");
	ASSERT(rowInfo->fields[3].type.id == 291u, "");
	ASSERT(rowInfo->fields[3].rowCount == 0u, "");

	// Test row 2 content
	rowInfo = &templateInfo->fieldInfo[2].rows[1];
	ASSERT(rowInfo->fieldCount == 4, "");
	ASSERT(rowInfo->templateId == BASIC_LIST_TEMPLATE_ID, "");

	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[0].offset))) == 11u, ""); // ingressInterface = 10
	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[1].offset))) == 1u, ""); // sourceIPv4Address = 0.0.0.0
	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[2].offset))) == 0u, ""); // destinationIPv4Address = 0.0.0.1
	// Last entry should be an empty basicList
	ASSERT(rowInfo->fields[3].semantic == 3, "");
	ASSERT(rowInfo->fields[3].type.id == 291u, "");
	ASSERT(rowInfo->fields[3].rowCount == 0u, "");
}

void SubTemplateMultiListTest::run() {
	uint16_t size;
	boost::shared_array<uint8_t> buffer = readFile("testpackets/structured_data/subTemplateMultiList.ipfix", size);

	parser->processPacket(buffer, size, sourceId);

	IpfixRecord *record = recordSender->records.back();
	IpfixDataRecord *dataRecord = dynamic_cast<IpfixDataRecord *>(record);

	REQUIRE(dataRecord != NULL);

	// Verify that template information matches
	TemplateInfo *templateInfo = dataRecord->templateInfo.get();

	ASSERT(templateInfo->templateId == SUB_TEMPLATE_MULTI_LIST_TEMPLATE_ID, "");
	ASSERT(templateInfo->fieldCount == 3u, "");

	// Verify that the static field values match
	TemplateInfo::FieldInfo *fieldInfo = templateInfo->fieldInfo;
	IpfixRecord::Data *data = dataRecord->data;

	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[0].offset))) == 9u, ""); // ingressInterface = 9
	ASSERT(ntohl(*((uint32_t *) (data + fieldInfo[1].offset))) == 0xffffffff, ""); // sourceIPv4Address

	// Test generic subTemplateList
	ASSERT(fieldInfo[2].rowCount == 2u, "");
	REQUIRE(fieldInfo[2].rows != NULL);
	ASSERT(fieldInfo[2].semantic == 3u, "");

	// Test row 1 content
	TemplateInfo::StructuredDataRow *rowInfo = &templateInfo->fieldInfo[2].rows[0];
	ASSERT(rowInfo->fieldCount == 4, "");
	ASSERT(rowInfo->templateId == BASIC_LIST_TEMPLATE_ID, "");

	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[0].offset))) == 10u, ""); // ingressInterface = 10
	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[1].offset))) == 0u, ""); // sourceIPv4Address = 0.0.0.0
	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[2].offset))) == 1u, ""); // destinationIPv4Address = 0.0.0.1
	// Last entry should be an empty basicList
	ASSERT(rowInfo->fields[3].semantic == 3, "");
	ASSERT(rowInfo->fields[3].type.id == 291u, "");
	ASSERT(rowInfo->fields[3].rowCount == 0u, "");

	// Test row 2 content
	rowInfo = &templateInfo->fieldInfo[2].rows[1];
	ASSERT(rowInfo->fieldCount == 4, "");
	ASSERT(rowInfo->templateId == BASIC_LIST_FIXED_LENGTH_TEMPLATE_ID, "");

	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[0].offset))) == 11u, ""); // ingressInterface = 10
	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[1].offset))) == 1u, ""); // sourceIPv4Address = 0.0.0.0
	ASSERT(ntohl(*((uint32_t *) (data + rowInfo->fields[2].offset))) == 0u, ""); // destinationIPv4Address = 0.0.0.1

	// Last entry should be an basicList with the entries 1, 4, 8
	ASSERT(rowInfo->fields[3].semantic == 3, "");
	ASSERT(rowInfo->fields[3].type.id == 291u, "");
	ASSERT(rowInfo->fields[3].rowCount == 3u, "");
}

bool IpfixRecordSenderStub::send(IpfixRecord *ipfixRecord) {
	records.push_back(ipfixRecord);

	return true;
}

StructuredDataTestSuite::StructuredDataTestSuite() {
	add(new BasicListTest());
	add(new BasicListFixedLengthTest());
	add(new BasicListVariableFieldLengthTest());
	add(new SubTemplateListTest());
	add(new SubTemplateMultiListTest());
}
