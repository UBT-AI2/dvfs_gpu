//
// Created by alex on 28.05.18.
//
#include <random>
#include <chrono>
#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include "../common_header/fullexpr_accum.h"
#include "../common_header/exceptions.h"
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../freq_core/benchmark.h"
#include "../freq_core/process_management.h"
#include "../freq_core/cli_utils.h"
#include "../freq_core/optimization_config.h"
#include "../freq_core/log_utils.h"
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
                                       "--mem_step_pct=<float>\t\tDefault memory clock step size of optimization algo in percentage of value range.\n\t"
                                       "--graph_idx_step_pct=<float>\tDefault core clock index step size of optimization algo in percentage of value range.\n\t"
                                       "--start_mem_clock=<int>\t\tStart gpu memory clock of optimization algo. Not possible when minimum hashrate is used\n\t"
                                       "--start_graph_clock=<int>\t\tStart gpu core clock of optimization algo. Not possible when minimum hashrate is used\n\t"
                                       "--rand_start_clocks\t\tUse random start clocks for optimization algo. Overwritten by explicitly specified start clock. Not possible when minimum hashrate is used\n\t"
                                       "--min_mem_clock=<int>\t\tMinimum allowed gpu memory clock.\n\t"
                                       "--max_mem_clock=<int>\t\tMaximum allowed gpu memory clock.\n\t"
                                       "--min_graph_clock=<int>\t\tMinimum allowed gpu core clock.\n\t"
                                       "--max_graph_clock=<int>\t\tMaximum allowed gpu core clock.\n\t"
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

        FLAGS_v = 1;
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
        double mem_step_pct = (cmd_args.count("--mem_step_pct")) ? std::stod(cmd_args.at("--mem_step_pct")) : 0.15;
        double graph_idx_step_pct = (cmd_args.count("--graph_idx_step_pct")) ? std::stod(
                cmd_args.at("--graph_idx_step_pct"))
                                                                             : 0.15;
        int min_mem_clock = (cmd_args.count("--min_mem_clock")) ? std::stoi(cmd_args.at("--min_mem_clock")) : -1;
        int max_mem_clock = (cmd_args.count("--max_mem_clock")) ? std::stoi(cmd_args.at("--max_mem_clock")) : -1;
        int min_graph_clock = (cmd_args.count("--min_graph_clock")) ? std::stoi(cmd_args.at("--min_graph_clock")) : -1;
        int max_graph_clock = (cmd_args.count("--max_graph_clock")) ? std::stoi(cmd_args.at("--max_graph_clock")) : -1;
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
        const benchmark_func &bf = (online_bench) ? std::bind(&run_benchmark_mining_online_log,
                                                              std::cref(mui),
                                                              online_bench_duration_sec * 1000,
                                                              std::placeholders::_1,
                                                              std::placeholders::_2,
                                                              std::placeholders::_3,
                                                              std::placeholders::_4)
                                                  : benchmark_func(&run_benchmark_mining_offline);

        //
        const device_clock_info &dci = device_clock_info::create_dci(device_id, min_mem_clock, max_mem_clock,
                                                                     min_graph_clock, max_graph_clock);
        int start_mem_oc = (cmd_args.count("--start_mem_clock") && min_hashrate_pct < 0) ?
                           dci.get_mem_oc(std::stoi(cmd_args.at("--start_mem_clock"))) : dci.max_mem_oc_;
        int start_graph_idx = (cmd_args.count("--start_graph_clock") && min_hashrate_pct < 0) ?
                              dci.find_graph_idx(std::stoi(cmd_args.at("--start_graph_clock"))) : 0;
        if (cmd_args.count("--rand_start_clocks") && min_hashrate_pct < 0) {
            std::default_random_engine eng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            if (!cmd_args.count("--start_mem_clock"))
                start_mem_oc = std::uniform_int_distribution<int>(dci.min_mem_oc_, dci.max_mem_oc_)(eng);
            if (!cmd_args.count("--start_graph_clock"))
                start_graph_idx = std::uniform_int_distribution<int>(0, dci.nvml_graph_clocks_.size() - 1)(eng);
        }
        //check boundaries
        start_mem_oc = std::min(dci.max_mem_oc_, std::max(start_mem_oc, dci.min_mem_oc_));
        start_graph_idx = std::min((int) dci.nvml_graph_clocks_.size() - 1, std::max(start_graph_idx, 0));

        //start power monitoring and mining
        start_power_monitoring_script(device_id);
        if (online_bench)
            start_mining_script(ct, dci, mui);
        measurement m;
        const measurement &start_node = bf(ct, dci, start_mem_oc, start_graph_idx);
        double min_hashrate = -1.0;
        if (min_hashrate_pct > 0)
            min_hashrate = (start_node.nvml_graph_clock_idx_ != 0 || start_node.mem_oc_ != dci.max_mem_oc_) ?
                           min_hashrate_pct * bf(ct, dci, dci.max_mem_oc_, 0).hashrate_ :
                           min_hashrate_pct * start_node.hashrate_;
        switch (opt_method) {
            case optimization_method::HILL_CLIMBING:
                m = freq_hill_climbing(bf, ct, dci, start_node, max_iterations, mem_step_pct, graph_idx_step_pct,
                                       min_hashrate);
                break;
            case optimization_method::SIMULATED_ANNEALING:
                m = freq_simulated_annealing(bf, ct, dci, start_node, max_iterations, mem_step_pct, graph_idx_step_pct,
                                             min_hashrate);
                break;
            case optimization_method::NELDER_MEAD:
                m = freq_nelder_mead(bf, ct, dci, start_node, max_iterations, mem_step_pct, graph_idx_step_pct,
                                     min_hashrate);
                break;
            default:
                THROW_RUNTIME_ERROR("Invalid enum value");
        }
        VLOG(0) << log_utils::gpu_log_prefix(ct, dci.device_id_nvml_) << "Computed optimal energy-hash ratio: "
                << m.energy_hash_ << " (mem=" << m.mem_clock_ << ",graph=" << m.graph_clock_ << ")" << std::endl;
        //stop power monitoring and mining
        if (online_bench)
            stop_mining_script(dci.device_id_nvml_);
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

