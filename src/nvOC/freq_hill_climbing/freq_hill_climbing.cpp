#include "freq_hill_climbing.h"

#include <vector>
#include <random>
#include <chrono>
#include "../benchmark/benchmark.h"


namespace frequency_scaling {

    static std::vector<measurement>
    explore_neighborhood(miner_script ms, const device_clock_info &dci, const measurement &current_node,
                         int mem_step, int graph_step_idx, bool use8Neighborhood) {
        std::vector<measurement> neighbor_nodes;
        int mem_plus = current_node.mem_oc + mem_step;
        int mem_minus = current_node.mem_oc - mem_step;
        int graph_plus_idx = current_node.nvml_graph_clock_idx + graph_step_idx;
        int graph_minus_idx = current_node.nvml_graph_clock_idx - graph_step_idx;

        if(use8Neighborhood) {
            //horizontal/vertical 4 neighborhood
            if (mem_plus <= dci.max_mem_oc)
                neighbor_nodes.push_back(
                        run_benchmark_script_nvml_nvapi(ms, dci, mem_plus, current_node.nvml_graph_clock_idx));
            if (mem_minus >= dci.min_mem_oc)
                neighbor_nodes.push_back(
                        run_benchmark_script_nvml_nvapi(ms, dci, mem_minus, current_node.nvml_graph_clock_idx));
            if (graph_plus_idx < dci.nvml_graph_clocks.size())
                neighbor_nodes.push_back(run_benchmark_script_nvml_nvapi(ms, dci, current_node.mem_oc, graph_plus_idx));
            if (graph_minus_idx >= 0)
                neighbor_nodes.push_back(
                        run_benchmark_script_nvml_nvapi(ms, dci, current_node.mem_oc, graph_minus_idx));
        }

        //diagonal 4 neighborhood
        if (mem_plus <= dci.max_mem_oc && graph_plus_idx < dci.nvml_graph_clocks.size())
            neighbor_nodes.push_back(
                    run_benchmark_script_nvml_nvapi(ms, dci, mem_plus, graph_plus_idx));
        if (mem_minus >= dci.min_mem_oc && graph_minus_idx >= 0)
            neighbor_nodes.push_back(
                    run_benchmark_script_nvml_nvapi(ms, dci, mem_minus, graph_minus_idx));
        if (mem_minus >= dci.min_mem_oc && graph_plus_idx < dci.nvml_graph_clocks.size())
            neighbor_nodes.push_back(
                    run_benchmark_script_nvml_nvapi(ms, dci, mem_minus, graph_plus_idx));
        if (mem_plus <= dci.max_mem_oc && graph_minus_idx >= 0)
            neighbor_nodes.push_back(
                    run_benchmark_script_nvml_nvapi(ms, dci, mem_plus, graph_minus_idx));


        return neighbor_nodes;
    }


    measurement freq_hill_climbing(miner_script ms, const device_clock_info &dci, int max_iterations,
                                   int mem_step, int graph_idx_step) {
        //get random numbers
        int initial_graph_idx, initial_mem_oc;
        {
            std::default_random_engine eng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<int> distr_graph(0, dci.nvml_graph_clocks.size() - 1);
            std::uniform_int_distribution<int> distr_mem(0, (dci.max_mem_oc - dci.min_mem_oc) / mem_step);
            initial_graph_idx = distr_graph(eng);
            initial_mem_oc = dci.min_mem_oc + distr_mem(eng) * mem_step;
        }

        //initial random guess
        measurement current_node = run_benchmark_script_nvml_nvapi(ms, dci, initial_mem_oc, initial_graph_idx);
        measurement best_node = current_node;

        //exploration
        for (int i = 0; i < max_iterations; i++) {
            const std::vector<measurement> &neighbors = explore_neighborhood(ms, dci, current_node, mem_step,
                                                                             graph_idx_step, false);
            float tmp_val = -1e37;
            for (const measurement& n : neighbors) {
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