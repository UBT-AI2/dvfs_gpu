#include "profit_calculation.h"
#include <cmath>
#include <limits>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <glog/logging.h>
#include "../common_header/exceptions.h"
#include "../freq_core/network_requests.h"
#include "../freq_core/log_utils.h"

namespace frequency_scaling {

    currency_info::currency_info(const currency_type &type, double approximated_earnings_eur_hour,
                                 double used_hashrate, const currency_stats &cs)
            : type_(type), approximated_earnings_eur_hour_(approximated_earnings_eur_hour),
              used_hashrate_(used_hashrate), cs_(cs) {}

    energy_hash_info::energy_hash_info(const currency_type &type,
                                       const measurement &optimal_configuration_offline,
                                       const measurement &optimal_configuration_online) :
            energy_hash_info(type, optimal_configuration_offline, optimal_configuration_online,
                             optimal_configuration_online) {
        optimal_configuration_profit_.hashrate_measure_dur_ms_ = 0;
        optimal_configuration_profit_.power_measure_dur_ms_ = 0;
    }

    energy_hash_info::energy_hash_info(const currency_type &type,
                                       const measurement &optimal_configuration_offline,
                                       const measurement &optimal_configuration_online,
                                       const measurement &optimal_configuration_profit) : type_(
            type), optimal_configuration_offline_(optimal_configuration_offline), optimal_configuration_online_(
            optimal_configuration_online), optimal_configuration_profit_(
            optimal_configuration_profit) {}


    best_profit_stats::device_stats::device_stats(const currency_type &ct, int ct_mem_clock, int ct_graph_clock,
                                                  long long int system_time_ms, double stock_price_eur, double earnings,
                                                  double costs,
                                                  double power, double hashrate) : ct_(ct), ct_mem_clock_(ct_mem_clock),
                                                                                   ct_graph_clock_(ct_graph_clock),
                                                                                   system_time_ms_(system_time_ms),
                                                                                   stock_price_eur_(stock_price_eur),
                                                                                   earnings_(
                                                                                           earnings),
                                                                                   costs_(costs),
                                                                                   profit_(earnings -
                                                                                           costs),
                                                                                   power_(power),
                                                                                   hashrate_(
                                                                                           hashrate),
                                                                                   energy_hash_(
                                                                                           hashrate /
                                                                                           power) {}

    bool best_profit_stats::device_stats::operator<(const best_profit_stats::device_stats &other) const {
        return profit_ < other.profit_;
    }

