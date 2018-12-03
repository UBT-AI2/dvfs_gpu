//
// Created by alex on 28.05.18.
//
#pragma once

#include "../freq_core/benchmark.h"

namespace frequency_scaling {

    measurement
    freq_simulated_annealing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                             int max_iterations, double mem_step_pct, double graph_idx_step_pct,
                             double min_hashrate_pct = -1.0);

    measurement
    freq_simulated_annealing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                             const measurement &start_node, int max_iterations, double mem_step_pct,
                             double graph_idx_step_pct,
                             double min_hashrate = -1.0);

}
