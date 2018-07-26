//
// Created by Alex on 10.06.2018.
//
#pragma once

#include <map>
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"

namespace frequency_scaling {

    enum class optimization_method {
        NELDER_MEAD, HILL_CLIMBING, SIMULATED_ANNEALING, count
    };


    struct optimization_method_params {
        optimization_method_params(optimization_method method, int min_hashrate);

        optimization_method_params(optimization_method method, int max_iterations,
                                   int mem_step, int graph_idx_step, int min_hashrate);

        optimization_method method_;
        int max_iterations_, mem_step_, graph_idx_step_, min_hashrate_;
    };


    struct optimization_config {
        std::vector<device_clock_info> dcis_;
        miner_user_info miner_user_infos_;
        std::map<currency_type, optimization_method_params> opt_method_params_;
        double energy_cost_kwh_;
        int monitoring_interval_sec_;
        int online_bench_duration_sec_ = 120;
    };


    optimization_config get_config_user_dialog();

    void write_config_json(const std::string &filename, const optimization_config &opt_config);

    optimization_config parse_config_json(const std::string &filename);

    std::string enum_to_string(optimization_method opt_method);

    optimization_method string_to_opt_method(const std::string &str);

}
