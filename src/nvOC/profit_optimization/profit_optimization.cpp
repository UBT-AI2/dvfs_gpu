//
// Created by alex on 16.05.18.
//
#include "profit_optimization.h"

#include <thread>
#include <future>
#include <condition_variable>
#include <atomic>
#include <set>
#include <list>
#include <glog/logging.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "../freq_nelder_mead/freq_nelder_mead.h"
#include "../freq_hill_climbing/freq_hill_climbing.h"
#include "../freq_simulated_annealing/freq_simulated_annealing.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/process_management.h"
#include "../common_header/constants.h"
#include "../common_header/exceptions.h"
#include "cli_utils.h"

namespace frequency_scaling {

    struct gpu_group {
        explicit gpu_group(const std::vector<std::pair<int, bool>> &members) :
                members_(members), failed_(false) {}

        gpu_group(const gpu_group &) = delete;

        gpu_group &operator=(const gpu_group &) = delete;

        bool has_member(int device_id) const {
            for (auto &elem : members_)
                if (elem.first == device_id) {
                    return true;
                }
            return false;
        }

        void set_member_complete(int device_id) {
            for (auto &elem : members_)
                if (elem.first == device_id) {
                    elem.second = true;
                    break;
                }
        }

        bool group_complete() const {
            for (auto &elem : members_)
                if (!elem.second)
                    return false;
            return true;
        }

        std::vector<std::pair<int, bool>> members_;
        std::atomic_bool failed_;
        std::mutex mutex_;
        std::condition_variable cond_var_;
    };

    struct benchmark_info {
        explicit benchmark_info(const benchmark_func &bf) :
                bf_(bf), offline_(true) {}

        benchmark_info(const benchmark_func &bf,
                       const std::map<currency_type, measurement> &start_values,
                       const miner_user_info &mui) :
                bf_(bf), offline_(false), start_values_(start_values), mui_(mui) {}

        const benchmark_func bf_;
        const bool offline_;
        const std::map<currency_type, measurement> start_values_;
        const miner_user_info mui_;
    };


