#pragma once

#include <map>

namespace frequency_scaling {

    enum class currency_type {
        ZEC, ETH
    };

    struct currency_info {
        currency_info(currency_type type, double difficulty,
                      double block_reward, double stock_price);

        currency_type type_;
        double difficulty_, block_reward_;
        double stock_price_usd_;

    };


    struct energy_hash_info {
        energy_hash_info(currency_type type_, double hashrate_hs_, double energy_consumption_js_);

        currency_type type_;
        double hashrate_hs_, energy_consumption_js_;
    };


    struct profit_calculator {
        profit_calculator(const std::map<currency_type, currency_info> &currency_info,
                          const std::map<currency_type, energy_hash_info> &energy_hash_info,
                          double power_cost_kwh,
                          double pool_fee_percentage);

        std::pair<currency_type, double> calc_best_currency() const;

        std::map<currency_type, currency_info> currency_info_;
        std::map<currency_type, energy_hash_info> energy_hash_info_;
        double power_cost_kwh_;
        double pool_fee_percentage_;

    };


    std::map<currency_type, currency_info> get_currency_infos();

}