#include <stdlib.h>
#include <stdio.h>
#include "nvmlOC.h"

using namespace frequency_scaling;

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s <device_id> <graph_freq> <mem_freq>\n", argv[0]);
        return 1;
    }
    int device_id = atoi(argv[1]);
    int graphClock = atoi(argv[2]);
    int memClock = atoi(argv[3]);
    //
    try {
        nvmlInit_();
        nvmlOC(device_id, graphClock, memClock);
        printf("Changed GPU %i clocks: mem:%iMHz graph:%iMHz\n", device_id, memClock, graphClock);
        nvmlShutdown_(false);
    } catch (const std::exception &ex) {
        printf("Overclocking failed: %s\n", ex.what());
        nvmlShutdown_(true);
        return 1;
    }
    return 0;
}