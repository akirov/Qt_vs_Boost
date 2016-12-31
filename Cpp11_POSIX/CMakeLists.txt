cmake_minimum_required(VERSION 2.8)

project(Cpp11_POSIX_Client_Server)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(client)
#add_subdirectory(server)
