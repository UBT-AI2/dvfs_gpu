//
// Created by alex on 28.05.18.
//
#include "freq_simulated_annealing.h"
#include <cmath>
#include <random>
#include <chrono>
#include <glog/logging.h>
#include "../common_header/fullexpr_accum.h"
#include "../common_header/exceptions.h"
#include "freq_hill_climbing.h"


namespace frequency_scaling {

    static double
    guess_start_temperature(const benchmark_func &benchmarkFunc, const currency_type &ct,
                            const device_clock_info &dci) {
        int min_graph_idx = dci.nvml_graph_clocks_.size() - 1;
        int min_mem_oc = dci.max_mem_oc_;
        const measurement &min_node = benchmarkFunc(ct, dci, min_mem_oc, min_graph_idx);
        int max_graph_idx = 0.5 * dci.nvml_graph_clocks_.size();
        int max_mem_oc = dci.min_mem_oc_ + 0.5 * (dci.max_mem_oc_ - dci.min_mem_oc_);
        const measurement &max_node = benchmarkFunc(ct, dci, max_mem_oc, max_graph_idx);
        //start temperature should be higher than the maximum energy difference between all configurations
        return abs(max_node.energy_hash_ - min_node.energy_hash_);
    }

    static measurement
    freq_simulated_annealing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                             const measurement &start_node,
                             double start_temperature, int max_iterations,
                             int mem_step, int graph_idx_step, double min_hashrate) {
        if (start_node.hashrate_ < min_hashrate) {
            //throw optimization_error("Minimum hashrate cannot be reached");
            LOG(ERROR) << "start_node does not have minimum hashrate (" <<
                       start_node.hashrate_ << " < " << min_hashrate << ")" << std::endl;
        }
        if (!dci.nvml_supported_ && !dci.nvapi_supported_)
            return start_node;

        measurement current_node = start_node;
        measurement best_node = current_node;
        //
        std::default_random_engine eng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<double> prob_check(0, 1);
        std::uniform_real_distribution<double> distr_graph_stepsize(0.33 * graph_idx_step, 3.0 * graph_idx_step);
        std::uniform_real_distribution<double> distr_mem_stepsize(0.33 * mem_step, 3.0 * mem_step);
        //
        double Tk = start_temperature;
        double c = 0.8;
        int cancel_count = 0;
        for (int i = 0; i < max_iterations; i++) {
            int cur_mem_step = std::lround(distr_mem_stepsize(eng));
            int cur_graph_idx_step = std::lround(std::max(distr_graph_stepsize(eng), 1.0));
            const measurement &neighbor_node = __freq_hill_climbing(benchmarkFunc, ct, dci, current_node, false, 1,
                                                                    cur_mem_step,
                                                                    cur_graph_idx_step,
                                                                    min_hashrate,
                                                                    (i % 2) ? exploration_type::NEIGHBORHOOD_4_STRAIGHT
                                                                            : exploration_type::NEIGHBORHOOD_4_DIAGONAL);

            if (neighbor_node.energy_hash_ > best_node.energy_hash_)
                best_node = neighbor_node;
            //
            double ea = -current_node.energy_hash_;
            double eb = -neighbor_node.energy_hash_;
            //system can use higher energy configuration with probability dependent on temperature
            if (eb < ea || exp(-(eb - ea) / Tk) > prob_check(eng)) {
                current_node = neighbor_node;
                cancel_count = 0;
            } else if (++cancel_count > 1) {
                //termination criterium
                VLOG(0) << "Simulated annealing convergence reached" << std::endl;
                break;
            }
            //decrease temperature
            Tk = c * Tk;
        }
        //
        if (!best_node.self_check())
            THROW_OPTIMIZATION_ERROR("simulated annealing resulted in invalid measurement");
        return best_node;
    }


    measurement
    freq_simulated_annealing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                             int max_iterations, double mem_step_pct, double graph_idx_step_pct,
                             double min_hashrate_pct) {
        //initial guess at maximum frequencies
        const measurement &start_node = benchmarkFunc(ct, dci, dci.max_mem_oc_, 0);
        double min_hashrate = min_hashrate_pct * start_node.hashrate_;
        return freq_simulated_annealing(benchmarkFunc, ct, dci, start_node,
                                        max_iterations, mem_step_pct, graph_idx_step_pct, min_hashrate);
    }


    measurement
    freq_simulated_annealing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                             const measurement &start_node, int max_iterations, double mem_step_pct,
                             double graph_idx_step_pct,
                             double min_hashrate) {
        double start_temperature = guess_start_temperature(benchmarkFunc, ct, dci);
        int mem_step = std::lround(mem_step_pct * (dci.max_mem_oc_ - dci.min_mem_oc_));
        int graph_idx_step = std::lround(std::max(graph_idx_step_pct * dci.nvml_graph_clocks_.size(), 1.0));
        return freq_simulated_annealing(benchmarkFunc, ct, dci, start_node, start_temperature,
                                        max_iterations, mem_step, graph_idx_step, min_hashrate);
    }

}