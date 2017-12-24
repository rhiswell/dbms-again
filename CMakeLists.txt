cmake_minimum_required(VERSION 3.8)
project(dbms_again)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "-fpermissive" )

set(SOURCE_FILES
        buf.cpp buf.h bd.cpp bd.h main.cpp bd_test.cpp types.h buf_test.cpp)

add_executable(dbms_again ${SOURCE_FILES})
