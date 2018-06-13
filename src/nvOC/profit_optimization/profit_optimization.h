#pragma once

#include <string>
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"
#include "profit_calculation.h"
#include "optimization_config.h"

namespace frequency_scaling {

    void mine_most_profitable_currency(const optimization_config &opt_config);

	void save_optimization_result(const std::string& filename,
		const std::map<int, std::map<currency_type, energy_hash_info>>& opt_result);

	std::map<int, std::map<currency_type, energy_hash_info>> 
		load_optimization_result(const std::string& filename);

}