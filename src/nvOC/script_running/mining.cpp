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
        if (vec.size() != 3)
            throw std::runtime_error("Invalid miner user info");
        wallet_adress_ = vec[0];
        worker_name_ = vec[1];
        email_adress_ = vec[2];
    }

    miner_user_info::miner_user_info(const std::string &wallet_adress_, const std::string &worker_name_,
                                     const std::string &email_adress_) : wallet_adress_(wallet_adress_),
                                                                         worker_name_(worker_name_),
                                                                         email_adress_(email_adress_) {}

    std::string miner_user_info::get_combined_user_info(miner_script ms) const {
        switch (ms) {
            case miner_script::ETHMINER:
            case miner_script::XMRSTAK:
                return wallet_adress_ + "." + worker_name_ + "/" + email_adress_;
            case miner_script::EXCAVATOR:
                return wallet_adress_ + "/" + worker_name_ + "/" + email_adress_;
            default:
                throw std::runtime_error("Invalid enum value");
        }
    }


    void start_mining_script(miner_script ms, const device_clock_info &dci, const miner_user_info &user_info) {
        //start mining in background process
        char cmd1[BUFFER_SIZE];
        switch (ms) {
            case miner_script::EXCAVATOR:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_excavator.sh %s %i",
                         user_info.get_combined_user_info(ms).c_str(), dci.device_id_cuda);
                break;
            case miner_script::ETHMINER:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_ethminer.sh %s %i",
                         user_info.get_combined_user_info(ms).c_str(), dci.device_id_cuda);
                break;
            case miner_script::XMRSTAK:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_xmrstak.sh %s %i",
                         user_info.get_combined_user_info(ms).c_str(), dci.device_id_cuda);
                break;
            default:
                throw std::runtime_error("Invalid enum value");
        }
        process_management::gpu_start_process(cmd1, dci.device_id_nvml, process_type::MINER, true);
    }


    void stop_mining_script(int device_id) {
        process_management::gpu_kill_background_process(device_id, process_type::MINER);
    }


}
