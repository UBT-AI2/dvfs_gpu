//
// Created by Alex on 10.06.2018.
//
#include "optimization_config.h"
#include <fstream>
#include <limits>
#include <glog/logging.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#else

#include <unistd.h>

#endif

#include "../nvml/nvmlOC.h"
#include "../common_header/exceptions.h"
#include "../common_header/constants.h"
#include "cli_utils.h"
#include "log_utils.h"


namespace frequency_scaling {

    optimization_method_params::optimization_method_params(optimization_method method, double min_hashrate_pct) :
            method_(method), min_hashrate_pct(min_hashrate_pct) {
        switch (method) {
            case optimization_method::NELDER_MEAD:
                max_iterations_ = 8;
                mem_step_pct_ = 0.15;
                graph_idx_step_pct_ = 0.15;
                break;
            case optimization_method::HILL_CLIMBING:
            case optimization_method::SIMULATED_ANNEALING:
                max_iterations_ = 6;
                mem_step_pct_ = 0.15;
                graph_idx_step_pct_ = 0.15;
                break;
            default:
                THROW_RUNTIME_ERROR("Invalid enum value");
        }
    }

    optimization_method_params::optimization_method_params(optimization_method method, int max_iterations,
                                                           double mem_step_pct,
                                                           double graph_idx_step_pct, double min_hashrate_pct)
            : method_(method),
              max_iterations_(
                      max_iterations),
              mem_step_pct_(mem_step_pct),
              graph_idx_step_pct_(
                      graph_idx_step_pct),
              min_hashrate_pct(
                      min_hashrate_pct) {}


    optimization_config get_config_user_dialog(const std::map<std::string, currency_type> &available_currencies) {
        optimization_config opt_config;
        std::string user_in, user_msg;
        //input energy costs
        //double energy_cost_kwh = network_requests::network_requests::get_energy_cost_stromdao(95440);
        double energy_cost_kwh = cli_get_float("Enter energy cost in euro per kwh:");
        opt_config.energy_cost_kwh_ = energy_cost_kwh;
        //select monitoring interval
        int monitoring_interval_sec = cli_get_int("Enter miner monitoring interval in seconds:");
        opt_config.monitoring_interval_sec_ = monitoring_interval_sec;
        user_in = cli_get_string("Skip offline phase during frequency optimization? [y/n]", "[yn]");
        opt_config.skip_offline_phase_ = user_in == "y";
        //
        user_in = cli_get_string("Let miner notify you via email? [y/n]", "[yn]");
        if (user_in == "y") {
            user_in = cli_get_string("Enter email address:", "[\\w-.]+@[\\w-.]+\\.[a-zA-Z]{2,6}");
            opt_config.miner_user_infos_.email_adress_ = user_in;
        }
        //select GPUs to mine on
        int device_count = nvmlGetNumDevices();
        for (int device_id = 0; device_id < device_count; device_id++) {
            user_msg = "Mine on GPU " + std::to_string(device_id) +
                       " (" + nvmlGetDeviceName(device_id) + ")? [y/n]";
            user_in = cli_get_string(user_msg, "[yn]");
            if (user_in != "y")
                continue;
            opt_config.dcis_.emplace_back(device_id);
            opt_config.miner_user_infos_.worker_names_.emplace(device_id, getworker_name(device_id));
        }
        if (opt_config.dcis_.empty())
            THROW_RUNTIME_ERROR("No device selected");

        //select currencies to mine
        for (auto &elem : available_currencies) {
            const currency_type &ct = elem.second;
            user_msg = "Include currency " + ct.currency_name_ + "? [y/n]";
            user_in = cli_get_string(user_msg, "[yn]");
            if (user_in != "y")
                continue;
            user_msg = "Enter wallet_address:";
            user_in = cli_get_string(user_msg, "[a-zA-Z0-9]+");
            opt_config.miner_user_infos_.wallet_addresses_.emplace(ct, user_in);
            //choose optimization method
            user_msg = "Select method used to optimize energy-hash ratio: [NM/HC/SA]";
            user_in = cli_get_string(user_msg, "NM|HC|SA");
            optimization_method opt_method = string_to_opt_method(user_in);
            //choose min_hashrate
            user_msg = "Enter min_hashrate or -1 if no min_hashrate constraint should be used:";
            int min_hashrate = cli_get_int(user_msg);
            opt_config.opt_method_params_.emplace(ct, optimization_method_params(opt_method, min_hashrate));
        }
        if (opt_config.miner_user_infos_.wallet_addresses_.empty())
            THROW_RUNTIME_ERROR("No currency selected");
        //save config dialog
        user_in = cli_get_string("Save configuration? [y/n]", "[yn]");
        if (user_in == "y") {
            user_in = cli_get_string("Enter filename:", "[\\w-.]+");
            write_config_json(user_in, opt_config);
        }
        //always save to logdir
        write_config_json(log_utils::get_autosave_user_config_filename(), opt_config);
        return opt_config;
    }


