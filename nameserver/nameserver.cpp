#include "utils.h"

int main(int argc, char **argv) {
    // Read Arguments
    Info info(argc, argv);
    // Operate
    switch (info.getMode()) {
        case Mode::RR:
            RoundRobin(info.getServers());
            break;
        case Mode::GEO:
            break;
        default:
            std::cerr << "Fail to Recognize Load Balancer" << std::endl;
            exit(1);
    }
    return 0;
}