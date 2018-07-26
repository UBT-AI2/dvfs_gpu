#pragma once

#include <map>
#include <set>
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"
#include "../script_running/network_requests.h"

namespace frequency_scaling {

    struct currency_info {
        currency_info(currency_type type, double approximated_earnings_eur_hour,
                      const currency_stats &cs);

        const currency_type type_;
        double approximated_earnings_eur_hour_;
        currency_stats cs_;
    };


    struct energy_hash_info {
        energy_hash_info(currency_type type,
                         const measurement &optimal_configuration_offline,
                         const measurement &optimal_configuration_online);

        energy_hash_info(currency_type type,
                         const measurement &optimal_configuration_offline,
                         const measurement &optimal_configuration_online,
                         const measurement &optimal_configuration_profit);

        const currency_type type_;
        const measurement optimal_configuration_offline_;
        measurement optimal_configuration_online_;
        measurement optimal_configuration_profit_;
    };


    class profit_calculator {
    public:
        profit_calculator(const device_clock_info &dci,
                          const std::map<currency_type, energy_hash_info> &energy_hash_info,
                          double power_cost_kwh);

        void recalculate_best_currency();

        void update_currency_info_nanopool();

        bool update_opt_config_hashrate_nanopool(currency_type current_mined_ct,
                                                 const miner_user_info &user_info, long long int system_time_start_ms);

        bool update_power_consumption(currency_type current_mined_ct, long long int system_time_start_ms);

        void update_energy_cost_stromdao(int plz);

        void update_opt_config_online(currency_type current_mined_ct,
                                      const measurement &new_config_online);

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

        const int window_dur_ms_ = 6 * 3600 * 1000;
    private:
        device_clock_info dci_;
        std::map<currency_type, currency_info> currency_info_;
        std::map<currency_type, energy_hash_info> energy_hash_info_;
        std::map<currency_type, measurement> last_profit_measurements_;
        double power_cost_kwh_;
        currency_type best_currency_;
        double best_currency_profit_;
    };

}