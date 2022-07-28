cmake_minimum_required(VERSION 3.12)

project(net_forwarder 
  VERSION 1.0
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # -std=gnu++11

# ccls
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(MYCODE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE)
set(COMM_DIR ${MYCODE_ROOT}/comm)

# global
include_directories(
	${COMM_DIR})

add_subdirectory(packet_proxy)
add_subdirectory(nat_server)
