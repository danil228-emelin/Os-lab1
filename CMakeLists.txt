cmake_minimum_required(VERSION 3.30)
project(CustomShell)

set(CMAKE_CXX_STANDARD 23)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES
        include/util.h
        include/process.h

        src/main.cpp
        src/util.cpp
        src/process.cpp
)

add_executable(CustomShell ${SOURCES})
