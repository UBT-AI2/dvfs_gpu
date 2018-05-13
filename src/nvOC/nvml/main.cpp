#include <stdlib.h>
#include <stdio.h>
#include "nvmlOC.h"

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s device_id core_oc vram_oc\n", argv[0]);
        return 1;
    }
    //
    using namespace frequency_scaling;
    nvmlInit_();
    try {
        nvmlOC(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
    } catch (const std::exception &ex) {
        nvmlShutdown_();
    }
    nvmlShutdown_();
    return 0;
}