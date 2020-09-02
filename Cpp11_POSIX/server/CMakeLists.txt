cmake_minimum_required(VERSION 3.1.0)

project(Cpp11_POSIX_Server)

include_directories (${COMMON_DIR})

set(SERVER_SRCS
    Server.hpp
    Server.cpp 
    Server_main.cpp
)

add_executable(server ${SERVER_SRCS})

target_link_libraries(server common)
