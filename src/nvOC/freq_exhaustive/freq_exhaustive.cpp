#include "freq_exhaustive.h"


namespace frequency_scaling {

    measurement freq_exhaustive(currency_type ms, const device_clock_info &dci, int nvapi_oc_interval,
                                bool use_nvmlUC, int nvml_clock_idx_interval) {
        int min_graph_oc = dci.min_graph_oc;
        if (use_nvmlUC)
            min_graph_oc = nvapi_oc_interval;

        measurement best_node;
        best_node.energy_hash_ = -1e37;
        //
        //for all supported mem clocks (nvapi)
        for (int mem_oc = dci.min_mem_oc; mem_oc <= dci.max_mem_oc; mem_oc += nvapi_oc_interval) {

            if (use_nvmlUC) {
                //for all supported graph clocks (nvml) -> underclocking
                for (int j = dci.nvml_graph_clocks.size() - 1; j >= 0; j -= nvml_clock_idx_interval) {
                    const measurement &m = run_benchmark_script_nvml_nvapi(ms, dci, mem_oc, j);
                    if (m.energy_hash_ > best_node.energy_hash_)
                        best_node = m;
                }
            }

            //for all supported graph clocks (nvapi) -> overclocking
            for (int graph_oc = min_graph_oc; graph_oc <= dci.max_graph_oc; graph_oc += nvapi_oc_interval) {
                const measurement &m = run_benchmark_script_nvapi_only(ms, dci, mem_oc, graph_oc);
                if (m.energy_hash_ > best_node.energy_hash_)
                    best_node = m;
            }
        }

        return best_node;
    }

}
