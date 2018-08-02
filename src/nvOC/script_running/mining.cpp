//
// Created by alex on 16.05.18.
//
#include "mining.h"

#include <regex>
#include <thread>
#include <fstream>
#include <glog/logging.h>
#include "../common_header/exceptions.h"
#include "../common_header/constants.h"
#include "process_management.h"
#include "log_utils.h"
#include "network_requests.h"

namespace frequency_scaling {


    bool start_mining_script(currency_type ct, const device_clock_info &dci, const miner_user_info &user_info) {
        const std::string &wallet_addr = user_info.wallet_addresses_.at(ct);
        const std::string &worker_name = user_info.worker_names_.at(dci.device_id_nvml_);
        //start mining in background process
        char cmd1[BUFFER_SIZE];
        switch (ct) {
            case currency_type::ETH:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_eth.sh %i %i %s %s %s %s",
                         dci.device_id_nvml_, dci.device_id_cuda_, wallet_addr.c_str(),
                         worker_name.c_str(), user_info.email_adress_.c_str(), log_utils::get_logdir_name().c_str());
                break;
            case currency_type::ZEC:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_zec.sh %i %i %s %s %s %s",
                         dci.device_id_nvml_, dci.device_id_cuda_, wallet_addr.c_str(),
                         worker_name.c_str(), user_info.email_adress_.c_str(), log_utils::get_logdir_name().c_str());
                break;
            case currency_type::XMR:
                snprintf(cmd1, BUFFER_SIZE, "bash ../scripts/start_mining_xmr.sh %i %i %s %s %s %s",
                         dci.device_id_nvml_, dci.device_id_cuda_, wallet_addr.c_str(),
                         worker_name.c_str(), user_info.email_adress_.c_str(), log_utils::get_logdir_name().c_str());
                break;
            default:
                THROW_RUNTIME_ERROR("Invalid enum value");
        }
        //
        return process_management::gpu_start_process(cmd1, dci.device_id_nvml_, process_type::MINER, true);
    }


    bool stop_mining_script(int device_id) {
        return process_management::gpu_kill_background_process(device_id, process_type::MINER);
    }


    double get_avg_hashrate_online_log(currency_type ct, int device_id, long long int system_timestamp_start_ms,
                                       long long int system_timestamp_end_ms) {

        std::string filename = log_utils::get_logdir_name() + "/" +
                               log_utils::get_hash_log_filename(ct, device_id);
        std::ifstream file(filename);
        if (!file)
            THROW_IO_ERROR("Cannot open " + filename);
        file.exceptions(std::ifstream::badbit);
        std::string line;
        std::string::size_type sz = 0;
        double res = 0;
        int count = 0;
        while (std::getline(file, line)) {
            try {
                long long int time = std::stoll(line, &sz);
                if (time >= system_timestamp_start_ms && time <= system_timestamp_end_ms) {
                    res += std::stod(line.substr(sz + 1, std::string::npos));
                    count++;
                }
            }
            catch (const std::invalid_argument &ex) {
                LOG(ERROR) << "Failed to parse hashrate logfile entry: " << ex.what() << std::endl;
            }
        }
        return res / count;
    }

    static measurement run_benchmark_mining_online(
            const std::function<double(currency_type, int, long long int, long long int)> &hashrate_func,
            const miner_user_info &user_info, int period_ms,
            currency_type ct, const device_clock_info &dci, int mem_oc,
            int nvml_graph_clock_idx) {
        int mem_clock = dci.nvapi_default_mem_clock_ + mem_oc;
        int graph_clock = dci.nvml_graph_clocks_[nvml_graph_clock_idx];
        VLOG(0) << gpu_log_prefix(ct, dci.device_id_nvml_) <<
                "Running online benchmark with clocks: mem:" << mem_clock << ",graph:" << graph_clock
                << std::endl;
        //change graph and mem clocks and start mining
        change_gpu_clocks(dci, mem_oc, nvml_graph_clock_idx);
        bool mining_started = start_mining_script(ct, dci, user_info);
        //sleep
        long long int system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        std::this_thread::sleep_for(std::chrono::milliseconds(period_ms));
        //get power and hashrate and stop mining
        long long int system_time_now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        double power = get_avg_power_usage(dci.device_id_nvml_, system_time_start_ms, system_time_now_ms);
        double hashrate = hashrate_func(ct, dci.device_id_nvml_, system_time_start_ms, system_time_now_ms);
        if (mining_started)
            stop_mining_script(dci.device_id_nvml_);
        //create measurement
        measurement m(mem_clock, graph_clock, power, hashrate);
        m.hashrate_measure_dur_ms_ = system_time_now_ms - system_time_start_ms;
        m.power_measure_dur_ms_ = system_time_now_ms - system_time_start_ms;
        m.nvml_graph_clock_idx = nvml_graph_clock_idx;
        m.mem_oc = mem_oc;
        m.graph_oc = graph_clock - dci.nvapi_default_graph_clock_;
        //
        std::string filename = log_utils::get_logdir_name() + "/" +
                               log_utils::get_online_bench_filename(ct, dci.device_id_nvml_);
        std::ofstream logfile(filename, std::ofstream::app);
        if (!logfile)
            THROW_IO_ERROR("Cannot open " + filename);
        logfile.exceptions(std::ifstream::badbit);
        logfile << m.mem_clock_ << "," << m.graph_clock_ << "," << m.power_ << "," << m.hashrate_
                << "," << m.energy_hash_ << "," << m.hashrate_measure_dur_ms_ << "," << system_time_start_ms
                << std::endl;
        return m;
    }

    measurement run_benchmark_mining_online_nanopool(const miner_user_info &user_info, int period_ms,
                                                     currency_type ct, const device_clock_info &dci, int mem_oc,
                                                     int nvml_graph_clock_idx) {
        auto hashrate_func = [&user_info, period_ms](currency_type ct, int device_id,
                                                     long long int, long long int) -> double {
            const std::map<std::string, double> &avg_hashrates = get_avg_hashrate_per_worker_nanopool(
                    ct, user_info.wallet_addresses_.at(ct), period_ms);
            const std::string worker = user_info.worker_names_.at(device_id);
            return avg_hashrates.at(worker);
        };
        //
        return run_benchmark_mining_online(hashrate_func, user_info, period_ms, ct, dci, mem_oc, nvml_graph_clock_idx);
    }


    measurement run_benchmark_mining_online_log(const miner_user_info &user_info, int period_ms,
                                                currency_type ct, const device_clock_info &dci, int mem_oc,
                                                int nvml_graph_clock_idx) {
        return run_benchmark_mining_online(&get_avg_hashrate_online_log, user_info, period_ms, ct, dci, mem_oc,
                                           nvml_graph_clock_idx);
    }

}
