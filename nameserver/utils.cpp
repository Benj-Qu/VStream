#include "utils.h"

Info::Info(int argc, char** argv) {
    // Check Argument Number
    if (argc != 5) {
        usage();
    }
    // Check and Read Load Balancer Mode
    if (strcmp(argv[1], "--rr") == 0) {
        this->mode = Mode::RR;
    }
    else if (strcmp(argv[1], "--geo") == 0) {
        this->mode = Mode::GEO;
    }
    else {
        usage();
    }
    // Read Port, Servers, and Log
    this->port = atoi(argv[2]);
    this->servers = argv[3];
    this->log = argv[4];
}

void Info::usage() {
    std::cerr << "./nameserver [--geo|--rr] <port> <servers> <log>" << std::endl;
    exit(1);
}