    void
    best_profit_stats::update_device_stats(int device_id,
                                           const std::multiset<best_profit_stats::device_stats> &device_currency_stats) {
        //TODO handle empty device_stats (blacklist, what to mine then)
        if (device_currency_stats.empty())
            return;
        {
            std::lock_guard<std::mutex> lock(map_mutex_);
            if (!stats_map_.count(device_id))
                stats_map_.emplace(device_id, std::vector<std::multiset<best_profit_stats::device_stats>>());
            stats_map_.at(device_id).emplace_back(device_currency_stats);
        }
        const best_profit_stats::device_stats device_best_currency_stats = *device_currency_stats.rbegin();
        long long int log_timestamp = device_currency_stats.rbegin()->system_time_ms_;
        std::lock_guard<std::mutex> lock(logfile_mutex_);
        //print and log device local profit stats for each currency sorted by profit descending
        for (auto it = device_currency_stats.rbegin(); it != device_currency_stats.rend(); ++it) {
            VLOG(1) << log_utils::gpu_log_prefix(it->ct_, device_id) << "Profit calculation using: hashrate=" <<
                    it->hashrate_ << ", power=" << it->power_ << ", energy_hash=" << it->energy_hash_ <<
                    ", stock_price=" << it->stock_price_eur_ << std::endl;
            VLOG(1)
            << log_utils::gpu_log_prefix(it->ct_, device_id) << "Calculated profit [eur/hour]: approximated earnings="
            <<
            it->earnings_ << ", energy_cost=" << it->costs_ << ", profit=" << it->profit_ << std::endl;
            //
            bool write_csv_header = !log_utils::check_file_existance(
                    log_utils::get_profit_stats_filename(it->ct_, device_id));
            std::ofstream logfile(log_utils::get_profit_stats_filename(it->ct_, device_id), std::ofstream::app);
            if (!logfile)
                THROW_IO_ERROR("Cannot open " + log_utils::get_profit_stats_filename(it->ct_, device_id));
            if (write_csv_header)
                logfile
                        << "#timestamp,hashrate,power,energy_hash,stock_price,earnings,costs,profit,mem_clock,graph_clock"
                        << std::endl;
            logfile << log_timestamp << "," << it->hashrate_ << "," << it->power_ << "," << it->energy_hash_ <<
                    "," << it->stock_price_eur_ << "," << it->earnings_ << "," << it->costs_ << "," << it->profit_
                    << "," << it->ct_mem_clock_ << "," << it->ct_graph_clock_ << std::endl;
        }
        //print and log best device local profit stats
        {
            VLOG(0)
            << log_utils::gpu_log_prefix(device_id) << "Local profit stats [eur/hour]: currency="
            << device_best_currency_stats.ct_.currency_name_ << ", approximated earnings=" <<
            device_best_currency_stats.earnings_ << ", energy_cost=" << device_best_currency_stats.costs_ <<
            " (" << device_best_currency_stats.power_ << "W), profit=" << device_best_currency_stats.profit_
            << std::endl;
            //
            bool write_csv_header = !log_utils::check_file_existance(log_utils::get_profit_stats_filename(device_id));
            std::ofstream logfile(log_utils::get_profit_stats_filename(device_id), std::ofstream::app);
            if (!logfile)
                THROW_IO_ERROR("Cannot open " + log_utils::get_profit_stats_filename(device_id));
            if (write_csv_header)
                logfile
                        << "#timestamp,currency_name,hashrate,power,energy_hash,stock_price,earnings,costs,profit,mem_clock,graph_clock"
                        << std::endl;
            logfile << log_timestamp << "," << device_best_currency_stats.ct_.currency_name_ << ","
                    << device_best_currency_stats.hashrate_
                    << "," << device_best_currency_stats.power_ << "," << device_best_currency_stats.energy_hash_
                    << "," << device_best_currency_stats.stock_price_eur_ << "," <<
                    device_best_currency_stats.earnings_ << "," << device_best_currency_stats.costs_ << ","
                    << device_best_currency_stats.profit_
                    << "," << device_best_currency_stats.ct_mem_clock_ << ","
                    << device_best_currency_stats.ct_graph_clock_ << std::endl;
        }
        //print and log best global profit stats
        {
            VLOG(0) << "Global profit stats [eur/hour]: approximated earnings=" <<
                    get_global_earnings() << ", energy_cost=" << get_global_costs() <<
                    " (" << get_global_power() << "W), profit=" << get_global_profit() << std::endl;
            //
            bool write_csv_header = !log_utils::check_file_existance(log_utils::get_profit_stats_filename());
            std::ofstream logfile(log_utils::get_profit_stats_filename(), std::ofstream::app);
            if (!logfile)
                THROW_IO_ERROR("Cannot open " + log_utils::get_profit_stats_filename());
            if (write_csv_header)
                logfile << "#timestamp,earnings,costs,power,profit" << std::endl;
            logfile << log_timestamp << "," << get_global_earnings() << "," << get_global_costs() <<
                    "," << get_global_power() << "," << get_global_profit() << std::endl;
        }
    }

    double best_profit_stats::get_global_earnings() const {
        std::lock_guard<std::mutex> lock(map_mutex_);
        double sum = 0;
        for (auto &elem : stats_map_)
            sum += elem.second.back().rbegin()->earnings_;
        return sum;
    }


    double best_profit_stats::get_global_costs() const {
        std::lock_guard<std::mutex> lock(map_mutex_);
        double sum = 0;
        for (auto &elem : stats_map_)
            sum += elem.second.back().rbegin()->costs_;
        return sum;
    }


    double best_profit_stats::get_global_profit() const {
        std::lock_guard<std::mutex> lock(map_mutex_);
        double sum = 0;
        for (auto &elem : stats_map_)
            sum += elem.second.back().rbegin()->profit_;
        return sum;
    }


