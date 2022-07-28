#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include "packet_forwarder.h"

static void Usage(char* prog) {
    printf("Usage: %s [OPTION]\n", prog);
    printf("  -h, --host=127.0.0.1\t\t\tListen ip\n");
    printf("  -p, --port=62000\t\t\tListen port\n");
    exit(0);
}

///
int main(int argc, char* argv[]) {
    struct option long_options[] = {{"host", required_argument, 0, 'h'},
                                    {"port", required_argument, 0, 'p'},
                                    {"help", no_argument, 0, 'v'}};

    char        opt;
    std::string listen_ip;
    int         listen_port = 0;

    while (-1 !=
           (opt = getopt_long(argc, argv, "h:p:v", long_options, nullptr))) {
        switch (opt) {
            case 'h':
                listen_ip = optarg;
                break;
            case 'p':
                listen_port = atoi(optarg);
                break;
            case 'v':
                Usage(basename(argv[0]));
                break;
            default:
                Usage(basename(argv[0]));
                return 1;
        }
    }

    fprintf(stdout, "Started nat server\n");

    NatServer server(listen_ip, listen_port);
    server.Init();
    server.Run();

    return 0;
}
