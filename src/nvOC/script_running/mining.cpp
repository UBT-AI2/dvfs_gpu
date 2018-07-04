//
// Created by alex on 16.05.18.
//
#include "mining.h"

#include <stdexcept>
#include <regex>
#include <thread>
#include <fstream>
#include "process_management.h"
#include "network_requests.h"

namespace frequency_scaling {

    static const int BUFFER_SIZE = 1024;


    bool start_mining_script(currency_type ct, const device_clock_info &dci, const miner_user_info &user_info) {
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
        return process_management::gpu_start_process(cmd1, dci.device_id_nvml, process_type::MINER, true);
    }


    bool stop_mining_script(int device_id) {
        return process_management::gpu_kill_background_process(device_id, process_type::MINER);
    }


    double get_avg_hashrate_online_log(currency_type ct, int device_id, long long int system_timestamp_start_ms,
                                       long long int system_timestamp_end_ms) {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open("hash_log_" + enum_to_string(ct) + "_" + std::to_string(device_id) + ".txt");
        file.exceptions(std::ifstream::badbit);
        std::string line;
        std::string::size_type sz = 0;
        double res = 0;
        int count = 0;
        while (std::getline(file, line)) {
            long long int time = std::stoll(line, &sz);
            if (time >= system_timestamp_start_ms && time <= system_timestamp_end_ms) {
                res += std::stod(line.substr(sz + 1, std::string::npos));
                count++;
            }
        }
        return res / count;
    }

    static measurement run_benchmark_mining_online(
            const std::function<double(currency_type, int, long long int, long long int)> &hashrate_func,
            const miner_user_info &user_info, int period_ms,
            currency_type ct, const device_clock_info &dci, int mem_oc,
            int nvml_graph_clock_idx) {
        //change graph and mem clocks and start mining
        change_clocks_nvml_nvapi(dci, mem_oc, nvml_graph_clock_idx);
        bool mining_started = start_mining_script(ct, dci, user_info);
        //sleep
        long long int system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        std::this_thread::sleep_for(std::chrono::milliseconds(period_ms));
        //get power and hashrate and stop mining
        long long int system_time_now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        double power = get_avg_power_usage(dci.device_id_nvml, system_time_start_ms, system_time_now_ms);
        double hashrate = hashrate_func(ct, dci.device_id_nvml, system_time_start_ms, system_time_now_ms);
        if(mining_started)
            stop_mining_script(dci.device_id_nvml);
        //create measurement
        int mem_clock = dci.nvapi_default_mem_clock + mem_oc;
        int graph_clock = dci.nvml_graph_clocks[nvml_graph_clock_idx];
        measurement m(mem_clock, graph_clock, power, hashrate);
        m.hashrate_measure_dur_ms_ = period_ms;
        m.power_measure_dur_ms_ = period_ms;
        m.nvml_graph_clock_idx = nvml_graph_clock_idx;
        m.mem_oc = mem_oc;
        m.graph_oc = 0;
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


    measurement run_benchmark_mining_online_log(const miner_user_info &user_info, int period_ms, currency_type ct,
                                                const device_clock_info &dci, int mem_oc, int nvml_graph_clock_idx) {

        auto hashrate_func = [](currency_type ct, int device_id,
                                long long int sys_start, long long int sys_end) -> double {
            return get_avg_hashrate_online_log(ct, device_id, sys_start, sys_end);
        };
        //
        return run_benchmark_mining_online(hashrate_func, user_info, period_ms, ct, dci, mem_oc, nvml_graph_clock_idx);
    }

}
