#include <stdlib.h>
#include <stdio.h>
#include "nvmlOC.h"

using namespace frequency_scaling;

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s device_id core_oc vram_oc\n", argv[0]);
        return 1;
    }
    //
    try {
        nvmlInit_();
        nvmlOC(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
        nvmlShutdown_(false);
    } catch (const std::exception &ex) {
        nvmlShutdown_(true);
        return 1;
    }
    return 0;
}