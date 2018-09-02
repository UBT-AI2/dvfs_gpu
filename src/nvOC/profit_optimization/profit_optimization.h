#pragma once

#include <string>
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"
#include "profit_calculation.h"
#include "optimization_config.h"

namespace frequency_scaling {

    struct device_opt_result {
        device_opt_result(int device_id, const std::string &device_name,
                          const std::map<currency_type, energy_hash_info> &currency_ehi) : device_id_(device_id),
                                                                                           device_name_(device_name),
                                                                                           currency_ehi_(
                                                                                                   currency_ehi) {}

        int device_id_;
        std::string device_name_;
        std::map<currency_type, energy_hash_info> currency_ehi_;
    };

    void mine_most_profitable_currency(const optimization_config &opt_config,
                                       const std::map<int, device_opt_result> &opt_results_in =
                                       std::map<int, device_opt_result>());

    void save_optimization_result(const std::string &filename,
                                  const std::map<int, device_opt_result> &opt_result);

    std::map<int, device_opt_result>
    load_optimization_result(const std::string &filename,
                             const std::map<std::string, currency_type> &available_currencies);

}