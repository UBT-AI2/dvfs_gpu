#pragma once

#include <vector>
#include <functional>
#include "currency_type.h"

namespace frequency_scaling {

    struct device_clock_info {
        device_clock_info(int device_id_nvml, int min_mem_oc, int min_graph_oc,
                          int max_mem_oc, int max_graph_oc);

        int device_id_nvml, device_id_nvapi, device_id_cuda;
        int nvapi_default_mem_clock, nvapi_default_graph_clock;
        std::vector<int> nvml_mem_clocks, nvml_graph_clocks;
        int min_mem_oc, min_graph_oc;
        int max_mem_oc, max_graph_oc;
        bool nvml_supported_;
    };

    struct measurement {
        measurement();

        measurement(int mem_clock, int graph_clock, double power, double hashrate);

        void update_power(double power);

        void update_hashrate(double hashrate);

        int mem_clock_, graph_clock_;
        double power_, hashrate_, energy_hash_;
        int nvml_graph_clock_idx;
        int mem_oc, graph_oc;
    };

    typedef std::function<measurement(currency_type, const device_clock_info &, int, int)> benchmark_func;


    void start_power_monitoring_script(int device_id, int interval_sleep_ms = 100);

    void stop_power_monitoring_script(int device_id);

    double get_avg_power_usage(int device_id, long long int system_timestamp_ms);

    void change_clocks_nvml_nvapi(const device_clock_info &dci,
                                  int mem_oc, int nvml_graph_clock_idx);

    void change_clocks_nvapi_only(const device_clock_info &dci,
                                  int mem_oc, int graph_oc);

    measurement
    run_benchmark_script_nvml_nvapi(currency_type ct, const device_clock_info &dci, int mem_oc,
                                    int nvml_graph_clock_idx);

    measurement
    run_benchmark_script_nvapi_only(currency_type ct, const device_clock_info &dci, int mem_oc, int graph_oc);

}