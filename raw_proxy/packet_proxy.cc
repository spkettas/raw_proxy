/**
 * Copyright (c) 1998-2020 Github Inc. All rights reserved.
 *
 * @file pkt_receiver.cpp
 * @author spkettas
 * @date 2022-04-19
 *
 * @brief 数据包引流工具
 */
#include "packet_proxy.h"

#include <arpa/inet.h>
#include <assert.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <pcap.h>
#include <thread>
#include <yaml-cpp/yaml.h>

using namespace std;

void GetPacket(u_char *arg, const struct pcap_pkthdr *pkthdr,
               const u_char *packet) {
  CPktProxy *pThis = (CPktProxy *)arg;
  pThis->HandleFrame((char *)packet, pkthdr->len);
}

CPktProxy::CPktProxy(const string &dir) : dir_(std::move(dir)) {}

CPktProxy::~CPktProxy() {
  if (packet_info_) {
    delete[] packet_info_;
    packet_info_ = nullptr;
  }
}

bool CPktProxy::Init() {
  string path = dir_ + "/conf/app.yaml";
  printf("config path: %s\n", path.c_str());
  auto config = YAML::LoadFile(path);

  eth_ = config["Eth"].as<string>();
  dstport_ = config["Port"].as<int>();
  peer_ip_ = config["NatServer"].as<string>();
  peer_port_ = config["NatPort"].as<int>();
  honey_ip_ = config["RealServer"].as<string>();
  honey_port_ = config["RealPort"].as<int>();

  udp_forward_sock_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_forward_sock_ < 0) {
    printf("Create send udp socket failed, %s", strerror(errno));
    return false;
  }

  // addr
  memset(&addr_srv_, 0, sizeof(sockaddr_in));
  addr_srv_.sin_family = AF_INET;
  addr_srv_.sin_addr.s_addr = inet_addr(peer_ip_.c_str());
  addr_srv_.sin_port = htons(peer_port_);

  // honey server
  packet_info_ = (PacketInfo *)new char[MAX_PKT_INFO_LEN];
  packet_info_->hp_addr.sin_family = AF_INET;

  sockaddr_in addr_send;
  memset(&addr_send, 0, sizeof(sockaddr_in));
  addr_send.sin_family = AF_INET;
  addr_send.sin_addr.s_addr = INADDR_ANY;
  addr_send.sin_port = htons(62001);

  // bind
  if (bind(udp_forward_sock_, (sockaddr *)&addr_send, sizeof(addr_send)) < 0) {
    close(udp_forward_sock_);
    printf("Bind failed, %s\n", strerror(errno));
    return false;
  }

  printf("listen eth=%s\tfilter_port=%d\n"
         "peer_ip=%s\tpeer_port=%d\n"
         "vm_ip=%s\tvm_port=%d\n\n",
         eth_.c_str(), dstport_, peer_ip_.c_str(), peer_port_,
         honey_ip_.c_str(), honey_port_);

  return true;
}

void CPktProxy::CapturePkt() {
  char err_buff[1024] = {};

  // timeout 一定要设置值，设置为0则无限等待
  // pcap_t* device = pcap_open_live(eth_.c_str(), 65535, 1, 0, err_buff);
  pcap_t *device = pcap_open_live(eth_.c_str(), 65535, 1, 10, err_buff);
  if (device == nullptr) {
    printf("ERROR: open pcap %s\n", err_buff);
    exit(1);
  }

  if (dstport_ > 0) {
    char exp[64] = {};
    sprintf(exp, "tcp and port %d", dstport_);
    printf("exp: %s\n", exp);

    struct bpf_program filter;
    if (pcap_compile(device, &filter, exp, 1, 0) == -1) {
      printf("ERROR: pcap_compile %s\n", pcap_geterr(device));
      exit(1);
    }

    if (pcap_setfilter(device, &filter) == -1) {
      printf("ERROR: pcap_setfilter %s\n", pcap_geterr(device));
      exit(1);
    }
  }

  pcap_loop(device, -1, GetPacket, (u_char *)this);
  // pcap_close(device);
  return;
}

