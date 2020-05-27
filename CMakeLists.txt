cmake_minimum_required(VERSION 3.15)
project(thread_pool)

set(CMAKE_CXX_STANDARD 14)

add_executable(thread_pool threadPool.h main.c)

TARGET_LINK_LIBRARIES(thread_pool pthread)