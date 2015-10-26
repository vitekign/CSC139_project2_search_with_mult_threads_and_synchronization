cmake_minimum_required(VERSION 3.3)
project(csc139_SecondProject)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(csc139_SecondProject ${SOURCE_FILES})