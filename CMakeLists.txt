# Copyright (c) Facebook, Inc. and its affiliates. All Rights Reserved.
# See https://pytorch.org/tutorials/advanced/cpp_frontend.html
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(curl_test)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 11)
add_definitions( -DCURL_STATICLIB )  # only windows need

# curl
set(CURL_LIBRARY ${CMAKE_SOURCE_DIR}/lib)
set(CURL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
find_package(CURL)

# jsoncpp
set(JSONCPP_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/jsoncpp/)
set(JSONCPP_SOURCES_PATH ${CMAKE_SOURCE_DIR}/jsoncpp/lib_json)
SET(JSONCPP_SOURCES
    ${JSONCPP_SOURCES_PATH}/json_tool.h
    ${JSONCPP_SOURCES_PATH}/json_reader.cpp
    ${JSONCPP_SOURCES_PATH}/json_valueiterator.inl
    ${JSONCPP_SOURCES_PATH}/json_value.cpp
    ${JSONCPP_SOURCES_PATH}/json_writer.cpp)

set(SRCS "./main.cpp")

include_directories( ${CURL_INCLUDE_DIRS} ${JSONCPP_INCLUDE_DIR} )
link_directories( ${CURL_LIBRARIES} )

add_executable(curl_test ${SRCS} ${JSONCPP_SOURCES})
if(WIN32)
	target_link_libraries(curl_test libcurl_a.lib ws2_32 wldap32)
elseif(UNIX)
	target_link_libraries(curl_test libcurl.a pthread)
endif()
