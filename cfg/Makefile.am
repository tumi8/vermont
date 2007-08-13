noinst_LIBRARIES=libcfg.a

libcfg_a_SOURCES= \
	edge.h edge.cc \
	graph.h graph.cc \
	node.h node.cc \
	XMLElement.h XMLElement.cpp \
	XMLDocument.h XMLDocument.cpp \
	XMLNode.h XMLNode.cpp

AM_CXXFLAGS=-I$(top_srcdir) `xml2-config --cflags` -Wall
