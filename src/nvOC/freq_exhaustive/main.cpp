#include <stdlib.h>
#include <stdio.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "../script_running/process_management.h"
#include "freq_exhaustive.h"

using namespace frequency_scaling;

int main(int argc, char **argv) {
    if (argc < 8) {
        printf("Usage: %s <currency_type> <device_id> <use_nvmlUC> <min_mem_oc> "
               "<max_mem_oc> <min_graph_oc> <max_graph_oc>", argv[0]);
        return 1;
    }
    currency_type ct = string_to_currency_type(argv[1]);
    unsigned int device_id = atoi(argv[2]);
    int interval = 50;
    int use_nvmlUC = atoi(argv[3]);
    int min_mem_oc = atoi(argv[4]), max_mem_oc = atoi(argv[5]);
    int min_graph_oc = atoi(argv[6]), max_graph_oc = atoi(argv[7]);
    if (use_nvmlUC)
        min_graph_oc = interval;

    //init apis
    nvapiInit();
    nvmlInit_();
    process_management::register_process_cleanup_sighandler();

    //start power monitoring
    start_power_monitoring_script(device_id);

    //
    device_clock_info dci(device_id, min_mem_oc, min_graph_oc, max_mem_oc, max_graph_oc);

    //
    const measurement &m = freq_exhaustive(ct, dci, interval, use_nvmlUC, 2);
    printf("Best energy-hash value: %f\n", m.energy_hash_);

    //stop power monitoring
    stop_power_monitoring_script(device_id);

    //unload apis
    nvapiUnload(1);
    nvmlShutdown_(true);
    return 0;
}
