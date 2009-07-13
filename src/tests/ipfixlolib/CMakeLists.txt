ADD_EXECUTABLE(test_everything
	test_everything.cc
)

TARGET_LINK_LIBRARIES(test_everything
	ipfixlolib
	common
)

ADD_EXECUTABLE(example_code
	example_code.cc
)

TARGET_LINK_LIBRARIES(example_code
	ipfixlolib
	common
)
