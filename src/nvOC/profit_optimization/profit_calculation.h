#pragma once

#include <map>
#include <set>
#include <mutex>
#include "../freq_core/benchmark.h"
#include "../freq_core/mining.h"
#include "../freq_core/network_requests.h"

namespace frequency_scaling {

    struct currency_info {
        currency_info(const currency_type &type, double approximated_earnings_eur_hour,
                      double used_hashrate, const currency_stats &cs);

        const currency_type type_;
        double approximated_earnings_eur_hour_;
        double used_hashrate_;
        currency_stats cs_;
    };


    struct energy_hash_info {
        energy_hash_info(const currency_type &type,
                         const measurement &optimal_configuration_offline,
                         const measurement &optimal_configuration_online);

        energy_hash_info(const currency_type &type,
                         const measurement &optimal_configuration_offline,
                         const measurement &optimal_configuration_online,
                         const measurement &optimal_configuration_profit);

        const currency_type type_;
        measurement optimal_configuration_offline_;
        measurement optimal_configuration_online_;
        measurement optimal_configuration_profit_;
    };

    class best_profit_stats {
    public:
        struct device_stats {
            device_stats(const currency_type &ct, int ct_mem_clock, int ct_graph_clock, long long int system_time_ms,
                         double stock_price_eur, double earnings, double costs, double power, double hashrate);

            bool operator<(const device_stats &other) const;

            currency_type ct_;
            int ct_mem_clock_, ct_graph_clock_;
            long long int system_time_ms_;
            double stock_price_eur_;
            double earnings_, costs_, profit_;
            double power_, hashrate_, energy_hash_;
        };

        void update_device_stats(int device_id, const std::multiset<device_stats> &device_currency_stats);

        double get_global_earnings() const;

        double get_global_costs() const;

        double get_global_profit() const;

        double get_global_power() const;

        const device_stats &get_device_stats(int device_id) const;

    private:
        mutable std::mutex map_mutex_, logfile_mutex_;
        std::map<int, std::vector<std::multiset<device_stats>>> stats_map_;
    };

    class profit_calculator {
    public:
        profit_calculator(const device_clock_info &dci,
                          const std::map<currency_type, energy_hash_info> &energy_hash_info,
                          double power_cost_kwh);

        void recalculate_best_currency();

        void update_currency_info();

        bool update_opt_config_profit_hashrate(const currency_type &current_mined_ct,
                                               const miner_user_info &user_info, long long int system_time_start_ms);

        bool update_power_consumption(const currency_type &current_mined_ct, long long int system_time_start_ms);

        void update_energy_cost_stromdao(int plz);

        void update_opt_config_online(const currency_type &current_mined_ct,
                                      const measurement &new_config_online);

        const std::map<currency_type, currency_info> &getCurrency_info_() const;

        const std::map<currency_type, energy_hash_info> &getEnergy_hash_info_() const;

        std::map<currency_type, measurement> get_opt_start_values() const;

        std::set<currency_type> get_used_currencies() const;

        double getPower_cost_kwh_() const;

        const device_clock_info &getDci_() const;

        currency_type getBest_currency_() const;

        double getBest_currency_profit_() const;

        double calculate_used_hashrate(const currency_type &ct) const;

        double get_used_hashrate(const currency_type &ct) const;

        double get_used_power(const currency_type &ct) const;

        double get_used_energy_hash(const currency_type &ct) const;

        void save_current_period(const currency_type &ct, long long int system_time_start_ms_no_window);

        static const best_profit_stats &get_best_profit_stats_global();

    private:
        double __get_avg_pool_hashrate(const currency_type &current_mined_ct,
                                       const miner_user_info &user_info, int period_ms) const;

        double __get_current_pool_hashrate(const currency_type &current_mined_ct,
                                           const miner_user_info &user_info, int period_ms) const;

        double __get_avg_hashrate_online_log(const currency_type &current_mined_ct,
                                             int device_id, long long int system_time_start_ms,
                                             long long int system_time_end_ms) const;

    public:
        const int window_dur_ms_ = 6 * 3600 * 1000;
    private:
        device_clock_info dci_;
        std::map<currency_type, currency_info> currency_info_;
        std::map<currency_type, energy_hash_info> energy_hash_info_;
        std::map<currency_type, measurement> last_profit_measurements_;
        double power_cost_kwh_;
        std::map<currency_type, std::vector<std::pair<long long int, int>>> currency_mining_timespans_;
        std::map<currency_type, std::vector<std::pair<long long int, double>>> timespan_current_pool_hashrates_;
        std::set<currency_type> currency_blacklist_;
        static best_profit_stats best_profit_stats_global_;
    };

}