    double best_profit_stats::get_global_power() const {
        std::lock_guard<std::mutex> lock(map_mutex_);
        double sum = 0;
        for (auto &elem : stats_map_)
            sum += elem.second.back().rbegin()->power_;
        return sum;
    }


    const best_profit_stats::device_stats &best_profit_stats::get_device_stats(int device_id) const {
        std::lock_guard<std::mutex> lock(map_mutex_);
        if (!stats_map_.count(device_id))
            THROW_RUNTIME_ERROR("No profit stats available for GPU " + std::to_string(device_id));
        return *stats_map_.at(device_id).back().rbegin();
    }


    best_profit_stats profit_calculator::best_profit_stats_global_;

    profit_calculator::profit_calculator(const device_clock_info &dci,
                                         const std::map<currency_type, energy_hash_info> &energy_hash_info,
                                         double power_cost_kwh) : dci_(dci),
                                                                  energy_hash_info_(energy_hash_info),
                                                                  power_cost_kwh_(power_cost_kwh) {
        for (auto &ehi : energy_hash_info) {
            //init last profit measurement with saved profit config
            last_profit_measurements_.emplace(ehi.first, energy_hash_info_.at(ehi.first).optimal_configuration_profit_);
            currency_mining_timespans_.emplace(ehi.first, std::vector<std::pair<long long int, int>>());
            timespan_current_pool_hashrates_.emplace(ehi.first, std::vector<std::pair<long long int, double>>());
        }
        //calculate initial profit stats
        update_currency_info();
        recalculate_best_currency();
    }


    void profit_calculator::recalculate_best_currency() {
        std::multiset<best_profit_stats::device_stats> device_stats;
        long long int log_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        for (auto &elem : energy_hash_info_) {
            const currency_type &ct = elem.first;
            if (currency_blacklist_.count(ct)) {
                LOG(WARNING) << log_utils::gpu_log_prefix(ct, dci_.device_id_nvml_) <<
                             "Skipping profit calculation: Currency on blacklist (probably pool or miner down)"
                             << std::endl;
                continue;
            }
            auto it_ci = currency_info_.find(ct);
            if (it_ci == currency_info_.end()) {
                LOG(WARNING) << log_utils::gpu_log_prefix(ct, dci_.device_id_nvml_) <<
                             "Skipping profit calculation: No currency info available" << std::endl;
                continue;
            }
            const currency_info &ci = it_ci->second;
            double costs_per_hour = get_used_power(ct) * (power_cost_kwh_ / 1000.0);
            int ct_mem_clock = energy_hash_info_.at(ct).optimal_configuration_profit_.mem_clock_;
            int ct_graph_clock = energy_hash_info_.at(ct).optimal_configuration_profit_.graph_clock_;
            //store device stats for currency
            device_stats.emplace(ct, ct_mem_clock, ct_graph_clock, log_timestamp, ci.cs_.stock_price_eur_,
                                 ci.approximated_earnings_eur_hour_, costs_per_hour, get_used_power(ct),
                                 get_used_hashrate(ct));
        }
        //update profit stats for this device
        profit_calculator::best_profit_stats_global_.update_device_stats(dci_.device_id_nvml_, device_stats);
    }

    void profit_calculator::update_currency_info() {
        for (auto &elem : energy_hash_info_) {
            const currency_type &ct = elem.first;
            try {
                double used_hashrate = calculate_used_hashrate(ct);
                const currency_stats &cs = network_requests::get_currency_stats(ct);
                double approximated_earnings;
                if (!ct.has_approximated_earnings_api()) {
                    approximated_earnings = cs.calc_approximated_earnings_eur_hour(used_hashrate);
                } else {
                    try {
                        approximated_earnings = network_requests::get_approximated_earnings_per_hour(ct, used_hashrate);
                    } catch (const network_error &err) {
                        //fallback
                        LOG(WARNING) << "Approximated earnings API call failed: " << err.what() <<
                                     ". Using fallback..." << std::endl;
                        approximated_earnings = cs.calc_approximated_earnings_eur_hour(used_hashrate);
                    }
                }
                currency_info_.erase(ct);
                currency_info_.emplace(ct, currency_info(ct, approximated_earnings, used_hashrate, cs));
                VLOG(1) << log_utils::gpu_log_prefix(ct, dci_.device_id_nvml_) << "Update currency info using hashrate "
                        << used_hashrate << std::endl;
            } catch (const network_error &err) {
                LOG(ERROR) << log_utils::gpu_log_prefix(ct, dci_.device_id_nvml_) <<
                           "Failed to update currency info: " << err.what() << std::endl;
            }
        }
    }

