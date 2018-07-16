#include <glog/logging.h>
#include "../common_header/fullexpr_accum.h"
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/process_management.h"
#include "profit_optimization.h"
#include "optimization_config.h"
#include "cli_utils.h"

using namespace frequency_scaling;

int main(int argc, char **argv) {
    try {
		//init logging stuff
		process_management::init_logging("profit-optimization-logs", "glog-profit-optimization-", 0, argv[0]);
        //parse cmd options
        const std::map<std::string, std::string> &cmd_args = parse_cmd_options(argc, argv);
        if (cmd_args.count("--help")) {
            full_expression_accumulator(std::cout) << "Options:\n\t"
                                                      "--help\t\t\t\tshows this message\n\t"
                                                      "--user_config=<filename>\tuser configuration json file\n\t"
                                                      "--opt_result=<filename>\t\toptimization result json file"
                                                   << std::endl;
            return 1;
        }

        //init apis
        nvapiInit();
        nvmlInit_();
        process_management::register_process_cleanup_sighandler();

        //start mining and monitoring best currency;
        const optimization_config &opt_config = (cmd_args.count("--user_config")) ? parse_config_json(
                cmd_args.at("--user_config")) : get_config_user_dialog();
        if (cmd_args.count("--opt_result")) {
            const std::map<int, std::map<currency_type, energy_hash_info>> &opt_result =
                    load_optimization_result(cmd_args.at("--opt_result"));
            mine_most_profitable_currency(opt_config, opt_result);
        } else {
            mine_most_profitable_currency(opt_config);
        }

        //unload apis
        nvapiUnload(1);
        nvmlShutdown_(true);
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Main caught exception: " << ex.what() << std::endl;
        LOG(ERROR) << "Perform cleanup and exit..." << std::endl;
        process_management::kill_all_processes(false);
        nvapiUnload(1);
        nvmlShutdown_(true);
        return 1;
    }
    catch (...) {
        LOG(ERROR) << "Main caught unknown exception" << std::endl;
        process_management::kill_all_processes(false);
        nvapiUnload(1);
        nvmlShutdown_(true);
        return 1;
    }

    return 0;
}