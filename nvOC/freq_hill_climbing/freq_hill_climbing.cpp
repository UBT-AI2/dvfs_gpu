#include <stdlib.h>
#include <vector>
#include "../benchmark.h"


namespace frequency_scaling {

    static std::vector<measurement> explore_neighborhood(const device_clock_info &dci, const measurement &current_node,
                                                         int mem_step, int graph_step_idx) {
        std::vector<measurement> neighbor_nodes;
        int mem_plus = current_node.mem_oc + mem_step;
        int mem_minus = current_node.mem_oc - mem_step;
        int graph_plus_idx = current_node.nvml_graph_clock_idx + graph_step_idx;
        int graph_minus_idx = current_node.nvml_graph_clock_idx - graph_step_idx;
        //oc memory
        if (mem_plus <= dci.max_mem_oc)
            neighbor_nodes.push_back(run_benchmark_nvml_nvapi(dci, mem_plus, current_node.nvml_graph_clock_idx));
        if (mem_minus >= dci.min_mem_oc)
            neighbor_nodes.push_back(run_benchmark_nvml_nvapi(dci, mem_minus, current_node.nvml_graph_clock_idx));

        //oc graphics
        if (graph_plus_idx < dci.nvml_graph_clocks.size())
            neighbor_nodes.push_back(run_benchmark_nvml_nvapi(dci, current_node.mem_oc, graph_plus_idx));
        if (graph_minus_idx >= 0)
            neighbor_nodes.push_back(run_benchmark_nvml_nvapi(dci, current_node.mem_oc, graph_minus_idx));

        return neighbor_nodes;
    }


    measurement freq_hill_climbing(const device_clock_info &dci, int max_iterations,
                                   int mem_step, int graph_idx_step) {
        //initial guess
        measurement current_node = run_benchmark_nvml_nvapi(dci, (dci.max_mem_oc - dci.min_mem_oc) / 2,
                                                 dci.nvml_graph_clocks.size() / 2);
        measurement best_node = current_node;
        //exploration
        for (int i = 0; i < max_iterations; i++) {
            const std::vector<measurement> &neighbors = explore_neighborhood(dci, current_node, mem_step,
                                                                             graph_idx_step);
            float tmp_val = -1e37;
            for (measurement n : neighbors) {
                if (n.energy_hash_ > tmp_val) {
                    current_node = n;
                    tmp_val = n.energy_hash_;
                }
            }
            if (current_node.energy_hash_ > best_node.energy_hash_) {
                best_node = current_node;
            }
        }

        return best_node;
    }

}