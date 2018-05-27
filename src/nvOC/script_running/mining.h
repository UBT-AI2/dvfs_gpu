#pragma once

#include <string>
#include "miner_script.h"
#include "benchmark.h"

namespace frequency_scaling {


    void start_mining_script(miner_script ms, const device_clock_info &dci, const std::string &user_info);

    void stop_mining_script(miner_script ms, const std::string &user_info, int device_id);


}