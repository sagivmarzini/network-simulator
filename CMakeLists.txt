cmake_minimum_required(VERSION 3.20)

project(network-simulator LANGUAGES C)

set(CMAKE_C_STANDARD 11)

find_package(ZLIB REQUIRED)

add_executable(network-simulator
        main.c
        physical.h
        physical.c
        ethernet.h
        packet.h
        ethernet.c
)

target_link_libraries(network-simulator PRIVATE ZLIB::ZLIB)
