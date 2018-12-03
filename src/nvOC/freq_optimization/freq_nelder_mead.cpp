#include "freq_nelder_mead.h"
#include <random>
#include <chrono>
#include <glog/logging.h>
#include <numerical-methods/optimization/nelder-mead-method.h>
#include "../common_header/fullexpr_accum.h"
#include "../common_header/exceptions.h"


namespace frequency_scaling {

    typedef Eigen::Matrix<double, 1, 1> vec_type1D;
    typedef Eigen::Matrix<double, 2, 1> vec_type2D;

    struct nm_specific_options {
        nm_specific_options(int min_iterations, const std::pair<double, double> &init_scale, double func_tolerace,
                            double param_tolerance) : min_iterations_(min_iterations), init_scale_(init_scale),
                                                      func_tolerace_(func_tolerace),
                                                      param_tolerance_(param_tolerance) {}

        int min_iterations_;
        std::pair<double, double> init_scale_;
        double func_tolerace_, param_tolerance_;
    };

    static measurement
    freq_nelder_mead1D(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                       const measurement &start_node,
                       int max_iterations,
                       bool graph,
                       int mem_step, int graph_idx_step,
                       double min_hashrate,
                       const nm_specific_options &nm_opt) {

        int dimension_ = 1;
        vec_type1D init_guess(dimension_);
        init_guess(0) = (graph) ? start_node.nvml_graph_clock_idx_ / (double) graph_idx_step :
                        (start_node.mem_oc_ - dci.min_mem_oc_) / (double) mem_step;

        // function to optimize
        measurement best_measurement = start_node;
        int num_func_evals = 0;

        auto function = [&benchmarkFunc, ct, &dci, &best_measurement, &start_node, mem_step, graph_idx_step,
                min_hashrate, &num_func_evals, graph](
                const vec_type1D &x) -> double {
            num_func_evals++;
            int graph_idx = (graph) ? std::lround(x(0) * graph_idx_step) : start_node.nvml_graph_clock_idx_;
            int mem_oc = (!graph) ? dci.min_mem_oc_ + std::lround(x(0) * mem_step) : start_node.mem_oc_;
            VLOG(2) << "NM function args: " << x(0) << std::endl;
            if (mem_oc > dci.max_mem_oc_ || mem_oc < dci.min_mem_oc_ ||
                graph_idx >= dci.nvml_graph_clocks_.size() || graph_idx < 0) {
                return std::numeric_limits<double>::max();
            }
            //
            const measurement &m = (num_func_evals == 1) ? best_measurement : benchmarkFunc(ct, dci, mem_oc, graph_idx);
            if (m.hashrate_ < min_hashrate) {
                if (num_func_evals == 1) {
                    LOG(WARNING) << "start_node does not have minimum hashrate (" <<
                                 m.hashrate_ << " < " << min_hashrate << ")" << std::endl;
                }
                return std::numeric_limits<double>::max();
            }

            if (m.energy_hash_ > best_measurement.energy_hash_)
                best_measurement = m;
            VLOG(2) << "NM measured energy-hash: " << m.energy_hash_ << std::endl;
            //note minimize function
            return -m.energy_hash_;
        };

        //nelder mead optimization
        numerical_methods::NelderMeadMethod<double, 1> method;
        method.options.setMinIterations(nm_opt.min_iterations_);
        method.options.setMaxIterations(max_iterations);
        method.options.setParamTolerance(nm_opt.param_tolerance_);
        method.options.setFuncTolerance(nm_opt.func_tolerace_);
        method.options.setInitScale(nm_opt.init_scale_);
        const vec_type1D &glob_minimum = method.minimize(function, init_guess);
        VLOG(2) << "Nelder-mead number of function evaluations: " << num_func_evals << std::endl;

        //run script_running at proposed minimum
        int graph_idx = (graph) ? std::lround(glob_minimum(0) * graph_idx_step) : start_node.nvml_graph_clock_idx_;
        int mem_oc = (!graph) ? dci.min_mem_oc_ + std::lround(glob_minimum(0) * mem_step) : start_node.mem_oc_;
        measurement glob_min_measurement;
        glob_min_measurement.energy_hash_ = std::numeric_limits<double>::lowest();
        if (mem_oc <= dci.max_mem_oc_ && mem_oc >= dci.min_mem_oc_ &&
            graph_idx < dci.nvml_graph_clocks_.size() && graph_idx >= 0) {
            glob_min_measurement =
                    benchmarkFunc(ct, dci, mem_oc, graph_idx);
        }
        const measurement &res = (best_measurement.energy_hash_ > glob_min_measurement.energy_hash_) ?
                                 best_measurement : glob_min_measurement;
        //
        if (!res.self_check())
            THROW_OPTIMIZATION_ERROR("nelder mead resulted in invalid measurement");
        return res;
    }

