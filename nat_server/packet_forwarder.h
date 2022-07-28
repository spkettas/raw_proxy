/**
 * Copyright (c) 1998-2020 Github Inc. All rights reserved.
 *
 * @file packet_forwarder.h
 * @author spkettas
 * @date 2022-07-23
 *
 * @brief nat转发器
 */
#pragma once

#include <string>
#include "address_rewriter.h"
#include "tap_tun.h"

class NatServer {
   public:
    NatServer(const std::string& listen_ip, int listen_port);
    ~NatServer();

   public:
    bool Init();

    // 上行数据包处理函数
    // 将请求数据包写入虚拟网卡
    int Upstream();

    // 下行数据包处理函数
    // 将响应数据包发送给代理
    int Downstream();

    void Run();

   private:
    std::string listen_ip_   = "127.0.0.1";
    int         listen_port_ = 62000;

    TapTun          g_tun_hp;
    AddressRewriter g_port_forwarder;
};