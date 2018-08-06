//
// Created by Alex on 06.06.2018.
//
#include "currency_type.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "../common_header/exceptions.h"
#include "../nvml/nvmlOC.h"

namespace frequency_scaling {

    currency_type::currency_type(const std::string &currency_name) : currency_name_(currency_name) {}

    bool currency_type::has_avg_hashrate_api() const {
        return !pool_avg_hashrate_api_address_.empty() && !pool_avg_hashrate_json_path_.empty() &&
               pool_avg_hashrate_api_unit_factor_hashrate_ > 0 && pool_avg_hashrate_api_unit_factor_period_ > 0;
    }

    bool currency_type::has_current_hashrate_api() const {
        return !pool_current_hashrate_api_address_.empty() && !pool_current_hashrate_json_path_.empty() &&
               pool_current_hashrate_api_unit_factor_hashrate_ > 0;
    }

    bool currency_type::has_approximated_earnings_api() const {
        return !pool_approximated_earnings_api_address_.empty() && !pool_approximated_earnings_json_path_.empty() &&
               pool_approximated_earnings_api_unit_factor_hashrate_ > 0 &&
               pool_approximated_earnings_api_unit_factor_period_ > 0;
    }

    int currency_type::avg_hashrate_min_period_ms() const {
        return 3600 * 1000;
    }

    int currency_type::current_hashrate_min_period_ms() const {
        return 3600 * 1000;
    }

    bool currency_type::operator<(const currency_type &other) const {
        return currency_name_ < other.currency_name_;
    }

    bool currency_type::operator==(const currency_type &other) const {
        return currency_name_ == other.currency_name_;
    }

    bool currency_type::operator!=(const currency_type &other) const {
        return currency_name_ != other.currency_name_;
    }

    std::string gpu_log_prefix(int device_id_nvml) {
        return "GPU " + std::to_string(device_id_nvml) + " [" +
               nvmlGetDeviceName(device_id_nvml) + "]: ";
    }

    std::string gpu_log_prefix(const currency_type &ct, int device_id_nvml) {
        return "GPU " + std::to_string(device_id_nvml) + " [" +
               nvmlGetDeviceName(device_id_nvml) + "]: " + ct.currency_name_ + ": ";
    }


