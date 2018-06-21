//
// Created by alex on 16.05.18.
//

#include "mining.h"
#include <stdexcept>
#include <regex>
#include "process_management.h"

namespace frequency_scaling {

    static const int BUFFER_SIZE = 1024;


    void start_mining_script(currency_type ct, const device_clock_info &dci, const miner_user_info &user_info) {
        const std::string &wallet_addr = user_info.wallet_addresses_.at(ct);
        const std::string &worker_name = user_info.worker_names_.at(dci.device_id_nvml);
        //start mining in background process
        char cmd1[BUFFER_SIZE];
        switch (ct) {
            case currency_type::ETH:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_eth.sh %i %i %s %s %s",
                         dci.device_id_nvml, dci.device_id_cuda, wallet_addr.c_str(),
                         worker_name.c_str(), user_info.email_adress_.c_str());
                break;
            case currency_type::ZEC:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_zec.sh %i %i %s %s %s",
                         dci.device_id_nvml, dci.device_id_cuda, wallet_addr.c_str(),
                         worker_name.c_str(), user_info.email_adress_.c_str());
                break;
            case currency_type::XMR:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_xmr.sh %i %i %s %s %s",
                         dci.device_id_nvml, dci.device_id_cuda, wallet_addr.c_str(),
                         worker_name.c_str(), user_info.email_adress_.c_str());
                break;
            default:
                throw std::runtime_error("Invalid enum value");
        }
        //
        process_management::gpu_start_process(cmd1, dci.device_id_nvml, process_type::MINER, true);
    }


    void stop_mining_script(int device_id) {
        process_management::gpu_kill_background_process(device_id, process_type::MINER);
    }

}
