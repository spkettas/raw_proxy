#pragma once

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/types.h>

struct PacketInfo {
    sockaddr_in hp_addr;         // 下一跳服务器
    uint16_t    packet_len;      // 隧道数据包长度
    char        packet_data[0];  // 隧道数据
};
// __attribute__((__packed__));

const int IP_PACKET_LEN = 64 * 1024;
const int MAX_PKT_INFO_LEN =
    IP_PACKET_LEN + sizeof(sockaddr_in) + sizeof(uint16_t);
