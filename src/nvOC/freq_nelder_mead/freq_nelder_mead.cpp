#include "freq_nelder_mead.h"

#include <iostream>
#include <random>
#include <chrono>
#include <numerical-methods/optimization/nelder-mead-method.h>


namespace frequency_scaling {

    typedef Eigen::Matrix<double, 2, 1> vec_type;

    measurement freq_nelder_mead(miner_script ms, const device_clock_info &dci,
                                 int min_iterations, int max_iterations,
                                 int mem_step, int graph_idx_step,
                                 double param_tolerance, double func_tolerance) {

        //get random numbers
        double initial_graph_idx, initial_mem_oc;
        {
            std::default_random_engine eng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            std::uniform_real_distribution<double> distr_graph(0, (dci.nvml_graph_clocks.size() - 1) /
                                                                  (double) graph_idx_step);
            std::uniform_real_distribution<double> distr_mem(0, (dci.max_mem_oc - dci.min_mem_oc) / (double) mem_step);
            initial_graph_idx = distr_graph(eng);
            initial_mem_oc = distr_mem(eng);
        }
        //initial random guess
        int dimension_ = 2;
        vec_type init_guess(dimension_);
        init_guess(0) = initial_mem_oc;
        init_guess(1) = initial_graph_idx;

        // function to optimize
        measurement best_measurement;
        best_measurement.energy_hash_ = std::numeric_limits<float>::min();

        auto function = [ms, &dci, dimension_, &best_measurement, mem_step, graph_idx_step](
                const vec_type &x) -> double {
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
        method.options.setParamTolerance(param_tolerance);
        method.options.setFuncTolerance(func_tolerance);
        method.options.setInitScale(std::make_pair(1.5, 1.5));
        const vec_type &glob_minimum = method.minimize(function, init_guess);
        return best_measurement;
        /*
        //run script_running at proposed minimum
        int mem_oc = dci.min_mem_oc + std::lround(glob_minimum(0) * mem_step);
        int graph_idx = std::lround(glob_minimum(1) * graph_idx_step);
        return run_benchmark_script_nvml_nvapi(ms, dci, mem_oc, graph_idx);
         */
    }

}