    bool profit_calculator::update_opt_config_profit_hashrate(const currency_type &current_mined_ct,
                                                              const miner_user_info &user_info,
                                                              long long int system_time_start_ms) {
        long long int system_time_now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        int period_ms = system_time_now_ms - system_time_start_ms;
        double cur_hashrate = 0;
        if (current_mined_ct.has_avg_hashrate_api()) {
            cur_hashrate = __get_avg_pool_hashrate(current_mined_ct, user_info, period_ms);
        }
        if (cur_hashrate <= 0 && current_mined_ct.has_current_hashrate_api()) {
            cur_hashrate = __get_current_pool_hashrate(current_mined_ct, user_info, period_ms);
            if (cur_hashrate > 0) {
                timespan_current_pool_hashrates_.at(current_mined_ct).emplace_back(system_time_now_ms, cur_hashrate);
                int counter = 0;
                cur_hashrate = 0;
                for (auto &elem : timespan_current_pool_hashrates_.at(current_mined_ct))
                    if (elem.first >= system_time_start_ms) {
                        cur_hashrate += elem.second;
                        counter++;
                    }
                cur_hashrate = (counter == 0) ? 0 : cur_hashrate / counter;
            }
        }
        if (cur_hashrate <= 0) {
            cur_hashrate = __get_avg_hashrate_online_log(current_mined_ct, dci_.device_id_nvml_,
                                                         system_time_start_ms, system_time_now_ms);
        }
        //
        if (cur_hashrate <= 0) {//non-critical eg not mined long enough
            if (cur_hashrate < 0) //critical
                currency_blacklist_.emplace(current_mined_ct);
            return false;
        } else {
            currency_blacklist_.erase(current_mined_ct);
        }
        //update hashrate
        double last_hashrate = last_profit_measurements_.at(current_mined_ct).hashrate_;
        int cur_period_ms = period_ms;
        int last_period_ms = last_profit_measurements_.at(current_mined_ct).hashrate_measure_dur_ms_;
        int total_period_ms = last_period_ms + cur_period_ms;
        double new_hashrate = (cur_period_ms / (double) total_period_ms) * cur_hashrate +
                              (last_period_ms / (double) total_period_ms) * last_hashrate;
        energy_hash_info_.at(current_mined_ct).optimal_configuration_profit_.update_hashrate(new_hashrate,
                                                                                             total_period_ms);
        VLOG(1) << log_utils::gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                "Updated avg profit hashrate: " << new_hashrate << std::endl;
        return true;
    }

    bool profit_calculator::update_power_consumption(const currency_type &current_mined_ct,
                                                     long long int system_time_start_ms) {
        long long int system_time_now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        //update power
        double cur_power = get_avg_power_usage(dci_.device_id_nvml_, system_time_start_ms, system_time_now_ms);
        if (cur_power <= 0 || !std::isfinite(cur_power)) {
            LOG(ERROR) << log_utils::gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                       "Failed to get avg profit power" << std::endl;
            return false;
        }
        double last_power = last_profit_measurements_.at(current_mined_ct).power_;
        int cur_period_ms = system_time_now_ms - system_time_start_ms;
        int last_period_ms = last_profit_measurements_.at(current_mined_ct).power_measure_dur_ms_;
        int total_period_ms = last_period_ms + cur_period_ms;
        double new_power = (cur_period_ms / (double) total_period_ms) * cur_power +
                           (last_period_ms / (double) total_period_ms) * last_power;
        energy_hash_info_.at(current_mined_ct).optimal_configuration_profit_.update_power(new_power, total_period_ms);
        VLOG(1) << log_utils::gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                "Update profit power: " << new_power << std::endl;
        return true;
    }

