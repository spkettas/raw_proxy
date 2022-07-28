project(nat_server)

get_filename_component(MYCODE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/" ABSOLUTE)
set(SRC_DIR ${MYCODE_ROOT})

add_executable(nat_server
    ${COMM_DIR}/tap_tun.cpp
    ${SRC_DIR}/packet_forwarder.cpp
    ${SRC_DIR}/address_rewriter.cpp
    ${SRC_DIR}/main.cc)
target_link_libraries(nat_server pthread pcap)
