#include "profit_calculation.h"

#include <cmath>
#include <limits>
#include <chrono>
#include <algorithm>
#include <glog/logging.h>
#include "../common_header/exceptions.h"
#include "../script_running/network_requests.h"

namespace frequency_scaling {

    currency_info::currency_info(currency_type type, double approximated_earnings_eur_hour,
                                 const currency_stats &cs)
            : type_(type), approximated_earnings_eur_hour_(approximated_earnings_eur_hour),
              cs_(cs) {}

    energy_hash_info::energy_hash_info(currency_type type,
                                       const measurement &optimal_configuration_offline,
                                       const measurement &optimal_configuration_online) :
            energy_hash_info(type, optimal_configuration_offline, optimal_configuration_online,
                             optimal_configuration_online) {
        optimal_configuration_profit_.hashrate_measure_dur_ms_ = 0;
        optimal_configuration_profit_.power_measure_dur_ms_ = 0;
    }

    energy_hash_info::energy_hash_info(currency_type type,
                                       const measurement &optimal_configuration_offline,
                                       const measurement &optimal_configuration_online,
                                       const measurement &optimal_configuration_profit) : type_(
            type), optimal_configuration_offline_(optimal_configuration_offline), optimal_configuration_online_(
            optimal_configuration_online), optimal_configuration_profit_(
            optimal_configuration_profit) {}


    void best_profit_stats::update_device_stats(int device_id, currency_type ct, double earnings,
                                                double costs, double power, double hashrate) {
        std::lock_guard<std::mutex> lock(map_mutex_);
        device_stats ds;
        ds.ct_ = ct;
        ds.earnings_ = earnings;
        ds.costs_ = costs;
        ds.profit_ = earnings - costs;
        ds.power_ = power;
        ds.hashrate_ = hashrate;
        ds.energy_hash_ = hashrate / power;
        stats_map_.erase(device_id);
        stats_map_.emplace(device_id, ds);
    }

    double best_profit_stats::get_global_earnings() const {
        std::lock_guard<std::mutex> lock(map_mutex_);
        double sum = 0;
        for (auto &elem : stats_map_)
            sum += elem.second.earnings_;
        return sum;
    }

    double best_profit_stats::get_global_costs() const {
        std::lock_guard<std::mutex> lock(map_mutex_);
        double sum = 0;
        for (auto &elem : stats_map_)
            sum += elem.second.costs_;
        return sum;
    }

    double best_profit_stats::get_global_profit() const {
        std::lock_guard<std::mutex> lock(map_mutex_);
        double sum = 0;
        for (auto &elem : stats_map_)
            sum += elem.second.profit_;
        return sum;
    }

    double best_profit_stats::get_global_power() const {
        std::lock_guard<std::mutex> lock(map_mutex_);
        double sum = 0;
        for (auto &elem : stats_map_)
            sum += elem.second.power_;
        return sum;
    }

    const best_profit_stats::device_stats &best_profit_stats::get_device_stats(int device_id) const {
        std::lock_guard<std::mutex> lock(map_mutex_);
        return stats_map_.at(device_id);
    }


    best_profit_stats profit_calculator::best_profit_stats_global_;

    profit_calculator::profit_calculator(const device_clock_info &dci,
                                         const std::map<currency_type, energy_hash_info> &energy_hash_info,
                                         double power_cost_kwh) : dci_(dci),
                                                                  energy_hash_info_(energy_hash_info),
                                                                  power_cost_kwh_(power_cost_kwh) {
        for (auto &ehi : energy_hash_info) {
            last_profit_measurements_.emplace(ehi.first, measurement());
            save_current_period(ehi.first);
        }
        update_currency_info_nanopool();
        recalculate_best_currency();
    }