    void write_config_json(const std::string &filename, const optimization_config &opt_config) {
        namespace pt = boost::property_tree;
        pt::ptree root;
        root.put("energy_cost", opt_config.energy_cost_kwh_);
        root.put("monitoring_interval", opt_config.monitoring_interval_sec_);
        root.put("online_bench_duration", opt_config.online_bench_duration_sec_);
        root.put("skip_offline_phase", opt_config.skip_offline_phase_);
        root.put("email", opt_config.miner_user_infos_.email_adress_);
        //write devices to use
        pt::ptree devices_to_use;
        for (auto &dci : opt_config.dcis_) {
            pt::ptree pt_device;
            pt_device.put("index", dci.device_id_nvml_);
            pt_device.put("name", nvmlGetDeviceName(dci.device_id_nvml_));
            pt_device.put("min_mem_oc", dci.min_mem_oc_);
            pt_device.put("min_graph_oc", dci.min_graph_oc_);
            pt_device.put("max_mem_oc", dci.max_mem_oc_);
            pt_device.put("max_graph_oc", dci.max_graph_oc_);
            pt_device.put("worker_name", opt_config.miner_user_infos_.worker_names_.at(dci.device_id_nvml_));
            devices_to_use.push_back(std::make_pair("", pt_device));
        }
        root.add_child("devices_to_use", devices_to_use);
        //write currencies to use
        pt::ptree currencies_to_use;
        for (auto &elem : opt_config.miner_user_infos_.wallet_addresses_) {
            const currency_type &ct = elem.first;
            if (!opt_config.opt_method_params_.count(ct))
                THROW_RUNTIME_ERROR("Invalid opt_config: no opt_method_params for " + ct.currency_name_ +
                                    " wallet_address available");
            const optimization_method_params &opt_params = opt_config.opt_method_params_.at(ct);
            //
            pt::ptree pt_currency;
            pt::ptree pt_opt_method_params;
            pt_currency.put("wallet_address", elem.second);
            pt_opt_method_params.put("method", enum_to_string(opt_params.method_));
            pt_opt_method_params.put("min_hashrate", opt_params.min_hashrate_pct);
            pt_opt_method_params.put("max_iterations", opt_params.max_iterations_);
            pt_opt_method_params.put("mem_step", opt_params.mem_step_pct_);
            pt_opt_method_params.put("graph_idx_step", opt_params.graph_idx_step_pct_);
            pt_currency.add_child("opt_method_params", pt_opt_method_params);
            currencies_to_use.push_back(std::make_pair(ct.currency_name_, pt_currency));
        }
        root.add_child("currencies_to_use", currencies_to_use);
        pt::write_json(filename, root);
    }

