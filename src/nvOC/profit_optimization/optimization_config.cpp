//
// Created by Alex on 10.06.2018.
//

#include "optimization_config.h"
#include <iostream>
#include <limits>
#include <regex>
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
            user_msg = "Select method used to optimize energy-hash ratio: [nm/hc/sa]";
            user_in = cli_get_string(user_msg, "nm|hc|sa");
            optimization_method opt_method = optimization_method::NELDER_MEAD;
            if (user_in == "hc")
                opt_method = optimization_method::HILL_CLIMBING;
            else if(user_in == "sa")
                opt_method = optimization_method::SIMULATED_ANNEALING;
            user_msg = "Enter min_hashrate or -1 if no min_hashrate constraint should be used:";
            int min_hashrate = cli_get_int(user_msg);
            opt_config.opt_method_params_.emplace(ct, optimization_method_params(opt_method, min_hashrate));
        }
		if (opt_config.miner_user_infos_.empty())
			throw std::runtime_error("No currency selected");
        //input energy costs
		//double energy_cost_kwh = get_energy_cost_stromdao(95440);
        double energy_cost_kwh = cli_get_float("Enter energy cost in euro per kwh:");
        opt_config.energy_cost_kwh_ = energy_cost_kwh;
        //select monitoring interval
        int monitoring_interval_sec = cli_get_int("Enter miner monitoring interval in seconds:");
        opt_config.monitoring_interval_sec_ = monitoring_interval_sec;
        return opt_config;
    }


    void write_config_json(const std::string& filename, const optimization_config& opt_config){

    }

    optimization_config parse_config_json(const std::string& filename){
        return optimization_config();
    }

}
