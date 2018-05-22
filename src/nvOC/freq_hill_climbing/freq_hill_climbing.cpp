#include "freq_hill_climbing.h"

#include <random>
#include <chrono>
#include <iostream>


namespace frequency_scaling {

    static std::pair<double, double> compute_derivatives(const device_clock_info &dci,
                                                         const measurement &current_node,
                                                         const measurement &last_node, double last_slope) {
        //compute slope
        int memDiff = current_node.mem_oc - last_node.mem_oc;
        int graphDiff = dci.nvml_graph_clocks[current_node.nvml_graph_clock_idx] -
                        dci.nvml_graph_clocks[last_node.nvml_graph_clock_idx];
        double deltaX = std::sqrt(memDiff * memDiff + graphDiff * graphDiff);
        double deltaY = current_node.energy_hash_ - last_node.energy_hash_;
        double current_slope = (deltaX == 0) ? 0 : deltaY / deltaX;
        double slopediff = current_slope - last_slope;
        return std::make_pair(current_slope, slopediff);
    };


    static std::vector<measurement>
    explore_neighborhood(miner_script ms, const device_clock_info &dci,
                         const measurement &current_node, double current_slope,
                         int mem_step, int graph_step_idx, double min_hashrate, bool use8Neighborhood) {
        std::vector<measurement> neighbor_nodes;
        std::vector<std::pair<std::pair<double, double>, measurement>> deriv_info;
        {
            int mem_plus = current_node.mem_oc + mem_step;
            int mem_minus = current_node.mem_oc - mem_step;
            int graph_plus_idx = current_node.nvml_graph_clock_idx + graph_step_idx;
            int graph_minus_idx = current_node.nvml_graph_clock_idx - graph_step_idx;

            //horizontal/vertical 4 neighborhood
            if (use8Neighborhood) {
                if (mem_plus <= dci.max_mem_oc) {
                    const measurement& m = run_benchmark_script_nvml_nvapi(ms, dci, mem_plus, current_node.nvml_graph_clock_idx);
                    if(m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
                }
                if (mem_minus >= dci.min_mem_oc) {
                    const measurement& m = run_benchmark_script_nvml_nvapi(ms, dci, mem_minus, current_node.nvml_graph_clock_idx);
                    if(m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
                }
                if (graph_plus_idx < dci.nvml_graph_clocks.size()) {
                    const measurement& m = run_benchmark_script_nvml_nvapi(ms, dci, current_node.mem_oc, graph_plus_idx);
                    if(m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
                }
                if (graph_minus_idx >= 0) {
                    const measurement& m = run_benchmark_script_nvml_nvapi(ms, dci, current_node.mem_oc, graph_minus_idx);
                    if(m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
                }
            }

            //diagonal 4 neighborhood
            if (mem_plus <= dci.max_mem_oc && graph_plus_idx < dci.nvml_graph_clocks.size()) {
                const measurement& m = run_benchmark_script_nvml_nvapi(ms, dci, mem_plus, graph_plus_idx);
                if(m.hashrate_ >= min_hashrate) {
                    neighbor_nodes.push_back(m);
                    deriv_info.emplace_back(
                            compute_derivatives(dci, m, current_node, current_slope), m);
                }
            }
            if (mem_minus >= dci.min_mem_oc && graph_minus_idx >= 0) {
                const measurement& m = run_benchmark_script_nvml_nvapi(ms, dci, mem_minus, graph_minus_idx);
                if(m.hashrate_ >= min_hashrate) {
                    neighbor_nodes.push_back(m);
                    deriv_info.emplace_back(
                            compute_derivatives(dci, m, current_node, current_slope), m);
                }
            }
            if (mem_minus >= dci.min_mem_oc && graph_plus_idx < dci.nvml_graph_clocks.size()) {
                const measurement& m = run_benchmark_script_nvml_nvapi(ms, dci, mem_minus, graph_plus_idx);
                if(m.hashrate_ >= min_hashrate) {
                    neighbor_nodes.push_back(m);
                    deriv_info.emplace_back(
                            compute_derivatives(dci, m, current_node, current_slope), m);
                }
            }
            if (mem_plus <= dci.max_mem_oc && graph_minus_idx >= 0) {
                const measurement& m = run_benchmark_script_nvml_nvapi(ms, dci, mem_plus, graph_minus_idx);
                if(m.hashrate_ >= min_hashrate) {
                    neighbor_nodes.push_back(m);
                    deriv_info.emplace_back(
                            compute_derivatives(dci, m, current_node, current_slope), m);
                }
            }


        }

        //find measurement with maximum slopediff (slope of slope)
        measurement max_deriv_measurement;
        double max_deriv_slopediff = std::numeric_limits<double>::lowest(), max_deriv_slope;
        for (auto &di : deriv_info) {
            if (di.first.second > max_deriv_slopediff) {
                max_deriv_slopediff = di.first.second;
                max_deriv_slope = di.first.first;
                max_deriv_measurement = di.second;
            }
        }

        //iff slope of slope is positive explore in that direction as long it stays positive
        measurement max_deriv_current_node = current_node;
        while (max_deriv_slopediff > 0) {
#ifdef DEBUG
            std::cout << "max_deriv_slopediff: " << max_deriv_slopediff << std::endl;
#endif
            int max_deriv_mem_oc_diff = max_deriv_measurement.mem_oc - max_deriv_current_node.mem_oc;
            int max_deriv_graph_idx_diff =
                    max_deriv_measurement.nvml_graph_clock_idx - max_deriv_current_node.nvml_graph_clock_idx;
            int max_deriv_mem_oc = max_deriv_measurement.mem_oc + max_deriv_mem_oc_diff;
            int max_deriv_graph_idx = max_deriv_measurement.nvml_graph_clock_idx + max_deriv_graph_idx_diff;
            if (max_deriv_mem_oc > dci.max_mem_oc || max_deriv_mem_oc < dci.min_mem_oc ||
                max_deriv_graph_idx >= dci.nvml_graph_clocks.size() || max_deriv_graph_idx < 0)
                break;
            const measurement& m = run_benchmark_script_nvml_nvapi(ms, dci, max_deriv_mem_oc, max_deriv_graph_idx);
            if(m.hashrate_ < min_hashrate)
                break;
            neighbor_nodes.push_back(m);
            max_deriv_current_node = max_deriv_measurement;
            max_deriv_measurement = m;
            std::pair<double, double> tmp = compute_derivatives(dci, max_deriv_measurement, max_deriv_current_node,
                                                                max_deriv_slope);
            max_deriv_slopediff = tmp.second;
            max_deriv_slope = tmp.first;
        }


        return neighbor_nodes;
    }


    measurement freq_hill_climbing(miner_script ms, const device_clock_info &dci, int max_iterations,
                                   int mem_step, int graph_idx_step, double min_hashrate) {

        //initial guess at maximum frequencies
        int initial_graph_idx = 0, initial_mem_oc = dci.max_mem_oc;
        measurement current_node = run_benchmark_script_nvml_nvapi(ms, dci, initial_mem_oc, initial_graph_idx);
        if (current_node.mem_oc == dci.max_mem_oc &&
            current_node.nvml_graph_clock_idx == dci.nvml_graph_clocks.size()-1) {
            throw std::runtime_error("Minimum hashrate cannot be reached");
        }
        measurement best_node = current_node;

        std::default_random_engine eng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<double> distr_stepsize(1.0, 2.0);
        double currentslope = 0, slopediff = 0;//corresponds to first/second derivative
        int cur_mem_step = mem_step, cur_graph_idx_step = graph_idx_step;
        //exploration
        for (int i = 0; i < max_iterations; i++) {
            if (slopediff > 0) {//slope increasing
                cur_mem_step = std::lround(mem_step * distr_stepsize(eng));
                cur_graph_idx_step = std::lround(graph_idx_step * distr_stepsize(eng));
            } else if (slopediff < 0) { //slope decreasing
                cur_mem_step = std::lround(mem_step / distr_stepsize(eng));
                cur_graph_idx_step = std::lround(std::max(graph_idx_step / distr_stepsize(eng), 1.0));
            }

            const std::vector<measurement> &neighbors = explore_neighborhood(ms, dci, current_node, currentslope,
                                                                             cur_mem_step,
                                                                             cur_graph_idx_step, min_hashrate, false);
            measurement last_node = current_node;
            float tmp_val = std::numeric_limits<float>::lowest();
            for (const measurement &n : neighbors) {
                if (n.energy_hash_ > tmp_val) {
                    current_node = n;
                    tmp_val = n.energy_hash_;
                }
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

            if (current_node.energy_hash_ > best_node.energy_hash_) {
                best_node = current_node;
            }
        }

        return best_node;
    }

}