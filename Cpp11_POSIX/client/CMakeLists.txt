cmake_minimum_required(VERSION 3.1.0)

project(Cpp11_POSIX_Client)

include_directories (${COMMON_DIR})

set(CLIENT_SRCS
    Client.hpp
    Client.cpp 
    Client_main.cpp
)

add_executable(client ${CLIENT_SRCS})

target_link_libraries(server common)
