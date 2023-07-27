cmake_minimum_required(VERSION 2.5)
project(mymuduo11)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGES} -g -std=c++11")


aux_source_directory(./net SRC_LIST)
aux_source_directory(./base SRC_LIST)

add_library(mymuduo11 SHARED ${SRC_LIST})