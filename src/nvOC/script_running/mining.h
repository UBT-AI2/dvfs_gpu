#pragma once

#include <string>
#include "miner_script.h"
#include "benchmark.h"

namespace frequency_scaling {

    class miner_user_info {
    public:
        miner_user_info(const std::string &combined_user_info, miner_script ms);

        miner_user_info(const std::string &wallet_adress_, const std::string &worker_name_,
                        const std::string &email_adress_);

        std::string get_combined_user_info(miner_script ms) const;

    private:
        std::string wallet_adress_, worker_name_, email_adress_;
    };

    void start_mining_script(miner_script ms, const device_clock_info &dci, const miner_user_info &user_info);

    void stop_mining_script(miner_script ms);


}