    optimization_config
    parse_config_json(const std::string &filename,
                      const std::map<std::string, currency_type> &available_currencies) {
        namespace pt = boost::property_tree;
        pt::ptree root;
        pt::read_json(filename, root);
        optimization_config opt_config;
        opt_config.energy_cost_kwh_ = root.get<double>("energy_cost");
        opt_config.monitoring_interval_sec_ = root.get<int>("monitoring_interval");
        opt_config.online_bench_duration_sec_ = root.get<int>("online_bench_duration",
                                                              opt_config.online_bench_duration_sec_);
        opt_config.skip_offline_phase_ = root.get<bool>("skip_offline_phase", opt_config.skip_offline_phase_);
        opt_config.miner_user_infos_.email_adress_ = root.get<std::string>("email", "");
        //read device infos
        for (const pt::ptree::value_type &array_elem : root.get_child("devices_to_use")) {
            const boost::property_tree::ptree &pt_device = array_elem.second;
            int device_id = pt_device.get<int>("index");
            if (device_id >= nvmlGetNumDevices()) {
                LOG(WARNING) << "Invalid opt_config: GPU " << device_id << " does not exist. Skipping GPU..."
                             << std::endl;
                continue;
            }
            if (pt_device.get<std::string>("name") == nvmlGetDeviceName(device_id)) {
                opt_config.dcis_.emplace_back(device_id, pt_device.get<int>("min_mem_oc", 1),
                                              pt_device.get<int>("min_graph_oc", 1),
                                              pt_device.get<int>("max_mem_oc", -1),
                                              pt_device.get<int>("max_graph_oc", -1));
            } else {
                LOG(WARNING) << "Invalid opt_config: GPU " << device_id
                             << " has different type. Creating default dci..."
                             << std::endl;
                opt_config.dcis_.emplace_back(device_id);
            }
            opt_config.miner_user_infos_.worker_names_.emplace(device_id,
                                                               pt_device.get<std::string>("worker_name"));
        }
        if (opt_config.dcis_.empty())
            THROW_RUNTIME_ERROR("No device available");
        //read currencies to use
        for (const pt::ptree::value_type &array_elem : root.get_child("currencies_to_use")) {
            if (!available_currencies.count(array_elem.first)) {
                LOG(WARNING) << "Invalid opt_config: Currency " + array_elem.first +
                                " not available in currency config. Skipping currency..." << std::endl;
                continue;
            }
            const currency_type &ct = available_currencies.at(array_elem.first);
            const boost::property_tree::ptree &pt_currency = array_elem.second;
            const boost::property_tree::ptree &pt_opt_method_params = array_elem.second.get_child(
                    "opt_method_params");
            opt_config.miner_user_infos_.wallet_addresses_.emplace(ct,
                                                                   pt_currency.get<std::string>("wallet_address"));
            optimization_method_params opt_method_params(
                    string_to_opt_method(pt_opt_method_params.get<std::string>("method")),
                    pt_opt_method_params.get<double>("min_hashrate"));
            opt_method_params.max_iterations_ = pt_opt_method_params.get<int>("max_iterations",
                                                                              opt_method_params.max_iterations_);
            opt_method_params.mem_step_pct_ = pt_opt_method_params.get<double>("mem_step",
                                                                               opt_method_params.mem_step_pct_);
            opt_method_params.graph_idx_step_pct_ = pt_opt_method_params.get<double>("graph_idx_step",
                                                                                     opt_method_params.graph_idx_step_pct_);
            opt_config.opt_method_params_.emplace(ct, opt_method_params);
        }
        if (opt_config.miner_user_infos_.wallet_addresses_.empty())
            THROW_RUNTIME_ERROR("No currency available");
        //always save to logdir
        write_config_json(log_utils::get_autosave_user_config_filename(), opt_config);
        return opt_config;
    }

    std::string getworker_name(int device_id) {
        char buf[BUFFER_SIZE];
#ifdef _WIN32
        DWORD size = sizeof(buf);
        GetComputerName(buf, &size);
#else
        gethostname(buf, sizeof(buf));
#endif
        return buf + std::string("_gpu") + std::to_string(device_id);
    }

    std::string enum_to_string(optimization_method opt_method) {
        switch (opt_method) {
            case optimization_method::NELDER_MEAD:
                return "NM";
            case optimization_method::HILL_CLIMBING:
                return "HC";
            case optimization_method::SIMULATED_ANNEALING:
                return "SA";
            default:
                THROW_RUNTIME_ERROR("Invalid enum value");
        }
    }


    optimization_method string_to_opt_method(const std::string &str) {
        if (str == "nm" || str == "NM")
            return optimization_method::NELDER_MEAD;
        else if (str == "hc" || str == "HC")
            return optimization_method::HILL_CLIMBING;
        else if (str == "sa" || str == "SA")
            return optimization_method::SIMULATED_ANNEALING;
        else
            THROW_RUNTIME_ERROR("Optimization method " + str + " not available");
    }

}
