#pragma once

#include <string>
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"
#include "profit_calculation.h"

namespace frequency_scaling {

    enum class optimization_method {
        NELDER_MEAD, HILL_CLIMBING, SIMULATED_ANNEALING, count
    };

    struct optimization_info {
        optimization_info(optimization_method method, int min_hashrate);

        optimization_info(optimization_method method, int max_iterations,
                          int mem_step, int graph_idx_step, int min_hashrate);

        optimization_method method_;
        int max_iterations_, mem_step_, graph_idx_step_, min_hashrate_;
    };

    void mine_most_profitable_currency(const std::map<currency_type, miner_user_info> &user_infos,
                                       const std::vector<device_clock_info> &dcis,
                                       const optimization_info &opt_info,
                                       int monitoring_interval_sec);

}