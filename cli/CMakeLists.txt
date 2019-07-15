CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

project(SNetCli)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

aux_source_directory(./src cli_src)

include_directories(../snet)

add_executable(${PROJECT_NAME} ${cli_src})