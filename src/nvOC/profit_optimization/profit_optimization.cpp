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
#include "../freq_optimization/freq_nelder_mead.h"
#include "../freq_optimization/freq_hill_climbing.h"
#include "../freq_optimization/freq_simulated_annealing.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/process_management.h"
#include "../common_header/constants.h"
#include "../common_header/exceptions.h"
#include "../script_running/cli_utils.h"
#include "../script_running/log_utils.h"

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
        mutable std::mutex mutex_;
        mutable std::condition_variable cond_var_;
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
                                 const currency_type &ct, const optimization_method_params &opt_params_ct) {
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

            int online_div = (!bi.start_values_.count(ct)) ? 1 : 2;
            const measurement &start_node = (!bi.start_values_.count(ct)) ? bi.bf_(ct, dci, dci.max_mem_oc_, 0) :
                                            bi.bf_(ct, dci, bi.start_values_.at(ct).mem_oc,
                                                   bi.start_values_.at(ct).nvml_graph_clock_idx);
            double min_hashrate = -1.0;
            if (opt_params_ct.min_hashrate_pct > 0)
                min_hashrate = (start_node.nvml_graph_clock_idx != 0 || start_node.mem_oc != dci.max_mem_oc_) ?
                               opt_params_ct.min_hashrate_pct * bi.bf_(ct, dci, dci.max_mem_oc_, 0).hashrate_ :
                               opt_params_ct.min_hashrate_pct * start_node.hashrate_;

            //
            measurement optimal_config;
            switch (opt_params_ct.method_) {
                case optimization_method::NELDER_MEAD:
                    optimal_config = freq_nelder_mead(bi.bf_, ct, dci,
                                                      start_node,
                                                      opt_params_ct.max_iterations_ / online_div,
                                                      opt_params_ct.mem_step_pct_ / online_div,
                                                      opt_params_ct.graph_idx_step_pct_ / online_div,
                                                      min_hashrate);
                    break;
                case optimization_method::HILL_CLIMBING:
                    optimal_config = freq_hill_climbing(bi.bf_, ct, dci,
                                                        start_node,
                                                        opt_params_ct.max_iterations_ / online_div,
                                                        opt_params_ct.mem_step_pct_ / online_div,
                                                        opt_params_ct.graph_idx_step_pct_ / online_div,
                                                        min_hashrate);
                    break;
                case optimization_method::SIMULATED_ANNEALING:
                    optimal_config = freq_simulated_annealing(bi.bf_, ct, dci,
                                                              start_node,
                                                              opt_params_ct.max_iterations_ / online_div,
                                                              opt_params_ct.mem_step_pct_ / online_div,
                                                              opt_params_ct.graph_idx_step_pct_ / online_div,
                                                              min_hashrate);
                    break;
                default:
                    THROW_RUNTIME_ERROR("Invalid enum value");
            }
            //
            VLOG(0) << log_utils::gpu_log_prefix(ct, dci.device_id_nvml_) <<
                    "Computed optimal energy-hash ratio: "
                    << optimal_config.energy_hash_ << " (mem=" << optimal_config.mem_clock_ << ",graph="
                    << optimal_config.graph_clock_ << ")"
                    << std::endl;
            if (mining_started)
                stop_mining_script(dci.device_id_nvml_);
            if (pm_started)
                stop_power_monitoring_script(dci.device_id_nvml_);
            return std::make_pair(true, optimal_config);
        }
        catch (const custom_error &err) {
            LOG(ERROR) << log_utils::gpu_log_prefix(ct, dci.device_id_nvml_) << "Optimization failed: " << err.what()
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
        VLOG(0) << "PM_STARTED: " << pm_started << std::endl;
        //stops mining if its running!!!
        if (!bi.offline_)
            stop_mining_script(dci.device_id_nvml_);
        //
        for (auto &ct : currencies) {
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
        const currency_type &best_currency = profit_calc.getBest_currency_();
        start_mining_script(best_currency, profit_calc.getDci_(), user_infos);
        VLOG(0) << log_utils::gpu_log_prefix(device_id) <<
                "Started mining best currency " << best_currency.currency_name_
                << " with approximated profit of "
                << profit_calc.getBest_currency_profit_() << " euro/h"
                << std::endl;
        //change frequencies at the end (danger to trigger nvml unknown error)
        const energy_hash_info &ehi = profit_calc.getEnergy_hash_info_().at(best_currency);
        change_gpu_clocks(profit_calc.getDci_(), ehi.optimal_configuration_online_.mem_oc,
                          ehi.optimal_configuration_online_.nvml_graph_clock_idx);
    }

    static bool monitoring_sanity_check(const profit_calculator &profit_calc, const miner_user_info &user_infos) {
        int device_id = profit_calc.getDci_().device_id_nvml_;
        bool res = true;
        //check if miner and power_monitor still running and restart if one somehow crashed
        if (!process_management::gpu_has_background_process(device_id, process_type::MINER)) {
            LOG(ERROR) << log_utils::gpu_log_prefix(profit_calc.getBest_currency_(), device_id)
                       << "Monitoring sanity check failed: Miner down" << std::endl;
            start_mining_best_currency(profit_calc, user_infos);
            res = false;
        }
        if (!process_management::gpu_has_background_process(device_id, process_type::POWER_MONITOR)) {
            LOG(ERROR) << log_utils::gpu_log_prefix(profit_calc.getBest_currency_(), device_id)
                       << "Monitoring sanity check failed: Power monitor down" << std::endl;
            start_power_monitoring_script(device_id);
            res = false;
        }
        return res;
    }

    static void
    start_profit_monitoring(profit_calculator &profit_calc,
                            const miner_user_info &user_infos,
                            const std::map<currency_type, optimization_method_params> &opt_method_params,
                            int update_interval_ms, int online_bench_duration_ms,
                            std::mutex &mutex, std::condition_variable &cond_var, const std::atomic<int> &terminate) {
        //start power monitoring and mining of best currency
        int device_id = profit_calc.getDci_().device_id_nvml_;
        bool pm_started = start_power_monitoring_script(device_id);
        start_mining_best_currency(profit_calc, user_infos);
        long long int system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        long long int system_time_start_ms_no_window = system_time_start_ms;
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
            if (!monitoring_sanity_check(profit_calc, user_infos)) {
                system_time_start_ms = system_time_start_ms_no_window = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                continue;
            }
            //currently mined currency
            const currency_type &old_best_currency = profit_calc.getBest_currency_();
            int current_monitoring_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count() - system_time_start_ms;
            if (current_monitoring_time_ms > profit_calc.window_dur_ms_) {
                system_time_start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count() - profit_calc.window_dur_ms_;
                current_monitoring_time_ms = profit_calc.window_dur_ms_;
            }
            VLOG(0)
            << log_utils::gpu_log_prefix(old_best_currency, device_id)
            << "Entering monitor update cycle. Current time window: "
            << current_monitoring_time_ms / (3600 * 1000.0) << "h." << std::endl;
            //update power only if hashrate update was successful -> pool could be down!!!
            if (profit_calc.update_opt_config_profit_hashrate(old_best_currency, user_infos, system_time_start_ms))
                profit_calc.update_power_consumption(old_best_currency, system_time_start_ms);
            //update approximated earnings based on current power,hashrate and stock price
            profit_calc.update_currency_info();
            // recalc and check for new best currency
            profit_calc.recalculate_best_currency();
            const currency_type &new_best_currency = profit_calc.getBest_currency_();
            if (old_best_currency != new_best_currency) {
                //stop mining former best currency
                profit_calc.save_current_period(old_best_currency, system_time_start_ms_no_window);
                stop_mining_script(device_id);
                VLOG(0) << log_utils::gpu_log_prefix(device_id) <<
                        "Stopped mining currency "
                        << old_best_currency.currency_name_ << std::endl;
                //start mining new best currency
                start_mining_best_currency(profit_calc, user_infos);
                VLOG(0) << log_utils::gpu_log_prefix(device_id) << "Reoptimizing currency "
                        << new_best_currency.currency_name_ << std::endl;
                //reset timestamp
                system_time_start_ms = system_time_start_ms_no_window = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                //reoptimize frequencies
                const std::pair<bool, measurement> &new_opt_config_online =
                        find_optimal_config_currency(benchmark_info(std::bind(&run_benchmark_mining_online_log,
                                                                              std::cref(user_infos),
                                                                              online_bench_duration_ms,
                                                                              std::placeholders::_1,
                                                                              std::placeholders::_2,
                                                                              std::placeholders::_3,
                                                                              std::placeholders::_4),
                                                                    profit_calc.get_opt_start_values(),
                                                                    user_infos),
                                                     profit_calc.getDci_(), new_best_currency,
                                                     opt_method_params.at(new_best_currency));
                //update config and change frequencies if optimization was successful
                if (new_opt_config_online.first) {
                    profit_calc.update_opt_config_online(new_best_currency, new_opt_config_online.second);
                    change_gpu_clocks(profit_calc.getDci_(), new_opt_config_online.second.mem_oc,
                                      new_opt_config_online.second.nvml_graph_clock_idx);
                }
            }
        }
        //stop mining and power monitoring
        profit_calc.save_current_period(profit_calc.getBest_currency_(), system_time_start_ms_no_window);
        stop_mining_script(device_id);
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
                //devices considered equal iff their name is equal
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
                                const std::map<int, device_opt_result> &opt_results) {
        std::map<int, std::set<currency_type>> res;
        for (auto &eq_vec : equal_gpus)
            for (auto &device_id_nvml : eq_vec.members_)
                res.emplace(device_id_nvml.first, std::set<currency_type>());

        for (auto &eg : equal_gpus) {
            int next_insert_idx = 0;
            for (auto &ct : currencies) {
                for (int i = next_insert_idx; i < eg.members_.size(); i++) {
                    int gpu = eg.members_.at(i).first;
                    //skip iff entry in opt_results for gpu and currency is available
                    // and gpu was not exchanged with gpu of different type
                    if (opt_results.count(gpu) && opt_results.at(gpu).device_name_ == nvmlGetDeviceName(gpu) &&
                        opt_results.at(gpu).currency_ehi_.count(ct))
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
    complete_optimization_results(std::map<int, device_opt_result> &optimization_results,
                                  const gpu_group &eq_vec) {
        for (int i = 0; i < eq_vec.members_.size(); i++) {
            std::map<currency_type, energy_hash_info> &cur_or = optimization_results.at(
                    eq_vec.members_[i].first).currency_ehi_;
            for (int j = 0; j < eq_vec.members_.size(); j++) {
                if (i == j)
                    continue;
                const std::map<currency_type, energy_hash_info> &other_or = optimization_results.at(
                        eq_vec.members_[j].first).currency_ehi_;
                for (auto &elem : other_or)
                    cur_or.emplace(elem.first, elem.second);
            }
        }
    }

    static std::map<int, device_opt_result> clean_opt_result(const std::map<int, device_opt_result> &opt_results_in,
                                                             const optimization_config &opt_config,
                                                             const std::set<currency_type> &currencies) {
        std::map<int, device_opt_result> opt_results_out(opt_results_in.begin(),
                                                         opt_results_in.end());
        //check for new gpus of available type or swap of gpus of different type + remove invalid gpus
        for (auto &dci : opt_config.dcis_) {
            if (opt_results_in.count(dci.device_id_nvml_) &&
                dci.device_name_ == opt_results_in.at(dci.device_id_nvml_).device_name_)
                continue;
            bool repaired = false;
            for (auto &elem : opt_results_in) {
                if (dci.device_name_ == elem.second.device_name_) {
                    opt_results_out.at(dci.device_id_nvml_) = elem.second;
                    repaired = true;
                    break;
                }
            }
            if (!repaired)
                opt_results_out.erase(dci.device_id_nvml_);
        }
        //remove gpus/currencies that are not in opt_config from available opt_results
        for (auto it = opt_results_out.begin(); it != opt_results_out.end();) {
            int dci_idx = -1;
            for (int i = 0; i < opt_config.dcis_.size(); i++)
                if (opt_config.dcis_[i].device_id_nvml_ == it->first) {
                    dci_idx = i;
                    break;
                }
            if (dci_idx < 0) {
                it = opt_results_out.erase(it);
                continue;
            }
            //
            const device_clock_info &dci_to_check = opt_config.dcis_[dci_idx];
            for (auto it_inner = it->second.currency_ehi_.begin(); it_inner != it->second.currency_ehi_.end();) {
                if (!currencies.count(it_inner->first)) {
                    it_inner = it->second.currency_ehi_.erase(it_inner);
                    continue;
                }
                bool valid_freqs = true;
                std::vector<std::reference_wrapper<measurement>> temp;
                temp.emplace_back(it_inner->second.optimal_configuration_offline_);
                temp.emplace_back(it_inner->second.optimal_configuration_online_);
                temp.emplace_back(it_inner->second.optimal_configuration_profit_);
                for (measurement &m : temp) {
                    if (m.graph_clock_ < dci_to_check.nvml_graph_clocks_.back() ||
                        m.graph_clock_ > dci_to_check.nvml_graph_clocks_.front() ||
                        m.mem_oc < dci_to_check.min_mem_oc_ || m.mem_oc > dci_to_check.max_mem_oc_) {
                        valid_freqs = false;
                        break;
                    }
                    if (m.nvml_graph_clock_idx >= 0 &&
                        m.nvml_graph_clock_idx < dci_to_check.nvml_graph_clocks_.size() &&
                        m.graph_clock_ == dci_to_check.nvml_graph_clocks_[m.nvml_graph_clock_idx])
                        continue;
                    //fix index
                    for (int i = 0; i < dci_to_check.nvml_graph_clocks_.size(); i++) {
                        int cur_val = dci_to_check.nvml_graph_clocks_[i];
                        if (cur_val <= m.graph_clock_) {
                            m.nvml_graph_clock_idx = i;
                            m.graph_clock_ = cur_val;
                            m.graph_oc = m.graph_clock_ - dci_to_check.nvapi_default_graph_clock_;
                            break;
                        }
                    }
                }
                if (!valid_freqs)
                    it_inner = it->second.currency_ehi_.erase(it_inner);
                else
                    ++it_inner;
            }
            //
            if (it->second.currency_ehi_.empty())
                it = opt_results_out.erase(it);
            else
                ++it;
        }
        return opt_results_out;
    }

    static void gpu_thread_func(const optimization_config &opt_config,
                                const device_clock_info &gpu_dci,
                                const std::set<currency_type> &gpu_optimization_work,
                                std::map<int, device_opt_result> &opt_results,
                                gpu_group &group, std::mutex &all_mutex, std::condition_variable &cond_var,
                                const std::atomic<int> &terminate,
                                std::promise<std::pair<int, device_opt_result>> &&p) {
        try {
            if (gpu_optimization_work.empty()) {
                VLOG(0)
                << log_utils::gpu_log_prefix(gpu_dci.device_id_nvml_) << "Skipping optimization phase..." << std::endl;
            } else {
                VLOG(0)
                << log_utils::gpu_log_prefix(gpu_dci.device_id_nvml_) << "Starting optimization phase..." << std::endl;
                //offline optimization
                std::map<currency_type, measurement> gpu_optimal_config_offline;
                if (!opt_config.skip_offline_phase_) {
                    gpu_optimal_config_offline =
                            find_optimal_config(benchmark_info(&run_benchmark_mining_offline),
                                                gpu_dci, gpu_optimization_work,
                                                opt_config.opt_method_params_);
                }
                //online optimization
                const std::map<currency_type, measurement> &gpu_optimal_config_online =
                        find_optimal_config(benchmark_info(std::bind(&run_benchmark_mining_online_log,
                                                                     std::cref(opt_config.miner_user_infos_),
                                                                     opt_config.online_bench_duration_sec_ * 1000,
                                                                     std::placeholders::_1,
                                                                     std::placeholders::_2,
                                                                     std::placeholders::_3,
                                                                     std::placeholders::_4),
                                                           gpu_optimal_config_offline,
                                                           opt_config.miner_user_infos_),
                                            gpu_dci, gpu_optimization_work,
                                            opt_config.opt_method_params_);
                //save results
                std::map<currency_type, energy_hash_info> ehi;
                for (auto &elem : gpu_optimal_config_online) {
                    ehi.emplace(elem.first, energy_hash_info(elem.first, (gpu_optimal_config_offline.count(elem.first))
                                                                         ? gpu_optimal_config_offline.at(elem.first)
                                                                         : elem.second, elem.second));
                }
                {
                    std::lock_guard<std::mutex> lock_all(all_mutex);
                    auto it_pair = opt_results.emplace(gpu_dci.device_id_nvml_,
                                                       device_opt_result(gpu_dci.device_id_nvml_,
                                                                         nvmlGetDeviceName(gpu_dci.device_id_nvml_),
                                                                         ehi));
                    if (!it_pair.second) {
                        std::map<currency_type, energy_hash_info> &existing_ehi = it_pair.first->second.currency_ehi_;
                        existing_ehi.insert(ehi.begin(), ehi.end());
                    }
                }
                VLOG(0)
                << log_utils::gpu_log_prefix(gpu_dci.device_id_nvml_) << "Finished optimization phase..." << std::endl;
                {
                    std::unique_lock<std::mutex> lock_group(group.mutex_);
                    group.set_member_complete(gpu_dci.device_id_nvml_);
                    if (group.group_complete()) {
                        {
                            //exchange frequency configurations (equal gpus have same optimal frequency configurations)
                            std::lock_guard<std::mutex> lock_all(all_mutex);
                            complete_optimization_results(opt_results, group);
                            //save intermediate result to logdir
                            save_optimization_result(log_utils::get_autosave_opt_result_optphase_filename(),
                                                     opt_results);
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
                LOG(ERROR) << log_utils::gpu_log_prefix(gpu_dci.device_id_nvml_)
                           << "Optimization incomplete: Group failure"
                           << std::endl;
                //TODO maybe recover
                //TODO promises?
                return;
            }
        }
        catch (const std::exception &ex) {
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
                << log_utils::gpu_log_prefix(gpu_dci.device_id_nvml_)
                << "Main set terminate flag. Skipping monitoring phase..."
                << std::endl;
                return;
            }
            VLOG(0)
            << log_utils::gpu_log_prefix(gpu_dci.device_id_nvml_) << "Starting mining and monitoring..." << std::endl;
            std::map<currency_type, energy_hash_info> gpu_opt_res;
            {
                std::lock_guard<std::mutex> lock_all(all_mutex);
                gpu_opt_res = opt_results.at(gpu_dci.device_id_nvml_).currency_ehi_;
            }
            //create profit calculator for gpu and start monitoring
            profit_calculator pc(gpu_dci, gpu_opt_res, opt_config.energy_cost_kwh_);
            start_profit_monitoring(pc, opt_config.miner_user_infos_,
                                    opt_config.opt_method_params_,
                                    opt_config.monitoring_interval_sec_ * 1000,
                                    opt_config.online_bench_duration_sec_ * 1000,
                                    all_mutex, cond_var, terminate);
            //set return value
            p.set_value(std::make_pair(gpu_dci.device_id_nvml_,
                                       device_opt_result(gpu_dci.device_id_nvml_,
                                                         nvmlGetDeviceName(gpu_dci.device_id_nvml_),
                                                         pc.getEnergy_hash_info_())));
        }
        catch (const std::exception &ex) {
            //Unhandled exception. Dont stop mining!!!
            LOG(ERROR) << "Exception during mining/monitoring phase in thread for GPU " <<
                       gpu_dci.device_id_nvml_ << ": " << ex.what() << std::endl;
            p.set_exception(std::current_exception()); //future.get() triggers the exception
        }
    }

    void mine_most_profitable_currency(const optimization_config &opt_config,
                                       const std::map<int, device_opt_result> &opt_results_in) {
        //check for equal gpus and extract currencies according opt_config
        std::list<gpu_group> &&equal_gpus = find_equal_gpus(opt_config.dcis_);
        std::set<currency_type> currencies;
        for (auto &ui : opt_config.miner_user_infos_.wallet_addresses_)
            currencies.insert(ui.first);
        //clean and complete opt_result
        std::map<int, device_opt_result> &&opt_results_optphase = clean_opt_result(opt_results_in, opt_config,
                                                                                   currencies);
        //distribute optimization work exploiting equal gpus and already available opt_results
        const std::map<int, std::set<currency_type>> &gpu_distr =
                find_optimization_gpu_distr(equal_gpus, currencies, opt_results_optphase);
        //start optimization and monitoring on each gpu
        std::vector<std::thread> threads;
        std::vector<std::future<std::pair<int, device_opt_result>>> futures;
        std::mutex mutex;
        extern std::condition_variable glob_cond_var;
        extern std::atomic<int> glob_terminate;
        std::condition_variable &cond_var = glob_cond_var;
        std::atomic<int> &terminate = glob_terminate;
        for (const device_clock_info &gpu_dci : opt_config.dcis_) {
            std::promise<std::pair<int, device_opt_result>> promise;
            futures.push_back(promise.get_future());
            //find group to which gpu belongs
            auto it = equal_gpus.begin();
            for (; it != equal_gpus.end(); ++it) {
                if (it->has_member(gpu_dci.device_id_nvml_))
                    break;
            }
            gpu_group &group = *it;
            //start gpu thread
            threads.emplace_back(&gpu_thread_func, std::cref(opt_config), std::cref(gpu_dci),
                                 std::cref(gpu_distr.at(gpu_dci.device_id_nvml_)),
                                 std::ref(opt_results_optphase), std::ref(group),
                                 std::ref(mutex), std::ref(cond_var), std::cref(terminate), std::move(promise));
        }

        //wait for user to terminate
        std::string user_in = cli_get_string("Performing mining and monitoring...\nEnter q to stop.", "q");
        if (user_in == "q") {
            terminate = 1;
            cond_var.notify_all();
        }
        //join threads
        for (auto &t : threads)
            t.join();

        //get updated optimization results
        std::map<int, device_opt_result> opt_results_monitoringphase(
                opt_results_optphase.begin(),
                opt_results_optphase.end());
        for (int i = 0; i < futures.size(); i++) {
            try {
                const auto &item = futures[i].get();
                opt_results_monitoringphase.erase(item.first);
                opt_results_monitoringphase.insert(item);
            }
            catch (const std::exception &ex) {
                const device_clock_info &gpu_dci = opt_config.dcis_[i];
                LOG(ERROR) << log_utils::gpu_log_prefix(gpu_dci.device_id_nvml_) <<
                           "Failed to get updated optimization result. "
                           "GPU-Thread terminated with unhandled exception: " << ex.what()
                           << std::endl;
                //cleanup
                stop_mining_script(gpu_dci.device_id_nvml_);
                stop_power_monitoring_script(gpu_dci.device_id_nvml_);
            }
        }

        //always save to logdir
        save_optimization_result(log_utils::get_autosave_opt_result_monitorphase_filename(),
                                 opt_results_monitoringphase);
        //save opt results dialog if killed by stdin
        if(terminate > 0) {
            user_in = cli_get_string("Save optimization results? [y/n]", "[yn]");
            if (user_in == "y") {
                user_in = cli_get_string("Enter filename:", "[\\w-.]+");
                save_optimization_result(user_in, opt_results_monitoringphase);
            }
        }
    }

    void save_optimization_result(const std::string &filename,
                                  const std::map<int, device_opt_result> &opt_results) {
        namespace pt = boost::property_tree;
        pt::ptree root;
        //write devices
        for (auto &device : opt_results) {
            pt::ptree pt_device;
            pt_device.put("device_name", nvmlGetDeviceName(device.first));
            pt::ptree pt_currencies;
            //write currency
            for (auto &currency : device.second.currency_ehi_) {
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
                pt_currencies.add_child(currency.first.currency_name_, pt_config_type);
            }
            //
            pt_device.add_child("currency_opt_result", pt_currencies);
            root.add_child(std::to_string(device.first), pt_device);
        }
        pt::write_json(filename, root);
    }


    std::map<int, device_opt_result> load_optimization_result(const std::string &filename,
                                                              const std::map<std::string, currency_type> &available_currencies) {
        std::map<int, device_opt_result> opt_results;
        namespace pt = boost::property_tree;
        pt::ptree root;
        pt::read_json(filename, root);
        //read device infos
        for (const pt::ptree::value_type &array_elem : root) {
            const boost::property_tree::ptree &pt_device = array_elem.second;
            int device_id = std::stoi(array_elem.first);
            const std::string &device_name = pt_device.get<std::string>("device_name");
            std::map<currency_type, energy_hash_info> opt_res_device;
            //read currencies
            for (const pt::ptree::value_type &array_elem2 : pt_device.get_child("currency_opt_result")) {
                const currency_type &ct = available_currencies.at(array_elem2.first);
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
            opt_results.emplace(device_id, device_opt_result(device_id,
                                                             device_name, opt_res_device));
        }
        return opt_results;
    }

}
