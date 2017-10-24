cmake_minimum_required(VERSION 3.8)
project(dbms_again)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        bd.cpp bd.h buf.cpp buf.h main.cpp)

add_executable(dbms_again ${SOURCE_FILES})