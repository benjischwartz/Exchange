cmake_minimum_required(VERSION 3.14)
project(Exchange)

set(CMAKE_CXX_STANDARD 23)

add_library(exchange_lib exchange.cpp)
add_executable(exchange main.cpp)
