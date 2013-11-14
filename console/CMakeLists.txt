PROJECT(tboot)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

ADD_EXECUTABLE(tboot 
	src/loader.cpp
	src/config.cpp
	src/device.cpp
	src/datafile.cpp
	src/filestream.cpp
	src/strutils.cpp
	src/timer.cpp
	src/rs232.cpp
	src/ui.cpp
)


find_package(GLog REQUIRED)
include_directories(${GLOG_INCLUDE_DIR})
#set(LIBS ${LIBS} ${GLOG_LIBRARY})
target_link_libraries (tboot ${GLOG_LIBRARY})


#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections")
#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-gc-sections" )
#SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-gc-sections" )
#SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-gc-sections" )
