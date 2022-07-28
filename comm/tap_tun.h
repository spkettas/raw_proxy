#pragma once

#include <cstddef>

class TapTun {
   public:
    TapTun();
    int     init(const char* dev_name);
    ssize_t read_packet(char* buf, size_t len) const;
    ssize_t write_packet(const char* data, size_t len) const;

   private:
    static int execute_cmd(char* cmd_line[]);

    int fd_;
};
