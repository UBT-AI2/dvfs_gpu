#include "freq_exhaustive.h"
#include <limits>

namespace frequency_scaling {

    measurement freq_exhaustive(const benchmark_func &benchmarkFunc, const currency_type &ct,
                                const device_clock_info &dci, int mem_oc_interval, int clock_idx_interval) {
        measurement best_node;
        best_node.energy_hash_ = std::numeric_limits<double>::lowest();
        //for all supported mem clocks
        for (int mem_oc = dci.min_mem_oc_; mem_oc <= dci.max_mem_oc_; mem_oc += mem_oc_interval) {
            //for all supported graph clocks
            for (int j = dci.nvml_graph_clocks_.size() - 1; j >= 0; j -= clock_idx_interval) {
                const measurement &m = benchmarkFunc(ct, dci, mem_oc, j);
                if (m.energy_hash_ > best_node.energy_hash_)
                    best_node = m;
            }
        }

        return best_node;
    }

}