    void
    profit_calculator::recalculate_best_currency() {
        double best_profit = std::numeric_limits<double>::lowest();
        for (int i = 0; i < static_cast<int>(currency_type::count); i++) {
            currency_type ct = static_cast<currency_type>(i);
            auto it_ehi = energy_hash_info_.find(ct);
            auto it_ci = currency_info_.find(ct);
            if (it_ehi == energy_hash_info_.end() || it_ci == currency_info_.end())
                continue;
            const energy_hash_info &ehi = it_ehi->second;
            const currency_info &ci = it_ci->second;
            double costs_per_hour = ehi.optimal_configuration_profit_.power_ * (power_cost_kwh_ / 1000.0);
            double profit_per_hour = ci.approximated_earnings_eur_hour_ - costs_per_hour;
            if (profit_per_hour > best_profit) {
                best_profit = profit_per_hour;
                //update global profit stats for this device
                profit_calculator::best_profit_stats_global_.update_device_stats(dci_.device_id_nvml_, ct,
                                                                                 ci.approximated_earnings_eur_hour_,
                                                                                 costs_per_hour,
                                                                                 ehi.optimal_configuration_profit_.power_,
                                                                                 ehi.optimal_configuration_profit_.hashrate_);
            }
            //print stats
            VLOG(0) << gpu_log_prefix(ct, dci_.device_id_nvml_) <<
                    "Profit calculation using: hashrate=" <<
                    get_used_hashrate(ct) << ", power=" << get_used_power(ct) <<
                    ", energy_hash=" << get_used_energy_hash(ct) <<
                    ", stock_price=" << it_ci->second.cs_.stock_price_eur_
                    << std::endl;
            VLOG(0) << gpu_log_prefix(ct, dci_.device_id_nvml_) <<
                    "Calculated profit [eur/hour]: approximated earnings=" <<
                    ci.approximated_earnings_eur_hour_ << ", energy_cost="
                    << costs_per_hour << ", profit="
                    << profit_per_hour << std::endl;
        }
        const best_profit_stats &bps = profit_calculator::best_profit_stats_global_;
        VLOG(0) << "Global profit stats [eur/hour]: approximated earnings=" <<
                bps.get_global_earnings() << ", energy_cost=" << bps.get_global_costs() <<
                "(" << bps.get_global_power() << "W), profit=" << bps.get_global_profit() << std::endl;

    }

    void profit_calculator::update_currency_info_nanopool() {
        for (auto &elem : energy_hash_info_) {
            currency_type ct = elem.first;
            try {
                double used_hashrate = get_used_hashrate(ct);
                const currency_stats &cs = get_currency_stats(ct);
                double approximated_earnings;
                try {
                    approximated_earnings = get_approximated_earnings_per_hour_nanopool(ct, used_hashrate);
                } catch (const network_error &err) {
                    //fallback
                    VLOG(1) << "Nanopool approximated earnings API call failed: " << err.what() <<
                            ". Using fallback..." << std::endl;
                    approximated_earnings = cs.calc_approximated_earnings_eur_hour(used_hashrate);
                }
                currency_info_.erase(ct);
                currency_info_.emplace(ct, currency_info(ct, approximated_earnings, cs));
                VLOG(0) << gpu_log_prefix(ct, dci_.device_id_nvml_) << "Update currency info using hashrate "
                        << used_hashrate << std::endl;
            } catch (const network_error &err) {
                LOG(ERROR) << gpu_log_prefix(ct, dci_.device_id_nvml_) <<
                           "Failed to update currency info: " << err.what() << std::endl;
            }
        }
    }

