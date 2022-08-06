/**
 * Copyright (c) 1998-2020 Github Inc. All rights reserved.
 *
 * @file main.cc
 * @author spkettas
 * @date 2022-05-01
 *
 * @brief 服务引流测试工具
 */
#include "packet_proxy.h"
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>

/// main
int main(int argc, char **argv) {
  CPktProxy pkt_tool(dirname(argv[0]));
  pkt_tool.Init();
  pkt_tool.Run();

  return 0;
}
