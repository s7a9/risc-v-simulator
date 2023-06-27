cmake_minimum_required(VERSION 3.15)

project(risc_v_simulator)

set(CMAKE_CXX_STANDARD 17)

include_directories(./source/headers)

aux_source_directory(./source/src/ SRC_LIST)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/)

add_executable(code ${SRC_LIST})

set(CMAKE_CXX_FLAGS "-O3")