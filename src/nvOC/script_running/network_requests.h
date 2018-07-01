//
// Created by alex on 25.05.18.
//
#pragma once

#include "../profit_optimization/profit_calculation.h"
#include <map>

namespace frequency_scaling {

    double get_approximated_earnings_per_hour_nanopool(currency_type ct, double hashrate_hs,
                                                       int trials = 3, int trial_timeout_ms = 1000);

    std::map<std::string, double>
    get_avg_hashrate_per_worker_nanopool(currency_type ct, const std::string &wallet_address, double period_hours,
                                         int trials = 3, int trial_timeout_ms = 1000);

    double get_current_stock_price_nanopool(currency_type ct, int trials = 3, int trial_timeout_ms = 1000);

    /**
     * Returns the current energy cost in euro per kWh in region with plz.
     * @param plz
     * @return
     */
    double get_energy_cost_stromdao(int plz, int trials = 3, int trial_timeout_ms = 1000);

}