    void profit_calculator::update_opt_config_online(const currency_type &current_mined_ct,
                                                     const measurement &new_config_online) {

        energy_hash_info &ehi = energy_hash_info_.at(current_mined_ct);
        ehi.optimal_configuration_online_ = new_config_online;
        ehi.optimal_configuration_profit_.update_freq_config(new_config_online);
        //update profit hashrate and power also if no profit measurement available yet
        if (ehi.optimal_configuration_profit_.hashrate_measure_dur_ms_ <= 0)
            ehi.optimal_configuration_profit_.update_hashrate(new_config_online.hashrate_, 0);
        if (ehi.optimal_configuration_profit_.power_measure_dur_ms_ <= 0)
            ehi.optimal_configuration_profit_.update_power(new_config_online.power_, 0);
        VLOG(1) << log_utils::gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                "Update opt_config_online" << std::endl;
    }


    void profit_calculator::update_energy_cost_stromdao(int plz) {
        power_cost_kwh_ = network_requests::get_energy_cost_stromdao(plz);
    }

    const std::map<currency_type, currency_info> &profit_calculator::getCurrency_info_() const {
        return currency_info_;
    }

    const std::map<currency_type, energy_hash_info> &profit_calculator::getEnergy_hash_info_() const {
        return energy_hash_info_;
    }

    std::map<currency_type, measurement> profit_calculator::get_opt_start_values() const {
        std::map<currency_type, measurement> res;
        for (auto &ehi : energy_hash_info_)
            res.emplace(ehi.first, ehi.second.optimal_configuration_online_);
        return res;
    };

    std::set<currency_type> profit_calculator::get_used_currencies() const {
        std::set<currency_type> res;
        for (auto &ehi : energy_hash_info_)
            res.insert(ehi.first);
        return res;
    }

    double profit_calculator::getPower_cost_kwh_() const {
        return power_cost_kwh_;
    }

    const device_clock_info &profit_calculator::getDci_() const {
        return dci_;
    }

    currency_type profit_calculator::getBest_currency_() const {
        return profit_calculator::best_profit_stats_global_.get_device_stats(dci_.device_id_nvml_).ct_;
    }

    double profit_calculator::getBest_currency_profit_() const {
        return profit_calculator::best_profit_stats_global_.get_device_stats(dci_.device_id_nvml_).profit_;
    }


    double profit_calculator::calculate_used_hashrate(const currency_type &ct) const {
        const measurement &cur_profit_measurement = energy_hash_info_.at(ct).optimal_configuration_profit_;
        const measurement &cur_online_measurement = energy_hash_info_.at(ct).optimal_configuration_online_;
        //currently mined currency?
        if (currency_mining_timespans_.at(ct).empty() || ct == getBest_currency_()) {
            double alpha = std::min(1.0, cur_profit_measurement.hashrate_measure_dur_ms_ / (double) window_dur_ms_);
            return alpha * cur_profit_measurement.hashrate_ + (1 - alpha) * cur_online_measurement.hashrate_;
        } else {
            long long int system_time_now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            long long int time_since_last_mined = system_time_now_ms -
                                                  (currency_mining_timespans_.at(ct).back().first +
                                                   currency_mining_timespans_.at(ct).back().second);

            double alpha = std::min(1.0, time_since_last_mined / (double) window_dur_ms_);
            return alpha * cur_online_measurement.hashrate_ + (1 - alpha) * cur_profit_measurement.hashrate_;
        }
    }

    double profit_calculator::get_used_hashrate(const currency_type &ct) const {
        if (!currency_info_.count(ct))
            return -1;
        return currency_info_.at(ct).used_hashrate_;
    }

    double profit_calculator::get_used_power(const currency_type &ct) const {
        return energy_hash_info_.at(ct).optimal_configuration_profit_.power_;
    }

    double profit_calculator::get_used_energy_hash(const currency_type &ct) const {
        return get_used_hashrate(ct) / get_used_power(ct);
    }