    static std::pair<bool, measurement>
    find_optimal_config_currency(const benchmark_info &bi, const device_clock_info &dci,
                                 currency_type ct, const optimization_method_params &opt_params_ct) {
        //start power monitoring
        bool pm_started = start_power_monitoring_script(dci.device_id_nvml_);
        bool mining_started = false;
        if (!bi.offline_) {
            mining_started = start_mining_script(ct, dci, bi.mui_);
            //wait to ensure reasonable hashrates from the beginning
            //TODO: check if file exists
            std::this_thread::sleep_for(std::chrono::seconds(120));
        }
        //
        try {
            int online_div = 2;
            //
            measurement optimal_config;
            switch (opt_params_ct.method_) {
                case optimization_method::NELDER_MEAD:
                    if (bi.offline_)
                        optimal_config = freq_nelder_mead(bi.bf_, ct, dci,
                                                          1, opt_params_ct.max_iterations_, opt_params_ct.mem_step_,
                                                          opt_params_ct.graph_idx_step_,
                                                          opt_params_ct.min_hashrate_);
                    else
                        optimal_config = freq_nelder_mead(bi.bf_, ct, dci,
                                                          bi.start_values_.at(ct),
                                                          1, opt_params_ct.max_iterations_ / online_div,
                                                          opt_params_ct.mem_step_ / online_div,
                                                          opt_params_ct.graph_idx_step_ / online_div,
                                                          opt_params_ct.min_hashrate_);
                    break;
                case optimization_method::HILL_CLIMBING:
                    if (bi.offline_)
                        optimal_config = freq_hill_climbing(bi.bf_, ct, dci,
                                                            opt_params_ct.max_iterations_, opt_params_ct.mem_step_,
                                                            opt_params_ct.graph_idx_step_,
                                                            opt_params_ct.min_hashrate_);
                    else
                        optimal_config = freq_hill_climbing(bi.bf_, ct, dci,
                                                            bi.start_values_.at(ct), true,
                                                            opt_params_ct.max_iterations_ / online_div,
                                                            opt_params_ct.mem_step_ / online_div,
                                                            opt_params_ct.graph_idx_step_ / online_div,
                                                            opt_params_ct.min_hashrate_);
                    break;
                case optimization_method::SIMULATED_ANNEALING:
                    if (bi.offline_)
                        optimal_config = freq_simulated_annealing(bi.bf_, ct, dci,
                                                                  opt_params_ct.max_iterations_,
                                                                  opt_params_ct.mem_step_,
                                                                  opt_params_ct.graph_idx_step_,
                                                                  opt_params_ct.min_hashrate_);
                    else
                        optimal_config = freq_simulated_annealing(bi.bf_, ct, dci,
                                                                  bi.start_values_.at(ct),
                                                                  opt_params_ct.max_iterations_ / online_div,
                                                                  opt_params_ct.mem_step_ / online_div,
                                                                  opt_params_ct.graph_idx_step_ / online_div,
                                                                  opt_params_ct.min_hashrate_);
                    break;
                default:
                    THROW_RUNTIME_ERROR("Invalid enum value");
            }
            //
            VLOG(0) << gpu_log_prefix(ct, dci.device_id_nvml_) <<
                    "Computed optimal energy-hash ratio: "
                    << optimal_config.energy_hash_ << " (mem=" << optimal_config.mem_clock_ << ",graph="
                    << optimal_config.graph_clock_ << ")"
                    << std::endl;
            if (mining_started)
                stop_mining_script(dci.device_id_nvml_);
            if (pm_started)
                stop_power_monitoring_script(dci.device_id_nvml_);
            return std::make_pair(true, optimal_config);
        } catch (const optimization_error &err) {
            LOG(ERROR) << gpu_log_prefix(ct, dci.device_id_nvml_) << "Optimization failed: " << err.what()
                       << std::endl;
            if (mining_started)
                stop_mining_script(dci.device_id_nvml_);
            if (pm_started)
                stop_power_monitoring_script(dci.device_id_nvml_);
            return std::make_pair(false, measurement());
        }
    }

    static std::map<currency_type, measurement>
    find_optimal_config(const benchmark_info &bi, const device_clock_info &dci,
                        const std::set<currency_type> &currencies,
                        const std::map<currency_type, optimization_method_params> &opt_method_params) {
        std::map<currency_type, measurement> energy_hash_infos;
        //start power monitoring
        bool pm_started = start_power_monitoring_script(dci.device_id_nvml_);
        //stops mining if its running!!!
        if (!bi.offline_)
            stop_mining_script(dci.device_id_nvml_);
        //
        for (currency_type ct : currencies) {
            const std::pair<bool, measurement> &opt_config =
                    find_optimal_config_currency(bi, dci, ct, opt_method_params.at(ct));
            if (opt_config.first)
                energy_hash_infos.emplace(ct, opt_config.second);
        }
        if (pm_started)
            stop_power_monitoring_script(dci.device_id_nvml_);
        return energy_hash_infos;
    }


    static void start_mining_best_currency(const profit_calculator &profit_calc,
                                           const miner_user_info &user_infos) {
        int device_id = profit_calc.getDci_().device_id_nvml_;
        //stops mining if its running!!!
        stop_mining_script(device_id);
        currency_type best_currency = profit_calc.getBest_currency_();
        start_mining_script(best_currency, profit_calc.getDci_(), user_infos);
        VLOG(0) << "GPU " << device_id <<
                ": Started mining best currency " << enum_to_string(best_currency)
                << " with approximated profit of "
                << profit_calc.getBest_currency_profit_() << " euro/h"
                << std::endl;
        //change frequencies at the end (danger to trigger nvml unknown error)
        const energy_hash_info &ehi = profit_calc.getEnergy_hash_info_().at(best_currency);
        change_clocks_nvml_nvapi(profit_calc.getDci_(), ehi.optimal_configuration_online_.mem_oc,
                                 ehi.optimal_configuration_online_.nvml_graph_clock_idx);
    }

