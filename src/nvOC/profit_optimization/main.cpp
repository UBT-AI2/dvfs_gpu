#include <stdlib.h>
#include <stdio.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "profit_optimization.h"

int main(int argc, char **argv) {
    if (argc < 8) {
        printf("Usage: %s mining_user_info device_id max_iterations mem_step graph_idx_step min_mem_oc max_mem_oc",
               argv[0]);
        return 1;
    }
    std::string mining_user_info(argv[1]);
    unsigned int device_id = atoi(argv[2]);
    int max_iterations = atoi(argv[3]);
    int mem_step = atoi(argv[4]);
    int graph_idx_step = atoi(argv[5]);
    int min_mem_oc = atoi(argv[6]);
    int max_mem_oc = atoi(argv[7]);


    using namespace frequency_scaling;
    //init apis
    nvapiInit();
    nvmlInit_();

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

    mine_most_profitable_currency(mining_user_info, dci, max_iterations, mem_step, graph_idx_step);

    //unload apis
    nvapiUnload(0);
    nvmlShutdown_(false);

    return 0;
}