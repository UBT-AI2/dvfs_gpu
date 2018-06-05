#include <stdlib.h>
#include <stdio.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "../script_running/process_management.h"
#include "freq_exhaustive.h"


int main(int argc, char **argv) {
    if (argc < 7) {
        printf("Usage: %s device_id use_nvmlUC min_mem_oc max_mem_oc min_graph_oc max_graph_oc", argv[0]);
        return 1;
    }
    unsigned int device_id = atoi(argv[1]);
    int interval = 50;
    int use_nvmlUC = atoi(argv[2]);
    int min_mem_oc = atoi(argv[3]), max_mem_oc = atoi(argv[4]);
    int min_graph_oc = atoi(argv[5]), max_graph_oc = atoi(argv[6]);
    if (use_nvmlUC)
        min_graph_oc = interval;

    using namespace frequency_scaling;
    //init apis
    nvapiInit();
    nvmlInit_();
    process_management::register_process_cleanup_sighandler();

    //start power monitoring
    start_power_monitoring_script(device_id);

    //
    device_clock_info dci(device_id, min_mem_oc, min_graph_oc, max_mem_oc, max_graph_oc);

    //
    const measurement &m = freq_exhaustive(miner_script::ETHMINER, dci, interval, use_nvmlUC, 2);
    printf("Best energy-hash value: %f\n", m.energy_hash_);

    //stop power monitoring
    stop_power_monitoring_script(device_id);

    //unload apis
    nvapiUnload(1);
    nvmlShutdown_(true);
    return 0;
}