    void profit_calculator::save_current_period(const currency_type &ct, long long int system_time_start_ms_no_window) {
        last_profit_measurements_.at(ct) = energy_hash_info_.at(ct).optimal_configuration_profit_;
        long long int system_time_now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        int period_ms = system_time_now_ms - system_time_start_ms_no_window;
        currency_mining_timespans_.at(ct).emplace_back(system_time_start_ms_no_window, period_ms);
        //mining duration stats
        for (auto &elem : currency_mining_timespans_) {
            int log_level = (elem.first == ct) ? 0 : 1;
            int ct_period_dur = (!elem.second.empty()) ? elem.second.back().second : 0;
            int ct_total_dur = 0;
            for (auto &dur : elem.second)
                ct_total_dur += dur.second;
            VLOG(log_level)
            << log_utils::gpu_log_prefix(elem.first, dci_.device_id_nvml_) << "Currency mining duration: last_period="
            <<
            ct_period_dur / (3600 * 1000.0) << "h, total=" << ct_total_dur / (3600 * 1000.0) << "h" << std::endl;
        }
    }

    const best_profit_stats &profit_calculator::get_best_profit_stats_global() {
        return profit_calculator::best_profit_stats_global_;
    }

    double profit_calculator::__get_avg_pool_hashrate(const currency_type &current_mined_ct,
                                                      const miner_user_info &user_info,
                                                      int period_ms) const {
        try {
            //update with pool hashrates only if currency is mined >= 1h
            if (!current_mined_ct.has_avg_hashrate_api() || period_ms < current_mined_ct.avg_hashrate_min_period_ms())
                return 0;
            const std::string &worker = user_info.worker_names_.at(dci_.device_id_nvml_);
            double avg_hashrate = network_requests::get_avg_worker_hashrate(current_mined_ct,
                                                                            user_info.wallet_addresses_.at(
                                                                                    current_mined_ct),
                                                                            worker, period_ms);
            if (avg_hashrate <= 0 || !std::isfinite(avg_hashrate)) {
                LOG(ERROR) << log_utils::gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                           "Failed to get avg profit hashrate: Worker "
                           << worker << " not available" << std::endl;
                return -1;
            }
            return avg_hashrate;
        } catch (const network_error &err) {
            LOG(ERROR) << log_utils::gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                       "Failed to get avg profit hashrate: " << err.what() << std::endl;
            return -1;
        }
    }

    double profit_calculator::__get_current_pool_hashrate(const currency_type &current_mined_ct,
                                                          const miner_user_info &user_info,
                                                          int period_ms) const {
        try {
            //update with pool hashrates only if currency is mined >= 1h
            if (!current_mined_ct.has_current_hashrate_api() ||
                period_ms < current_mined_ct.current_hashrate_min_period_ms())
                return 0;
            const std::string &worker = user_info.worker_names_.at(dci_.device_id_nvml_);
            double current_hashrate = network_requests::get_current_worker_hashrate(current_mined_ct,
                                                                                    user_info.wallet_addresses_.at(
                                                                                            current_mined_ct),
                                                                                    worker, period_ms);
            if (current_hashrate <= 0 || !std::isfinite(current_hashrate)) {
                LOG(ERROR) << log_utils::gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                           "Failed to get current profit hashrate: Worker "
                           << worker << " not available" << std::endl;
                return -1;
            }
            return current_hashrate;
        } catch (const network_error &err) {
            LOG(ERROR) << log_utils::gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                       "Failed to get current profit hashrate: " << err.what() << std::endl;
            return -1;
        }
    }

    double profit_calculator::__get_avg_hashrate_online_log(const currency_type &current_mined_ct,
                                                            int device_id, long long int system_time_start_ms,
                                                            long long int system_time_end_ms) const {
        double avg_hashrate = get_avg_hashrate_online_log(current_mined_ct, dci_.device_id_nvml_,
                                                          system_time_start_ms, system_time_end_ms);
        if (avg_hashrate <= 0 || !std::isfinite(avg_hashrate)) {
            LOG(ERROR) << log_utils::gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                       "Failed to get online log hashrate" << std::endl;
            return -1;
        }
        return avg_hashrate;
    }

}
