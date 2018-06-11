//
// Created by alex on 16.05.18.
//

#include "profit_optimization.h"

#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <set>
#include "../freq_nelder_mead/freq_nelder_mead.h"
#include "../freq_hill_climbing/freq_hill_climbing.h"
#include "../freq_simulated_annealing/freq_simulated_annealing.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/process_management.h"
#include "../exceptions.h"
#include "network_requests.h"

namespace frequency_scaling {

    static const int BUFFER_SIZE = 1024;

    static std::map<currency_type, energy_hash_info> find_optimal_config(const device_clock_info &dci,
                                                                         const std::set<currency_type> &currencies,
                                                                         const std::map<currency_type, optimization_method_params> &opt_method_params) {
        std::map<currency_type, energy_hash_info> energy_hash_infos;
        //start power monitoring
        start_power_monitoring_script(dci.device_id_nvml);
        for (currency_type ct : currencies) {
            try {
                measurement optimal_config;
                const optimization_method_params &opt_params_ct = opt_method_params.at(ct);
                switch (opt_params_ct.method_) {
                    case optimization_method::NELDER_MEAD:
                        optimal_config = freq_nelder_mead(ct, dci, 1,
                                                          opt_params_ct.max_iterations_, opt_params_ct.mem_step_,
                                                          opt_params_ct.graph_idx_step_, opt_params_ct.min_hashrate_);
                        break;
                    case optimization_method::HILL_CLIMBING:
                        optimal_config = freq_hill_climbing(ct, dci,
                                                            opt_params_ct.max_iterations_, opt_params_ct.mem_step_,
                                                            opt_params_ct.graph_idx_step_, opt_params_ct.min_hashrate_);
                        break;
                    case optimization_method::SIMULATED_ANNEALING:
                        optimal_config = freq_simulated_annealing(ct, dci,
                                                                  opt_params_ct.max_iterations_,
                                                                  opt_params_ct.mem_step_,
                                                                  opt_params_ct.graph_idx_step_,
                                                                  opt_params_ct.min_hashrate_);
                        break;
                    default:
                        throw std::runtime_error("Invalid enum value");
                }
                //save result
                energy_hash_infos.emplace(ct, energy_hash_info(ct, optimal_config));
                //move generated result files
                char cmd[BUFFER_SIZE];
                snprintf(cmd, BUFFER_SIZE, "bash ../scripts/move_result_files.sh %i %s",
                         dci.device_id_nvml, enum_to_string(ct).c_str());
                process_management::start_process(cmd, false);
            } catch (const optimization_error &err) {
                std::cerr << "Optimization for currency " << enum_to_string(ct) <<
                          " on GPU " << dci.device_id_nvml << "failed: " << err.what() << std::endl;
                //remove already generated result files
                char cmd[BUFFER_SIZE];
                snprintf(cmd, BUFFER_SIZE, "bash ../scripts/remove_result_files.sh %i %s",
                         dci.device_id_nvml, enum_to_string(ct).c_str());
                process_management::start_process(cmd, false);
            }
            //sleep 60 seconds to let gpu cool down
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
        stop_power_monitoring_script(dci.device_id_nvml);
        return energy_hash_infos;
    };


    static void start_mining_best_currency(const profit_calculator &profit_calc,
                                           const std::map<currency_type, miner_user_info> &user_infos) {
        const std::pair<currency_type, double> &best_currency = profit_calc.calc_best_currency();
        const energy_hash_info &ehi = profit_calc.getEnergy_hash_info_().at(best_currency.first);
        //
        change_clocks_nvml_nvapi(profit_calc.getDci_(), ehi.optimal_configuration_.mem_oc,
                                 ehi.optimal_configuration_.nvml_graph_clock_idx);
        start_mining_script(best_currency.first, profit_calc.getDci_(), user_infos.at(best_currency.first));
    }


    static void
    start_profit_monitoring(profit_calculator &profit_calc,
                            const std::map<currency_type, miner_user_info> &user_infos, int update_interval_sec,
                            std::mutex &mutex, std::condition_variable &cond_var, const std::atomic_bool &terminate) {
        while (true) {
            std::cv_status stat;
            int remaining_sleep_ms = update_interval_sec * 1000;
            do {
                std::unique_lock<std::mutex> lock(mutex);
                auto start = std::chrono::steady_clock::now();
                stat = cond_var.wait_for(lock, std::chrono::milliseconds(remaining_sleep_ms));
                remaining_sleep_ms -= std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start).count();
            } while (stat == std::cv_status::no_timeout && !terminate);//prevent spurious wakeups
            if (terminate)
                break;
            //monitoring code
            try {
                const std::pair<currency_type, double> &old_best_currency = profit_calc.calc_best_currency();
                profit_calc.update_currency_info_nanopool();
                const std::pair<currency_type, double> &new_best_currency = profit_calc.calc_best_currency();
                if (old_best_currency.first != new_best_currency.first) {
                    stop_mining_script(profit_calc.getDci_().device_id_nvml);
                    start_mining_best_currency(profit_calc, user_infos);
                }
            } catch (const network_error &err) {
                std::cerr << "Network request for currency update failed: " << err.what() << std::endl;
            }
        }
    }


