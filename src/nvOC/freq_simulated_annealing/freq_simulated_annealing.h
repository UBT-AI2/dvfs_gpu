//
// Created by alex on 28.05.18.
//
#pragma once

#include "../script_running/benchmark.h"

namespace frequency_scaling {

    measurement
    freq_simulated_annealing(const benchmark_func &benchmarkFunc, currency_type ct, const device_clock_info &dci,
                             int max_iterations, int mem_step, int graph_idx_step, double min_hashrate = -1.0);

    measurement
    freq_simulated_annealing(const benchmark_func &benchmarkFunc, currency_type ct, const device_clock_info &dci,
                             const measurement &start_node, int max_iterations, int mem_step, int graph_idx_step,
                             double min_hashrate = -1.0);

}
