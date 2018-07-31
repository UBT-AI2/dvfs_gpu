//
// Created by Alex on 12.05.2018.
//
#include "benchmark.h"

#include <string.h>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <cuda.h>
#include <glog/logging.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../common_header/exceptions.h"
#include "../common_header/constants.h"
#include "process_management.h"
#include "log_utils.h"

namespace frequency_scaling {

    device_clock_info::device_clock_info(int device_id_nvml) : device_clock_info(device_id_nvml, 1, 1, -1, -1) {}

    device_clock_info::device_clock_info(int device_id_nvml, int min_mem_oc,
                                         int min_graph_oc, int max_mem_oc,
                                         int max_graph_oc) : device_id_nvml_(device_id_nvml), min_mem_oc_(min_mem_oc),
                                                             min_graph_oc_(min_graph_oc), max_mem_oc_(max_mem_oc),
                                                             max_graph_oc_(max_graph_oc) {
        nvml_supported_ = nvmlCheckOCSupport(device_id_nvml);
        device_id_nvapi_ = nvapiGetDeviceIndexByBusId(nvmlGetBusId(device_id_nvml));
        nvapi_supported_ = (device_id_nvapi_ < 0) ? false : nvapiCheckSupport(device_id_nvapi_);
        CUresult res = cuDeviceGetByPCIBusId(&device_id_cuda_, nvmlGetBusIdString(device_id_nvml).c_str());
        if (res == CUDA_ERROR_NOT_INITIALIZED) {
            cuInit(0);
            cuDeviceGetByPCIBusId(&device_id_cuda_, nvmlGetBusIdString(device_id_nvml).c_str());
        }
        VLOG(0) << "Matching device by PCIBusId: NVML-Id=" << device_id_nvml << ", NVAPI-Id=" <<
                device_id_nvapi_ << ", CUDA-Id=" << device_id_cuda_ << std::endl;
        if (!nvapi_supported_ && !nvml_supported_)
            THROW_RUNTIME_ERROR("GPU " + std::to_string(device_id_nvml) +
                                ": NVML and NVAPI not supported: No kind of OC possible");

        //##############################
        if (nvml_supported_)
            nvml_register_gpu(device_id_nvml);
        if (nvapi_supported_) {
            nvapi_register_gpu(device_id_nvapi_);
            const nvapi_clock_info &nvapi_ci_mem = nvapiGetMemClockInfo(device_id_nvapi_);
            nvapi_default_mem_clock_ = nvapi_ci_mem.current_freq_;
            if (min_mem_oc > 0)
                min_mem_oc_ = nvapi_ci_mem.min_oc_;
            if (max_mem_oc < 0)
                max_mem_oc_ = std::min(900, nvapi_ci_mem.max_oc_);

            const nvapi_clock_info &nvapi_ci_graph = nvapiGetGraphClockInfo(device_id_nvapi_);
            nvapi_default_graph_clock_ = nvapi_ci_graph.current_freq_;
            if (min_graph_oc > 0)
                min_graph_oc_ = nvapi_ci_graph.min_oc_;
            if (max_graph_oc < 0)
                max_graph_oc_ = std::min(150, nvapi_ci_graph.max_oc_);
        } else {
            min_mem_oc_ = max_mem_oc_ = 0;
            nvapi_default_mem_clock_ = nvmlGetAvailableMemClocks(device_id_nvml)[0];
            const std::vector<int> &graph_clocks = nvmlGetAvailableGraphClocks(device_id_nvml,
                                                                               nvapi_default_mem_clock_);
            nvapi_default_graph_clock_ = graph_clocks[0];
            max_graph_oc_ = 0;
            if (!nvml_supported_)
                min_graph_oc_ = 0;
            else if (min_graph_oc > 0)
                min_graph_oc_ = graph_clocks.back() - nvapi_default_graph_clock_;
        }

        if (min_mem_oc_ > max_mem_oc_)
            throw std::invalid_argument("max_mem_oc >= min_mem_oc violated");
        if (min_graph_oc_ > max_graph_oc_)
            throw std::invalid_argument("max_graph_oc >= min_graph_oc violated");

        //###############################

        int oc_interval = 15;
        if (nvml_supported_) {
            nvml_mem_clocks_ = nvmlGetAvailableMemClocks(device_id_nvml);
            if (nvapi_supported_) {
                for (int graph_oc = max_graph_oc_; graph_oc >= oc_interval; graph_oc -= oc_interval) {
                    if (min_graph_oc <= 0 && graph_oc < min_graph_oc_)
                        break;
                    nvml_graph_clocks_.push_back(nvapi_default_graph_clock_ + graph_oc);
                }
            }
            for (int graph_clock : nvmlGetAvailableGraphClocks(device_id_nvml, nvml_mem_clocks_[0])) {
                int graph_oc = graph_clock - nvapi_default_graph_clock_;
                if ((min_graph_oc <= 0 && graph_oc < min_graph_oc_) ||
                    (max_graph_oc >= 0 && graph_oc > max_graph_oc_))
                    continue;
                if (graph_clock <= nvapi_default_graph_clock_)
                    nvml_graph_clocks_.push_back(graph_clock);
            }
        } else if (nvapi_supported_) {
            //fake nvml vectors
            for (int graph_oc = max_graph_oc_; graph_oc >= min_graph_oc_; graph_oc -= oc_interval) {
                nvml_graph_clocks_.push_back(nvapi_default_graph_clock_ + graph_oc);
            }
            for (int mem_oc = max_mem_oc_; mem_oc >= min_mem_oc_; mem_oc -= oc_interval) {
                nvml_mem_clocks_.push_back(nvapi_default_mem_clock_ + mem_oc);
            }
        } else {
            nvml_graph_clocks_.push_back(nvapi_default_graph_clock_);
            nvml_mem_clocks_.push_back(nvapi_default_mem_clock_);
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

    bool measurement::self_check() const {
        if (hashrate_ <= 0 || !std::isfinite(hashrate_))
            return false;
        if (power_ <= 0 || !std::isfinite(power_))
            return false;
        return true;
    }


    static measurement run_benchmark_script(currency_type ct, const device_clock_info &dci,
                                            int graph_clock, int mem_clock) {
        {
            VLOG(0) << gpu_log_prefix(ct, dci.device_id_nvml_) <<
                    "Running offline benchmark with clocks: mem:" << mem_clock << ",graph:" << graph_clock
                    << std::endl;
            //run benchmark script to get measurement
            char cmd2[BUFFER_SIZE];
            switch (ct) {
                case currency_type::ETH:
                    snprintf(cmd2, BUFFER_SIZE, "bash ../scripts/run_benchmark_eth.sh %i %i %i %i %s",
                             dci.device_id_nvml_, dci.device_id_cuda_, mem_clock, graph_clock,
                             log_utils::get_logdir_name().c_str());
                    break;
                case currency_type::ZEC:
                    snprintf(cmd2, BUFFER_SIZE, "bash ../scripts/run_benchmark_zec.sh %i %i %i %i %s",
                             dci.device_id_nvml_, dci.device_id_cuda_, mem_clock, graph_clock,
                             log_utils::get_logdir_name().c_str());
                    break;
                case currency_type::XMR:
                    snprintf(cmd2, BUFFER_SIZE, "bash ../scripts/run_benchmark_xmr.sh %i %i %i %i %s",
                             dci.device_id_nvml_, dci.device_id_cuda_, mem_clock, graph_clock,
                             log_utils::get_logdir_name().c_str());
                    break;
                default:
                    THROW_RUNTIME_ERROR("Invalid enum value");
            }
            process_management::gpu_start_process(cmd2, dci.device_id_nvml_, process_type::MINER, false);
        }

        //get last measurement from data file
        std::vector<double> data;
        {
            std::string filename = log_utils::get_logdir_name() + "/" +
                                   log_utils::get_offline_bench_filename(ct, dci.device_id_nvml_);
            std::ifstream file(filename, std::ios_base::ate);
            if (!file)
                THROW_IO_ERROR("Cannot open " + filename);
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
            while (pt != nullptr) {
                data.push_back(std::stod(pt));
                pt = strtok(nullptr, ",");
            }
        }
        measurement m(mem_clock, graph_clock, data.at(2), data.at(3));
        m.hashrate_measure_dur_ms_ = data.at(5);
        m.power_measure_dur_ms_ = data.at(5);
        return m;
    }


    bool start_power_monitoring_script(int device_id, int interval_sleep_ms) {
        //start power monitoring in background process
        char cmd[BUFFER_SIZE];
        snprintf(cmd, BUFFER_SIZE, "./gpu_power_monitor %i %i %s", device_id, interval_sleep_ms,
                 log_utils::get_logdir_name().c_str());
        return process_management::gpu_start_process(cmd, device_id, process_type::POWER_MONITOR, true);
    }

    bool stop_power_monitoring_script(int device_id) {
        //stop power monitoring
        return process_management::gpu_kill_background_process(device_id, process_type::POWER_MONITOR);
    }

    double get_avg_power_usage(int device_id, long long int system_timestamp_start_ms,
                               long long int system_timestamp_end_ms) {

        std::string filename = log_utils::get_logdir_name() + "/" +
                               log_utils::get_power_log_filename(device_id);
        std::ifstream file(filename);
        if (!file)
            THROW_IO_ERROR("Cannot open " + filename);
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
        int graph_clock = dci.nvml_graph_clocks_[nvml_graph_clock_idx];
        if (dci.nvapi_supported_) {
            if (!dci.nvml_supported_ || graph_clock > dci.nvapi_default_graph_clock_) {
                int graph_oc = graph_clock - dci.nvapi_default_graph_clock_;
                nvapiOC(dci.device_id_nvapi_, graph_oc, mem_oc);
            } else {
                nvapiOC(dci.device_id_nvapi_, 0, mem_oc);
                nvmlOC(dci.device_id_nvml_, graph_clock, dci.nvml_mem_clocks_[0]);
            }
        } else {
            if (mem_oc != 0)
                THROW_RUNTIME_ERROR("mem_oc != 0 althought nvapi not supported");
            nvmlOC(dci.device_id_nvml_, graph_clock, dci.nvml_mem_clocks_[0]);
        }
    }

    void change_clocks_nvapi_only(const device_clock_info &dci,
                                  int mem_oc, int graph_oc) {
        nvapiOC(dci.device_id_nvapi_, graph_oc, mem_oc);
    }

    measurement run_benchmark_script_nvml_nvapi(currency_type ct, const device_clock_info &dci,
                                                int mem_oc, int nvml_graph_clock_idx) {
        //change graph and mem clocks
        change_clocks_nvml_nvapi(dci, mem_oc, nvml_graph_clock_idx);
        int mem_clock = dci.nvapi_default_mem_clock_ + mem_oc;
        int graph_clock = dci.nvml_graph_clocks_[nvml_graph_clock_idx];
        //run script_running
        measurement &&m = run_benchmark_script(ct, dci, graph_clock, mem_clock);
        m.nvml_graph_clock_idx = nvml_graph_clock_idx;
        m.mem_oc = mem_oc;
        m.graph_oc = graph_clock - dci.nvapi_default_graph_clock_;
        return m;
    }

    measurement run_benchmark_script_nvapi_only(currency_type ct, const device_clock_info &dci,
                                                int mem_oc, int graph_oc) {
        //change graph and mem clocks
        change_clocks_nvapi_only(dci, mem_oc, graph_oc);
        int mem_clock = dci.nvapi_default_mem_clock_ + mem_oc;
        int graph_clock = dci.nvapi_default_graph_clock_ + graph_oc;
        //run script_running
        measurement &&m = run_benchmark_script(ct, dci, graph_clock, mem_clock);
        m.nvml_graph_clock_idx = -1;
        m.mem_oc = mem_oc;
        m.graph_oc = graph_oc;
        return m;
    }


}