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

    bool start_mining_script(const currency_type &ct, const device_clock_info &dci,
                             const miner_user_info &user_info);

    bool stop_mining_script(int device_id);

    double
    get_avg_hashrate_online_log(const currency_type &ct, int device_id, long long int system_timestamp_start_ms,
                                long long int system_timestamp_end_ms);

    measurement run_benchmark_mining_online_pool(const miner_user_info &user_info, int period_ms,
                                                 const currency_type &ct, const device_clock_info &dci,
                                                 int mem_oc,
                                                 int nvml_graph_clock_idx);

    measurement run_benchmark_mining_online_log(const miner_user_info &user_info, int period_ms,
                                                const currency_type &ct, const device_clock_info &dci,
                                                int mem_oc,
                                                int nvml_graph_clock_idx);

}