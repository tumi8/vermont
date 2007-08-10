ADD_EXECUTABLE(testCollector
	testCollector.cpp
)

TARGET_LINK_LIBRARIES(testCollector
	concentrator
	ipfixlolib
	common
	pthread
	pcap
)

