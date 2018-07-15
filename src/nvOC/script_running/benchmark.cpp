//
// Created by Alex on 12.05.2018.
//
#include "benchmark.h"

#include <string.h>
#include <fstream>
#include <cuda.h>
#include <glog/logging.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../common_header/exceptions.h"
#include "process_management.h"

namespace frequency_scaling {

    static const int BUFFER_SIZE = 1024;

    device_clock_info::device_clock_info(int device_id_nvml, int min_mem_oc,
                                         int min_graph_oc, int max_mem_oc,
                                         int max_graph_oc) : device_id_nvml(device_id_nvml), min_mem_oc(min_mem_oc),
                                                             min_graph_oc(min_graph_oc), max_mem_oc(max_mem_oc),
                                                             max_graph_oc(max_graph_oc) {
        if (min_mem_oc > max_mem_oc)
            throw std::invalid_argument("max_mem_oc >= min_mem_oc violated");
        if (min_graph_oc > max_graph_oc)
            throw std::invalid_argument("max_graph_oc >= min_graph_oc violated");
        nvml_supported_ = nvmlCheckOCSupport(device_id_nvml);

        device_id_nvapi = nvapiGetDeviceIndexByBusId(nvmlGetBusId(device_id_nvml));
        CUresult res = cuDeviceGetByPCIBusId(&device_id_cuda, nvmlGetBusIdString(device_id_nvml).c_str());
        if (res == CUDA_ERROR_NOT_INITIALIZED) {
            cuInit(0);
            cuDeviceGetByPCIBusId(&device_id_cuda, nvmlGetBusIdString(device_id_nvml).c_str());
        }
        VLOG(0) << "Matching device by PCIBusId: NVML-Id=" << device_id_nvml << ", NVAPI-Id=" <<
                device_id_nvapi << ", CUDA-Id=" << device_id_cuda << std::endl;
        if (nvml_supported_)
            nvml_register_gpu(device_id_nvml);
        nvapi_register_gpu(device_id_nvapi);

        nvapi_default_mem_clock = nvapiGetCurrentMemClock(device_id_nvapi);
        nvapi_default_graph_clock = nvapiGetCurrentGraphClock(device_id_nvapi);

        if (nvml_supported_) {
            nvml_mem_clocks = nvmlGetAvailableMemClocks(device_id_nvml);
            nvml_graph_clocks = nvmlGetAvailableGraphClocks(device_id_nvml, nvml_mem_clocks[1]);
        } else {
            //fake nvml vectors
            for (int graph_oc = max_graph_oc; graph_oc >= min_graph_oc; graph_oc -= 10) {
                nvml_graph_clocks.push_back(nvapi_default_graph_clock + graph_oc);
            }
            for (int mem_oc = max_mem_oc; mem_oc >= min_mem_oc; mem_oc -= 10) {
                nvml_mem_clocks.push_back(nvapi_default_mem_clock + mem_oc);
            }
        }
    }


    measurement::measurement() : mem_clock_(0), graph_clock_(0), power_(0),
                                 hashrate_(0), energy_hash_(0),
                                 nvml_graph_clock_idx(-1), mem_oc(0), graph_oc(0) {}

    measurement::measurement(int mem_clock, int graph_clock, double power, double hashrate) :
            mem_clock_(mem_clock), graph_clock_(graph_clock), power_(power),
            hashrate_(hashrate), energy_hash_(hashrate / power),
            nvml_graph_clock_idx(-1), mem_oc(0), graph_oc(0) {}


    void measurement::update_power(double power, int power_measure_dur_ms) {
        power_ = power;
        power_measure_dur_ms_ = power_measure_dur_ms;
        energy_hash_ = hashrate_ / power;
    }

    void measurement::update_hashrate(double hashrate, int hashrate_measure_dur_ms) {
        hashrate_ = hashrate;
        hashrate_measure_dur_ms_ = hashrate_measure_dur_ms;
        energy_hash_ = hashrate / power_;
    }

    void measurement::update_freq_config(const measurement &other) {
        mem_clock_ = other.mem_clock_;
        graph_clock_ = other.graph_clock_;
        mem_oc = other.mem_oc;
        graph_oc = other.graph_oc;
        nvml_graph_clock_idx = other.nvml_graph_clock_idx;
    }

