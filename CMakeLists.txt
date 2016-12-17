cmake_minimum_required(VERSION 3.6)
project(processes)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)

set(-pthread)
add_executable(processes ${SOURCE_FILES})

find_package(Threads REQUIRED)
target_link_libraries(processes Threads::Threads)