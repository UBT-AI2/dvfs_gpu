#pragma once

#include <string>
#include "currency_type.h"
#include "benchmark.h"

namespace frequency_scaling {

    struct miner_user_info {
        miner_user_info(const std::string &combined_user_info);

        miner_user_info(const std::string &wallet_adress_, const std::string &worker_name_,
                        const std::string &email_adress_);

        std::string wallet_address_, worker_name_, email_adress_;
    };

    void start_mining_script(currency_type ct, const device_clock_info &dci, const miner_user_info &user_info);

    void stop_mining_script(int device_id);


}