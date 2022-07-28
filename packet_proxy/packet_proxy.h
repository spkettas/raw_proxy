/**
 * Copyright (c) 1998-2020 Github Inc. All rights reserved.
 *
 * @file pkt_tool.h
 * @author spkettas
 * @date 2022-04-25
 *
 * @brief 代理工具
 */

#pragma once

#include <arpa/inet.h>

#include <string>

#include "common_def.h"

class CPktProxy {
public:
  CPktProxy(const std::string &eth, int port, const std::string &peer_ip,
            int peer_port, const std::string &honey_ip, int honey_port);
  ~CPktProxy();

  bool Init();
  void Run();

  // 处理帧
  void HandleFrame(char *pdata, int n);

  // 负载均衡选择服务器
  void SelectLB(struct PacketInfo *packet_info);

  // raw socket 抓取流量
  void CapturePkt();
  // 收取响应包
  int DownStream();

private:
  std::string eth_;
  int dstport_;

  int udp_forward_sock_ = 0;
  sockaddr_in addr_srv_;
  std::string peer_ip_;
  int peer_port_;
  std::string honey_ip_;
  int honey_port_;

  PacketInfo *packet_info_ = nullptr;
};
