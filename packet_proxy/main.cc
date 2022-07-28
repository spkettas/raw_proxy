/**
 * Copyright (c) 1998-2020 Github Inc. All rights reserved.
 *
 * @file main.cc
 * @author spkettas
 * @date 2022-05-01
 *
 * @brief 蜜罐引流测试工具
 */
#include "packet_proxy.h"
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>

static void Usage(char *prog) {
  printf("Usage: %s [OPTION]\n", prog);
  printf("  -i, --interface=eth0\t\t\tInterface name\n");
  printf("  -R, --remote addr\t\t\tRemote real ip\n");
  printf("  -r, --remote port\t\t\tRemote real port\n");
  printf("  -P, --dstport=80\t\t\tDestination port\n");
  printf("  -h, --vm_ip=127.29.110.31\t\t\tVm ip\n");
  printf("  -p, --vm_port=27017\t\t\tVm port\n");
  printf("  -v, --help\t\t\t\tDisplay this help and exit\n");
}

/// main
int main(int argc, char **argv) {
  struct option long_options[] = {{"interface", required_argument, 0, 'i'},
                                  {"port", required_argument, 0, 'P'},
                                  {"peer_ip", required_argument, 0, 'R'},
                                  {"peer_port", required_argument, 0, 'r'},
                                  {"vm_ip", required_argument, 0, 'h'},
                                  {"vm_port", required_argument, 0, 'p'},
                                  {"help", no_argument, 0, 'v'},
                                  {0, 0, 0, 0}};

  std::string eth;
  int port = 0;
  std::string peer_ip = "127.0.0.1";
  int peer_port = 62000;
  std::string honey_ip;
  int honey_port = 8080;
  int c = 0;

  while ((c = getopt_long(argc, argv, "i:R:r:P:h:p:cv", long_options,
                          nullptr)) != -1) {
    switch (c) {
    case 'i':
      eth = optarg;
      break;
    case 'R':
      peer_ip = optarg;
      break;
    case 'r':
      peer_port = atoi(optarg);
      break;
    case 'P':
      port = atoi(optarg);
      break;
    case 'h':
      honey_ip = optarg;
      break;
    case 'p':
      honey_port = atoi(optarg);
      break;
    case 'v':
      Usage(basename(argv[0]));
      return 1;
    default:
      Usage(basename(argv[0]));
      return 1;
    }
  }

  CPktProxy pkt_tool(eth, port, peer_ip, peer_port, honey_ip, honey_port);
  pkt_tool.Init();
  pkt_tool.Run();

  return 0;
}
