cmake_minimum_required(VERSION 3.17)
project(cpm C)

set(CMAKE_C_STANDARD 99)

add_executable(cpm fsystest.c cpmfsys.h cpmfsys.c diskSimulator.h diskSimulator.c)

