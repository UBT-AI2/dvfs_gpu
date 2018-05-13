#pragma once

#include "../benchmark/benchmark.h"

namespace frequency_scaling {

    measurement freq_exhaustive(miner_script ms, const device_clock_info &dci, int nvapi_oc_interval,
                                bool use_nvmlUC, int nvml_clock_idx_interval);

}