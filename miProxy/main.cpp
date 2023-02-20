#include "miProxy.h"

int main(int argc, char* argv[]) {
    try {
        MiProxy miProxy;
        miProxy.get_options(argc, argv);
        miProxy.init();
        miProxy.run();
    } catch (runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
