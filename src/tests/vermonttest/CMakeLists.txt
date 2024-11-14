#
# VERMONT build scripts for CMake
# Copyright (C) 2007 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

ADD_EXECUTABLE(vermonttest
	test_concentrator.cpp
	TestSuiteBase.cpp
	AggregationPerfTest.cpp
	ReconfTest.cpp
	VermontTest.cpp
	BloomFilterTest.cpp 
	ConnectionFilterTest.cpp
	ConfigTester.cpp
	PrinterModule.cpp
)

TARGET_LINK_LIBRARIES(vermonttest
	modules
        ipfixlolib  
	core
        common        
        osdep      
	${CMAKE_THREAD_LIBS_INIT}
	Boost::regex
	Boost::filesystem
	Boost::system
	${PCAP_LIBRARY}
	${LIBXML2_LIBRARIES}
)

IF (MYSQL_FOUND)
	TARGET_LINK_LIBRARIES(vermonttest
		${MYSQL_LIBRARIES}
	)
ENDIF (MYSQL_FOUND)

IF (CONNECTION_FILTER)
	TARGET_LINK_LIBRARIES(vermonttest
		${GSL_LIBRARIES}
	)
ENDIF (CONNECTION_FILTER)

IF (JOURNALD_FOUND)
	TARGET_LINK_LIBRARIES(vermonttest
		${JOURNALD_LIBRARIES}
	)
ENDIF (JOURNALD_FOUND)

ADD_TEST(vermonttest_build "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target vermonttest --config $<CONFIG>)
ADD_TEST(vermont vermonttest -c "${CMAKE_CURRENT_SOURCE_DIR}/test_configs/")
SET_TESTS_PROPERTIES(vermont PROPERTIES DEPENDS vermonttest_build)
