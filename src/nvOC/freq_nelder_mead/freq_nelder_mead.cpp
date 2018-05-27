#include "freq_nelder_mead.h"

#include <iostream>
#include <random>
#include <chrono>
#include <numerical-methods/optimization/nelder-mead-method.h>
#include "../exceptions.h"



namespace frequency_scaling {

    typedef Eigen::Matrix<double, 2, 1> vec_type;

    measurement freq_nelder_mead(miner_script ms, const device_clock_info &dci,
                                 int min_iterations, int max_iterations,
                                 int mem_step, int graph_idx_step,
                                 double min_hashrate,
                                 double mem_scale, double graph_scale) {

        //initial guess at maximum frequencies
        int dimension_ = 2;
        vec_type init_guess(dimension_);
        init_guess(0) = (dci.max_mem_oc - dci.min_mem_oc) / (double) mem_step;
        init_guess(1) = 0;

        // function to optimize
        measurement best_measurement;
        best_measurement.energy_hash_ = std::numeric_limits<float>::lowest();
        int num_func_evals = 0;

        auto function = [ms, &dci, dimension_, &best_measurement, mem_step, graph_idx_step,
                min_hashrate, &num_func_evals](
                const vec_type &x) -> double {
            num_func_evals++;
            int mem_oc = dci.min_mem_oc + std::lround(x(0) * mem_step);
            int graph_idx = std::lround(x(1) * graph_idx_step);
#ifdef DEBUG
            std::cout << "NM function args: " << x(0) << "," << x(1) << std::endl;
#endif
            if (mem_oc > dci.max_mem_oc || mem_oc < dci.min_mem_oc ||
                graph_idx >= dci.nvml_graph_clocks.size() || graph_idx < 0) {
                return std::numeric_limits<double>::max();
            }
            //
            const measurement &m = run_benchmark_script_nvml_nvapi(ms, dci, mem_oc, graph_idx);
            if(m.hashrate_ < min_hashrate) {
                if(num_func_evals == 1)
                    throw optimization_error("Minimum hashrate cannot be reached");
                return std::numeric_limits<double>::max();
            }

            if (m.energy_hash_ > best_measurement.energy_hash_)
                best_measurement = m;
#ifdef DEBUG
            std::cout << "NM measured energy-hash: " << m.energy_hash_ << std::endl;
#endif
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
        const vec_type &glob_minimum = method.minimize(function, init_guess);
        std::cout << "Nelder-mead number of function evaluations: " << num_func_evals << std::endl;

        //run script_running at proposed minimum
        int mem_oc = dci.min_mem_oc + std::lround(glob_minimum(0) * mem_step);
        int graph_idx = std::lround(glob_minimum(1) * graph_idx_step);
        const measurement& glob_min_measurement =
                run_benchmark_script_nvml_nvapi(ms, dci, mem_oc, graph_idx);
        return (best_measurement.energy_hash_ > glob_min_measurement.energy_hash_) ?
               best_measurement : glob_min_measurement;
    }

}