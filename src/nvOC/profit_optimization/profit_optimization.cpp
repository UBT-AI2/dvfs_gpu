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
                                           const std::map<currency_type, miner_user_info> &user_infos) {
        currency_type best_currency = profit_calc.getBest_currency_();
        const energy_hash_info &ehi = profit_calc.getEnergy_hash_info_().at(best_currency);
        //
        change_clocks_nvml_nvapi(profit_calc.getDci_(), ehi.optimal_configuration_.mem_oc,
                                 ehi.optimal_configuration_.nvml_graph_clock_idx);
        start_mining_script(best_currency, profit_calc.getDci_(), user_infos.at(best_currency));
        std::cout << "GPU " << profit_calc.getDci_().device_id_nvml <<
                  ": Started mining best currency " << enum_to_string(best_currency)
                  << " with approximated profit of " << profit_calc.getBest_currency_profit_() << " euro/h"
                  << std::endl;
    }


    static void
    start_profit_monitoring(profit_calculator &profit_calc,
                            const std::map<currency_type, miner_user_info> &user_infos, int update_interval_sec,
                            std::mutex &mutex, std::condition_variable &cond_var, const std::atomic_bool &terminate) {
        int current_monitoring_time_sec = 0;
        long long int system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
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
            //monitoring code
            try {
                //currently mined currency
                currency_type old_best_currency = profit_calc.getBest_currency_();
                //update with pool hashrates and long time power_consumption if currency is mined > 1h
                if (current_monitoring_time_sec > 3600) {
                    profit_calc.update_opt_config_hashrate_nanopool(old_best_currency, user_infos.at(old_best_currency),
                                                                    current_monitoring_time_sec / 3600.0);
                    profit_calc.update_power_consumption(old_best_currency, system_time_start_ms);
                }
                //update approximated earnings based on current hashrate and stock price
                profit_calc.update_currency_info_nanopool();
                // recalc and check for new best currency
                profit_calc.recalculate_best_currency();
                currency_type new_best_currency = profit_calc.getBest_currency_();
                if (old_best_currency != new_best_currency) {
                    stop_mining_script(profit_calc.getDci_().device_id_nvml);
                    std::cout << "GPU " << profit_calc.getDci_().device_id_nvml <<
                              ": Stopped mining currency " << enum_to_string(old_best_currency) << std::endl;
                    start_mining_best_currency(profit_calc, user_infos);
                    current_monitoring_time_sec = 0;
                    system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();
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
        /*{
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
        std::cout << "Finished optimization phase..." << std::endl;*/

		for (auto& dci : opt_config.dcis_) {
			std::map<currency_type, energy_hash_info> cur;
			measurement meth, mzec, mxmr;
			meth.energy_hash_ = 262372;
			meth.power_ = 121.16;
			meth.hashrate_ = 31788991;
			meth.nvml_graph_clock_idx = 60;
			meth.mem_oc = 300;
			meth.mem_clock_ = 5305;
			meth.graph_clock_ = 1151;
			//
			mzec.energy_hash_ = 4.01;
			mzec.power_ = 108.37;
			mzec.hashrate_ = 435.37;
			mzec.nvml_graph_clock_idx = 57;
			mzec.mem_oc = -667;
			mzec.mem_clock_ = 4338;
			mzec.graph_clock_ = 1189;
			//
			mxmr.energy_hash_ = 8.384;
			mxmr.power_ = 103.59;
			mxmr.hashrate_ = 868.5;
			mxmr.nvml_graph_clock_idx = 41;
			mxmr.mem_oc = 900;
			mxmr.mem_clock_ = 5905;
			mxmr.graph_clock_ = 1392;
			//
			cur.emplace(currency_type::ETH, energy_hash_info(currency_type::ETH, meth));
			cur.emplace(currency_type::ZEC, energy_hash_info(currency_type::ZEC, mzec));
			cur.emplace(currency_type::XMR, energy_hash_info(currency_type::XMR, mxmr));
			//
			optimization_results.emplace(dci.device_id_nvml, cur);
		}

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
						stop_mining_script(gpu_dci.device_id_nvml);
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
            //process_management::kill_all_processes(false);
        }

    }


	void save_optimization_result(const std::string& filename,
		const std::map<int, std::map<currency_type, energy_hash_info>>& opt_results) {

	}

	std::map<int, std::map<currency_type, energy_hash_info>> load_optimization_result(const std::string& filename) {
		return std::map<int, std::map<currency_type, energy_hash_info>>();
	}


}