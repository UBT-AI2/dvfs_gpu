//
// Created by alex on 28.05.18.
//
#include "freq_simulated_annealing.h"

#include <cmath>
#include <random>
#include <chrono>
#include "../freq_hill_climbing/freq_hill_climbing.h"
#include "../common_header/exceptions.h"


namespace frequency_scaling {

    static double
    guess_start_temperature(const benchmark_func &benchmarkFunc, currency_type ms, const device_clock_info &dci) {
        int min_graph_idx = dci.nvml_graph_clocks.size() - 1;
        int min_mem_oc = dci.max_mem_oc;
        const measurement &min_node = benchmarkFunc(ms, dci, min_mem_oc, min_graph_idx);
        int max_graph_idx = 0.5 * dci.nvml_graph_clocks.size();
        int max_mem_oc = dci.min_mem_oc + 0.5 * (dci.max_mem_oc - dci.min_mem_oc);
        const measurement &max_node = benchmarkFunc(ms, dci, max_mem_oc, max_graph_idx);
        //start temperature should be higher than the maximum energy difference between all configurations
        return abs(max_node.energy_hash_ - min_node.energy_hash_);
    }

    static measurement
    freq_simulated_annealing(const benchmark_func &benchmarkFunc, currency_type ct, const device_clock_info &dci,
                             const measurement &start_node,
                             double start_temperature, int max_iterations,
                             int mem_step, int graph_idx_step, double min_hashrate) {
        if (start_node.hashrate_ < min_hashrate) {
            throw optimization_error("Minimum hashrate cannot be reached");
        }
        std::default_random_engine eng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<double> prob_check(0, 1);
        measurement current_node = start_node;
        measurement best_node = current_node;
        //
        std::uniform_real_distribution<double> distr_stepsize(1.0, 2.0);
        double currentslope = 0, slopediff = 0;//corresponds to first/second derivative
        int cur_mem_step = mem_step, cur_graph_idx_step = graph_idx_step;
        //
        double Tk = start_temperature;
        double c = 0.8;
        for (int k = 1; k <= max_iterations; k++) {
            if (slopediff > 0) {//slope increasing
                cur_mem_step = std::lround(mem_step * distr_stepsize(eng));
                cur_graph_idx_step = std::lround(graph_idx_step * distr_stepsize(eng));
            } else if (slopediff < 0) { //slope decreasing
                cur_mem_step = std::lround(mem_step / distr_stepsize(eng));
                cur_graph_idx_step = std::lround(std::max(graph_idx_step / distr_stepsize(eng), 1.0));
            }

            const measurement &neighbor_node = freq_hill_climbing(benchmarkFunc, ct, dci, current_node, false, 1,
                                                                  cur_mem_step,
                                                                  cur_graph_idx_step,
                                                                  min_hashrate);
            if (neighbor_node.energy_hash_ > best_node.energy_hash_)
                best_node = neighbor_node;
            //
            measurement last_node = current_node;
            double ea = -current_node.energy_hash_;
            double eb = -neighbor_node.energy_hash_;
            //system can use higher energy configuration with probability dependent on temperature
            if (eb < ea || exp(-(eb - ea) / Tk) > prob_check(eng)) {
                current_node = neighbor_node;
            }

            //compute slope
            int memDiff = current_node.mem_oc - last_node.mem_oc;
            int graphDiff = dci.nvml_graph_clocks[current_node.nvml_graph_clock_idx] -
                            dci.nvml_graph_clocks[last_node.nvml_graph_clock_idx];
            double deltaX = std::sqrt(memDiff * memDiff + graphDiff * graphDiff);
            double deltaY = current_node.energy_hash_ - last_node.energy_hash_;
            double tmp_slope = (deltaX == 0) ? 0 : deltaY / deltaX;
            slopediff = tmp_slope - currentslope;
            currentslope = tmp_slope;

            //decrease temperature
            Tk = c * Tk;
        }
        return best_node;
    }


    measurement
    freq_simulated_annealing(const benchmark_func &benchmarkFunc, currency_type ct, const device_clock_info &dci,
                             int max_iterations, int mem_step, int graph_idx_step, double min_hashrate) {
        double start_temperature = guess_start_temperature(benchmarkFunc, ct, dci);
        //initial guess at maximum frequencies
        int initial_graph_idx = 0, initial_mem_oc = dci.max_mem_oc;
        const measurement &initial_node = benchmarkFunc(ct, dci, initial_mem_oc, initial_graph_idx);
        return freq_simulated_annealing(benchmarkFunc, ct, dci, initial_node, start_temperature,
                                        max_iterations, mem_step, graph_idx_step, min_hashrate);
    }


    measurement
    freq_simulated_annealing(const benchmark_func &benchmarkFunc, currency_type ct, const device_clock_info &dci,
                             const measurement &start_node, int max_iterations, int mem_step, int graph_idx_step,
                             double min_hashrate) {
        double start_temperature = guess_start_temperature(benchmarkFunc, ct, dci);
        return freq_simulated_annealing(benchmarkFunc, ct, dci, start_node, start_temperature,
                                        max_iterations, mem_step, graph_idx_step, min_hashrate);
    }

}