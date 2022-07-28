#include <fcntl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>


#include "tap_tun.h"

const char* const TUN_DEV = "/dev/net/tun";

TapTun::TapTun() : fd_(-1) {}

int TapTun::init(const char* dev_name) {
    fd_ = open(TUN_DEV, O_RDWR);
    if (fd_ < 0) {
        return -errno;
    }

    ifreq ifr = {0};
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_NO_PI | IFF_TUN;
    strncpy(ifr.ifr_name, dev_name, strlen(dev_name) + 1);

    if (ioctl(fd_, TUNSETIFF, (void*)&ifr) < 0) {
        return -errno;
    }

    return 0;
}

ssize_t TapTun::read_packet(char* buf, size_t len) const {
    return read(fd_, buf, len);
}

ssize_t TapTun::write_packet(const char* data, size_t len) const {
    return write(fd_, data, len);
}

int TapTun::execute_cmd(char* cmd_line[]) {
    pid_t pid = fork();
    if (pid < 0) {
        return -errno;
    } else if (pid > 0) {
        int status;
        if (-1 == waitpid(pid, &status, 0)) {
            return -errno;
        }
        if (0 != WEXITSTATUS(status)) {
            return WEXITSTATUS(status);
        }
        return 0;
    } else {
        execvp(cmd_line[0], cmd_line);
        return -errno;
    }
}
