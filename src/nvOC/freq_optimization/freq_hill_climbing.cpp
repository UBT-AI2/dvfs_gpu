#include "freq_hill_climbing.h"
#include <random>
#include <chrono>
#include <glog/logging.h>
#include "../common_header/exceptions.h"


namespace frequency_scaling {


    static bool is_origin_direction(const measurement &current_node, const measurement &last_node,
                                    int mem_step_sgn, int graph_idx_step_sgn) {
        /*int mem_diff = current_node.mem_oc - last_node.mem_oc;
        int graph_idx_diff = current_node.nvml_graph_clock_idx - last_node.nvml_graph_clock_idx;
        auto sgn = [](int a) {
            if (a < 0) return -1;
            if (a > 0) return 1;
            return 0;
        };
        return sgn(mem_diff) == mem_step_sgn && sgn(graph_idx_diff) == graph_idx_step_sgn;*/
        return false;
    }

    static std::pair<double, double> compute_derivatives(const device_clock_info &dci,
                                                         const measurement &current_node,
                                                         const measurement &last_node, double last_slope) {
        //compute slope
        int memDiff = current_node.mem_oc - last_node.mem_oc;
        int graphDiff = dci.nvml_graph_clocks_[current_node.nvml_graph_clock_idx] -
                        dci.nvml_graph_clocks_[last_node.nvml_graph_clock_idx];
        double deltaX = std::sqrt(memDiff * memDiff + graphDiff * graphDiff);
        double deltaY = current_node.energy_hash_ - last_node.energy_hash_;
        double current_slope = (deltaX == 0) ? 0 : deltaY / deltaX;
        double slopediff = current_slope - last_slope;
        return std::make_pair(current_slope, slopediff);
    };


