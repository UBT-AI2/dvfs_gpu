#pragma once

#include <vector>
#include "miner_script.h"

namespace frequency_scaling {

    struct device_clock_info {
        int device_id_nvml, device_id_nvapi;
        int nvapi_default_mem_clock, nvapi_default_graph_clock;
        std::vector<int> nvml_mem_clocks, nvml_graph_clocks;
        int min_mem_oc, min_graph_oc;
        int max_mem_oc, max_graph_oc;
    };

    struct measurement {
        measurement() : mem_clock_(0), graph_clock_(0), power_(0),
                        hashrate_(0), energy_hash_(0),
                        nvml_graph_clock_idx(-1), mem_oc(0), graph_oc(0) {}

        measurement(int mem_clock, int graph_clock, float power, float hashrate) :
                mem_clock_(mem_clock), graph_clock_(graph_clock), power_(power),
                hashrate_(hashrate), energy_hash_(hashrate / power),
                nvml_graph_clock_idx(-1), mem_oc(0), graph_oc(0) {}

        int mem_clock_, graph_clock_;
        float power_, hashrate_, energy_hash_;
        int nvml_graph_clock_idx;
        int mem_oc, graph_oc;
    };

    void start_power_monitoring_script(int device_id);

    void stop_power_monitoring_script(int device_id);

    void change_clocks_nvml_nvapi(const device_clock_info &dci,
                                  int mem_oc, int nvml_graph_clock_idx);

    void change_clocks_nvapi_only(const device_clock_info &dci,
                                  int mem_oc, int graph_oc);

    measurement
    run_benchmark_script_nvml_nvapi(miner_script ms, const device_clock_info &dci, int mem_oc,
                                    int nvml_graph_clock_idx);

    measurement
    run_benchmark_script_nvapi_only(miner_script ms, const device_clock_info &dci, int mem_oc, int graph_oc);
}