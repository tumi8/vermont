noinst_LIBRARIES=libcfg.a

libcfg_a_SOURCES= \
	Edge.h Edge.cpp \
	Graph.h Graph.cpp \
	Node.h Node.cpp \
	XMLAttribute.h XMLAttribute.cpp \
	XMLElement.h XMLElement.cpp \
	XMLDocument.h XMLDocument.cpp \
	XMLNode.h XMLNode.cpp \
	XMLTextNode.h XMLTextNode.cpp

AM_CXXFLAGS=-I$(top_srcdir) `xml2-config --cflags` -Wall
