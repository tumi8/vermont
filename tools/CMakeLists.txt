ADD_EXECUTABLE(testCollector
	testCollector.cpp
)

TARGET_LINK_LIBRARIES(testCollector
	concentrator
	ipfixlolib
	common
	${CMAKE_THREAD_LIBS_INIT}
	${PCAP_LIBRARY}
)