    std::map<std::string, currency_type> create_default_currency_config() {
        std::map<std::string, currency_type> res;
        {
            currency_type ct_eth("ETH");
            ct_eth.bench_script_path_ = "../scripts/run_benchmark_eth_ethminer.sh";
            ct_eth.mining_script_path_ = "../scripts/start_mining_eth_ethminer.sh";
            ct_eth.pool_address_ = "eth-eu1.nanopool.org:9999";
            ct_eth.whattomine_coin_id_ = 151;
            ct_eth.cryptocompare_fsym_ = "ETH";
            ct_eth.pool_avg_hashrate_api_address_ = "https://api.nanopool.org/v1/eth/avghashratelimited/%s/%s/%s";
            ct_eth.pool_avg_hashrate_json_path_ = "data";
            ct_eth.pool_avg_hashrate_api_unit_factor_hashrate_ = 1e6;
            ct_eth.pool_avg_hashrate_api_unit_factor_period_ = 3600 * 1000;
            ct_eth.pool_approximated_earnings_api_address_ = "https://api.nanopool.org/v1/eth/approximated_earnings/%s";
            ct_eth.pool_approximated_earnings_json_path_ = "data.hour.coins";
            ct_eth.pool_approximated_earnings_api_unit_factor_hashrate_ = 1e6;
            ct_eth.pool_approximated_earnings_api_unit_factor_period_ = 1;
            res.emplace(ct_eth.currency_name_, ct_eth);
        }
        {
            currency_type ct_zec("ZEC");
            ct_zec.bench_script_path_ = "../scripts/run_benchmark_zec_excavator.sh";
            ct_zec.mining_script_path_ = "../scripts/start_mining_zec_excavator.sh";
            ct_zec.pool_address_ = "zec-eu1.nanopool.org:6666";
            ct_zec.whattomine_coin_id_ = 166;
            ct_zec.cryptocompare_fsym_ = "ZEC";
            ct_zec.pool_avg_hashrate_api_address_ = "https://api.nanopool.org/v1/zec/avghashratelimited/%s/%s/%s";
            ct_zec.pool_avg_hashrate_json_path_ = "data";
            ct_zec.pool_avg_hashrate_api_unit_factor_hashrate_ = 1;
            ct_zec.pool_avg_hashrate_api_unit_factor_period_ = 3600 * 1000;
            ct_zec.pool_approximated_earnings_api_address_ = "https://api.nanopool.org/v1/zec/approximated_earnings/%s";
            ct_zec.pool_approximated_earnings_json_path_ = "data.hour.coins";
            ct_zec.pool_approximated_earnings_api_unit_factor_hashrate_ = 1;
            ct_zec.pool_approximated_earnings_api_unit_factor_period_ = 1;
            res.emplace(ct_zec.currency_name_, ct_zec);
        }
        {
            currency_type ct_xmr("XMR");
            ct_xmr.bench_script_path_ = "../scripts/run_benchmark_xmr_xmrstak.sh";
            ct_xmr.mining_script_path_ = "../scripts/start_mining_xmr_xmrstak.sh";
            ct_xmr.pool_address_ = "xmr-eu1.nanopool.org:14433";
            ct_xmr.whattomine_coin_id_ = 101;
            ct_xmr.cryptocompare_fsym_ = "XMR";
            ct_xmr.pool_avg_hashrate_api_address_ = "https://api.nanopool.org/v1/xmr/avghashratelimited/%s/%s/%s";
            ct_xmr.pool_avg_hashrate_json_path_ = "data";
            ct_xmr.pool_avg_hashrate_api_unit_factor_hashrate_ = 1;
            ct_xmr.pool_avg_hashrate_api_unit_factor_period_ = 3600 * 1000;
            ct_xmr.pool_approximated_earnings_api_address_ = "https://api.nanopool.org/v1/xmr/approximated_earnings/%s";
            ct_xmr.pool_approximated_earnings_json_path_ = "data.hour.coins";
            ct_xmr.pool_approximated_earnings_api_unit_factor_hashrate_ = 1;
            ct_xmr.pool_approximated_earnings_api_unit_factor_period_ = 1;
            res.emplace(ct_xmr.currency_name_, ct_xmr);
        }
        return res;
    }


    std::map<std::string, currency_type> read_currency_config(const std::string &filename) {
        std::map<std::string, currency_type> currency_config;
        namespace pt = boost::property_tree;
        pt::ptree root;
        pt::read_json(filename, root);
        //read device infos
        for (const pt::ptree::value_type &array_elem : root.get_child("available_currencies")) {
            const boost::property_tree::ptree &pt_currency_type = array_elem.second;
            currency_type ct(pt_currency_type.get<std::string>("currency_name"));
            ct.bench_script_path_ = pt_currency_type.get<std::string>("bench_script_path");
            ct.mining_script_path_ = pt_currency_type.get<std::string>("mining_script_path");
            ct.pool_address_ = pt_currency_type.get<std::string>("pool_address");
            ct.whattomine_coin_id_ = pt_currency_type.get<int>("whattomine_coin_id");
            //optional fields
            ct.cryptocompare_fsym_ = pt_currency_type.get<std::string>("cryptocompare_fsym", ct.currency_name_);
            ct.pool_avg_hashrate_api_address_ = pt_currency_type.get<std::string>("pool_avg_hashrate_api_address", "");
            ct.pool_avg_hashrate_json_path_ = pt_currency_type.get<std::string>("pool_avg_hashrate_json_path", "");
            ct.pool_avg_hashrate_api_unit_factor_hashrate_ = pt_currency_type.get<double>(
                    "pool_avg_hashrate_api_unit_factor_hashrate", -1);
            ct.pool_avg_hashrate_api_unit_factor_period_ = pt_currency_type.get<double>(
                    "pool_avg_hashrate_api_unit_factor_period", -1);
            ct.pool_current_hashrate_api_address_ = pt_currency_type.get<std::string>(
                    "pool_current_hashrate_api_address", "");
            ct.pool_current_hashrate_json_path_ = pt_currency_type.get<std::string>("pool_current_hashrate_json_path",
                                                                                    "");
            ct.pool_current_hashrate_api_unit_factor_hashrate_ = pt_currency_type.get<double>(
                    "pool_current_hashrate_api_unit_factor_hashrate", -1);
            ct.pool_approximated_earnings_api_address_ = pt_currency_type.get<std::string>(
                    "pool_approximated_earnings_api_address", "");
            ct.pool_approximated_earnings_json_path_ = pt_currency_type.get<std::string>(
                    "pool_approximated_earnings_json_path", "");
            ct.pool_approximated_earnings_api_unit_factor_hashrate_ = pt_currency_type.get<double>(
                    "pool_approximated_earnings_api_unit_factor_hashrate", -1);
            ct.pool_approximated_earnings_api_unit_factor_period_ = pt_currency_type.get<double>(
                    "pool_approximated_earnings_api_unit_factor_period", -1);
            currency_config.emplace(ct.currency_name_, ct);
        }
        return currency_config;
    }

