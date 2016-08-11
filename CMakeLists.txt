cmake_minimum_required(VERSION 3.2)

add_executable(
    calc
    calc.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -g -O0")


## Crutches ##

if (CYGWIN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--image-base -Wl,0x10000000" )
endif (CYGWIN)