    static measurement run_benchmark_script(currency_type ct, const device_clock_info &dci,
                                            int graph_clock, int mem_clock) {
        {
            VLOG(0) << gpu_log_prefix(ct, dci.device_id_nvml) <<
                    "Running offline benchmark with clocks: mem:" << mem_clock << ",graph:" << graph_clock
                    << std::endl;
            //run benchmark script to get measurement
            char cmd2[BUFFER_SIZE];
            switch (ct) {
                case currency_type::ETH:
                    snprintf(cmd2, BUFFER_SIZE, "bash ../scripts/run_benchmark_eth.sh %i %i %i %i",
                             dci.device_id_nvml, dci.device_id_cuda, mem_clock, graph_clock);
                    break;
                case currency_type::ZEC:
                    snprintf(cmd2, BUFFER_SIZE, "bash ../scripts/run_benchmark_zec.sh %i %i %i %i",
                             dci.device_id_nvml, dci.device_id_cuda, mem_clock, graph_clock);
                    break;
                case currency_type::XMR:
                    snprintf(cmd2, BUFFER_SIZE, "bash ../scripts/run_benchmark_xmr.sh %i %i %i %i",
                             dci.device_id_nvml, dci.device_id_cuda, mem_clock, graph_clock);
                    break;
                default:
                    THROW_RUNTIME_ERROR("Invalid enum value");
            }
            process_management::gpu_start_process(cmd2, dci.device_id_nvml, process_type::MINER, false);
        }

        //get last measurement from data file
        double data[6] = {0};
        {
            std::string filename = "bench_result_gpu" + std::to_string(dci.device_id_nvml)
                                   + "_" + enum_to_string(ct) + ".dat";
            std::ifstream file;
            file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            file.open(filename, std::ios_base::ate);//open file
            file.exceptions(std::ifstream::badbit);
            int c = 0;
            int length = file.tellg();//Get file size
            // loop backward over the file
            for (int i = length - 2; i > 0; i--) {
                file.seekg(i);
                c = file.get();
                if (c == '\r' || c == '\n')//new line?
                    break;
            }
            std::string tmp;
            std::getline(file, tmp);//read last line
            char *pt;
            pt = strtok(&tmp[0], ",");
            int idx = 0;
            while (pt != nullptr) {
                data[idx++] = std::stod(pt);
                pt = strtok(nullptr, ",");
            }
        }
        measurement m(mem_clock, graph_clock, data[2], data[3]);
        m.hashrate_measure_dur_ms_ = data[5];
        m.power_measure_dur_ms_ = data[5];
        return m;
    }


    bool start_power_monitoring_script(int device_id, int interval_sleep_ms) {
        //start power monitoring in background process
        char cmd[BUFFER_SIZE];
        snprintf(cmd, BUFFER_SIZE, "./gpu_power_monitor %i %i", device_id, interval_sleep_ms);
        return process_management::gpu_start_process(cmd, device_id, process_type::POWER_MONITOR, true);
    }

    bool stop_power_monitoring_script(int device_id) {
        //stop power monitoring
        return process_management::gpu_kill_background_process(device_id, process_type::POWER_MONITOR);
    }

    double get_avg_power_usage(int device_id, long long int system_timestamp_start_ms,
                               long long int system_timestamp_end_ms) {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open("power_results_" + std::to_string(device_id) + ".txt");
        file.exceptions(std::ifstream::badbit);
        std::string line;
        std::string::size_type sz = 0;
        double res = 0;
        int count = 0;
        while (std::getline(file, line)) {
            try {
                long long int time = std::stoll(line, &sz);
                if (time >= system_timestamp_start_ms && time <= system_timestamp_end_ms) {
                    res += std::stod(line.substr(sz + 1, std::string::npos));
                    count++;
                }
            }
            catch (const std::invalid_argument &ex) {
                LOG(ERROR) << "Failed to parse power logfile entry: " << ex.what() << std::endl;
            }
        }
        return res / count;
    }

    void change_clocks_nvml_nvapi(const device_clock_info &dci,
                                  int mem_oc, int nvml_graph_clock_idx) {
        int graph_clock = dci.nvml_graph_clocks[nvml_graph_clock_idx];
        if (dci.nvml_supported_) {
            nvapiOC(dci.device_id_nvapi, 0, mem_oc);
            nvmlOC(dci.device_id_nvml, graph_clock, dci.nvml_mem_clocks[1]);
        } else {
            int graph_oc = graph_clock - dci.nvapi_default_graph_clock;
            nvapiOC(dci.device_id_nvapi, graph_oc, mem_oc);
        }
    }

    void change_clocks_nvapi_only(const device_clock_info &dci,
                                  int mem_oc, int graph_oc) {
        nvapiOC(dci.device_id_nvapi, graph_oc, mem_oc);
    }

    measurement run_benchmark_script_nvml_nvapi(currency_type ct, const device_clock_info &dci,
                                                int mem_oc, int nvml_graph_clock_idx) {
        //change graph and mem clocks
        change_clocks_nvml_nvapi(dci, mem_oc, nvml_graph_clock_idx);
        int mem_clock = dci.nvapi_default_mem_clock + mem_oc;
        int graph_clock = dci.nvml_graph_clocks[nvml_graph_clock_idx];
        //run script_running
        measurement &&m = run_benchmark_script(ct, dci, graph_clock, mem_clock);
        m.nvml_graph_clock_idx = nvml_graph_clock_idx;
        m.mem_oc = mem_oc;
        m.graph_oc = 0;
        return m;
    }

    measurement run_benchmark_script_nvapi_only(currency_type ct, const device_clock_info &dci,
                                                int mem_oc, int graph_oc) {
        //change graph and mem clocks
        change_clocks_nvapi_only(dci, mem_oc, graph_oc);
        int mem_clock = dci.nvapi_default_mem_clock + mem_oc;
        int graph_clock = dci.nvapi_default_graph_clock + graph_oc;
        //run script_running
        measurement &&m = run_benchmark_script(ct, dci, graph_clock, mem_clock);
        m.nvml_graph_clock_idx = -1;
        m.mem_oc = mem_oc;
        m.graph_oc = graph_oc;
        return m;
    }


}