    static std::vector<std::vector<int>> find_equal_gpus(const std::vector<device_clock_info> &dcis) {
        std::vector<std::vector<int>> res;
        std::set<int> remaining_devices;
        for (auto &dci : dcis)
            remaining_devices.insert(dci.device_id_nvml);
        while (!remaining_devices.empty()) {
            int i = *remaining_devices.begin();
            std::vector<int> equal_devs({i});
            for (int j : remaining_devices) {
                if (i == j)
                    continue;
                if (nvmlGetDeviceName(i) == nvmlGetDeviceName(j))
                    equal_devs.push_back(j);
            }
            for (int to_remove : equal_devs)
                remaining_devices.erase(to_remove);
            res.push_back(equal_devs);
        }
        return res;
    }


    static std::map<int, std::set<currency_type>>
    find_optimization_gpu_distr(const std::vector<std::vector<int>> &equal_gpus,
                                const std::set<currency_type> &currencies) {
        std::map<int, std::set<currency_type>> res;
        for (auto &eq_vec : equal_gpus)
            for (int device_id_nvml : eq_vec)
                res.emplace(device_id_nvml, std::set<currency_type>());

        for (auto &eg : equal_gpus) {
            int i;
            for (i = 0; i < eg.size(); i++) {
                int gpu = eg[i];
                if (i < static_cast<int>(currency_type::count)) {
                    currency_type ct = static_cast<currency_type>(i);
                    if (currencies.count(ct) != 0)
                        res.at(gpu).insert(ct);
                }
            }
            for (; i < static_cast<int>(currency_type::count); i++) {
                currency_type ct = static_cast<currency_type>(i);
                if (currencies.count(ct) != 0)
                    res.at(eg.front()).insert(ct);
            }
        }
        return res;
    };


    static void
    complete_optimization_results(std::map<int, std::map<currency_type, energy_hash_info>> &optimization_results,
                                  const std::vector<std::vector<int>> &equal_gpus) {
        for (auto &eq_vec : equal_gpus) {
            for (int i = 0; i < eq_vec.size(); i++) {
                std::map<currency_type, energy_hash_info> &cur_or = optimization_results.at(eq_vec[i]);
                for (int j = 0; j < eq_vec.size(); j++) {
                    if (i == j)
                        continue;
                    const std::map<currency_type, energy_hash_info> &other_or = optimization_results.at(eq_vec[j]);
                    for (auto &elem : other_or)
                        cur_or.emplace(elem.first, elem.second);
                }
            }
        }
    }


