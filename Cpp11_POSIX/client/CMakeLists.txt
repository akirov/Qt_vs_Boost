cmake_minimum_required(VERSION 2.8)

project(Cpp11_POSIX_Client)

set(CLIENT_SRCS
    Client.hpp
    Client.cpp 
    Client_main.cpp
)

add_executable(client ${CLIENT_SRCS})