    static std::vector<measurement>
    explore_neighborhood(const benchmark_func &benchmarkFunc, const currency_type &ms, const device_clock_info &dci,
                         const measurement &current_node, const measurement &last_node, double current_slope,
                         int mem_step, int graph_step_idx, double min_hashrate, exploration_type expl_type,
                         int iteration) {
        std::vector<measurement> neighbor_nodes;
        std::vector<std::pair<std::pair<double, double>, measurement>> deriv_info;
        {
            int mem_plus = current_node.mem_oc + mem_step;
            int mem_minus = current_node.mem_oc - mem_step;
            int graph_plus_idx = current_node.nvml_graph_clock_idx + graph_step_idx;
            int graph_minus_idx = current_node.nvml_graph_clock_idx - graph_step_idx;
            bool optimization2D = dci.is_graph_oc_supported() && dci.is_mem_oc_supported();

            //horizontal/vertical 4 neighborhood
            if (!optimization2D ||
                expl_type == exploration_type::NEIGHBORHOOD_8 ||
                expl_type == exploration_type::NEIGHBORHOOD_4_STRAIGHT ||
                (expl_type == exploration_type::NEIGHBORHOOD_4_ALTERNATING && (iteration % 2))) {
                if (dci.is_mem_oc_supported() && mem_plus <= dci.max_mem_oc_ &&
                    !is_origin_direction(current_node, last_node, 1, 0)) {
                    const measurement &m = benchmarkFunc(ms, dci, mem_plus,
                                                         current_node.nvml_graph_clock_idx);
                    if (m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
                }
                if (dci.is_mem_oc_supported() && mem_minus >= dci.min_mem_oc_ &&
                    !is_origin_direction(current_node, last_node, -1, 0)) {
                    const measurement &m = benchmarkFunc(ms, dci, mem_minus,
                                                         current_node.nvml_graph_clock_idx);
                    if (m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
                }
                if (dci.is_graph_oc_supported() && graph_plus_idx < dci.nvml_graph_clocks_.size() &&
                    !is_origin_direction(current_node, last_node, 0, 1)) {
                    const measurement &m = benchmarkFunc(ms, dci, current_node.mem_oc,
                                                         graph_plus_idx);
                    if (m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
                }
                if (dci.is_graph_oc_supported() && graph_minus_idx >= 0 &&
                    !is_origin_direction(current_node, last_node, 0, -1)) {
                    const measurement &m = benchmarkFunc(ms, dci, current_node.mem_oc,
                                                         graph_minus_idx);
                    if (m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
                }
            }

            //diagonal 4 neighborhood
            if (optimization2D &&
                (expl_type == exploration_type::NEIGHBORHOOD_8 ||
                 expl_type == exploration_type::NEIGHBORHOOD_4_DIAGONAL ||
                 (expl_type == exploration_type::NEIGHBORHOOD_4_ALTERNATING &&
                  !(iteration % 2)))) {
                if (mem_plus <= dci.max_mem_oc_ && graph_plus_idx < dci.nvml_graph_clocks_.size() &&
                    !is_origin_direction(current_node, last_node, 1, 1)) {
                    const measurement &m = benchmarkFunc(ms, dci, mem_plus, graph_plus_idx);
                    if (m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
                }
                if (mem_minus >= dci.min_mem_oc_ && graph_minus_idx >= 0 &&
                    !is_origin_direction(current_node, last_node, -1, -1)) {
                    const measurement &m = benchmarkFunc(ms, dci, mem_minus, graph_minus_idx);
                    if (m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
                }
                if (mem_minus >= dci.min_mem_oc_ && graph_plus_idx < dci.nvml_graph_clocks_.size() &&
                    !is_origin_direction(current_node, last_node, -1, 1)) {
                    const measurement &m = benchmarkFunc(ms, dci, mem_minus, graph_plus_idx);
                    if (m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
                }
                if (mem_plus <= dci.max_mem_oc_ && graph_minus_idx >= 0 &&
                    !is_origin_direction(current_node, last_node, 1, -1)) {
                    const measurement &m = benchmarkFunc(ms, dci, mem_plus, graph_minus_idx);
                    if (m.hashrate_ >= min_hashrate) {
                        neighbor_nodes.push_back(m);
                        deriv_info.emplace_back(
                                compute_derivatives(dci, m, current_node, current_slope), m);
                    }
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
            VLOG(2) << "HC iteration" << iteration << ": max_deriv_slopediff: " << max_deriv_slopediff << std::endl;
            int max_deriv_mem_oc_diff = max_deriv_measurement.mem_oc - max_deriv_current_node.mem_oc;
            int max_deriv_graph_idx_diff =
                    max_deriv_measurement.nvml_graph_clock_idx - max_deriv_current_node.nvml_graph_clock_idx;
            int max_deriv_mem_oc = max_deriv_measurement.mem_oc + max_deriv_mem_oc_diff;
            int max_deriv_graph_idx = max_deriv_measurement.nvml_graph_clock_idx + max_deriv_graph_idx_diff;
            if (max_deriv_mem_oc > dci.max_mem_oc_ || max_deriv_mem_oc < dci.min_mem_oc_ ||
                max_deriv_graph_idx >= dci.nvml_graph_clocks_.size() || max_deriv_graph_idx < 0)
                break;
            const measurement &m = benchmarkFunc(ms, dci, max_deriv_mem_oc, max_deriv_graph_idx);
            if (m.hashrate_ < min_hashrate)
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

    measurement
    __freq_hill_climbing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                         const measurement &start_node, bool allow_start_node_result,
                         int max_iterations, int mem_step, int graph_idx_step, double min_hashrate,
                         exploration_type expl_type) {
        if (start_node.hashrate_ < min_hashrate) {
            LOG(WARNING) << "start_node does not have minimum hashrate (" <<
                       start_node.hashrate_ << " < " << min_hashrate << ")" << std::endl;
        }
        if (!dci.is_graph_oc_supported() && !dci.is_mem_oc_supported())
            return start_node;

        measurement current_node = start_node;
        measurement best_node;
        if (allow_start_node_result)
            best_node = current_node;
        else
            best_node.energy_hash_ = std::numeric_limits<double>::lowest();
        std::default_random_engine eng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<double> distr_stepsize(1.0, 2.0);

        double currentslope = 0, slopediff = 0;//corresponds to first/second derivative
        int cur_mem_step = mem_step, cur_graph_idx_step = graph_idx_step;
        int cancel_count = 0;
        double cancel_val = std::numeric_limits<double>::lowest();
        measurement last_node = current_node;
        //exploration
        for (int i = 0; i < max_iterations; i++) {
            if (currentslope > 0) {//slope increasing
                cur_mem_step = std::lround(mem_step * distr_stepsize(eng));
                cur_graph_idx_step = std::lround(graph_idx_step * distr_stepsize(eng));
            } else if (currentslope < 0) { //slope decreasing
                cur_mem_step = std::lround(mem_step / distr_stepsize(eng));
                cur_graph_idx_step = std::lround(std::max(graph_idx_step / distr_stepsize(eng), 1.0));
            }

            const std::vector<measurement> &neighbors = explore_neighborhood(benchmarkFunc, ct, dci, current_node,
                                                                             last_node,
                                                                             currentslope,
                                                                             cur_mem_step,
                                                                             cur_graph_idx_step, min_hashrate,
                                                                             expl_type,
                                                                             i);
            last_node = current_node;
            double tmp_val = std::numeric_limits<double>::lowest();
            for (const measurement &n : neighbors) {
                if (n.energy_hash_ > tmp_val) {
                    current_node = n;
                    tmp_val = n.energy_hash_;
                }
            }
            cancel_val = std::max(last_node.energy_hash_, cancel_val);
            //termination criterium
            if (tmp_val < cancel_val && ++cancel_count > 2) {
                VLOG(0) << "Hill Climbing convergence reached" << std::endl;
                break;
            } else {
                cancel_count = 0;
                cancel_val = std::numeric_limits<double>::lowest();
            }
            //compute slope
            int memDiff = current_node.mem_oc - last_node.mem_oc;
            int graphDiff = dci.nvml_graph_clocks_[current_node.nvml_graph_clock_idx] -
                            dci.nvml_graph_clocks_[last_node.nvml_graph_clock_idx];
            double deltaX = std::sqrt(memDiff * memDiff + graphDiff * graphDiff);
            double deltaY = current_node.energy_hash_ - last_node.energy_hash_;
            double tmp_slope = (deltaX == 0) ? 0 : deltaY / deltaX;
            slopediff = tmp_slope - currentslope;
            currentslope = tmp_slope;

            if (current_node.energy_hash_ > best_node.energy_hash_) {
                best_node = current_node;
            }
        }
        //
        if (!best_node.self_check())
            THROW_OPTIMIZATION_ERROR("hill climbing resulted in invalid measurement");
        return best_node;
    }


    measurement
    freq_hill_climbing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                       int max_iterations, double mem_step_pct, double graph_idx_step_pct, double min_hashrate_pct,
                       exploration_type expl_type) {
        //initial guess at maximum frequencies
        const measurement &start_node = benchmarkFunc(ct, dci, dci.max_mem_oc_, 0);
        double min_hashrate = min_hashrate_pct * start_node.hashrate_;
        return freq_hill_climbing(benchmarkFunc, ct, dci, start_node, true, max_iterations, mem_step_pct,
                                  graph_idx_step_pct,
                                  min_hashrate, expl_type);
    }


    measurement
    freq_hill_climbing(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                       const measurement &start_node, bool allow_start_node_result,
                       int max_iterations, double mem_step_pct, double graph_idx_step_pct, double min_hashrate,
                       exploration_type expl_type) {
        int mem_step = std::lround(mem_step_pct * (dci.max_mem_oc_ - dci.min_mem_oc_));
        int graph_idx_step = std::lround(std::max(graph_idx_step_pct * dci.nvml_graph_clocks_.size(), 1.0));
        return __freq_hill_climbing(benchmarkFunc, ct, dci, start_node, allow_start_node_result, max_iterations,
                                    mem_step,
                                    graph_idx_step,
                                    min_hashrate, expl_type);
    }


}