project(packet_proxy)

get_filename_component(MYCODE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE)
set(SRC_DIR ${MYCODE_ROOT})

add_executable(packet_proxy
    ${COMM_DIR}/tap_tun.cpp
    ${SRC_DIR}/packet_proxy.cc
    ${SRC_DIR}/main.cc)
target_link_libraries(packet_proxy pthread pcap)
