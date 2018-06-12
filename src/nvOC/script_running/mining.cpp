//
// Created by alex on 16.05.18.
//

#include "mining.h"
#include <stdexcept>
#include <regex>
#include "process_management.h"

namespace frequency_scaling {

    static const int BUFFER_SIZE = 1024;


    miner_user_info::miner_user_info(const std::string &combined_user_info) {
        std::string regex = "/";
        //regex = "\\.|/";
        std::regex re(regex);
        std::sregex_token_iterator first{combined_user_info.begin(), combined_user_info.end(), re, -1}, last;
        std::vector<std::string> vec(first, last);
        if (vec.size() != 2 && vec.size() != 3)
            throw std::runtime_error("Invalid miner user info");
        wallet_address_ = vec[0];
        worker_name_prefix_ = vec[1];
        if (vec.size() == 3)
            email_adress_ = vec[2];
    }

    miner_user_info::miner_user_info(const std::string &wallet_adress, const std::string &worker_name_prefix,
                                     const std::string &email_adress) : wallet_address_(wallet_adress),
                                                                        email_adress_(email_adress),
                                                                        worker_name_prefix_(worker_name_prefix) {}

    const std::string &miner_user_info::get_worker_name_prefix() const {
        return worker_name_prefix_;
    }

    std::string miner_user_info::get_worker_name(int device_id) const {
        return worker_name_prefix_ + "_gpu" + std::to_string(device_id);
    }


    void start_mining_script(currency_type ct, const device_clock_info &dci, const miner_user_info &user_info) {
        //start mining in background process
        char cmd1[BUFFER_SIZE];
        switch (ct) {
            case currency_type::ETH:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_eth.sh %i %i %s %s %s",
                         dci.device_id_nvml, dci.device_id_cuda, user_info.wallet_address_.c_str(),
                         user_info.get_worker_name(dci.device_id_nvml).c_str(), user_info.email_adress_.c_str());
                break;
            case currency_type::ZEC:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_zec.sh %i %i %s %s %s",
                         dci.device_id_nvml, dci.device_id_cuda, user_info.wallet_address_.c_str(),
                         user_info.get_worker_name(dci.device_id_nvml).c_str(), user_info.email_adress_.c_str());
                break;
            case currency_type::XMR:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_xmr.sh %i %i %s %s %s",
                         dci.device_id_nvml, dci.device_id_cuda, user_info.wallet_address_.c_str(),
                         user_info.get_worker_name(dci.device_id_nvml).c_str(), user_info.email_adress_.c_str());
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
