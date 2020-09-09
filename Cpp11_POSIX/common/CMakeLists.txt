cmake_minimum_required(VERSION 3.1.0)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

set (COMMON_SOURCES log.hpp log.cpp)

add_library (common STATIC ${COMMON_SOURCES})
