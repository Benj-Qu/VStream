#include "utils.h"

int main(int argc, char **argv) {
    Info info(argc, argv);
    std::cout << info.mode << info.port 
              << info.servers << info.log << std::endl;
    return 0;
}