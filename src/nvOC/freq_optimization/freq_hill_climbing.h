#pragma once

#include "../script_running/benchmark.h"

namespace frequency_scaling {

    enum class exploration_type {
        NEIGHBORHOOD_4_DIAGONAL, NEIGHBORHOOD_4_STRAIGHT, NEIGHBORHOOD_4_ALTERNATING, NEIGHBORHOOD_8, count
    };

    measurement
    freq_hill_climbing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                       int max_iterations, double mem_step_pct, double graph_idx_step_pct,
                       double min_hashrate_pct = -1.0,
                       exploration_type expl_type = exploration_type::NEIGHBORHOOD_4_ALTERNATING);

    measurement
    freq_hill_climbing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                       const measurement &start_node,
                       int max_iterations, double mem_step_pct, double graph_idx_step_pct, double min_hashrate = -1.0,
                       exploration_type expl_type = exploration_type::NEIGHBORHOOD_4_ALTERNATING);

    measurement
    __freq_hill_climbing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                         const measurement &start_node, bool allow_start_node_result,
                         int max_iterations, int mem_step, int graph_idx_step, double min_hashrate,
                         exploration_type expl_type);

}