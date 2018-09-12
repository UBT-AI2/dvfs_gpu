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

    static measurement
    freq_nelder_mead1D(const benchmark_func &benchmarkFunc, const currency_type &ct, const device_clock_info &dci,
                       const measurement &start_node,
                       int min_iterations, int max_iterations,
                       int mem_step, int graph_idx_step,
                       double min_hashrate,
                       double mem_scale, double graph_scale) {

        int dimension_ = 1;
        vec_type1D init_guess(dimension_);
        init_guess(0) = start_node.nvml_graph_clock_idx / graph_idx_step;

        // function to optimize
        measurement best_measurement = start_node;
        int num_func_evals = 0;

        auto function = [&benchmarkFunc, ct, &dci, &best_measurement, mem_step, graph_idx_step,
                min_hashrate, &num_func_evals](
                const vec_type1D &x) -> double {
            num_func_evals++;
            int graph_idx = std::lround(x(0) * graph_idx_step);
            VLOG(2) << "NM function args: " << x(0) << std::endl;
            if (graph_idx >= dci.nvml_graph_clocks_.size() || graph_idx < 0) {
                return std::numeric_limits<double>::max();
            }
            //
            const measurement &m = (num_func_evals == 1) ? best_measurement : benchmarkFunc(ct, dci, 0, graph_idx);
            if (m.hashrate_ < min_hashrate) {
                if (num_func_evals == 1) {
                    //throw optimization_error("Minimum hashrate cannot be reached");
                    LOG(ERROR) << "start_node does not have minimum hashrate (" <<
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
        method.options.setMinIterations(min_iterations);
        method.options.setMaxIterations(max_iterations);
        method.options.setParamTolerance(1e-3);
        method.options.setFuncTolerance(1e-3);
        method.options.setInitScale(std::make_pair(mem_scale, graph_scale));
        const vec_type1D &glob_minimum = method.minimize(function, init_guess);
        VLOG(2) << "Nelder-mead number of function evaluations: " << num_func_evals << std::endl;

        //run script_running at proposed minimum
        int graph_idx = std::lround(glob_minimum(0) * graph_idx_step);
        measurement glob_min_measurement;
        glob_min_measurement.energy_hash_ = std::numeric_limits<double>::lowest();
        if (graph_idx < dci.nvml_graph_clocks_.size() && graph_idx >= 0) {
            glob_min_measurement =
                    benchmarkFunc(ct, dci, 0, graph_idx);
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
                       int min_iterations, int max_iterations,
                       int mem_step, int graph_idx_step,
                       double min_hashrate,
                       double mem_scale, double graph_scale) {

        int dimension_ = 2;
        vec_type2D init_guess(dimension_);
        init_guess(0) = (start_node.mem_oc - dci.min_mem_oc_) / (double) mem_step;
        init_guess(1) = start_node.nvml_graph_clock_idx / graph_idx_step;

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
                    //throw optimization_error("Minimum hashrate cannot be reached");
                    LOG(ERROR) << "start_node does not have minimum hashrate (" <<
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
        method.options.setMinIterations(min_iterations);
        method.options.setMaxIterations(max_iterations);
        method.options.setParamTolerance(1e-3);
        method.options.setFuncTolerance(1e-3);
        method.options.setInitScale(std::make_pair(mem_scale, graph_scale));
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

    measurement freq_nelder_mead(const benchmark_func &benchmarkFunc, currency_type ct, const device_clock_info &dci,
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

    measurement freq_nelder_mead(const benchmark_func &benchmarkFunc, currency_type ct, const device_clock_info &dci,
                                 const measurement &start_node,
                                 int max_iterations,
                                 double mem_step_pct, double graph_idx_step_pct,
                                 double min_hashrate) {
        int min_iterations = 1;
        double mem_scale = 1.5, graph_scale = 2.5;
        int mem_step = std::lround(mem_step_pct * (dci.max_mem_oc_ - dci.min_mem_oc_));
        int graph_idx_step = std::lround(std::max(graph_idx_step_pct * dci.nvml_graph_clocks_.size(), 1.0));
        //
        if (dci.nvapi_supported_)
            return freq_nelder_mead2D(benchmarkFunc, ct, dci, start_node, min_iterations, max_iterations, mem_step,
                                      graph_idx_step,
                                      min_hashrate, mem_scale, graph_scale);
        else if (dci.nvml_supported_)
            return freq_nelder_mead1D(benchmarkFunc, ct, dci, start_node, min_iterations, max_iterations, mem_step,
                                      graph_idx_step,
                                      min_hashrate, mem_scale, graph_scale);
        else {
            if (start_node.hashrate_ < min_hashrate) {
                //throw optimization_error("Minimum hashrate cannot be reached");
                LOG(ERROR) << "start_node does not have minimum hashrate (" <<
                           start_node.hashrate_ << " < " << min_hashrate << ")" << std::endl;
            }
            return start_node;
        }
    }

}