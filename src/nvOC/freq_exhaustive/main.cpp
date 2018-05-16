#include <stdlib.h>
#include <stdio.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
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
    dci.min_graph_oc = min_graph_oc;
    dci.min_mem_oc = min_mem_oc;
    dci.max_graph_oc = max_graph_oc;
    dci.max_mem_oc = max_mem_oc;

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
