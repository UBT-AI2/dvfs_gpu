#include <stdlib.h>
#include <stdio.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "freq_nelder_mead.h"


int main(int argc, char **argv) {
    if (argc < 7) {
        printf("Usage: %s device_id max_iterations mem_step graph_idx_step min_mem_oc max_mem_oc [min_hashrate]",
               argv[0]);
        return 1;
    }
    unsigned int device_id = atoi(argv[1]);
    int max_iterations = atoi(argv[2]);
    int mem_step = atoi(argv[3]);
    int graph_idx_step = atoi(argv[4]);
    int min_mem_oc = atoi(argv[5]);
    int max_mem_oc = atoi(argv[6]);
    double min_hashrate = -1.0;
    if (argc > 7)
        min_hashrate = atof(argv[7]);

    using namespace frequency_scaling;
    //init apis
    nvapiInit();
    nvmlInit_();

    //start power monitoring
    start_power_monitoring_script(device_id);

    //
    device_clock_info dci(device_id, min_mem_oc, 0, max_mem_oc, 0);

    //
    const measurement &m = freq_nelder_mead(miner_script::ETHMINER, dci, 1, max_iterations, mem_step, graph_idx_step,
                                            min_hashrate);
    printf("Best energy-hash value: %f\n", m.energy_hash_);

    //stop power monitoring
    stop_power_monitoring_script(device_id);

    //unload apis
    nvapiUnload(1);
    nvmlShutdown_(true);
    return 0;
}
