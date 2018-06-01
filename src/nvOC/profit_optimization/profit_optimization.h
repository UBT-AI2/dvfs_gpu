#pragma once

#include <string>
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"

namespace frequency_scaling {

    void mine_most_profitable_currency(const miner_user_info &user_info, const device_clock_info &dci,
                                       int max_iterations, int mem_step, int graph_idx_step);

}