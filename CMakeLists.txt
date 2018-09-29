cmake_minimum_required(VERSION 3.12)
project(utm-api C)

set(CMAKE_C_STANDARD 99)
add_executable(utm-api src/main.c)
target_link_libraries (utm-api m)