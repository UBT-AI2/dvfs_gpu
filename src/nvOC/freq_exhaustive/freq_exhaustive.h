#pragma once

#include "../script_running/benchmark.h"

namespace frequency_scaling {

    measurement freq_exhaustive(const benchmark_func &benchmarkFunc, const currency_type &ct,
                                const device_clock_info &dci, int mem_oc_interval, int clock_idx_interval);

}