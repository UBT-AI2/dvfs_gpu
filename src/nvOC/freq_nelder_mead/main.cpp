#include <stdlib.h>
#include <stdio.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "../script_running/process_management.h"
#include "freq_nelder_mead.h"

using namespace frequency_scaling;

int main(int argc, char **argv) {
    if (argc < 8) {
        printf("Usage: %s <currency_type> <device_id> <max_iterations> <mem_step> "
               "<graph_idx_step> <min_mem_oc> <max_mem_oc> [<min_hashrate>]", argv[0]);
        return 1;
    }
    currency_type ct = string_to_currency_type(argv[1]);
    unsigned int device_id = atoi(argv[2]);
    int max_iterations = atoi(argv[3]);
    int mem_step = atoi(argv[4]);
    int graph_idx_step = atoi(argv[5]);
    int min_mem_oc = atoi(argv[6]);
    int max_mem_oc = atoi(argv[7]);
    double min_hashrate = -1.0;
    if (argc > 8)
        min_hashrate = atof(argv[8]);

    //init apis
    nvapiInit();
    nvmlInit_();
    process_management::register_process_cleanup_sighandler();

    //start power monitoring
    start_power_monitoring_script(device_id);

    //
    device_clock_info dci(device_id, min_mem_oc, 0, max_mem_oc, 0);

    //
    const measurement &m = freq_nelder_mead(ct, dci, 1, max_iterations, mem_step, graph_idx_step,
                                            min_hashrate);
    printf("Best energy-hash value: %f\n", m.energy_hash_);

    //stop power monitoring
    stop_power_monitoring_script(device_id);

    //unload apis
    nvapiUnload(1);
    nvmlShutdown_(true);
    return 0;
}
