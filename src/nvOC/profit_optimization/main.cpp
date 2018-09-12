#include <glog/logging.h>
#include "../common_header/fullexpr_accum.h"
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/process_management.h"
#include "../script_running/log_utils.h"
#include "../script_running/optimization_config.h"
#include "../script_running/cli_utils.h"
#include "profit_optimization.h"

using namespace frequency_scaling;

int main(int argc, char **argv) {
    try {
        //parse cmd options
        const std::map<std::string, std::string> &cmd_args = parse_cmd_options(argc, argv);
        if (cmd_args.count("--help")) {
            fullexpr_accum<>(std::cout) << "Options:\n\t"
                                           "--currency_config=<filename>\tCurrency configuration to use.\n\t"
                                           "--user_config=<filename>\tUser configuration to use.\n\t"
                                           "--opt_result=<filename>\t\tOptimization result to use.\n\t"
                                           "--help\t\t\t\tShows this message.\n\t" << std::endl;
            return 1;
        }
        //init logging stuff
        log_utils::init_logging("profit-optimization-logs", "glog-profit-optimization-", 1, argv[0]);
        //read currency config if given, otherwise create default one
        std::map<std::string, currency_type> available_currencies;
        if (cmd_args.count("--currency_config")) {
            available_currencies = read_currency_config(cmd_args.at("--currency_config"));
        } else {
            write_currency_config("currency_config_default.json", create_default_currency_config());
            available_currencies = read_currency_config("currency_config_default.json");
        }
        //init apis
        nvapiInit();
        nvmlInit_();
        process_management::register_process_cleanup_sighandler();

        //start mining and monitoring best currency;
        const optimization_config &opt_config = (cmd_args.count("--user_config")) ? parse_config_json(
                cmd_args.at("--user_config"), available_currencies) : get_config_user_dialog(available_currencies);
        if (cmd_args.count("--opt_result")) {
            const std::map<int, device_opt_result> &opt_result =
                    load_optimization_result(cmd_args.at("--opt_result"), available_currencies);
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