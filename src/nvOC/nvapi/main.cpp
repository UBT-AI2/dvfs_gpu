#include <stdlib.h>
#include <stdio.h>
#include <exception>
#include "nvapiOC.h"

using namespace frequency_scaling;

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s <device_id> <graph_oc> <mem_oc>\n", argv[0]);
        return 1;
    }
    int device_id = atoi(argv[1]);
    int graphOC = atoi(argv[2]);
    int memOC = atoi(argv[3]);
    //
    try {
        nvapiInit();
        nvapi_register_gpu(device_id);
        nvapiOC(device_id, graphOC, memOC);
        int graphClock = nvapiGetGraphClockInfo(device_id).current_freq_;
        int memClock = nvapiGetMemClockInfo(device_id).current_freq_;
        printf("Overclocked GPU %i: mem:%iMHz graph:%iMHz mem_oc:%iMHz graph_oc:%iMHz\n",
               device_id, memClock, graphClock, memOC, graphOC);
        nvapiUnload(0);
    } catch (const std::exception &ex) {
        printf("Overclocking failed: %s\n", ex.what());
        nvapiUnload(1);
        return 1;
    }
    return 0;
}