    bool profit_calculator::update_opt_config_hashrate_nanopool(currency_type current_mined_ct,
                                                                const miner_user_info &user_info,
                                                                long long int system_time_start_ms) {
        try {
            long long int system_time_now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            int period_ms = system_time_now_ms - system_time_start_ms;
            //update with pool hashrates only if currency is mined >= 1h
            if (period_ms < 3600 * 1000)
                return false;
            const std::map<std::string, double> &avg_hashrates = get_avg_hashrate_per_worker_nanopool(
                    current_mined_ct, user_info.wallet_addresses_.at(current_mined_ct), period_ms);
            const std::string worker = user_info.worker_names_.at(dci_.device_id_nvml_);
            auto it_hr = avg_hashrates.find(worker);
            if (it_hr == avg_hashrates.end() || it_hr->second <= 0 || !std::isfinite(it_hr->second)) {
                LOG(ERROR) << gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                           "Failed to get avg profit hashrate: Worker "
                           << worker << " not available" << std::endl;
                return false;
            }
            //update hashrate
            double cur_hashrate = it_hr->second;
            double last_hashrate = last_profit_measurements_.at(current_mined_ct).hashrate_;
            int cur_period_ms = period_ms;
            int last_period_ms = last_profit_measurements_.at(current_mined_ct).hashrate_measure_dur_ms_;
            int total_period_ms = last_period_ms + cur_period_ms;
            double new_hashrate = (cur_period_ms / (double) total_period_ms) * cur_hashrate +
                                  (last_period_ms / (double) total_period_ms) * last_hashrate;
            energy_hash_info_.at(current_mined_ct).optimal_configuration_profit_.update_hashrate(new_hashrate,
                                                                                                 total_period_ms);
            VLOG(0) << gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                    "Updated avg profit hashrate: " << new_hashrate << std::endl;
            return true;
        } catch (const network_error &err) {
            LOG(ERROR) << gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                       "Failed to get avg profit hashrate: " << err.what() << std::endl;
            return false;
        }
    }

    bool profit_calculator::update_power_consumption(currency_type current_mined_ct,
                                                     long long int system_time_start_ms) {
        long long int system_time_now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        //update power
        double cur_power = get_avg_power_usage(dci_.device_id_nvml_, system_time_start_ms, system_time_now_ms);
        if (cur_power <= 0 || !std::isfinite(cur_power)) {
            LOG(ERROR) << gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
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
        VLOG(0) << gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                "Update profit power: " << new_power << std::endl;
        return true;
    }

    void profit_calculator::update_opt_config_online(currency_type current_mined_ct,
                                                     const measurement &new_config_online) {

        energy_hash_info &ehi = energy_hash_info_.at(current_mined_ct);
        ehi.optimal_configuration_online_ = new_config_online;
        ehi.optimal_configuration_profit_.update_freq_config(new_config_online);
        //update profit hashrate and power also if no profit measurement available yet
        if (ehi.optimal_configuration_profit_.hashrate_measure_dur_ms_ <= 0)
            ehi.optimal_configuration_profit_.update_hashrate(new_config_online.hashrate_, 0);
        if (ehi.optimal_configuration_profit_.power_measure_dur_ms_ <= 0)
            ehi.optimal_configuration_profit_.update_power(new_config_online.power_, 0);
        VLOG(0) << gpu_log_prefix(current_mined_ct, dci_.device_id_nvml_) <<
                "Update opt_config_online" << std::endl;
    }


    void profit_calculator::update_energy_cost_stromdao(int plz) {
        power_cost_kwh_ = get_energy_cost_stromdao(plz);
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


    double profit_calculator::get_used_hashrate(currency_type ct) const {
        const measurement &cur_profit_measurement = energy_hash_info_.at(ct).optimal_configuration_profit_;
        const measurement &cur_online_measurement = energy_hash_info_.at(ct).optimal_configuration_online_;
        double alpha = std::min(1.0, cur_profit_measurement.hashrate_measure_dur_ms_ / (double) window_dur_ms_);
        return alpha * cur_profit_measurement.hashrate_ + (1 - alpha) * cur_online_measurement.hashrate_;
    }

    double profit_calculator::get_used_power(currency_type ct) const {
        return energy_hash_info_.at(ct).optimal_configuration_profit_.power_;
    }

    double profit_calculator::get_used_energy_hash(currency_type ct) const {
        return get_used_hashrate(ct) / get_used_power(ct);
    }

    void profit_calculator::save_current_period(currency_type ct) {
        last_profit_measurements_.at(ct) = energy_hash_info_.at(ct).optimal_configuration_profit_;
    }

    const best_profit_stats &profit_calculator::get_best_profit_stats_global() {
        return profit_calculator::best_profit_stats_global_;
    }

}
