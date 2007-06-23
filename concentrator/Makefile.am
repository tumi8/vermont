noinst_LIBRARIES=libconcentrator.a

libconcentrator_a_SOURCES=IpfixAggregator.cpp IpfixAggregator.hpp crc16.cpp crc16.hpp Hashtable.cpp \
Hashtable.hpp ipfix.cpp ipfix.hpp \
IpfixReceiver.cpp IpfixReceiver.hpp \
IpfixReceiverUdpIpV4.cpp IpfixReceiverUdpIpV4.hpp \
IpfixRawdirReader.cpp IpfixRawdirReader.hpp \
FlowSource.hpp FlowSource.cpp \
FlowSink.hpp FlowSink.cpp \
IpfixRecord.hpp IpfixRecord.cpp \
IpfixPrinter.cpp IpfixPrinter.hpp \
IpfixParser.cpp IpfixParser.hpp \
IpfixPacketProcessor.cpp IpfixPacketProcessor.hpp \
IpfixCollector.cpp IpfixCollector.hpp \
Rules.cpp Rules.hpp Rule.cpp Rule.hpp \
IpfixSender.cpp IpfixSender.hpp \
IpfixRawdirWriter.cpp IpfixRawdirWriter.hpp \
TemplateBuffer.cpp TemplateBuffer.hpp \
IpfixDbReader.hpp IpfixDbReader.cpp IpfixDbWriter.hpp IpfixDbWriter.cpp

AM_CXXFLAGS=-I$(top_srcdir) $(MYSQL_CFLAGS) -Wall -Werror
