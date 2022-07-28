#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>
#include "packet_forwarder.h"

using namespace std;

const char* const TUN_DEV_NAME = "tun_hp";

NatServer::NatServer(const std::string& listen_ip, int listen_port)
    : listen_ip_(std::move(listen_ip)), listen_port_(listen_port) {}

NatServer::~NatServer() {}

bool NatServer::Init() {
    // 初始化虚拟网卡
    if (0 != g_tun_hp.init(TUN_DEV_NAME)) {
        return false;
    }

    g_port_forwarder.init();
    return true;
}

int NatServer::Upstream() {
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        printf("Create udp socket failed, %s", strerror(errno));
        return -1;
    }

    sockaddr_in addr_serv;
    memset(&addr_serv, 0, sizeof(sockaddr_in));
    addr_serv.sin_family      = AF_INET;
    addr_serv.sin_addr.s_addr = inet_addr(listen_ip_.c_str());
    addr_serv.sin_port        = htons(listen_port_);
    if (bind(udp_sock, (sockaddr*)&addr_serv, sizeof(addr_serv)) < 0) {
        printf("bind failed, %s\n", strerror(errno));
        close(udp_sock);
        return -1;
    }

    PacketInfo* packet_info = (PacketInfo*)new char[MAX_PKT_INFO_LEN];
    sockaddr    src_addr;
    socklen_t   addrlen = sizeof(src_addr);

    while (true) {
        int ret = recvfrom(udp_sock, packet_info, MAX_PKT_INFO_LEN, 0,
                           &src_addr, &addrlen);
        if (ret <= 0) {
            printf("Receive from udp socket failed, %s", strerror(errno));
            break;
        }

        printf("write to tunap len=%d\n", packet_info->packet_len);
        if (0 != g_port_forwarder.forward(packet_info, &src_addr)) {
            continue;
        }

        ret = g_tun_hp.write_packet(packet_info->packet_data,
                                    packet_info->packet_len);
        if (ret < 0) {
            printf("Write to tun failed, %d", ret);
            break;
        }
    }

    delete[](char*) packet_info;
    close(udp_sock);

    return 0;
}

// 下行数据包处理函数
// 将响应数据包发送给代理
int NatServer::Downstream() {
    int udp_send_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_send_sock < 0) {
        printf("Create send udp socket failed, %s", strerror(errno));
        return -1;
    }

    PacketInfo* packet_info = (PacketInfo*)new char[MAX_PKT_INFO_LEN];
    sockaddr    forward_addr;
    while (true) {
        int ret = g_tun_hp.read_packet(packet_info->packet_data, IP_PACKET_LEN);
        if (ret <= 0) {
            printf("Read from tun failed, %d", ret);
            break;
        }

        printf("recv from tuntap len=%d\n", ret);
        packet_info->packet_len = ret;

        if (0 != g_port_forwarder.recover(packet_info, forward_addr)) {
            continue;
        }

        ret = sendto(udp_send_sock, packet_info, sizeof(PacketInfo) + ret, 0,
                     (sockaddr*)&forward_addr, sizeof(sockaddr));
        if (ret <= 0) {
            printf("Send to udp socket failed, %s", strerror(errno));
            break;
        }
    }

    close(udp_send_sock);

    return 0;
}

void NatServer::Run() {
    std::thread up_thread(&NatServer::Upstream, this);
    std::thread down_thread(&NatServer::Downstream, this);

    up_thread.join();
    down_thread.join();
}
