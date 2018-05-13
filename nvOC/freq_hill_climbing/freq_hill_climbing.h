#pragma once

#include "../benchmark.h"

namespace frequency_scaling {

    measurement freq_hill_climbing(const device_clock_info &dci, int max_iterations,
                                   int mem_step, int graph_idx_step);

}