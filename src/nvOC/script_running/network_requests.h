//
// Created by alex on 25.05.18.
//
#pragma once

#include <map>
#include "currency_type.h"

namespace frequency_scaling {

    struct currency_stats {
        double calc_approximated_earnings_eur_hour(double user_hashrate_hs) const;

        double stock_price_eur_;
        double difficulty_;
        double block_time_sec_;
        double nethash_;
        double block_reward_;
    };

    /**
     * Get approximated earnings when mining given currency with given hashrate in eur/h
     * using nanopool api.
     * @param ct currency
     * @param hashrate_hs hashrate in h/s
     * @return approximated earnings in eur/h
     */
    double get_approximated_earnings_per_hour_nanopool(currency_type ct, double hashrate_hs,
                                                       int trials = 3, int trial_timeout_ms = 1000);

    /**
     * Get average hashrate for all workers that use specified wallet_address
     * from nanopool in the specified time window.
     * @param ct currency
     * @param wallet_address wallet address that workers use
     * @param period_ms time window
     * @return map of worker name hashrate pairs
     */
    std::map<std::string, double>
    get_avg_hashrate_per_worker_nanopool(currency_type ct, const std::string &wallet_address, int period_ms,
                                         int trials = 3, int trial_timeout_ms = 1000);

    /**
     * Get recent curreny stats for specified currency.
     * @param ct currency
     * @return currency stats
     */
    currency_stats get_currency_stats(currency_type ct, int trials = 3, int trial_timeout_ms = 1000);

    /**
     * Returns the current energy cost in euro per kWh in region with plz.
     * @param plz post code
     * @return energy cost eur/kWh
     */
    double get_energy_cost_stromdao(int plz, int trials = 3, int trial_timeout_ms = 1000);

}
