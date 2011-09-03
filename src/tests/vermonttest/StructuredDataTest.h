#ifndef STRUCTUREDDATATEST_H_
#define STRUCTUREDDATATEST_H_

#include <modules/ipfix/IpfixParser.hpp>
#include "TestSuiteBase.h"
#include <boost/format.hpp>

#define BASIC_LIST_TEMPLATE_ID 256
#define BASIC_LIST_FIXED_LENGTH_TEMPLATE_ID 257
#define SUB_TEMPLATE_LIST_TEMPLATE_ID 258
#define SUB_TEMPLATE_MULTI_LIST_TEMPLATE_ID 259

class IpfixRecordSenderStub : public IpfixRecordSender {
public:
	bool send(IpfixRecord* ipfixRecord);

	std::vector<IpfixRecord *> records;
};

class StructuredDataTest : public Test {
public:
	StructuredDataTest();

	virtual TestResult execTest();
protected:
	virtual void run() = 0;

	virtual void setup();
	virtual void cleanup();

	void addTemplate(const std::string &filename);

	boost::shared_array<uint8_t> readFile(const std::string &filename, uint16_t &size) const;
protected:
	IpfixRecordSenderStub *recordSender;
	IpfixParser *parser;
	boost::shared_ptr<IpfixRecord::SourceID> sourceId;

private:
	void testBasicList();
	void testBasicListVariableFieldLength();
	void testBasicListFixedLength();
	void testSubTemplateList();
};

class BasicListTest : public StructuredDataTest {
protected:
	void run();
};

class BasicListVariableFieldLengthTest : public StructuredDataTest {
protected:
	void run();
};

class BasicListFixedLengthTest : public StructuredDataTest {
protected:
	void run();
};

class SubTemplateListTest : public StructuredDataTest {
protected:
	void run();
};

class SubTemplateMultiListTest : public StructuredDataTest {
protected:
	void run();
};

class StructuredDataTestSuite : public TestSuite {
public:
	StructuredDataTestSuite();
};

#endif
