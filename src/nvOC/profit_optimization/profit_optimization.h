#pragma once

#include <string>
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"
#include "profit_calculation.h"

namespace frequency_scaling {

    void mine_most_profitable_currency(const std::map<currency_type, miner_user_info> &user_infos,
                                       const std::vector<device_clock_info> &dcis, int monitoring_interval_sec,
                                       int max_iterations = 8, int mem_step = 300, int graph_idx_step = 10);

}