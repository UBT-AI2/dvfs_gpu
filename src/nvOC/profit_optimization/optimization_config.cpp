//
// Created by Alex on 10.06.2018.
//

#include "optimization_config.h"
#include <iostream>
#include <fstream>
#include <limits>
#include <regex>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "../nvml/nvmlOC.h"


namespace frequency_scaling{

    optimization_method_params::optimization_method_params(optimization_method method, int min_hashrate) :
    method_(method), min_hashrate_(min_hashrate) {
        switch (method) {
            case optimization_method::NELDER_MEAD:
                max_iterations_ = 8;
                mem_step_ = 300;
                graph_idx_step_ = 10;
                break;
            case optimization_method::HILL_CLIMBING:
            case optimization_method::SIMULATED_ANNEALING:
                max_iterations_ = 6;
                mem_step_ = 300;
                graph_idx_step_ = 10;
                break;
            default:
                throw std::runtime_error("Invalid enum value");
        }
    }

    optimization_method_params::optimization_method_params(optimization_method method, int max_iterations, int mem_step,
    int graph_idx_step, int min_hashrate) : method_(method),
    max_iterations_(max_iterations),
    mem_step_(mem_step),
    graph_idx_step_(graph_idx_step),
    min_hashrate_(min_hashrate) {}



    static std::string cli_get_string(const std::string& user_msg,
                                      const std::string& regex_to_match){
		std::cout << user_msg << std::endl;
        while(true){
            std::string str;
            std::cin >> str;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if(std::regex_match(str, std::regex(regex_to_match)))
                return str;
			std::cout << "Invalid input. Try again." << std::endl;
        }
    }


    static int cli_get_int(const std::string& user_msg){
		return std::stoi(cli_get_string(user_msg, "[+-]?[[:digit:]]+"));
    }

	static double cli_get_float(const std::string& user_msg) {
		return std::stod(cli_get_string(user_msg, "[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?"));
	}


    optimization_config get_config_user_dialog() {
        optimization_config opt_config;
        //input energy costs
        //double energy_cost_kwh = get_energy_cost_stromdao(95440);
        double energy_cost_kwh = cli_get_float("Enter energy cost in euro per kwh:");
        opt_config.energy_cost_kwh_ = energy_cost_kwh;
        //select monitoring interval
        int monitoring_interval_sec = cli_get_int("Enter miner monitoring interval in seconds:");
        opt_config.monitoring_interval_sec_ = monitoring_interval_sec;
        //select GPUs to mine on
        int device_count = nvmlGetNumDevices();
        for (int device_id = 0; device_id < device_count; device_id++) {
            std::string user_msg = "Mine on GPU " + std::to_string(device_id) +
                    " (" + nvmlGetDeviceName(device_id) + ")? [y/n]";
            std::string user_in = cli_get_string(user_msg, "[yn]");
            if (user_in != "y")
                continue;
            user_msg = "Enter min_mem_oc:";
            int min_mem_oc = cli_get_int(user_msg);
            user_msg = "Enter max_mem_oc:";
            int max_mem_oc = cli_get_int(user_msg);
            opt_config.dcis_.emplace_back(device_id, min_mem_oc, 0, max_mem_oc, 0);
        }
		if (opt_config.dcis_.empty())
			throw std::runtime_error("No device selected");
        //select currencies to mine
        for (int i = 0; i < static_cast<int>(currency_type::count); i++) {
            currency_type ct = static_cast<currency_type>(i);
            std::string user_msg = "Include currency " + enum_to_string(ct) + "? [y/n]";
            std::string user_in = cli_get_string(user_msg, "[yn]");
            if (user_in != "y")
                continue;
            user_msg = "Enter mining user info in the format <wallet_address/worker_name[/e-mail]>:";
            //^[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,6}$
            user_in = cli_get_string(user_msg, "[a-zA-Z0-9]+/\\w+(/\\w+(\\.\\w+)?@\\w+\\.[a-zA-Z]{2,6})?");
            opt_config.miner_user_infos_.emplace(ct, miner_user_info(user_in));
            //choose optimization method
            user_msg = "Select method used to optimize energy-hash ratio: [NM/HC/SA]";
            user_in = cli_get_string(user_msg, "NM|HC|SA");
            optimization_method opt_method = string_to_opt_method(user_in);
            //choose min_hashrate
            user_msg = "Enter min_hashrate or -1 if no min_hashrate constraint should be used:";
            int min_hashrate = cli_get_int(user_msg);
            opt_config.opt_method_params_.emplace(ct, optimization_method_params(opt_method, min_hashrate));
        }
		if (opt_config.miner_user_infos_.empty())
			throw std::runtime_error("No currency selected");
        //save config dialog
        std::string user_in = cli_get_string("Save configuration? [y/n]", "[yn]");
        if(user_in != "y")
            return opt_config;
        user_in = cli_get_string("Enter filename:", "\\w+\\.\\w+");
        write_config_json(user_in, opt_config);
        return opt_config;
    }


