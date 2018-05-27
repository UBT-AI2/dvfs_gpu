//
// Created by alex on 25.05.18.
//
#pragma once

#include "profit_calculation.h"
#include <map>

namespace frequency_scaling {
    double get_approximated_earnings_per_hour_nanopool(currency_type type, double hashrate_hs);

    std::map<std::string, double>
    get_avg_hashrate_per_worker_nanopool(currency_type type, const std::string &wallet_address, double period_hours);

    double get_current_stock_price_nanopool(currency_type type);

    /**
     * Returns the current energy cost in euro per kWh.
     * @param plz
     * @return
     */
    double get_energy_cost_stromdao(int plz);
}
