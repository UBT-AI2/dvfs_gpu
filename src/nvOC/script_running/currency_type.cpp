//
// Created by Alex on 06.06.2018.
//
#include "currency_type.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "../common_header/exceptions.h"
#include "../nvml/nvmlOC.h"
#include "log_utils.h"

namespace frequency_scaling {

    currency_type::currency_type(const std::string &currency_name, bool use_ccminer) :
            currency_name_(currency_name), use_ccminer_(use_ccminer) {
        if (use_ccminer_) {
            bench_script_path_ = "../scripts/run_benchmark_generic_ccminer.sh";
            mining_script_path_ = "../scripts/start_mining_generic_ccminer.sh";
        }
    }

    bool currency_type::has_avg_hashrate_api() const {
        return !pool_avg_hashrate_api_address_.empty() && !pool_avg_hashrate_json_path_worker_array_.empty() &&
               !pool_avg_hashrate_json_path_worker_name_.empty() && !pool_avg_hashrate_json_path_hashrate_.empty() &&
               pool_avg_hashrate_api_unit_factor_hashrate_ > 0 && pool_avg_hashrate_api_unit_factor_period_ > 0;
    }

    bool currency_type::has_current_hashrate_api() const {
        return !pool_current_hashrate_api_address_.empty() && !pool_current_hashrate_json_path_worker_array_.empty() &&
               !pool_current_hashrate_json_path_worker_name_.empty() &&
               !pool_current_hashrate_json_path_hashrate_.empty() &&
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
            currency_type ct_eth("ETH", false);
            ct_eth.bench_script_path_ = "../scripts/run_benchmark_eth_ethminer.sh";
            ct_eth.mining_script_path_ = "../scripts/start_mining_eth_ethminer.sh";
            ct_eth.pool_addresses_ = {"eth-eu1.nanopool.org:9999", "eth-eu2.nanopool.org:9999",
                                      "eth-au1.nanopool.org:9999", "eth-jp1.nanopool.org:9999",
                                      "eth-us-west1.nanopool.org:9999", "eth-us-east1.nanopool.org:9999",
                                      "eth-asia1.nanopool.org:9999"};
            ct_eth.whattomine_coin_id_ = 151;
            ct_eth.cryptocompare_fsym_ = "ETH";
            ct_eth.pool_avg_hashrate_api_address_ = "https://api.nanopool.org/v1/eth/avghashrateworkers/%s/%s";
            ct_eth.pool_avg_hashrate_json_path_worker_array_ = "data";
            ct_eth.pool_avg_hashrate_json_path_worker_name_ = "worker";
            ct_eth.pool_avg_hashrate_json_path_hashrate_ = "hashrate";
            ct_eth.pool_avg_hashrate_api_unit_factor_hashrate_ = 1e6;
            ct_eth.pool_avg_hashrate_api_unit_factor_period_ = 3600 * 1000;
            ct_eth.pool_approximated_earnings_api_address_ = "https://api.nanopool.org/v1/eth/approximated_earnings/%s";
            ct_eth.pool_approximated_earnings_json_path_ = "data.hour.coins";
            ct_eth.pool_approximated_earnings_api_unit_factor_hashrate_ = 1e6;
            ct_eth.pool_approximated_earnings_api_unit_factor_period_ = 1;
            res.emplace(ct_eth.currency_name_, ct_eth);
        }
        {
#ifdef _WIN32
            currency_type ct_zec("ZEC", false);
            ct_zec.bench_script_path_ = "../scripts/run_benchmark_zec_excavator.sh";
            ct_zec.mining_script_path_ = "../scripts/start_mining_zec_excavator.sh";
            ct_zec.pool_addresses_ = {"zec-eu2.nanopool.org:6666"};
            ct_zec.whattomine_coin_id_ = 166;
            ct_zec.cryptocompare_fsym_ = "ZEC";
            ct_zec.pool_avg_hashrate_api_address_ = "https://api.nanopool.org/v1/zec/avghashrateworkers/%s/%s";
            ct_zec.pool_avg_hashrate_json_path_worker_array_ = "data";
            ct_zec.pool_avg_hashrate_json_path_worker_name_ = "worker";
            ct_zec.pool_avg_hashrate_json_path_hashrate_ = "hashrate";
            ct_zec.pool_avg_hashrate_api_unit_factor_hashrate_ = 1;
            ct_zec.pool_avg_hashrate_api_unit_factor_period_ = 3600 * 1000;
            ct_zec.pool_approximated_earnings_api_address_ = "https://api.nanopool.org/v1/zec/approximated_earnings/%s";
            ct_zec.pool_approximated_earnings_json_path_ = "data.hour.coins";
            ct_zec.pool_approximated_earnings_api_unit_factor_hashrate_ = 1;
            ct_zec.pool_approximated_earnings_api_unit_factor_period_ = 1;
#else
            currency_type ct_zec("ZEC", true);
            ct_zec.ccminer_algo_ = "equihash";
            ct_zec.pool_addresses_ = {"eu1-zcash.flypool.org:13333", "us1-zcash.flypool.org:13333",
                                      "asia1-zcash.flypool.org:13333"};
            ct_zec.whattomine_coin_id_ = 166;
            ct_zec.cryptocompare_fsym_ = "ZEC";
            ct_zec.pool_current_hashrate_api_address_ = "https://api-zcash.flypool.org/miner/%s/workers";
            ct_zec.pool_current_hashrate_json_path_worker_array_ = "data";
            ct_zec.pool_current_hashrate_json_path_worker_name_ = "worker";
            ct_zec.pool_current_hashrate_json_path_hashrate_ = "currentHashrate";
            ct_zec.pool_current_hashrate_api_unit_factor_hashrate_ = 1;
#endif
            res.emplace(ct_zec.currency_name_, ct_zec);
        }
        {
            currency_type ct_xmr("XMR", false);
            ct_xmr.bench_script_path_ = "../scripts/run_benchmark_xmr_xmrstak.sh";
            ct_xmr.mining_script_path_ = "../scripts/start_mining_xmr_xmrstak.sh";
            ct_xmr.pool_addresses_ = {"xmr-eu2.nanopool.org:14444"};
            ct_xmr.whattomine_coin_id_ = 101;
            ct_xmr.cryptocompare_fsym_ = "XMR";
            ct_xmr.pool_avg_hashrate_api_address_ = "https://api.nanopool.org/v1/xmr/avghashrateworkers/%s/%s";
            ct_xmr.pool_avg_hashrate_json_path_worker_array_ = "data";
            ct_xmr.pool_avg_hashrate_json_path_worker_name_ = "worker";
            ct_xmr.pool_avg_hashrate_json_path_hashrate_ = "hashrate";
            ct_xmr.pool_avg_hashrate_api_unit_factor_hashrate_ = 1;
            ct_xmr.pool_avg_hashrate_api_unit_factor_period_ = 3600 * 1000;
            ct_xmr.pool_approximated_earnings_api_address_ = "https://api.nanopool.org/v1/xmr/approximated_earnings/%s";
            ct_xmr.pool_approximated_earnings_json_path_ = "data.hour.coins";
            ct_xmr.pool_approximated_earnings_api_unit_factor_hashrate_ = 1;
            ct_xmr.pool_approximated_earnings_api_unit_factor_period_ = 1;
            res.emplace(ct_xmr.currency_name_, ct_xmr);
        }
        {
            currency_type ct_lux("LUX", true);
            ct_lux.ccminer_algo_ = "phi2";
            ct_lux.pool_addresses_ = {"phi2.mine.zergpool.com:8332"};
            ct_lux.pool_pass_ = "c=LUX,mc=LUX";
            ct_lux.whattomine_coin_id_ = 212;
            ct_lux.cryptocompare_fsym_ = "LUX";
            ct_lux.pool_current_hashrate_api_address_ = "http://api.zergpool.com:8080/api/walletEx?address=%s";
            ct_lux.pool_current_hashrate_json_path_worker_array_ = "miners";
            ct_lux.pool_current_hashrate_json_path_worker_name_ = "ID";
            ct_lux.pool_current_hashrate_json_path_hashrate_ = "accepted";
            ct_lux.pool_current_hashrate_api_unit_factor_hashrate_ = 1;
            res.emplace(ct_lux.currency_name_, ct_lux);
        }
        {
            currency_type ct_btx("BTX", true);
            ct_btx.ccminer_algo_ = "bitcore";
            ct_btx.pool_addresses_ = {"bitcore.mine.zergpool.com:3556"};
            ct_btx.pool_pass_ = "c=BTX,mc=BTX";
            ct_btx.whattomine_coin_id_ = 202;
            ct_btx.cryptocompare_fsym_ = "BTX";
            ct_btx.pool_current_hashrate_api_address_ = "http://api.zergpool.com:8080/api/walletEx?address=%s";
            ct_btx.pool_current_hashrate_json_path_worker_array_ = "miners";
            ct_btx.pool_current_hashrate_json_path_worker_name_ = "ID";
            ct_btx.pool_current_hashrate_json_path_hashrate_ = "accepted";
            ct_btx.pool_current_hashrate_api_unit_factor_hashrate_ = 1;
            res.emplace(ct_btx.currency_name_, ct_btx);
        }
        {
            currency_type ct_vtc("VTC", true);
            ct_vtc.ccminer_algo_ = "lyra2v2";
            ct_vtc.pool_addresses_ = {"lyra2v2.mine.zergpool.com:4533"};
            ct_vtc.pool_pass_ = "c=VTC,mc=VTC";
            ct_vtc.whattomine_coin_id_ = 5;
            ct_vtc.cryptocompare_fsym_ = "VTC";
            ct_vtc.pool_current_hashrate_api_address_ = "http://api.zergpool.com:8080/api/walletEx?address=%s";
            ct_vtc.pool_current_hashrate_json_path_worker_array_ = "miners";
            ct_vtc.pool_current_hashrate_json_path_worker_name_ = "ID";
            ct_vtc.pool_current_hashrate_json_path_hashrate_ = "accepted";
            ct_vtc.pool_current_hashrate_api_unit_factor_hashrate_ = 1;
            res.emplace(ct_vtc.currency_name_, ct_vtc);
        }
        {
            currency_type ct_rvn("RVN", true);
            ct_rvn.ccminer_algo_ = "x16r";
            ct_rvn.pool_addresses_ = {"x16r.mine.zergpool.com:3636"};
            ct_rvn.pool_pass_ = "c=RVN,mc=RVN";
            ct_rvn.whattomine_coin_id_ = 234;
            ct_rvn.cryptocompare_fsym_ = "RVN";
            ct_rvn.pool_current_hashrate_api_address_ = "http://api.zergpool.com:8080/api/walletEx?address=%s";
            ct_rvn.pool_current_hashrate_json_path_worker_array_ = "miners";
            ct_rvn.pool_current_hashrate_json_path_worker_name_ = "ID";
            ct_rvn.pool_current_hashrate_json_path_hashrate_ = "accepted";
            ct_rvn.pool_current_hashrate_api_unit_factor_hashrate_ = 1;
            res.emplace(ct_rvn.currency_name_, ct_rvn);
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
            currency_type ct(pt_currency_type.get<std::string>("currency_name"),
                             pt_currency_type.get<bool>("use_ccminer"));
            if (ct.use_ccminer_) {
                ct.ccminer_algo_ = pt_currency_type.get<std::string>("ccminer_algo");
            } else {
                ct.bench_script_path_ = pt_currency_type.get<std::string>("bench_script_path");
                ct.mining_script_path_ = pt_currency_type.get<std::string>("mining_script_path");
            }
            for (const pt::ptree::value_type &pt_pool : pt_currency_type.get_child("pool_addresses")) {
                ct.pool_addresses_.emplace_back(pt_pool.second.data());
            }
            ct.pool_pass_ = pt_currency_type.get<std::string>("pool_pass", "x");
            if (ct.pool_addresses_.empty())
                THROW_RUNTIME_ERROR("Currency config " + filename + " : No pool specified for " + ct.currency_name_);
            ct.whattomine_coin_id_ = pt_currency_type.get<int>("whattomine_coin_id");
            //optional fields
            ct.cryptocompare_fsym_ = pt_currency_type.get<std::string>("cryptocompare_fsym", ct.currency_name_);
            ct.pool_avg_hashrate_api_address_ = pt_currency_type.get<std::string>("pool_avg_hashrate_api_address", "");
            ct.pool_avg_hashrate_json_path_worker_array_ = pt_currency_type.get<std::string>(
                    "pool_avg_hashrate_json_path_worker_array", "");
            ct.pool_avg_hashrate_json_path_worker_name_ = pt_currency_type.get<std::string>(
                    "pool_avg_hashrate_json_path_worker_name", "");
            ct.pool_avg_hashrate_json_path_hashrate_ = pt_currency_type.get<std::string>(
                    "pool_avg_hashrate_json_path_hashrate", "");
            ct.pool_avg_hashrate_api_unit_factor_hashrate_ = pt_currency_type.get<double>(
                    "pool_avg_hashrate_api_unit_factor_hashrate", -1);
            ct.pool_avg_hashrate_api_unit_factor_period_ = pt_currency_type.get<double>(
                    "pool_avg_hashrate_api_unit_factor_period", -1);
            ct.pool_current_hashrate_api_address_ = pt_currency_type.get<std::string>(
                    "pool_current_hashrate_api_address", "");
            ct.pool_current_hashrate_json_path_worker_array_ = pt_currency_type.get<std::string>(
                    "pool_current_hashrate_json_path_worker_array", "");
            ct.pool_current_hashrate_json_path_worker_name_ = pt_currency_type.get<std::string>(
                    "pool_current_hashrate_json_path_worker_name", "");
            ct.pool_current_hashrate_json_path_hashrate_ = pt_currency_type.get<std::string>(
                    "pool_current_hashrate_json_path_hashrate", "");
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
        //write to logdir
        write_currency_config(log_utils::get_autosave_currency_config_filename(), currency_config);
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
            pt_currency_type.put("use_ccminer", ct.use_ccminer_);
            pt_currency_type.put("ccminer_algo", ct.ccminer_algo_);
            pt_currency_type.put("bench_script_path", ct.bench_script_path_);
            pt_currency_type.put("mining_script_path", ct.mining_script_path_);
            pt::ptree pt_pool_array;
            for (auto &pool : ct.pool_addresses_) {
                pt::ptree pt_pool;
                pt_pool.put("", pool);
                pt_pool_array.push_back(std::make_pair("", pt_pool));
            }
            pt_currency_type.add_child("pool_addresses", pt_pool_array);
            pt_currency_type.put("pool_pass", ct.pool_pass_);
            pt_currency_type.put("whattomine_coin_id", ct.whattomine_coin_id_);
            //optional fields
            pt_currency_type.put("cryptocompare_fsym", ct.cryptocompare_fsym_);
            pt_currency_type.put("pool_avg_hashrate_api_address", ct.pool_avg_hashrate_api_address_);
            pt_currency_type.put("pool_avg_hashrate_json_path_worker_array",
                                 ct.pool_avg_hashrate_json_path_worker_array_);
            pt_currency_type.put("pool_avg_hashrate_json_path_worker_name",
                                 ct.pool_avg_hashrate_json_path_worker_name_);
            pt_currency_type.put("pool_avg_hashrate_json_path_hashrate", ct.pool_avg_hashrate_json_path_hashrate_);
            pt_currency_type.put("pool_avg_hashrate_api_unit_factor_hashrate",
                                 ct.pool_avg_hashrate_api_unit_factor_hashrate_);
            pt_currency_type.put("pool_avg_hashrate_api_unit_factor_period",
                                 ct.pool_avg_hashrate_api_unit_factor_period_);
            pt_currency_type.put("pool_current_hashrate_api_address", ct.pool_current_hashrate_api_address_);
            pt_currency_type.put("pool_current_hashrate_json_path_worker_array",
                                 ct.pool_current_hashrate_json_path_worker_array_);
            pt_currency_type.put("pool_current_hashrate_json_path_worker_name",
                                 ct.pool_current_hashrate_json_path_worker_name_);
            pt_currency_type.put("pool_current_hashrate_json_path_hashrate",
                                 ct.pool_current_hashrate_json_path_hashrate_);
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

