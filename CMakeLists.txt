cmake_minimum_required(VERSION 3.20)

project(network-simulator LANGUAGES C)

set(CMAKE_C_STANDARD 11)

add_executable(network-simulator
        main.c
        device.h
        device.c
        ethernet.h
        packet.h
        ethernet.c
)