    void mine_most_profitable_currency(const optimization_config &opt_config) {
        //check for equal gpus and distribute optimization work accordingly
        const std::vector<std::vector<int>> &equal_gpus = find_equal_gpus(opt_config.dcis_);
        std::set<currency_type> currencies;
        for (auto &ui : opt_config.miner_user_infos_)
            currencies.insert(ui.first);
        const std::map<int, std::set<currency_type>> &gpu_distr =
                find_optimization_gpu_distr(equal_gpus, currencies);

        //find best frequency configurations for each gpu and currency
        //launch one thread per gpu
        std::cout << "Starting optimization phase..." << std::endl;
        std::map<int, std::map<currency_type, energy_hash_info>> optimization_results;
        {
            std::vector<std::thread> threads;
            std::mutex mutex;
            for (int i = 0; i < opt_config.dcis_.size(); i++) {
                threads.emplace_back([i, &mutex, &opt_config, &gpu_distr, &optimization_results]() {
                    try {
                        const device_clock_info &gpu_dci = opt_config.dcis_[i];
                        const std::map<currency_type, energy_hash_info> &gpu_optimal_config =
                                find_optimal_config(gpu_dci, gpu_distr.at(gpu_dci.device_id_nvml),
                                                    opt_config.opt_method_params_);
                        std::lock_guard<std::mutex> lock(mutex);
                        optimization_results.emplace(gpu_dci.device_id_nvml, gpu_optimal_config);
                    } catch (const std::exception &ex) {
                        std::cerr << "Exception in optimization thread for GPU " <<
                                  opt_config.dcis_[i].device_id_nvml << ": " << ex.what() << std::endl;
                    }
                });
            }
            //join threads
            for (auto &t : threads)
                t.join();
        }

        //exchange frequency configurations (equal gpus have same optimal frequency configurations)
        complete_optimization_results(optimization_results, equal_gpus);
        std::cout << "Finished optimization phase..." << std::endl;

        //start mining and monitoring currency with highest profit
        //launch one thread per gpu
        std::cout << "Starting mining and monitoring..." << std::endl;
        {
            std::vector<std::thread> threads;
            std::mutex mutex;
            std::condition_variable cond_var;
            std::atomic_bool terminate = ATOMIC_VAR_INIT(false);
            for (int i = 0; i < opt_config.dcis_.size(); i++) {
                threads.emplace_back([i, &opt_config, &optimization_results,
                                             &mutex, &cond_var, &terminate]() {
                    try {
                        const device_clock_info &gpu_dci = opt_config.dcis_[i];
                        const std::map<currency_type, energy_hash_info> &gpu_opt_res =
                                optimization_results.at(gpu_dci.device_id_nvml);
                        const std::map<currency_type, currency_info> &gpu_currency_infos =
                                get_currency_infos_nanopool(gpu_opt_res);
                        //create profit calculator for gpu
                        profit_calculator pc(gpu_dci, gpu_currency_infos, gpu_opt_res, opt_config.energy_cost_kwh_);
                        start_mining_best_currency(pc, opt_config.miner_user_infos_);
                        start_profit_monitoring(pc, opt_config.miner_user_infos_, opt_config.monitoring_interval_sec_,
                                                mutex, cond_var, terminate);
                    } catch (const std::exception &ex) {
                        std::cerr << "Exception in mining/monitoring thread for GPU " <<
                                  opt_config.dcis_[i].device_id_nvml << ": " << ex.what() << std::endl;
                    }
                });
            }
            //wait for user to terminate
            std::cout << "Performing mining and monitoring...\nEnter q to stop." << std::endl;
            while (true) {
                char c;
                std::cin >> c;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                if (c == 'q') {
                    terminate = true;
                    cond_var.notify_all();
                    break;
                } else {
                    std::cout << "Performing mining and monitoring...\nEnter q to stop." << std::endl;
                }
            }
            //join threads
            for (auto &t : threads)
                t.join();
            //cleanup processes
            process_management::kill_all_processes(false);
        }

    }


}