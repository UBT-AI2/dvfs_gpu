#pragma once

#include <vector>
#include <functional>
#include "currency_type.h"

namespace frequency_scaling {

    struct device_clock_info {
        device_clock_info(int device_id_nvml);

        device_clock_info(int device_id_nvml, int min_mem_oc, int min_graph_oc,
                          int max_mem_oc, int max_graph_oc);

        int device_id_nvml_, device_id_nvapi_, device_id_cuda_;
        int nvapi_default_mem_clock_, nvapi_default_graph_clock_;
        std::vector<int> nvml_mem_clocks_, nvml_graph_clocks_;
        int min_mem_oc_, min_graph_oc_;
        int max_mem_oc_, max_graph_oc_;
        bool nvml_supported_, nvapi_supported_;
    };

    struct measurement {
        measurement();

        measurement(int mem_clock, int graph_clock, double power, double hashrate);

        void update_power(double power, int power_measure_dur_ms);

        void update_hashrate(double hashrate, int hashrate_measure_dur_ms);

        void update_freq_config(const measurement &other);

        bool self_check() const;

        int mem_clock_, graph_clock_;
        double power_, hashrate_, energy_hash_;
        int nvml_graph_clock_idx;
        int mem_oc, graph_oc;
        int power_measure_dur_ms_ = 0, hashrate_measure_dur_ms_ = 0;
    };

    typedef std::function<measurement(currency_type, const device_clock_info &, int, int)> benchmark_func;

    bool start_power_monitoring_script(int device_id, int interval_sleep_ms = 200);

    bool stop_power_monitoring_script(int device_id);

    double get_avg_power_usage(int device_id, long long int system_timestamp_start_ms,
                               long long int system_timestamp_end_ms);

    void change_gpu_clocks(const device_clock_info &dci,
                           int mem_oc, int nvml_graph_clock_idx);

    measurement
    run_benchmark_mining_offline(currency_type ct, const device_clock_info &dci, int mem_oc,
                                 int nvml_graph_clock_idx);

}