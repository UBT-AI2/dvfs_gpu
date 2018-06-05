//
// Created by alex on 28.05.18.
//

#include "freq_simulated_annealing.h"
#include <cmath>
#include <random>
#include <chrono>
#include "../freq_hill_climbing/freq_hill_climbing.h"
#include "../exceptions.h"


namespace frequency_scaling {

    measurement freq_simulated_annealing(miner_script ms, const device_clock_info &dci, int max_iterations,
                                         int mem_step, int graph_idx_step, double min_hashrate) {
        //initial guess at maximum frequencies
        int initial_graph_idx = 0, initial_mem_oc = dci.max_mem_oc;
        const measurement& max_node = run_benchmark_script_nvml_nvapi(ms, dci, initial_mem_oc, initial_graph_idx);
        const measurement& min_node = run_benchmark_script_nvml_nvapi(ms, dci, dci.min_mem_oc,
                                                               dci.nvml_graph_clocks.size() - 1);
        //start temperature should be higher than the maximum energy difference between all configurations
        double start_temperature = abs(max_node.energy_hash_ - min_node.energy_hash_);
        return freq_simulated_annealing(ms, dci, max_node, start_temperature,
                                        max_iterations, mem_step, graph_idx_step, min_hashrate);
    }

    measurement freq_simulated_annealing(miner_script ms, const device_clock_info &dci, const measurement &start_node,
                                         double start_temperature, int max_iterations,
                                         int mem_step, int graph_idx_step, double min_hashrate) {
        if (start_node.hashrate_ < min_hashrate) {
            throw optimization_error("Minimum hashrate cannot be reached");
        }
        std::uniform_real_distribution<double> prob_check(0, 1);
        std::default_random_engine engine(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        measurement current_node = start_node;
        measurement best_node = current_node;
        //
        double Tk = start_temperature;
        double c = 0.8;
        for (int k = 1; k <= max_iterations; k++) {
            const measurement& neighbor_node = freq_hill_climbing(ms, dci, current_node, 1, mem_step, graph_idx_step,
                                                           min_hashrate);
            if (neighbor_node.energy_hash_ > best_node.energy_hash_)
                best_node = neighbor_node;
            //
            double ea = -current_node.energy_hash_;
            double eb = -neighbor_node.energy_hash_;
            //system can use higher energy configuration with probability dependent on temperature
            if (eb < ea || exp(-(eb - ea) / Tk) > prob_check(engine)) {
                current_node = neighbor_node;
            }
            //decrease temperature
            Tk = c * Tk;
        }
        return best_node;
    }

}