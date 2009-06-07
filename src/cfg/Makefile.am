noinst_LIBRARIES=libcfg.a

libcfg_a_SOURCES= \
	Cfg.h Cfg.cpp \
	CfgNode.h CfgNode.cpp \
	ConfigManager.h ConfigManager.cpp \
	Connector.h Connector.cpp \
	Edge.h Edge.cpp \
	Graph.h Graph.cpp \
	Node.h Node.cpp \
	ObserverCfg.cpp \
	PacketFilterCfg.h PacketFilterCfg.cpp \
	PacketPrinterCfg.h \
	QueueCfg.h \
	XMLAttribute.h XMLAttribute.cpp \
	XMLElement.h XMLElement.cpp \
	XMLDocument.h XMLDocument.cpp \
	XMLNode.h XMLNode.cpp \
	XMLTextNode.h XMLTextNode.cpp

AM_CXXFLAGS=-I$(top_srcdir) `xml2-config --cflags` -Wall
