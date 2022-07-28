#include "address_rewriter.h"

#include <string.h>

#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 56379

const int FLOW_TIMEOUT = 1800;
const int PURGE_INTERVAL = 3600;
const int MAX_TCP_FLOWS = 100000;

int AddressRewriter::init() {
  last_purge_time_ = time(nullptr);
  return 0;
}

int AddressRewriter::forward(PacketInfo *packet_info, sockaddr *forward_addr) {
  auto ip_header = (iphdr *)packet_info->packet_data;
  int ip_header_len = ((int)ip_header->ihl & 0xf) * 4;
  if (ip_header->protocol == IPPROTO_TCP) {
    auto tcp_header = (tcphdr *)((char *)ip_header + ip_header_len);

    uint32_t origin_daddr = ip_header->daddr;
    ip_header->daddr = packet_info->hp_addr.sin_addr.s_addr;

    ip_header->check = 0;
    ip_header->check = check_sum(0, (uint16_t *)ip_header, ip_header_len);

    uint16_t origin_dport = tcp_header->dest;
    tcp_header->dest = packet_info->hp_addr.sin_port;

    tcp_header->check = 0;
    tcp_header->check =
        check_sum_pseudo((unsigned char *)ip_header + ip_header_len,
                         ntohs(ip_header->tot_len) - ip_header_len,
                         ip_header->saddr, ip_header->daddr, IPPROTO_TCP);

    AddressPair address_pair(ip_header->saddr, ip_header->daddr,
                             tcp_header->source, tcp_header->dest);

    {
      std::lock_guard<std::mutex> gurad(flow_lock_);

      auto it = tcp_flows_.find(address_pair);
      time_t now = time(nullptr);
      if (it == tcp_flows_.end()) {
        if (last_purge_time_ + PURGE_INTERVAL < now) {
          purge_flows(now);
        }
        if (tcp_flows_.size() >= MAX_TCP_FLOWS) {
          printf("Too manny flows, can not create new");
          return 1;
        }
        tcp_flows_.insert(std::make_pair(
            address_pair,
            OriginAddrInfo(origin_daddr, origin_dport, forward_addr, now)));
      } else {
        it->second.ts_ = now;
      }
    }

    return 0;
  }

  printf("Not tcp %u", ip_header->protocol);
  return 1;
}

int AddressRewriter::recover(PacketInfo *packet_info, sockaddr &forward_addr) {
  auto ip_header = (iphdr *)packet_info->packet_data;
  int ip_header_len = ((int)ip_header->ihl & 0xf) * 4;
  if (ip_header->protocol == IPPROTO_TCP) {
    auto tcp_header = (tcphdr *)((char *)ip_header + ip_header_len);

    AddressPair address_pair(ip_header->daddr, ip_header->saddr,
                             tcp_header->dest, tcp_header->source);
    OriginAddrInfo addr_info;

    {
      std::lock_guard<std::mutex> guard(flow_lock_);

      auto it = tcp_flows_.find(address_pair);
      if (it == tcp_flows_.end()) {
        printf("Unknown flow, %u, %u, %u, %u", ip_header->saddr,
               ip_header->daddr, tcp_header->source, tcp_header->dest);
        return 1;
      }
      it->second.ts_ = time(nullptr);
      addr_info = it->second;
    }

    ip_header->saddr = addr_info.origin_ip_;
    ip_header->check = 0;
    ip_header->check = check_sum(0, (uint16_t *)ip_header, ip_header_len);

    tcp_header->source = addr_info.origin_port_;

    tcp_header->check = 0;
    tcp_header->check =
        check_sum_pseudo((unsigned char *)ip_header + ip_header_len,
                         ntohs(ip_header->tot_len) - ip_header_len,
                         ip_header->saddr, ip_header->daddr, IPPROTO_TCP);

    memset(&packet_info->hp_addr, 0, sizeof(packet_info->hp_addr));
    packet_info->hp_addr.sin_family = AF_INET;
    packet_info->hp_addr.sin_addr.s_addr = ip_header->daddr;
    packet_info->hp_addr.sin_port = tcp_header->dest;

    forward_addr = addr_info.forward_addr_;
    return 0;
  }

  printf("Not tcp %u\n", ip_header->protocol);
  return 1;
}

int AddressRewriter::purge_flows(time_t now) {
  int cnt = 0;
  auto it = tcp_flows_.begin();
  while (it != tcp_flows_.end()) {
    if (now - it->second.ts_ > FLOW_TIMEOUT) {
      it = tcp_flows_.erase(it);
      cnt++;
    } else {
      ++it;
    }
  }
  printf("Purge flows, %d", cnt);
  return cnt;
}

uint16_t AddressRewriter::check_sum(int sum, uint16_t *address, int len) {
  uint16_t *w = address;
  uint16_t answer = 0;
  while (len > 1) {
    sum += *w++;
    len -= 2;
  }
  if (len == 1) {
    *(unsigned char *)(&answer) = *(unsigned char *)w;
    sum += answer;
  }
  while (sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }
  answer = ~sum;
  return answer;
}

uint16_t AddressRewriter::check_sum_pseudo(unsigned char *buf, uint16_t len,
                                           uint32_t src, uint32_t dst,
                                           uint8_t proto) {
  uint32_t sum = 0;
  sum += (src & 0xffffU);
  sum += ((src >> 16) & 0xffffU);
  sum += (dst & 0xffffU);
  sum += ((dst >> 16) & 0xffffU);
  sum += htons((uint16_t)proto);
  sum += htons(len);
  return check_sum(sum, (uint16_t *)buf, len);
}
