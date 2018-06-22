//
// Created by alex on 16.05.18.
//

#include "profit_optimization.h"

#include <iostream>
#include <thread>
#include <future>
#include <condition_variable>
#include <atomic>
#include <set>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "../freq_nelder_mead/freq_nelder_mead.h"
#include "../freq_hill_climbing/freq_hill_climbing.h"
#include "../freq_simulated_annealing/freq_simulated_annealing.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/process_management.h"
#include "../exceptions.h"
#include "cli_utils.h"

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
                        optimal_config = freq_nelder_mead(&run_benchmark_script_nvml_nvapi, ct, dci, 1,
                                                          opt_params_ct.max_iterations_, opt_params_ct.mem_step_,
                                                          opt_params_ct.graph_idx_step_, opt_params_ct.min_hashrate_);
                        break;
                    case optimization_method::HILL_CLIMBING:
                        optimal_config = freq_hill_climbing(&run_benchmark_script_nvml_nvapi, ct, dci,
                                                            opt_params_ct.max_iterations_, opt_params_ct.mem_step_,
                                                            opt_params_ct.graph_idx_step_, opt_params_ct.min_hashrate_);
                        break;
                    case optimization_method::SIMULATED_ANNEALING:
                        optimal_config = freq_simulated_annealing(&run_benchmark_script_nvml_nvapi, ct, dci,
                                                                  opt_params_ct.max_iterations_,
                                                                  opt_params_ct.mem_step_,
                                                                  opt_params_ct.graph_idx_step_,
                                                                  opt_params_ct.min_hashrate_);
                        break;
                    default:
                        throw std::runtime_error("Invalid enum value");
                }
                //save result
                std::cout << "GPU " << dci.device_id_nvml <<
                          ": Computed optimal energy-hash ratio for currency " << enum_to_string(ct)
                          << ": " << optimal_config.energy_hash_ << std::endl;
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
                                           const miner_user_info &user_infos) {
        currency_type best_currency = profit_calc.getBest_currency_();
        const energy_hash_info &ehi = profit_calc.getEnergy_hash_info_().at(best_currency);
        //
        change_clocks_nvml_nvapi(profit_calc.getDci_(), ehi.optimal_configuration_.mem_oc,
                                 ehi.optimal_configuration_.nvml_graph_clock_idx);
        start_mining_script(best_currency, profit_calc.getDci_(), user_infos);
        std::cout << "GPU " << profit_calc.getDci_().device_id_nvml <<
                  ": Started mining best currency " << enum_to_string(best_currency)
                  << " with approximated profit of " << profit_calc.getBest_currency_profit_() << " euro/h"
                  << std::endl;
    }

    static bool monitoring_sanity_check(const profit_calculator &profit_calc, const miner_user_info &user_infos,
                                        long long int &system_time_start_ms, int &current_monitoring_time_sec) {
        int device_id = profit_calc.getDci_().device_id_nvml;
        //check if miner still running and restart if it somehow crashed
        if (!process_management::gpu_has_background_process(device_id, process_type::MINER)) {
            if (process_management::gpu_has_background_process(device_id, process_type::POWER_MONITOR))
                stop_power_monitoring_script(device_id);
            start_power_monitoring_script(device_id);
            start_mining_best_currency(profit_calc, user_infos);
            current_monitoring_time_sec = 0;
            system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            return false;
        }
        //
        if (!process_management::gpu_has_background_process(device_id, process_type::POWER_MONITOR)) {
            start_power_monitoring_script(device_id);
            system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            return false;
        }
        return true;
    }

    static void
    start_profit_monitoring(profit_calculator &profit_calc,
                            const miner_user_info &user_infos, int update_interval_sec,
                            std::mutex &mutex, std::condition_variable &cond_var, const std::atomic_bool &terminate) {
        //start power monitoring
        start_power_monitoring_script(profit_calc.getDci_().device_id_nvml);
        int current_monitoring_time_sec = 0;
        long long int system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        monitoring_sanity_check(profit_calc, user_infos, system_time_start_ms, current_monitoring_time_sec);
        //monitoring loop
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
            current_monitoring_time_sec += update_interval_sec;
            if (!monitoring_sanity_check(profit_calc, user_infos, system_time_start_ms, current_monitoring_time_sec))
                continue;
            //monitoring code
            try {
                //currently mined currency
                currency_type old_best_currency = profit_calc.getBest_currency_();
                //update with long time power_consumption
                profit_calc.update_power_consumption(old_best_currency, system_time_start_ms);
                //update with pool hashrates if currency is mined > 1h
                if (current_monitoring_time_sec > 3600) {
                    profit_calc.update_opt_config_hashrate_nanopool(old_best_currency, user_infos,
                                                                    current_monitoring_time_sec / 3600.0);
                }
                //update approximated earnings based on current hashrate and stock price
                profit_calc.update_currency_info_nanopool();
                // recalc and check for new best currency
                profit_calc.recalculate_best_currency();
                currency_type new_best_currency = profit_calc.getBest_currency_();
                if (old_best_currency != new_best_currency) {
                    //stop mining former best currency
                    stop_mining_script(profit_calc.getDci_().device_id_nvml);
                    stop_power_monitoring_script(profit_calc.getDci_().device_id_nvml);
                    std::cout << "GPU " << profit_calc.getDci_().device_id_nvml <<
                              ": Stopped mining currency " << enum_to_string(old_best_currency) << std::endl;
                    //start mining new best currency
                    start_power_monitoring_script(profit_calc.getDci_().device_id_nvml);
                    start_mining_best_currency(profit_calc, user_infos);
                    current_monitoring_time_sec = 0;
                    system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
                }
            } catch (const network_error &err) {
                std::cerr << "Network request for currency update failed: " << err.what() << std::endl;
            }
        }
        //
        stop_power_monitoring_script(profit_calc.getDci_().device_id_nvml);
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


    void mine_most_profitable_currency(const optimization_config &opt_config,
                                       const std::map<int, std::map<currency_type, energy_hash_info>> &opt_result) {
        //start mining and monitoring currency with highest profit
        //launch one thread per gpu
        std::cout << "Starting mining and monitoring..." << std::endl;

        std::vector<std::thread> threads;
        std::vector<std::future<std::pair<int, std::map<currency_type, energy_hash_info>>>> futures;
        std::mutex mutex;
        std::condition_variable cond_var;
        std::atomic_bool terminate = ATOMIC_VAR_INIT(false);
        for (int i = 0; i < opt_config.dcis_.size(); i++) {
            std::promise<std::pair<int, std::map<currency_type, energy_hash_info>>> promise;
            futures.push_back(promise.get_future());
            //
            threads.emplace_back([i, &opt_config, &opt_result,
                                         &mutex, &cond_var, &terminate](
                    std::promise<std::pair<int, std::map<currency_type, energy_hash_info>>> &&p) {
                try {
                    const device_clock_info &gpu_dci = opt_config.dcis_[i];
                    const std::map<currency_type, energy_hash_info> &gpu_opt_res =
                            opt_result.at(gpu_dci.device_id_nvml);
                    const std::map<currency_type, currency_info> &gpu_currency_infos =
                            get_currency_infos_nanopool(gpu_opt_res);
                    //create profit calculator for gpu
                    profit_calculator pc(gpu_dci, gpu_currency_infos, gpu_opt_res, opt_config.energy_cost_kwh_);
                    start_mining_best_currency(pc, opt_config.miner_user_infos_);
                    start_profit_monitoring(pc, opt_config.miner_user_infos_, opt_config.monitoring_interval_sec_,
                                            mutex, cond_var, terminate);
                    stop_mining_script(gpu_dci.device_id_nvml);
                    //set return value
                    p.set_value(std::make_pair(gpu_dci.device_id_nvml, pc.getEnergy_hash_info_()));
                } catch (const std::exception &ex) {
                    std::cerr << "Exception in mining/monitoring thread for GPU " <<
                              opt_config.dcis_[i].device_id_nvml << ": " << ex.what() << std::endl;
                    p.set_exception(std::current_exception()); //future.get() triggers the exception
                }
            }, std::move(promise));
        }
        //wait for user to terminate
        std::string user_in = cli_get_string("Performing mining and monitoring...\nEnter q to stop.", "q");
        if (user_in == "q") {
            terminate = true;
            cond_var.notify_all();
        }
        //join threads
        for (auto &t : threads)
            t.join();

        std::map<int, std::map<currency_type, energy_hash_info>> updated_opt_result;
        for (auto &f : futures) {
            updated_opt_result.insert(f.get());
        }

        //save opt results dialog
        user_in = cli_get_string("Save optimization results? [y/n]", "[yn]");
        if (user_in == "y") {
            user_in = cli_get_string("Enter filename:", "[\\w-.]+");
            save_optimization_result(user_in, updated_opt_result);
        }

    }


    void mine_most_profitable_currency(const optimization_config &opt_config) {
        //check for equal gpus and distribute optimization work accordingly
        const std::vector<std::vector<int>> &equal_gpus = find_equal_gpus(opt_config.dcis_);
        std::set<currency_type> currencies;
        for (auto &ui : opt_config.miner_user_infos_.wallet_addresses_)
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

        //
        mine_most_profitable_currency(opt_config, optimization_results);
    }


    void save_optimization_result(const std::string &filename,
                                  const std::map<int, std::map<currency_type, energy_hash_info>> &opt_results) {
        namespace pt = boost::property_tree;
        pt::ptree root;
        //write devices to use
        for (auto &device : opt_results) {
            //write currencies to use for device
            pt::ptree pt_currencies;
            for (auto &currency : device.second) {
                pt::ptree pt_ehi;
                pt_ehi.put("power", currency.second.optimal_configuration_.power_);
                pt_ehi.put("hashrate", currency.second.optimal_configuration_.hashrate_);
                pt_ehi.put("energy_hash", currency.second.optimal_configuration_.energy_hash_);
                pt_ehi.put("nvml_graph_clock_idx", currency.second.optimal_configuration_.nvml_graph_clock_idx);
                pt_ehi.put("mem_oc", currency.second.optimal_configuration_.mem_oc);
                pt_ehi.put("graph_oc", currency.second.optimal_configuration_.graph_oc);
                pt_ehi.put("graph_clock", currency.second.optimal_configuration_.graph_clock_);
                pt_ehi.put("mem_clock", currency.second.optimal_configuration_.mem_clock_);
                pt_currencies.add_child(enum_to_string(currency.first), pt_ehi);
            }
            root.add_child(std::to_string(device.first), pt_currencies);
        }
        pt::write_json(filename, root);
    }


    std::map<int, std::map<currency_type, energy_hash_info>> load_optimization_result(const std::string &filename) {
        std::map<int, std::map<currency_type, energy_hash_info>> opt_results;
        namespace pt = boost::property_tree;
        pt::ptree root;
        pt::read_json(filename, root);
        //read device infos
        for (const pt::ptree::value_type &array_elem : root) {
            const boost::property_tree::ptree &pt_device = array_elem.second;
            std::map<currency_type, energy_hash_info> cur_ehi;
            for (const pt::ptree::value_type &array_elem2 : pt_device) {
                const boost::property_tree::ptree &pt_ehi = array_elem2.second;
                currency_type ct = string_to_currency_type(array_elem2.first);
                measurement opt_config;
                opt_config.mem_clock_ = pt_ehi.get<int>("mem_clock");
                opt_config.graph_clock_ = pt_ehi.get<int>("graph_clock");
                opt_config.mem_oc = pt_ehi.get<int>("mem_oc");
                opt_config.graph_oc = pt_ehi.get<int>("graph_oc");
                opt_config.nvml_graph_clock_idx = pt_ehi.get<int>("nvml_graph_clock_idx");
                opt_config.power_ = pt_ehi.get<double>("power");
                opt_config.hashrate_ = pt_ehi.get<double>("hashrate");
                opt_config.energy_hash_ = pt_ehi.get<double>("energy_hash");
                cur_ehi.emplace(ct, energy_hash_info(ct, opt_config));
            }
            opt_results.emplace(std::stoi(array_elem.first), cur_ehi);
        }
        return opt_results;
    }


}