//
// Created by alex on 16.05.18.
//

#include "mining.h"
#include <stdexcept>
#include <regex>

namespace frequency_scaling {

    static const int BUFFER_SIZE = 1024;

    miner_user_info::miner_user_info(const std::string &combined_user_info, miner_script ms) {
        std::string regex;
        switch (ms) {
            case miner_script::ETHMINER:
            case miner_script::XMRSTAK:
                regex = "\\.|/";
                break;
            case miner_script::EXCAVATOR:
                regex = "/";
                break;
            default:
                throw std::runtime_error("Invalid enum value");
        }
        std::regex re(regex);
        std::sregex_token_iterator first{combined_user_info.begin(), combined_user_info.end(), re, -1}, last;
        std::vector<std::string> vec(first, last);
        if(vec.size() != 3)
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
                snprintf(cmd1, BUFFER_SIZE, "sh ../scripts/start_mining_excavator.sh %s %i",
                         user_info.get_combined_user_info(ms).c_str(), dci.device_id_cuda);
                break;
            case miner_script::ETHMINER:
                snprintf(cmd1, BUFFER_SIZE, "sh ../scripts/start_mining_ethminer.sh %s %i",
                         user_info.get_combined_user_info(ms).c_str(), dci.device_id_cuda);
                break;
            case miner_script::XMRSTAK:
                snprintf(cmd1, BUFFER_SIZE, "sh ../scripts/start_mining_xmrstak.sh %s %i",
                         user_info.get_combined_user_info(ms).c_str(), dci.device_id_cuda);
                break;
            default:
                throw std::runtime_error("Invalid enum value");
        }
        system(cmd1);

    }

    void stop_mining_script(miner_script ms) {
        //stop mining
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
