#pragma once

#include <string>
#include <map>
#include "currency_type.h"
#include "benchmark.h"

namespace frequency_scaling {

    struct miner_user_info {
        std::string email_adress_;
        std::map<currency_type, std::string> wallet_addresses_;
        std::map<int, std::string> worker_names_;
    };

    void start_mining_script(currency_type ct, const device_clock_info &dci, const miner_user_info &user_info);

    void stop_mining_script(int device_id);

    measurement run_benchmark_mining_online(const miner_user_info &user_info, int period_hours,
                                            currency_type ct, const device_clock_info &dci, int mem_oc,
                                            int nvml_graph_clock_idx);

}