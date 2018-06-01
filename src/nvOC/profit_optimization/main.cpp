#include <stdlib.h>
#include <stdio.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"
#include "profit_optimization.h"

int main(int argc, char **argv) {
    if (argc < 8) {
        printf("Usage: %s mining_user_info device_id max_iterations mem_step graph_idx_step min_mem_oc max_mem_oc",
               argv[0]);
        return 1;
    }
    std::string mui_str(argv[1]);
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

    //TODO: start a thread for each GPU
    //start mining and monitoring best currency
    device_clock_info dci(device_id, min_mem_oc, 0, max_mem_oc, 0);
    miner_user_info mui(mui_str, miner_script::EXCAVATOR);
    mine_most_profitable_currency(mui, dci, max_iterations, mem_step, graph_idx_step);

    //unload apis
    nvapiUnload(0);
    nvmlShutdown_(false);

    return 0;
}