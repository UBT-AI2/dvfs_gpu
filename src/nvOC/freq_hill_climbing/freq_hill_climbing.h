#pragma once

#include "../script_running/benchmark.h"

namespace frequency_scaling {

    measurement
    freq_hill_climbing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                       int max_iterations, int mem_step, int graph_idx_step, double min_hashrate = -1.0);

    measurement
    freq_hill_climbing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                       const measurement &start_node, bool allow_start_node_result,
                       int max_iterations, int mem_step, int graph_idx_step, double min_hashrate = -1.0);

}