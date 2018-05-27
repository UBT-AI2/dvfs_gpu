#pragma once

#include <map>

namespace frequency_scaling {

    enum class currency_type {
        ZEC, ETH, XMR, count
    };

    struct currency_info {
        currency_info(currency_type type, double approximated_earnings_eur_hour,
                      double stock_price_eur_);

        currency_type type_;
        double approximated_earnings_eur_hour_;
        double stock_price_eur_;
    };


    struct energy_hash_info {
        energy_hash_info(currency_type type_, double hashrate_hs_, double energy_consumption_js_);

        currency_type type_;
        double hashrate_hs_, energy_consumption_js_;
    };


    class profit_calculator {
    public:
        profit_calculator(const std::map<currency_type, currency_info> &currency_info,
                          const std::map<currency_type, energy_hash_info> &energy_hash_info,
                          double power_cost_kwh);

        std::pair<currency_type, double> calc_best_currency() const;
        void update_currency_info_nanopool();
        void update_energy_cost_stromdao(int plz);

        const std::map<currency_type, currency_info> &getCurrency_info_() const;
        const std::map<currency_type, energy_hash_info> &getEnergy_hash_info_() const;
        double getPower_cost_kwh_() const;

    private:
        std::map<currency_type, currency_info> currency_info_;
        std::map<currency_type, energy_hash_info> energy_hash_info_;
        double power_cost_kwh_;
    };


    std::map<currency_type, currency_info> get_currency_infos_nanopool(
            const std::map<currency_type, energy_hash_info> &ehi);

}