    static measurement
    freq_nelder_mead2D(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                       const measurement &start_node,
                       int max_iterations,
                       int mem_step, int graph_idx_step,
                       double min_hashrate,
                       const nm_specific_options &nm_opt) {

        int dimension_ = 2;
        vec_type2D init_guess(dimension_);
        init_guess(0) = (start_node.mem_oc_ - dci.min_mem_oc_) / (double) mem_step;
        init_guess(1) = start_node.nvml_graph_clock_idx_ / (double) graph_idx_step;

        // function to optimize
        measurement best_measurement = start_node;
        int num_func_evals = 0;

        auto function = [&benchmarkFunc, ct, &dci, &best_measurement, mem_step, graph_idx_step,
                min_hashrate, &num_func_evals](
                const vec_type2D &x) -> double {
            num_func_evals++;
            int mem_oc = dci.min_mem_oc_ + std::lround(x(0) * mem_step);
            int graph_idx = std::lround(x(1) * graph_idx_step);
            VLOG(2) << "NM function args: " << x(0) << "," << x(1) << std::endl;
            if (mem_oc > dci.max_mem_oc_ || mem_oc < dci.min_mem_oc_ ||
                graph_idx >= dci.nvml_graph_clocks_.size() || graph_idx < 0) {
                return std::numeric_limits<double>::max();
            }
            //
            const measurement &m = (num_func_evals == 1) ? best_measurement : benchmarkFunc(ct, dci, mem_oc, graph_idx);
            if (m.hashrate_ < min_hashrate) {
                if (num_func_evals == 1) {
                    LOG(WARNING) << "start_node does not have minimum hashrate (" <<
                                 m.hashrate_ << " < " << min_hashrate << ")" << std::endl;
                }
                return std::numeric_limits<double>::max();
            }

            if (m.energy_hash_ > best_measurement.energy_hash_)
                best_measurement = m;
            VLOG(2) << "NM measured energy-hash: " << m.energy_hash_ << std::endl;
            //note minimize function
            return -m.energy_hash_;
        };

        //nelder mead optimization
        numerical_methods::NelderMeadMethod<double, 2> method;
        method.options.setMinIterations(nm_opt.min_iterations_);
        method.options.setMaxIterations(max_iterations);
        method.options.setParamTolerance(nm_opt.param_tolerance_);
        method.options.setFuncTolerance(nm_opt.func_tolerace_);
        method.options.setInitScale(nm_opt.init_scale_);
        const vec_type2D &glob_minimum = method.minimize(function, init_guess);
        VLOG(2) << "Nelder-mead number of function evaluations: " << num_func_evals << std::endl;

        //run script_running at proposed minimum
        int mem_oc = dci.min_mem_oc_ + std::lround(glob_minimum(0) * mem_step);
        int graph_idx = std::lround(glob_minimum(1) * graph_idx_step);
        measurement glob_min_measurement;
        glob_min_measurement.energy_hash_ = std::numeric_limits<double>::lowest();
        if (mem_oc <= dci.max_mem_oc_ && mem_oc >= dci.min_mem_oc_ &&
            graph_idx < dci.nvml_graph_clocks_.size() && graph_idx >= 0) {
            glob_min_measurement =
                    benchmarkFunc(ct, dci, mem_oc, graph_idx);
        }
        const measurement &res = (best_measurement.energy_hash_ > glob_min_measurement.energy_hash_) ?
                                 best_measurement : glob_min_measurement;
        //
        if (!res.self_check())
            THROW_OPTIMIZATION_ERROR("nelder mead resulted in invalid measurement");
        return res;
    }

    measurement
    freq_nelder_mead(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                     int max_iterations,
                     double mem_step_pct, double graph_idx_step_pct,
                     double min_hashrate_pct) {

        //initial guess at maximum frequencies
        const measurement &start_node = benchmarkFunc(ct, dci, dci.max_mem_oc_, 0);
        double min_hashrate = min_hashrate_pct * start_node.hashrate_;
        return freq_nelder_mead(benchmarkFunc, ct, dci, start_node, max_iterations, mem_step_pct,
                                graph_idx_step_pct,
                                min_hashrate);
    }

    measurement
    freq_nelder_mead(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                     const measurement &start_node, int max_iterations,
                     double mem_step_pct, double graph_idx_step_pct, double min_hashrate) {
        int mem_step = std::lround(mem_step_pct * (dci.max_mem_oc_ - dci.min_mem_oc_));
        int graph_idx_step = std::lround(std::max(graph_idx_step_pct * dci.nvml_graph_clocks_.size(), 1.0));
        nm_specific_options nm_opt(std::min(1, max_iterations), std::make_pair(1.5, 2.5),
                                   std::numeric_limits<double>::max(), 0.4);
        //
        if (dci.is_graph_oc_supported() && dci.is_mem_oc_supported()) {
            return freq_nelder_mead2D(benchmarkFunc, ct, dci, start_node, max_iterations, mem_step,
                                      graph_idx_step, min_hashrate, nm_opt);
        } else if (dci.is_graph_oc_supported() || dci.is_mem_oc_supported()) {
            nm_opt.param_tolerance_ /= std::sqrt(2);
            return freq_nelder_mead1D(benchmarkFunc, ct, dci, start_node, max_iterations, dci.is_graph_oc_supported(),
                                      mem_step, graph_idx_step, min_hashrate, nm_opt);
        } else {
            if (start_node.hashrate_ < min_hashrate) {
                //throw optimization_error("Minimum hashrate cannot be reached");
                LOG(ERROR) << "start_node does not have minimum hashrate (" <<
                           start_node.hashrate_ << " < " << min_hashrate << ")" << std::endl;
            }
            return start_node;
        }
    }

}