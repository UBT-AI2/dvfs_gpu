//
// Created by alex on 28.05.18.
//
#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include "../common_header/fullexpr_accum.h"
#include "../common_header/exceptions.h"
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "../script_running/process_management.h"
#include "../script_running/cli_utils.h"
#include "../script_running/optimization_config.h"
#include "../script_running/log_utils.h"
#include "freq_simulated_annealing.h"
#include "freq_hill_climbing.h"
#include "freq_nelder_mead.h"

using namespace frequency_scaling;

static bool cmd_arg_check(int argc, char **argv, std::map<std::string, std::string> &cmd_args) {
    //parse cmd options
    cmd_args = parse_cmd_options(argc, argv);
    if (cmd_args.count("--help")) {
        fullexpr_accum<>(std::cout) << "Options:\n\t"
                                       "-d <int>\t\t\tGPU to use (required).\n\t"
                                       "-c <string>\t\t\tCurrency to use (required).\n\t"
                                       "-a <string>\t\t\tOptimization algo to use. HC=hill climbing, SA=simulated annealing, NM=nelder mead (required).\n\t"
                                       "--use_online_bench=<wallet>\tUsage of online benchmarks with specified wallet address.\n\t"
                                       "--online_bench_duration=<int>\tDuration of an online benchmark in seconds.\n\t"
                                       "--currency_config=<filename>\tCurrency configuration to use.\n\t"
                                       "--min_hashrate=<float>\t\tMinimum hashrate to adhere in percentage of maximum hashrate.\n\t"
                                       "--max_iterations=<int>\t\tMaximum iterations of optimization algo.\n\t"
                                       "--mem_step=<float>\t\tDefault memory clock step size of optimization algo in percentage of value range.\n\t"
                                       "--graph_idx_step=<float>\tDefault core clock index step size of optimization algo in percentage of value range.\n\t"
                                       "--min_mem_oc=<int>\t\tMinimum allowed gpu memory overclock.\n\t"
                                       "--max_mem_oc=<int>\t\tMaximum allowed gpu memory overclock.\n\t"
                                       "--min_graph_oc=<int>\t\tMinimum allowed gpu core overclock.\n\t"
                                       "--max_graph_oc=<int>\t\tMaximum allowed gpu core overclock.\n\t"
                                       "--help\t\t\t\tShows this message.\n\t" << std::endl;
        return false;
    }
    //
    std::string missing_args;
    for (const std::string &req_opt : {"-c", "-d", "-a"}) {
        if (!cmd_args.count(req_opt) || cmd_args.at(req_opt).empty())
            missing_args += ((missing_args.empty()) ? "" : ", ") + req_opt;
    }
    if (!missing_args.empty()) {
        fullexpr_accum<>(std::cout) << "Specify missing required arguments: " << missing_args << std::endl;
        fullexpr_accum<>(std::cout) << "See --help for all options" << std::endl;
        return false;
    }
    return true;
}

int main(int argc, char **argv) {
    try {
        //get cmd options
        std::map<std::string, std::string> cmd_args;
        if (!cmd_arg_check(argc, argv, cmd_args))
            return 1;

        //init apis
        nvapiInit();
        nvmlInit_();
        process_management::register_process_cleanup_sighandler();

        //
        const std::map<std::string, currency_type> &available_currencies =
                (cmd_args.count("--currency_config")) ? read_currency_config(cmd_args.at("--currency_config"))
                                                      : create_default_currency_config();
        int device_id = std::stoi(cmd_args.at("-d"));
        const currency_type &ct = available_currencies.at(boost::algorithm::to_upper_copy(cmd_args.at("-c")));
        optimization_method opt_method = string_to_opt_method(cmd_args.at("-a"));
        double min_hashrate_pct = (cmd_args.count("--min_hashrate")) ? std::stod(cmd_args.at("--min_hashrate")) : -1.0;
        int max_iterations = (cmd_args.count("--max_iterations")) ? std::stoi(cmd_args.at("--max_iterations"))
                                                                  : (opt_method == optimization_method::NELDER_MEAD) ? 8
                                                                                                                     : 5;
        double mem_step_pct = (cmd_args.count("--mem_step")) ? std::stod(cmd_args.at("--mem_step")) : 0.15;
        double graph_idx_step_pct = (cmd_args.count("--graph_idx_step")) ? std::stod(cmd_args.at("--graph_idx_step"))
                                                                         : 0.15;
        int min_mem_oc = (cmd_args.count("--min_mem_oc")) ? std::stoi(cmd_args.at("--min_mem_oc")) : 1;
        int max_mem_oc = (cmd_args.count("--max_mem_oc")) ? std::stoi(cmd_args.at("--max_mem_oc")) : -1;
        int min_graph_oc = (cmd_args.count("--min_graph_oc")) ? std::stoi(cmd_args.at("--min_graph_oc")) : 1;
        int max_graph_oc = (cmd_args.count("--max_graph_oc")) ? std::stoi(cmd_args.at("--max_graph_oc")) : -1;
        //online bench stuff
        bool online_bench = cmd_args.count("--use_online_bench") != 0;
        miner_user_info mui;
        int online_bench_duration_sec;
        if (online_bench) {
            mui.wallet_addresses_.emplace(ct, cmd_args.at("--use_online_bench"));
            mui.worker_names_.emplace(device_id, getworker_name(device_id));
            online_bench_duration_sec = (cmd_args.count("--online_bench_duration")) ? std::stoi(
                    cmd_args.at("--online_bench_duration")) : 120;
        }

        //start power monitoring
        start_power_monitoring_script(device_id);

        //
        device_clock_info dci(device_id, min_mem_oc, min_graph_oc, max_mem_oc, max_graph_oc);
        const benchmark_func &bf = (online_bench) ? std::bind(&run_benchmark_mining_online_log,
                                                              std::cref(mui),
                                                              online_bench_duration_sec * 1000,
                                                              std::placeholders::_1,
                                                              std::placeholders::_2,
                                                              std::placeholders::_3,
                                                              std::placeholders::_4)
                                                  : benchmark_func(&run_benchmark_mining_offline);
        measurement m;
        switch (opt_method) {
            case optimization_method::HILL_CLIMBING:
                m = freq_hill_climbing(bf, ct, dci, max_iterations, mem_step_pct, graph_idx_step_pct, min_hashrate_pct);
                break;
            case optimization_method::SIMULATED_ANNEALING:
                m = freq_simulated_annealing(bf, ct, dci, max_iterations, mem_step_pct, graph_idx_step_pct,
                                             min_hashrate_pct);
                break;
            case optimization_method::NELDER_MEAD:
                m = freq_nelder_mead(bf, ct, dci, max_iterations, mem_step_pct, graph_idx_step_pct, min_hashrate_pct);
                break;
            default:
                THROW_RUNTIME_ERROR("Invalid enum value");
        }
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