    void write_config_json(const std::string& filename, const optimization_config& opt_config){
        namespace pt = boost::property_tree;
        pt::ptree root;
        root.put("energy_cost", opt_config.energy_cost_kwh_);
        root.put("monitoring_interval", opt_config.monitoring_interval_sec_);
        //write devices to use
        pt::ptree devices_to_use;
        for(auto& dci : opt_config.dcis_) {
            pt::ptree pt_device;
            pt_device.put("index", dci.device_id_nvml);
            pt_device.put("min_mem_oc", dci.min_mem_oc);
            pt_device.put("max_mem_oc", dci.max_mem_oc);
            devices_to_use.push_back(std::make_pair("", pt_device));
        }
        root.add_child("devices_to_use", devices_to_use);
        //write currencies to use
        pt::ptree currencies_to_use;
        for(int i = 0; i < static_cast<int>(currency_type::count); i++){
            currency_type ct = static_cast<currency_type>(i);
            auto it_mui = opt_config.miner_user_infos_.find(ct);
            auto it_omp = opt_config.opt_method_params_.find(ct);
            if(it_mui == opt_config.miner_user_infos_.end() ||
               it_omp == opt_config.opt_method_params_.end())
                continue;
            //
            pt::ptree pt_currency;
            pt::ptree pt_miner_user_info, pt_opt_method_params;
            pt_miner_user_info.put("wallet_address", it_mui->second.wallet_address_);
            pt_miner_user_info.put("worker_name", it_mui->second.worker_name_);
            pt_miner_user_info.put("email", it_mui->second.email_adress_);
            pt_opt_method_params.put("method", enum_to_string(it_omp->second.method_));
            pt_opt_method_params.put("min_hashrate", it_omp->second.min_hashrate_);
            pt_currency.add_child("miner_user_info", pt_miner_user_info);
            pt_currency.add_child("opt_method_params", pt_opt_method_params);
            currencies_to_use.push_back(std::make_pair(enum_to_string(ct), pt_currency));
        }
        root.add_child("currencies_to_use", currencies_to_use);
        pt::write_json(filename, root);
    }

    optimization_config parse_config_json(const std::string& filename){
        namespace pt = boost::property_tree;
        pt::ptree root;
        pt::read_json(filename, root);
        optimization_config opt_config;
        opt_config.energy_cost_kwh_ = root.get<double>("energy_cost");
        opt_config.monitoring_interval_sec_ = root.get<int>("monitoring_interval");
        //read device infos
        for (const pt::ptree::value_type &array_elem : root.get_child("devices_to_use")) {
            const boost::property_tree::ptree &pt_device = array_elem.second;
            opt_config.dcis_.emplace_back(pt_device.get<int>("index"), pt_device.get<int>("min_mem_oc"),
                    0, pt_device.get<int>("max_mem_oc"), 0);
        }
        //read currencies to use
        for (const pt::ptree::value_type &array_elem : root.get_child("currencies_to_use")) {
            currency_type ct = string_to_currency_type(array_elem.first);
            const boost::property_tree::ptree &pt_miner_user_info = array_elem.second.get_child("miner_user_info");
            const boost::property_tree::ptree &pt_opt_method_params = array_elem.second.get_child("opt_method_params");
            opt_config.miner_user_infos_.emplace(ct, miner_user_info(pt_miner_user_info.get<std::string>("wallet_address"),
                                                                     pt_miner_user_info.get<std::string>("worker_name"),
                                                                     pt_miner_user_info.get<std::string>("email")));
            opt_config.opt_method_params_.emplace(ct, optimization_method_params(
                    string_to_opt_method(pt_opt_method_params.get<std::string>("method")),
                    pt_opt_method_params.get<int>("min_hashrate")));
        }
        return opt_config;
    }


    std::string enum_to_string(optimization_method opt_method){
        switch (opt_method) {
            case optimization_method::NELDER_MEAD:
                return "NM";
            case optimization_method::HILL_CLIMBING:
                return "HC";
            case optimization_method::SIMULATED_ANNEALING:
                return "SA";
            default:
                throw std::runtime_error("Invalid enum value");
        }
    }


    optimization_method string_to_opt_method(const std::string& str){
        if(str == "nm" || str == "NM")
            return optimization_method::NELDER_MEAD;
        else if(str == "hc" || str == "HC")
            return optimization_method::HILL_CLIMBING;
        else if(str == "sa" || str == "SA")
            return optimization_method::SIMULATED_ANNEALING;
        else
            throw std::runtime_error("Optimization method " + str + " not available");
    }

}
