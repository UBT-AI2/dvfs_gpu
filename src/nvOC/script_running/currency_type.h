#pragma once

#include <string>
#include <vector>
#include <map>

namespace frequency_scaling {

    struct currency_type {
        explicit currency_type(const std::string &currency_name, bool use_ccminer);

        bool has_avg_hashrate_api() const;

        bool has_current_hashrate_api() const;

        bool has_approximated_earnings_api() const;

        int avg_hashrate_min_period_ms() const;

        int current_hashrate_min_period_ms() const;

        bool operator<(const currency_type &other) const;

        bool operator==(const currency_type &other) const;

        bool operator!=(const currency_type &other) const;

        const std::string currency_name_;
        //relative to binary
        const bool use_ccminer_;
        std::string ccminer_algo_;//specify if ccminer
        std::string bench_script_path_, mining_script_path_;//specify if not ccminer
        std::vector<std::string> pool_addresses_;
        int whattomine_coin_id_;
        std::string cryptocompare_fsym_;
        //optional api options
        //#########################################################################
        //printf format string: wallet_address
        std::string pool_current_hashrate_api_address_;
        std::string pool_current_hashrate_json_path_worker_array_, pool_current_hashrate_json_path_worker_name_;
        std::string pool_current_hashrate_json_path_hashrate_;
        //target unit H/s
        double pool_current_hashrate_api_unit_factor_hashrate_ = -1;
        //#########################################################################
        //printf format string: wallet_address, period
        std::string pool_avg_hashrate_api_address_;
        std::string pool_avg_hashrate_json_path_worker_array_, pool_avg_hashrate_json_path_worker_name_;
        std::string pool_avg_hashrate_json_path_hashrate_;
        //target unit H/s | target unit ms
        double pool_avg_hashrate_api_unit_factor_hashrate_ = -1, pool_avg_hashrate_api_unit_factor_period_ = -1;
        //#########################################################################
        //printf format string: hashrate
        std::string pool_approximated_earnings_api_address_;
        std::string pool_approximated_earnings_json_path_;
        //target unit H/s | target unit hours
        double pool_approximated_earnings_api_unit_factor_hashrate_ = -1, pool_approximated_earnings_api_unit_factor_period_ = -1;
    };


    std::string gpu_log_prefix(int device_id_nvml);

    std::string gpu_log_prefix(const currency_type &ct, int device_id_nvml);

    std::map<std::string, currency_type> create_default_currency_config();

    std::map<std::string, currency_type> read_currency_config(const std::string &filename);

    void write_currency_config(const std::string &filename,
                               const std::map<std::string, currency_type> &currency_config);

}