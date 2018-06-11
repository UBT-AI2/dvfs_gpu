
#include <iostream>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/process_management.h"
#include "profit_optimization.h"
#include "optimization_config.h"

using namespace frequency_scaling;

int main(int argc, char **argv) {
    try {
        //init apis
        nvapiInit();
        nvmlInit_();
        process_management::register_process_cleanup_sighandler();

        const optimization_config& opt_config = get_config_user_dialog();

        //start mining and monitoring best currency;
        mine_most_profitable_currency(opt_config);

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