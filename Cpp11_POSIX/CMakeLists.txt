cmake_minimum_required(VERSION 3.1.0)

project(Cpp11_POSIX_Client_Server)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build/bin)

set(COMMON_DIR  ${CMAKE_SOURCE_DIR}/common)
set(SERVER_DIR  ${CMAKE_SOURCE_DIR}/server)
set(CLIENT_DIR  ${CMAKE_SOURCE_DIR}/client)

add_subdirectory(${COMMON_DIR})
add_subdirectory(${SERVER_DIR})
add_subdirectory(${CLIENT_DIR})
