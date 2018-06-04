#include <stdlib.h>
#include <stdio.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"
#include "profit_optimization.h"

int main(int argc, char **argv) {
    if (argc < 5) {
        printf("Usage: %s mining_user_info device_id min_mem_oc max_mem_oc",
               argv[0]);
        return 1;
    }
    std::string mui_str(argv[1]);
    unsigned int device_id = atoi(argv[2]);
    int min_mem_oc = atoi(argv[3]);
    int max_mem_oc = atoi(argv[4]);


    using namespace frequency_scaling;
    //init apis
    nvapiInit();
    nvmlInit_();

    //TODO: create dci and user_info for several gpus (user dialog)
    std::vector<device_clock_info> dcis;
    dcis.push_back(device_clock_info(device_id, min_mem_oc, 0, max_mem_oc, 0));
    std::map<currency_type, miner_user_info> user_infos;
    user_infos.emplace(currency_type::ETH, miner_user_info(mui_str));

    //start mining and monitoring best currency;
    mine_most_profitable_currency(user_infos, dcis, optimization_info(optimization_method::NELDER_MEAD, -1), 300);

    //unload apis
    nvapiUnload(0);
    nvmlShutdown_(false);

    return 0;
}