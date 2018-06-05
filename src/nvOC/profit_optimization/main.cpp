
#include <iostream>
#include <limits>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"
#include "../script_running/process_management.h"
#include "profit_optimization.h"

using namespace frequency_scaling;


static void user_dialog(std::vector<device_clock_info> &dcis,
                        std::map<currency_type, miner_user_info> &miner_user_infos) {
    //select GPUs to mine on
    int device_count = nvmlGetNumDevices();
    for (int device_id = 0; device_id < device_count; device_id++) {
        std::cout << "Mine on GPU " << device_id << " (" <<
                  nvmlGetDeviceName(device_id) << ") ? [y/n]" << std::endl;
        char c;
        std::cin >> c;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (c != 'y')
            continue;
        int min_mem_oc, max_mem_oc;
        std::cout << "Enter min_mem_oc:" << std::endl;
        std::cin >> min_mem_oc;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Enter max_mem_oc:" << std::endl;
        std::cin >> max_mem_oc;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        dcis.emplace_back(device_id, min_mem_oc, 0, max_mem_oc, 0);
    }
    //select currencies to mine
    for (int i = 0; i < static_cast<int>(currency_type::count); i++) {
        currency_type ct = static_cast<currency_type>(i);
        std::cout << "Include currency " << enum_to_string(ct) << "? [y/n]" << std::endl;
        char c;
        std::cin >> c;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (c != 'y')
            continue;
        std::cout << "Enter mining user info in the format wallet_address/worker_name/e-mail:" << std::endl;
        std::string mui_str;
        std::cin >> mui_str;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        miner_user_infos.emplace(ct, miner_user_info(mui_str));
    }
}

int main(int argc, char **argv) {
    //init apis
    nvapiInit();
    nvmlInit_();
    process_management::register_process_cleanup_sighandler();

    std::vector<device_clock_info> dcis;
    std::map<currency_type, miner_user_info> miner_user_infos;
    user_dialog(dcis, miner_user_infos);

    //start mining and monitoring best currency;
    mine_most_profitable_currency(miner_user_infos, dcis, optimization_info(optimization_method::NELDER_MEAD, -1), 300);

    //unload apis
    nvapiUnload(0);
    nvmlShutdown_(false);

    return 0;
}