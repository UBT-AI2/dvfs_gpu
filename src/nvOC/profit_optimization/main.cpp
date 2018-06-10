
#include <iostream>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/process_management.h"
#include "profit_optimization.h"
#include "start_config.h"

using namespace frequency_scaling;

int main(int argc, char **argv) {
    try {
        //init apis
        nvapiInit();
        nvmlInit_();
        process_management::register_process_cleanup_sighandler();

        std::vector<device_clock_info> dcis;
        std::map<currency_type, miner_user_info> miner_user_infos;
        user_dialog(dcis, miner_user_infos);

        //start mining and monitoring best currency;
        mine_most_profitable_currency(miner_user_infos, dcis, optimization_info(optimization_method::NELDER_MEAD, -1),
                                      300);

        //unload apis
        nvapiUnload(1);
        nvmlShutdown_(true);
    } catch (const std::exception &ex) {
        std::cerr << "Main caught exception: " << ex.what() << std::endl;
        std::cerr << "Perform cleanup and exit..." << std::endl;
        process_management::kill_all_processes(false);
        nvapiUnload(1);
        nvmlShutdown_(true);
        return 1;
    }

    return 0;
}