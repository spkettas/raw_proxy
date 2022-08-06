#pragma once

#include "common_def.h"
#include <cstddef>
#include <ctime>
#include <deque>
#include <map>
#include <mutex>
#include <unordered_map>

struct AddressPair {
  AddressPair(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port,
              uint16_t dst_port)
      : src_ip_(src_ip), dst_ip_(dst_ip), src_port_(src_port),
        dst_port_(dst_port) {}
  uint32_t src_ip_;
  uint32_t dst_ip_;
  uint16_t src_port_;
  uint16_t dst_port_;
};

struct AddressPairHash {
  std::size_t operator()(const AddressPair &key) const {
    return key.src_ip_ ^ key.dst_ip_ ^ key.src_port_ ^ key.dst_port_;
  }
};

struct AddressPairEqual {
  bool operator()(const AddressPair &lhs, const AddressPair &rhs) const {
    return lhs.src_ip_ == rhs.src_ip_ && lhs.dst_ip_ == rhs.dst_ip_ &&
           lhs.src_port_ == rhs.src_port_ && lhs.dst_port_ == rhs.dst_port_;
  }
};

struct OriginAddrInfo {
  OriginAddrInfo() {}
  OriginAddrInfo(uint32_t dst_ip, uint16_t dst_port, sockaddr *forward_addr,
                 time_t ts)
      : origin_ip_(dst_ip), origin_port_(dst_port),
        forward_addr_(*forward_addr), ts_(ts) {}
  uint32_t origin_ip_;
  uint16_t origin_port_;
  sockaddr forward_addr_;
  time_t ts_;
};

class AddressRewriter {
public:
  int init();
  int forward(PacketInfo *packet_info, sockaddr *forward_addr);
  int recover(PacketInfo *packet_info, sockaddr &forward_addr);

private:
  int purge_flows(time_t now);

  static unsigned short check_sum(int sum, unsigned short *address, int len);
  static unsigned short check_sum_pseudo(unsigned char *buf, uint16_t len,
                                         uint32_t src, uint32_t dst,
                                         uint8_t proto);

private:
  std::unordered_map<AddressPair, OriginAddrInfo, AddressPairHash,
                     AddressPairEqual>
      tcp_flows_;
  time_t last_purge_time_;

  std::mutex flow_lock_;
};
