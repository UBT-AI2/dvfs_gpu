#pragma once

#include "../script_running/benchmark.h"

namespace frequency_scaling {

    measurement freq_nelder_mead(const benchmark_func &benchmarkFunc, currency_type ct, const device_clock_info &dci,
                                 int max_iterations, int mem_step, int graph_idx_step, double min_hashrate = -1.0);

    measurement freq_nelder_mead(const benchmark_func &benchmarkFunc, currency_type ct, const device_clock_info &dci,
                                 const measurement &start_node,
                                 int max_iterations, int mem_step, int graph_idx_step, double min_hashrate = -1.0);
}