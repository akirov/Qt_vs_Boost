cmake_minimum_required(VERSION 2.8)

project(Cpp11_POSIX_Server)

set(SERVER_SRCS
    Server.hpp
    Server.cpp 
    Server_main.cpp
)

add_executable(server ${SERVER_SRCS})
