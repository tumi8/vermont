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
	boost_regex
	boost_filesystem
	boost_unit_test_framework
	pcap
	${MYSQL_LIBRARIES}
	${LIBXML2_LIBRARIES}
)

