#pragma once

#include "../script_running/benchmark.h"

namespace frequency_scaling {

    measurement freq_nelder_mead(miner_script ms, const device_clock_info &dci, int min_iterations, int max_iterations,
                                 int mem_step, int graph_idx_step, double min_hashrate = -1.0,
                                 double mem_scale = 1.5, double graph_scale = 1.5);

    measurement freq_nelder_mead(miner_script ms, const device_clock_info &dci, const measurement &start_node,
                                 int min_iterations, int max_iterations,
                                 int mem_step, int graph_idx_step, double min_hashrate = -1.0,
                                 double mem_scale = 1.5, double graph_scale = 1.5);
}