#include "profit_calculation.h"

#include <iostream>
#include <limits>
#include "../exceptions.h"
#include "network_requests.h"

namespace frequency_scaling {

    currency_info::currency_info(currency_type type, double approximated_earnings_eur_hour,
                                 double stock_price_eur)
            : type_(type), approximated_earnings_eur_hour_(approximated_earnings_eur_hour),
              stock_price_eur_(stock_price_eur) {}


    energy_hash_info::energy_hash_info(currency_type type,
                                       const measurement &optimal_configuration) : type_(
            type), optimal_configuration_(optimal_configuration) {}


    profit_calculator::profit_calculator(const device_clock_info &dci,
                                         const std::map<currency_type, currency_info> &currency_info,
                                         const std::map<currency_type, energy_hash_info> &energy_hash_info,
                                         double power_cost_kwh) : dci_(dci),
                                                                  currency_info_(currency_info),
                                                                  energy_hash_info_(energy_hash_info),
                                                                  power_cost_kwh_(power_cost_kwh) {}


    std::pair<currency_type, double>
    profit_calculator::calc_best_currency() const {
        int best_idx = 0;
        double best_profit = std::numeric_limits<double>::lowest();
        for (int i = 0; i < static_cast<int>(currency_type::count); i++) {
            auto it_ehi = energy_hash_info_.find(static_cast<currency_type>(i));
            auto it_ci = currency_info_.find(static_cast<currency_type>(i));
            if(it_ehi == energy_hash_info_.end() || it_ci == currency_info_.end())
                continue;
            const energy_hash_info &ehi = it_ehi->second;
            const currency_info &ci = it_ci->second;
            double costs_per_hour = ehi.optimal_configuration_.power_ * (power_cost_kwh_ / 1000.0);
            double profit_per_hour = ci.approximated_earnings_eur_hour_ - costs_per_hour;
            if (profit_per_hour > best_profit) {
                best_idx = i;
                best_profit = profit_per_hour;
            }
        }
        return std::make_pair(static_cast<currency_type>(best_idx), best_profit);
    }

    void profit_calculator::update_currency_info_nanopool() {
        currency_info_ = get_currency_infos_nanopool(energy_hash_info_);
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

    double profit_calculator::getPower_cost_kwh_() const {
        return power_cost_kwh_;
    }

    const device_clock_info &profit_calculator::getDci_() const {
        return dci_;
    }


    std::map<currency_type, currency_info> get_currency_infos_nanopool(
            const std::map<currency_type, energy_hash_info> &currency_to_ehi) {
        std::map<currency_type, currency_info> currency_infos;
        for (int i = 0; i < static_cast<int>(currency_type::count); i++) {
            currency_type ct = static_cast<currency_type>(i);
            auto it_ehi = currency_to_ehi.find(ct);
            if(it_ehi == currency_to_ehi.end())
                continue;
            try {
                const energy_hash_info &ehi = it_ehi->second;
                double approximated_earnings =
                        get_approximated_earnings_per_hour_nanopool(ct, ehi.optimal_configuration_.hashrate_);
                double stock_price = get_current_stock_price_nanopool(ct);
                currency_infos.emplace(static_cast<currency_type>(i),
                                       currency_info(ct, approximated_earnings, stock_price));
            }catch (const network_error& err){
                std::cerr << "Failed to get infos for currency " <<
                          enum_to_string(ct) << ": " << err.what() << std::endl;
            }
        }
        return currency_infos;
    }

}
