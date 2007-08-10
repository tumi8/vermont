ADD_EXECUTABLE(vermonttest
	test_concentrator.cpp
	AggregationPerfTest.cpp
	VermontTest.cpp
)

TARGET_LINK_LIBRARIES(vermonttest
	concentrator
	sampler
	ipfixlolib
	common
	${CMAKE_THREAD_LIBS_INIT}
	${Boost_REGEX_LIBRARY}
	${Boost_FILESYSTEM_LIBRARY}
	${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
	${PCAP_LIBRARY}
	${MYSQL_LIBRARIES}
	${LIBXML2_LIBRARIES}
)

