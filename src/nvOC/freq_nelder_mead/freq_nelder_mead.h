#pragma once

#include "../benchmark/benchmark.h"

namespace frequency_scaling {

    measurement freq_nelder_mead(miner_script ms, const device_clock_info &dci, int min_iterations, int max_iterations,
                                 int mem_step, int graph_idx_step,
                                 double param_tolerance = 1e-3, double func_tolerance = 1e-3);
}