    static bool monitoring_sanity_check(const profit_calculator &profit_calc, const miner_user_info &user_infos,
                                        long long int &system_time_start_ms, int &current_monitoring_time_ms) {
        int device_id = profit_calc.getDci_().device_id_nvml_;
        bool res = true;
        //check if miner and power_monitor still running and restart if one somehow crashed
        if (!process_management::gpu_has_background_process(device_id, process_type::MINER)) {
            LOG(ERROR) << gpu_log_prefix(profit_calc.getBest_currency_(), device_id)
                       << "Monitoring sanity check failed: Miner down" << std::endl;
            start_mining_best_currency(profit_calc, user_infos);
            current_monitoring_time_ms = 0;
            res = false;
        }
        if (!process_management::gpu_has_background_process(device_id, process_type::POWER_MONITOR)) {
            LOG(ERROR) << gpu_log_prefix(profit_calc.getBest_currency_(), device_id)
                       << "Monitoring sanity check failed: Power monitor down" << std::endl;
            start_power_monitoring_script(device_id);
            system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            res = false;
        }
        return res;
    }

    static void
    start_profit_monitoring(profit_calculator &profit_calc,
                            const miner_user_info &user_infos,
                            const std::map<currency_type, optimization_method_params> &opt_method_params,
                            int update_interval_ms,
                            std::mutex &mutex, std::condition_variable &cond_var, const std::atomic_bool &terminate) {
        //start power monitoring
        int device_id = profit_calc.getDci_().device_id_nvml_;
        bool pm_started = start_power_monitoring_script(device_id);
        int current_monitoring_time_ms = 0;
        long long int system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        monitoring_sanity_check(profit_calc, user_infos, system_time_start_ms, current_monitoring_time_ms);
        //monitoring loop
        while (!terminate) {
            std::cv_status stat;
            int remaining_sleep_ms = update_interval_ms;
            do {
                std::unique_lock<std::mutex> lock(mutex);
                auto start = std::chrono::steady_clock::now();
                stat = cond_var.wait_for(lock, std::chrono::milliseconds(remaining_sleep_ms));
                remaining_sleep_ms -= std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start).count();
            } while (stat == std::cv_status::no_timeout && !terminate);//prevent spurious wakeups
            if (terminate)
                break;
            current_monitoring_time_ms += update_interval_ms;
            if (!monitoring_sanity_check(profit_calc, user_infos, system_time_start_ms, current_monitoring_time_ms))
                continue;
            //currently mined currency
            currency_type old_best_currency = profit_calc.getBest_currency_();
            //update with pool hashrates and long time power consumption if currency is mined > 1h
            if (current_monitoring_time_ms > 3600 * 1000) {
                //update power only if hashrate update was successful -> pool could be down!!!
                if (profit_calc.update_opt_config_hashrate_nanopool(old_best_currency, user_infos,
                                                                    current_monitoring_time_ms))
                    profit_calc.update_power_consumption(old_best_currency, system_time_start_ms);

            }
            //update approximated earnings based on current power,hashrate and stock price
            profit_calc.update_currency_info_nanopool();
            // recalc and check for new best currency
            profit_calc.recalculate_best_currency();
            currency_type new_best_currency = profit_calc.getBest_currency_();
            if (old_best_currency != new_best_currency) {
                //stop mining former best currency
                profit_calc.save_current_period(old_best_currency);
                stop_mining_script(device_id);
                VLOG(0) << "GPU " << device_id <<
                        ": Stopped mining currency "
                        << enum_to_string(old_best_currency) << std::endl;
                //start mining new best currency
                start_mining_best_currency(profit_calc, user_infos);
                //reset timestamps
                current_monitoring_time_ms = 0;
                system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                //reoptimize frequencies
                const std::pair<bool, measurement> &new_opt_config_online =
                        find_optimal_config_currency(benchmark_info(std::bind(&run_benchmark_mining_online_log,
                                                                              std::cref(user_infos),
                                                                              2 * 60 * 1000,
                                                                              std::placeholders::_1,
                                                                              std::placeholders::_2,
                                                                              std::placeholders::_3,
                                                                              std::placeholders::_4),
                                                                    profit_calc.get_opt_start_values(),
                                                                    user_infos),
                                                     profit_calc.getDci_(), new_best_currency,
                                                     opt_method_params.at(new_best_currency));
                current_monitoring_time_ms += std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count() - system_time_start_ms;
                //update config and change frequencies if optimization was successful
                if (new_opt_config_online.first) {
                    profit_calc.update_opt_config_online(new_best_currency, new_opt_config_online.second);
                    change_clocks_nvml_nvapi(profit_calc.getDci_(), new_opt_config_online.second.mem_oc,
                                             new_opt_config_online.second.nvml_graph_clock_idx);
                }
            }
        }
        //
        if (pm_started)
            stop_power_monitoring_script(device_id);
    }


    static std::list<gpu_group> find_equal_gpus(const std::vector<device_clock_info> &dcis) {
        std::list<gpu_group> res;
        std::set<int> remaining_devices;
        for (auto &dci : dcis)
            remaining_devices.insert(dci.device_id_nvml_);
        while (!remaining_devices.empty()) {
            int i = *remaining_devices.begin();
            std::vector<std::pair<int, bool>> equal_devs({std::make_pair(i, false)});
            for (int j : remaining_devices) {
                if (i == j)
                    continue;
                if (nvmlGetDeviceName(i) == nvmlGetDeviceName(j))
                    equal_devs.emplace_back(std::make_pair(j, false));
            }
            for (auto &to_remove : equal_devs)
                remaining_devices.erase(to_remove.first);
            res.emplace_back(equal_devs);
        }
        return res;
    }


    static std::map<int, std::set<currency_type>>
    find_optimization_gpu_distr(const std::list<gpu_group> &equal_gpus,
                                const std::set<currency_type> &currencies,
                                const std::map<int, std::map<currency_type, energy_hash_info>> &opt_results) {
        std::map<int, std::set<currency_type>> res;
        for (auto &eq_vec : equal_gpus)
            for (auto &device_id_nvml : eq_vec.members_)
                res.emplace(device_id_nvml.first, std::set<currency_type>());

        for (auto &eg : equal_gpus) {
            int next_insert_idx = 0;
            for (currency_type ct : currencies) {
                for (int i = next_insert_idx; i < eg.members_.size(); i++) {
                    int gpu = eg.members_.at(i).first;
                    if (opt_results.count(gpu) && opt_results.at(gpu).count(ct))
                        continue;
                    res.at(gpu).insert(ct);
                    next_insert_idx = (i + 1) % eg.members_.size();
                    break;
                }
            }
        }
        return res;
    };


    static void
    complete_optimization_results(std::map<int, std::map<currency_type, energy_hash_info>> &optimization_results,
                                  const gpu_group &eq_vec) {
        for (int i = 0; i < eq_vec.members_.size(); i++) {
            std::map<currency_type, energy_hash_info> &cur_or = optimization_results.at(eq_vec.members_[i].first);
            for (int j = 0; j < eq_vec.members_.size(); j++) {
                if (i == j)
                    continue;
                const std::map<currency_type, energy_hash_info> &other_or = optimization_results.at(
                        eq_vec.members_[j].first);
                for (auto &elem : other_or)
                    cur_or.emplace(elem.first, elem.second);
            }
        }
    }

    static void gpu_thread_func(const optimization_config &opt_config,
                                const device_clock_info &gpu_dci,
                                const std::map<int, std::set<currency_type>> &gpu_distr,
                                std::map<int, std::map<currency_type, energy_hash_info>> &opt_results,
                                gpu_group &group, std::mutex &all_mutex, std::condition_variable &cond_var,
                                const std::atomic_bool &terminate,
                                std::promise<std::pair<int, std::map<currency_type, energy_hash_info>>> &&p) {
        try {
            if (gpu_distr.at(gpu_dci.device_id_nvml_).empty()) {
                VLOG(0) << "GPU " << gpu_dci.device_id_nvml_ << ": Skipping optimization phase..." << std::endl;
            } else {
                VLOG(0) << "GPU " << gpu_dci.device_id_nvml_ << ": Starting optimization phase..." << std::endl;
                const std::map<currency_type, measurement> &gpu_optimal_config_offline =
                        find_optimal_config(benchmark_info(&run_benchmark_script_nvml_nvapi),
                                            gpu_dci, gpu_distr.at(gpu_dci.device_id_nvml_),
                                            opt_config.opt_method_params_);
                std::set<currency_type> online_opt_currencies;
                for (auto &elem : gpu_optimal_config_offline)
                    online_opt_currencies.emplace(elem.first);
                const std::map<currency_type, measurement> &gpu_optimal_config_online =
                        find_optimal_config(benchmark_info(std::bind(&run_benchmark_mining_online_log,
                                                                     std::cref(opt_config.miner_user_infos_),
                                                                     2 * 60 * 1000,
                                                                     std::placeholders::_1,
                                                                     std::placeholders::_2,
                                                                     std::placeholders::_3,
                                                                     std::placeholders::_4),
                                                           gpu_optimal_config_offline,
                                                           opt_config.miner_user_infos_),
                                            gpu_dci, online_opt_currencies,
                                            opt_config.opt_method_params_);
                std::map<currency_type, energy_hash_info> ehi;
                for (auto &elem : gpu_optimal_config_offline) {
                    ehi.emplace(elem.first, energy_hash_info(elem.first, elem.second,
                                                             (gpu_optimal_config_online.count(elem.first))
                                                             ? gpu_optimal_config_online.at(elem.first)
                                                             : elem.second));
                }
                {
                    std::lock_guard<std::mutex> lock_all(all_mutex);
                    opt_results.emplace(gpu_dci.device_id_nvml_, ehi);
                }
                VLOG(0) << "GPU " << gpu_dci.device_id_nvml_ << ": Finished optimization phase..." << std::endl;
                {
                    std::unique_lock<std::mutex> lock_group(group.mutex_);
                    group.set_member_complete(gpu_dci.device_id_nvml_);
                    if (group.group_complete()) {
                        {
                            //exchange frequency configurations (equal gpus have same optimal frequency configurations)
                            std::lock_guard<std::mutex> lock_all(all_mutex);
                            complete_optimization_results(opt_results, group);
                        }
                        group.cond_var_.notify_all();
                    } else {
                        while (!group.group_complete() && !group.failed_) {
                            group.cond_var_.wait(lock_group);
                        }
                    }
                }
            }
            if (group.failed_) {
                LOG(ERROR) << "GPU " << gpu_dci.device_id_nvml_ << ": Optimization incomplete: Group failure"
                           << std::endl;
                //TODO maybe recover
                //TODO promises?
                return;
            }
        } catch (const std::exception &ex) {
            //Unhandled exception.
            group.failed_ = true;
            group.cond_var_.notify_all();
            LOG(ERROR) << "Exception during optimization phase in thread for GPU " <<
                       gpu_dci.device_id_nvml_ << ": " << ex.what()
                       << std::endl;
            p.set_exception(std::current_exception()); //future.get() triggers the exception
            return;
        }
        //##########################################################################################
        try {
            if (terminate) {
                VLOG(0)
                << "GPU " << gpu_dci.device_id_nvml_ << ": Main set terminate flag. Skipping monitoring phase..."
                << std::endl;
                return;
            }
            VLOG(0) << "GPU " << gpu_dci.device_id_nvml_ << ": Starting mining and monitoring..." << std::endl;
            std::map<currency_type, energy_hash_info> gpu_opt_res;
            {
                std::lock_guard<std::mutex> lock_all(all_mutex);
                gpu_opt_res = opt_results.at(gpu_dci.device_id_nvml_);
            }
            //create profit calculator for gpu
            profit_calculator pc(gpu_dci, gpu_opt_res, opt_config.energy_cost_kwh_);
            start_mining_best_currency(pc, opt_config.miner_user_infos_);
            start_profit_monitoring(pc, opt_config.miner_user_infos_,
                                    opt_config.opt_method_params_,
                                    opt_config.monitoring_interval_sec_ * 1000,
                                    all_mutex, cond_var, terminate);
            stop_mining_script(gpu_dci.device_id_nvml_);
            //set return value
            p.set_value(std::make_pair(gpu_dci.device_id_nvml_, pc.getEnergy_hash_info_()));
        } catch (const std::exception &ex) {
            //Unhandled exception. Dont stop mining!!!
            LOG(ERROR) << "Exception during mining/monitoring phase in thread for GPU " <<
                       gpu_dci.device_id_nvml_ << ": " << ex.what() << std::endl;
            p.set_exception(std::current_exception()); //future.get() triggers the exception
        }
    }

    void mine_most_profitable_currency(const optimization_config &opt_config,
                                       const std::map<int, std::map<currency_type, energy_hash_info>> &opt_results_in) {


        //check for equal gpus and distribute optimization work accordingly
        std::list<gpu_group> &&equal_gpus = find_equal_gpus(opt_config.dcis_);
        std::set<currency_type> currencies;
        for (auto &ui : opt_config.miner_user_infos_.wallet_addresses_)
            currencies.insert(ui.first);
        const std::map<int, std::set<currency_type>> &gpu_distr =
                find_optimization_gpu_distr(equal_gpus, currencies, opt_results_in);

        std::vector<std::thread> threads;
        std::vector<std::future<std::pair<int, std::map<currency_type, energy_hash_info>>>> futures;
        std::mutex mutex;
        std::condition_variable cond_var;
        std::atomic_bool terminate = ATOMIC_VAR_INIT(false);
        std::map<int, std::map<currency_type, energy_hash_info>> opt_results_optphase(opt_results_in.begin(),
                                                                                      opt_results_in.end());
        for (int i = 0; i < opt_config.dcis_.size(); i++) {
            std::promise<std::pair<int, std::map<currency_type, energy_hash_info>>> promise;
            futures.push_back(promise.get_future());
            const device_clock_info &gpu_dci = opt_config.dcis_[i];
            //find group to which gpu belongs
            auto it = equal_gpus.begin();
            for (; it != equal_gpus.end(); ++it) {
                if (it->has_member(gpu_dci.device_id_nvml_))
                    break;
            }
            gpu_group &group = *it;
            //start gpu threads
            threads.emplace_back(&gpu_thread_func, std::cref(opt_config), std::cref(gpu_dci), std::cref(gpu_distr),
                                 std::ref(opt_results_optphase), std::ref(group),
                                 std::ref(mutex), std::ref(cond_var), std::cref(terminate), std::move(promise));
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

        //get updated optimization results
        std::map<int, std::map<currency_type, energy_hash_info>> opt_results_monitoringphase(
                opt_results_optphase.begin(),
                opt_results_optphase.end());
        for (int i = 0; i < futures.size(); i++) {
            try {
                const auto &item = futures[i].get();
                opt_results_monitoringphase.erase(item.first);
                opt_results_monitoringphase.insert(item);
            } catch (const std::exception &ex) {
                const device_clock_info &gpu_dci = opt_config.dcis_[i];
                LOG(ERROR) << "GPU " << gpu_dci.device_id_nvml_ <<
                           ": Failed to get updated optimization result. "
                           "GPU-Thread terminated with unhandled exception: " << ex.what()
                           << std::endl;
                //cleanup
                stop_mining_script(gpu_dci.device_id_nvml_);
                stop_power_monitoring_script(gpu_dci.device_id_nvml_);
            }
        }

        //save opt results dialog
        user_in = cli_get_string("Save optimization results? [y/n]", "[yn]");
        if (user_in == "y") {
            user_in = cli_get_string("Enter filename:", "[\\w-.]+");
            save_optimization_result(user_in, opt_results_monitoringphase);
        }

    }

    void save_optimization_result(const std::string &filename,
                                  const std::map<int, std::map<currency_type, energy_hash_info>> &opt_results) {
        namespace pt = boost::property_tree;
        pt::ptree root;
        //write devices
        for (auto &device : opt_results) {
            pt::ptree pt_currencies;
            //write currency
            for (auto &currency : device.second) {
                pt::ptree pt_config_type;
                std::vector<std::pair<std::string, measurement>> config_type_vec =
                        {std::make_pair("offline", currency.second.optimal_configuration_offline_),
                         std::make_pair("online", currency.second.optimal_configuration_online_),
                         std::make_pair("profit", currency.second.optimal_configuration_profit_)};
                //write all ehi for currency
                for (auto &config_type : config_type_vec) {
                    pt::ptree pt_ehi;
                    pt_ehi.put("power", config_type.second.power_);
                    pt_ehi.put("hashrate", config_type.second.hashrate_);
                    pt_ehi.put("energy_hash", config_type.second.energy_hash_);
                    pt_ehi.put("nvml_graph_clock_idx",
                               config_type.second.nvml_graph_clock_idx);
                    pt_ehi.put("mem_oc", config_type.second.mem_oc);
                    pt_ehi.put("graph_oc", config_type.second.graph_oc);
                    pt_ehi.put("graph_clock", config_type.second.graph_clock_);
                    pt_ehi.put("mem_clock", config_type.second.mem_clock_);
                    pt_ehi.put("power_measure_dur_ms",
                               config_type.second.power_measure_dur_ms_);
                    pt_ehi.put("hashrate_measure_dur_ms",
                               config_type.second.hashrate_measure_dur_ms_);
                    pt_config_type.add_child(config_type.first, pt_ehi);
                }
                pt_currencies.add_child(enum_to_string(currency.first), pt_config_type);
            }
            //
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
            std::map<currency_type, energy_hash_info> opt_res_device;
            //read currencies
            for (const pt::ptree::value_type &array_elem2 : pt_device) {
                currency_type ct = string_to_currency_type(array_elem2.first);
                std::vector<std::pair<std::string, pt::ptree>> pt_config_type_vec =
                        {std::make_pair("offline", array_elem2.second.get_child("offline")),
                         std::make_pair("online", array_elem2.second.get_child("online")),
                         std::make_pair("profit", array_elem2.second.get_child("profit"))};
                std::map<std::string, measurement> opt_config_map;
                for (auto &config_type : pt_config_type_vec) {
                    measurement opt_config;
                    opt_config.mem_clock_ = config_type.second.get<int>("mem_clock");
                    opt_config.graph_clock_ = config_type.second.get<int>("graph_clock");
                    opt_config.mem_oc = config_type.second.get<int>("mem_oc");
                    opt_config.graph_oc = config_type.second.get<int>("graph_oc");
                    opt_config.nvml_graph_clock_idx = config_type.second.get<int>("nvml_graph_clock_idx");
                    opt_config.power_ = config_type.second.get<double>("power");
                    opt_config.hashrate_ = config_type.second.get<double>("hashrate");
                    opt_config.energy_hash_ = config_type.second.get<double>("energy_hash");
                    opt_config.power_measure_dur_ms_ = config_type.second.get<int>("power_measure_dur_ms");
                    opt_config.hashrate_measure_dur_ms_ = config_type.second.get<int>("hashrate_measure_dur_ms");
                    opt_config_map.emplace(config_type.first, opt_config);
                }
                //
                opt_res_device.emplace(ct,
                                       energy_hash_info(ct, opt_config_map.at("offline"),
                                                        opt_config_map.at("online"), opt_config_map.at("profit")));
            }
            //
            opt_results.emplace(std::stoi(array_elem.first), opt_res_device);
        }
        return opt_results;
    }


}