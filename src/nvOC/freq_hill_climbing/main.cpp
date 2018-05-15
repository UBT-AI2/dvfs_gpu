#include <stdlib.h>
#include <stdio.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../benchmark/benchmark.h"
#include "freq_hill_climbing.h"


int main(int argc, char **argv) {
    if (argc < 7) {
        printf("Usage: %s device_id max_iterations mem_step graph_idx_step min_mem_oc max_mem_oc", argv[0]);
        return 1;
    }
    unsigned int device_id = atoi(argv[1]);
    int max_iterations = atoi(argv[2]);
    int mem_step = atoi(argv[3]);
    int graph_idx_step = atoi(argv[4]);
    int min_mem_oc = atoi(argv[5]);
    int max_mem_oc = atoi(argv[6]);
    
    
    using namespace frequency_scaling;
    //init apis
    nvapiInit();
    nvmlInit_();

    //start power monitoring
    start_power_monitoring_script(device_id);

    //
    device_clock_info dci;
    dci.device_id_nvml = device_id;
    dci.device_id_nvapi = nvapiGetDeviceIndexByBusId(nvmlGetBusId(device_id));
    dci.nvml_mem_clocks = nvmlGetAvailableMemClocks(device_id);
    dci.nvml_graph_clocks = nvmlGetAvailableGraphClocks(device_id, dci.nvml_mem_clocks[1]);
    dci.nvapi_default_mem_clock = dci.nvml_mem_clocks[0];
    dci.nvapi_default_graph_clock = dci.nvml_graph_clocks[0];
    dci.min_graph_oc = 0;
    dci.min_mem_oc = min_mem_oc;
    dci.max_graph_oc = 0;
    dci.max_mem_oc = max_mem_oc;

    //
    const measurement &m = freq_hill_climbing(miner_script::EXCAVATOR, dci, max_iterations, mem_step, graph_idx_step);
    printf("Best energy-hash value: %f\n", m.energy_hash_);

    //stop power monitoring
    stop_power_monitoring_script(device_id);

    //unload apis
    nvapiUnload(1);
    nvmlShutdown_(true);
    return 0;
}
