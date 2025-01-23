cmake_minimum_required(VERSION 3.10)

set(PROJNAME "discovering-path-tracer")
project(${PROJNAME})


file(GLOB_RECURSE SOURCE_FILES 
    src/*.cpp
    src/*.c
    src/*.hpp
    src/*.h
)

add_executable(${PROJNAME} ${SOURCE_FILES})