void CPktProxy::SelectLB(struct PacketInfo *packet_info) {
  // 模拟选择一台服务器
  packet_info_->hp_addr.sin_addr.s_addr = inet_addr(honey_ip_.c_str());
  packet_info_->hp_addr.sin_port = htons(honey_port_);
}

void CPktProxy::HandleFrame(char *pdata, int len) {
  struct ethhdr *pe;
  struct iphdr *iphead;
  struct tcphdr *tcp;

  int offset = 0;
  pe = (struct ethhdr *)pdata;

  if (ntohs(pe->h_proto) == ETHERTYPE_VLAN) {
    offset = 4;
  } else if (ntohs(pe->h_proto) != ETHERTYPE_IP) {
    return;
  }

  /// ip
  iphead = (struct iphdr *)(pdata + offset + sizeof(struct ethhdr));
  if (NULL == iphead) {
    return;
  }

  /// tcp
  tcp = (struct tcphdr *)((char *)iphead + iphead->ihl * 4);
  if (NULL == tcp) {
    return;
  }

  auto ShowPkg = [&]() {
    char sip[64] = {};
    char dip[64] = {};

    struct in_addr saddr, daddr;
    saddr.s_addr = iphead->saddr;
    daddr.s_addr = iphead->daddr;

    strcpy(sip, inet_ntoa(saddr));
    strcpy(dip, inet_ntoa(daddr));

    printf(">>> %s:%d -> %s:%d len: %d\n", sip, ntohs(tcp->source), dip,
           ntohs(tcp->dest), packet_info_->packet_len);
  };

  // 负载均衡
  SelectLB(packet_info_);

  // 截取L2层数据，dstip指向真实地址，并包装头部
  packet_info_->packet_len = len - ((char *)iphead - (char *)pdata);
  memcpy(packet_info_->packet_data, (char *)iphead, packet_info_->packet_len);
  uint32_t total_len = sizeof(PacketInfo) + packet_info_->packet_len;

  // ShowPkg();
  printf("send to rs=%s len=%d use udp\n", inet_ntoa(addr_srv_.sin_addr),
         total_len);

  // udp隧道转发
  int ret = sendto(udp_forward_sock_, packet_info_, total_len, 0,
                   (sockaddr *)&addr_srv_, sizeof(sockaddr_in));
  if (ret <= 0) {
    printf("Send udp pkt failed, %s\n", strerror(errno));
  }
}

int CPktProxy::DownStream() {
  int raw_socket = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
  if (raw_socket < 0) {
    printf("Create raw socket failed, %s\n", strerror(errno));
    return 1;
  }

  int hdrincl = 1;
  if (setsockopt(raw_socket, IPPROTO_IP, IP_HDRINCL, &hdrincl,
                 sizeof(hdrincl)) < 0) {
    close(raw_socket);
    printf("Setup raw socket failed, %s\n", strerror(errno));
    return 1;
  }

  PacketInfo *packet_info = (PacketInfo *)new char[MAX_PKT_INFO_LEN];
  sockaddr src_addr;
  socklen_t addrlen = sizeof(src_addr);

  while (true) {
    int ret = recvfrom(udp_forward_sock_, packet_info, MAX_PKT_INFO_LEN, 0,
                       &src_addr, &addrlen);
    if (ret <= 0) {
      printf("Receive from udp socket failed, %s", strerror(errno));
      continue;
    }

    printf("send to client=%s len=%d use raw\n",
           inet_ntoa(packet_info->hp_addr.sin_addr), ret);
    ret = sendto(raw_socket, packet_info->packet_data, packet_info->packet_len,
                 0, (sockaddr *)&packet_info->hp_addr, sizeof(sockaddr_in));
    if (ret <= 0) {
      printf("Send to raw socket failed, %s", strerror(errno));
      continue;
    }
  }

  delete[] packet_info;
  close(raw_socket);
  return 0;
}

void CPktProxy::Run() {
  // 启动收包
  std::thread down_thread(&CPktProxy::DownStream, this);

  // 启动发包
  CapturePkt();
}
