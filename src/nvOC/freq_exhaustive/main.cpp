#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include "../common_header/fullexpr_accum.h"
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "../script_running/process_management.h"
#include "../script_running/optimization_config.h"
#include "../script_running/cli_utils.h"
#include "../script_running/log_utils.h"
#include "freq_exhaustive.h"

using namespace frequency_scaling;


static bool cmd_arg_check(int argc, char **argv, std::map<std::string, std::string> &cmd_args) {
    //parse cmd options
    cmd_args = parse_cmd_options(argc, argv);
    if (cmd_args.count("--help")) {
        fullexpr_accum<>(std::cout) << "Options:\n\t"
                                       "-d <int>\t\t\tGPU to use (required).\n\t"
                                       "-c <string>\t\t\tCurrency to use (required).\n\t"
                                       "--currency_config=<filename>\tCurrency configuration to use.\n\t"
                                       "--use_online_bench=<filename>\tUsage of online benchmark with specified user configuration.\n\t"
                                       "--mem_step=<int>\t\tDefault memory clock step size.\n\t"
                                       "--graph_idx_step=<int>\tDefault core clock index step size.\n\t"
                                       "--min_mem_oc=<int>\t\tMinimum allowed gpu memory overclock.\n\t"
                                       "--max_mem_oc=<int>\t\tMaximum allowed gpu memory overclock.\n\t"
                                       "--min_graph_oc=<int>\t\tMinimum allowed gpu core overclock.\n\t"
                                       "--max_graph_oc=<int>\t\tMaximum allowed gpu core overclock.\n\t"
                                       "--help\t\t\t\tShows this message.\n\t" << std::endl;
        return false;
    }
    //
    std::string missing_args;
    for (const std::string &req_opt : {"-c", "-d"}) {
        if (!cmd_args.count(req_opt) || cmd_args.at(req_opt).empty())
            missing_args += ((missing_args.empty()) ? "" : ", ") + req_opt;
    }
    if (!missing_args.empty()) {
        fullexpr_accum<>(std::cout) << "Specify missing arguments: " << missing_args << std::endl;
        fullexpr_accum<>(std::cout) << "See --help for all options" << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char **argv) {
    //get cmd options
    std::map<std::string, std::string> cmd_args;
    if (!cmd_arg_check(argc, argv, cmd_args))
        return 1;
    //
    bool online_bench = cmd_args.count("--use_online_bench") != 0;
    const std::map<std::string, currency_type> &available_currencies =
            (cmd_args.count("--currency_config")) ? read_currency_config(cmd_args.at("--currency_config"))
                                                  : create_default_currency_config();
    const optimization_config &opt_config =
            (online_bench) ? parse_config_json(cmd_args.at("--use_online_bench"), available_currencies)
                           : optimization_config();
    int device_id = std::stoi(cmd_args.at("-d"));
    const currency_type &ct = available_currencies.at(boost::algorithm::to_upper_copy(cmd_args.at("-c")));
    int mem_step = (cmd_args.count("--mem_step")) ? std::stoi(cmd_args.at("--mem_step")) : 50;
    int graph_idx_step = (cmd_args.count("--graph_idx_step")) ? std::stoi(cmd_args.at("--graph_idx_step")) : 2;
    int min_mem_oc = (cmd_args.count("--min_mem_oc")) ? std::stoi(cmd_args.at("--min_mem_oc")) : 1;
    int max_mem_oc = (cmd_args.count("--max_mem_oc")) ? std::stoi(cmd_args.at("--max_mem_oc")) : -1;
    int min_graph_oc = (cmd_args.count("--min_graph_oc")) ? std::stoi(cmd_args.at("--min_graph_oc")) : 1;
    int max_graph_oc = (cmd_args.count("--max_graph_oc")) ? std::stoi(cmd_args.at("--max_graph_oc")) : -1;
    //
    try {
        //init apis
        nvapiInit();
        nvmlInit_();
        process_management::register_process_cleanup_sighandler();

        //start power monitoring
        start_power_monitoring_script(device_id);
        //
        device_clock_info dci(device_id, min_mem_oc, min_graph_oc, max_mem_oc, max_graph_oc);
        const measurement &m = (online_bench) ? freq_exhaustive(std::bind(&run_benchmark_mining_online_log,
                                                                          std::cref(opt_config.miner_user_infos_),
                                                                          opt_config.online_bench_duration_sec_ * 1000,
                                                                          std::placeholders::_1,
                                                                          std::placeholders::_2,
                                                                          std::placeholders::_3,
                                                                          std::placeholders::_4), online_bench, ct, dci,
                                                                mem_step, graph_idx_step)
                                              : freq_exhaustive(&run_benchmark_mining_offline, online_bench, ct, dci,
                                                                mem_step, graph_idx_step);
        VLOG(0) << log_utils::gpu_log_prefix(ct, dci.device_id_nvml_) << "Computed optimal energy-hash ratio: "
                << m.energy_hash_ << " (mem=" << m.mem_clock_ << ",graph=" << m.graph_clock_ << ")" << std::endl;

        //stop power monitoring
        stop_power_monitoring_script(device_id);

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

    return 0;
}
