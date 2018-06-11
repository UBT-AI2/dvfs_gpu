#pragma once

#include <string>
#include "../script_running/benchmark.h"
#include "../script_running/mining.h"
#include "profit_calculation.h"
#include "optimization_config.h"

namespace frequency_scaling {

    void mine_most_profitable_currency(const optimization_config& opt_config);

}