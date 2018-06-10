//
// Created by Alex on 10.06.2018.
//
#pragma once

#include <map>
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"

namespace frequency_scaling{
    
    void user_dialog(std::vector<device_clock_info> &dcis,
                     std::map<currency_type, miner_user_info> &miner_user_infos);

}