    void write_currency_config(const std::string &filename,
                               const std::map<std::string, currency_type> &currency_config) {
        namespace pt = boost::property_tree;
        pt::ptree root;
        //write devices to use
        pt::ptree pt_available_currencies;
        for (auto &elem : currency_config) {
            const currency_type &ct = elem.second;
            pt::ptree pt_currency_type;
            pt_currency_type.put("currency_name", ct.currency_name_);
            pt_currency_type.put("bench_script_path", ct.bench_script_path_);
            pt_currency_type.put("mining_script_path", ct.mining_script_path_);
            pt_currency_type.put("pool_address", ct.pool_address_);
            pt_currency_type.put("whattomine_coin_id", ct.whattomine_coin_id_);
            //optional fields
            pt_currency_type.put("cryptocompare_fsym", ct.cryptocompare_fsym_);
            pt_currency_type.put("pool_avg_hashrate_api_address", ct.pool_avg_hashrate_api_address_);
            pt_currency_type.put("pool_avg_hashrate_json_path", ct.pool_avg_hashrate_json_path_);
            pt_currency_type.put("pool_avg_hashrate_api_unit_factor_hashrate",
                                 ct.pool_avg_hashrate_api_unit_factor_hashrate_);
            pt_currency_type.put("pool_avg_hashrate_api_unit_factor_period",
                                 ct.pool_avg_hashrate_api_unit_factor_period_);
            pt_currency_type.put("pool_current_hashrate_api_address", ct.pool_current_hashrate_api_address_);
            pt_currency_type.put("pool_current_hashrate_json_path", ct.pool_current_hashrate_json_path_);
            pt_currency_type.put("pool_current_hashrate_api_unit_factor_hashrate",
                                 ct.pool_current_hashrate_api_unit_factor_hashrate_);
            pt_currency_type.put("pool_approximated_earnings_api_address", ct.pool_approximated_earnings_api_address_);
            pt_currency_type.put("pool_approximated_earnings_json_path", ct.pool_approximated_earnings_json_path_);
            pt_currency_type.put("pool_approximated_earnings_api_unit_factor_hashrate",
                                 ct.pool_approximated_earnings_api_unit_factor_hashrate_);
            pt_currency_type.put("pool_approximated_earnings_api_unit_factor_period",
                                 ct.pool_approximated_earnings_api_unit_factor_period_);
            pt_available_currencies.push_back(std::make_pair("", pt_currency_type));
        }
        root.add_child("available_currencies", pt_available_currencies);
        pt::write_json(filename, root);
    }
}

