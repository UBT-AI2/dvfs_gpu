//
// Created by alex on 16.05.18.
//

#include "mining.h"
#include "benchmark.h"
#include <stdexcept>

namespace frequency_scaling {

    static const int BUFFER_SIZE = 1024;

    void start_mining_script(miner_script ms, const device_clock_info &dci, const std::string &user_info) {
        //start power monitoring in background process
        char cmd1[BUFFER_SIZE];
        switch (ms) {
            case miner_script::EXCAVATOR:
                snprintf(cmd1, BUFFER_SIZE, "sh ../scripts/start_mining_excavator.sh %s %i",
                         user_info.c_str(), dci.device_id_cuda);
                break;
            case miner_script::ETHMINER:
                snprintf(cmd1, BUFFER_SIZE, "sh ../scripts/start_mining_ethminer.sh %s %i",
                         user_info.c_str(), dci.device_id_cuda);
                break;
            case miner_script::XMRSTAK:
                snprintf(cmd1, BUFFER_SIZE, "sh ../scripts/start_mining_xmrstak.sh %s %i",
                         user_info.c_str(), dci.device_id_cuda);
                break;
            default:
                throw std::runtime_error("Invalid enum value");
        }
        system(cmd1);

    }

    void stop_mining_script(miner_script ms) {
        //stop power monitoring
        char cmd3[BUFFER_SIZE];
        switch (ms) {
            case miner_script::EXCAVATOR:
                snprintf(cmd3, BUFFER_SIZE, "sh ../scripts/kill_process.sh %s", "excavator");
                break;
            case miner_script::ETHMINER:
                snprintf(cmd3, BUFFER_SIZE, "sh ../scripts/kill_process.sh %s", "ethminer");
                break;
            case miner_script::XMRSTAK:
                snprintf(cmd3, BUFFER_SIZE, "sh ../scripts/kill_process.sh %s", "xmr-stak");
                break;
            default:
                throw std::runtime_error("Invalid enum value");
        }
        system(cmd3);
    }

}
