cmake_minimum_required(VERSION 4.0)
project(multiThreads)

set(CMAKE_CXX_STANDARD 20)

add_executable(multiThreads main.cpp
        Thread.cpp
        Thread.h)
