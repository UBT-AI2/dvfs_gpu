#pragma once

#include <map>
#include <set>
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"

namespace frequency_scaling {

    struct currency_info {
        currency_info(currency_type type, double approximated_earnings_eur_hour,
                      double stock_price_eur_);

        const currency_type type_;
        double approximated_earnings_eur_hour_;
        double stock_price_eur_;
    };


    struct energy_hash_info {
        energy_hash_info(currency_type type,
                         const measurement &optimal_configuration_offline);

        energy_hash_info(currency_type type,
                         const measurement &optimal_configuration_offline,
                         const measurement &optimal_configuration_online);

        const currency_type type_;
        measurement optimal_configuration_offline_;
        measurement optimal_configuration_online_;
    };


    class profit_calculator {
    public:
        profit_calculator(const device_clock_info &dci,
                          const std::map<currency_type, energy_hash_info> &energy_hash_info,
                          double power_cost_kwh);

        void recalculate_best_currency();

        void update_currency_info_nanopool();

        void update_opt_config_hashrate_nanopool(currency_type current_mined_ct,
                                                 const miner_user_info &user_info, int period_ms);

        void update_power_consumption(currency_type current_mined_ct, long long int system_time_start_ms);

        void update_energy_cost_stromdao(int plz);

        void update_opt_config_offline(currency_type current_mined_ct,
                                       const measurement &new_config_offline);

        const std::map<currency_type, currency_info> &getCurrency_info_() const;

        const std::map<currency_type, energy_hash_info> &getEnergy_hash_info_() const;

        std::map<currency_type, measurement> get_opt_start_values() const;

        std::set<currency_type> get_used_currencies() const;

        double getPower_cost_kwh_() const;

        const device_clock_info &getDci_() const;

        currency_type getBest_currency_() const;

        double getBest_currency_profit_() const;

        double get_used_hashrate(currency_type ct) const;

        double get_used_power(currency_type ct) const;

        double get_used_energy_hash(currency_type ct) const;

        void save_current_period(currency_type ct);

    private:
        std::string get_log_prefix(currency_type ct) const;

        device_clock_info dci_;
        std::map<currency_type, currency_info> currency_info_;
        std::map<currency_type, energy_hash_info> energy_hash_info_;
        std::map<currency_type, measurement> last_online_measurements_;
        double power_cost_kwh_;
        currency_type best_currency_;
        double best_currency_profit_;
        const int window_dur_ms_ = 6 * 3600 * 1000;
    };

}