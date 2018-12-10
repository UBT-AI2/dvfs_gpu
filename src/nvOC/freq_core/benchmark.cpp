//
// Created by Alex on 12.05.2018.
//
#include "benchmark.h"
#include <string.h>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <list>
#include <thread>
#include <cuda.h>
#include <glog/logging.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../common_header/exceptions.h"
#include "../common_header/constants.h"
#include "process_management.h"
#include "log_utils.h"

namespace frequency_scaling {

    static void safeCudaCall(CUresult res) {
        if (res != CUDA_SUCCESS) {
            THROW_RUNTIME_ERROR("Cuda call failed");
        }
    }

    device_clock_info::device_clock_info(int device_id_nvml) : device_clock_info(device_id_nvml, 1, -1, 1, -1) {}

    device_clock_info::device_clock_info(int device_id_nvml, int min_mem_oc, int max_mem_oc,
                                         int min_graph_oc, int max_graph_oc) : device_clock_info(device_id_nvml,
                                                                                                 nvmlGetDefaultMemClock(
                                                                                                         device_id_nvml),
                                                                                                 nvmlGetDefaultGraphClock(
                                                                                                         device_id_nvml),
                                                                                                 min_mem_oc, max_mem_oc,
                                                                                                 min_graph_oc,
                                                                                                 max_graph_oc, 1, -1) {}

    device_clock_info::device_clock_info(int device_id_nvml, int default_mem_clock, int default_graph_clock,
                                         int min_mem_oc, int max_mem_oc, int min_graph_oc, int max_graph_oc,
                                         int default_min_flag, int default_max_flag) :
            device_id_nvml_(device_id_nvml), device_name_(nvmlGetDeviceName(device_id_nvml)),
            nvapi_default_mem_clock_(default_mem_clock), nvapi_default_graph_clock_(default_graph_clock) {
        CUresult res = cuDeviceGetByPCIBusId(&device_id_cuda_, nvmlGetBusIdString(device_id_nvml).c_str());
        if (res == CUDA_ERROR_NOT_INITIALIZED) {
            cuInit(0);
            safeCudaCall(cuDeviceGetByPCIBusId(&device_id_cuda_, nvmlGetBusIdString(device_id_nvml).c_str()));
        } else {
            safeCudaCall(res);
        }
        int cuda_cc_major;
        safeCudaCall(
                cuDeviceGetAttribute(&cuda_cc_major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, device_id_cuda_));
        if (cuda_cc_major < 6) {
            LOG(WARNING) << log_utils::gpu_log_prefix(device_id_nvml) <<
                         "Architecture < Pascal: NVML frequency setting does not work correctly. Disabling NVML..."
                         << std::endl;
            nvml_supported_ = false;
        } else {
            nvml_supported_ = nvmlCheckOCSupport(device_id_nvml);
        }
        device_id_nvapi_ = nvapiGetDeviceIndexByBusId(nvmlGetBusId(device_id_nvml));
#ifdef _WIN32
        nvapi_supported_ = (device_id_nvapi_ < 0) ? false : nvapiCheckSupport(device_id_nvapi_);
#else
        nvapi_supported_ = (device_id_nvapi_ < 0 || (cuda_cc_major < 6 && !enablePerformanceState3(device_id_nvml))) ?
                           false : nvapiCheckSupport(device_id_nvapi_);
#endif
        VLOG(0)
        << log_utils::gpu_log_prefix(device_id_nvml) << "NVAPI support: " << ((nvapi_supported_) ? "Yes. " : "No. ") <<
        "NVML support: " << ((nvml_supported_) ? "Yes" : "No. ") << std::endl;
        LOG_IF(WARNING, !nvapi_supported_ && !nvml_supported_)
        << log_utils::gpu_log_prefix(device_id_nvml)
        << "NVML and NVAPI not supported: Frequency optimization not possible"
        << std::endl;

        VLOG(0) << "Matching device by PCIBusId: NVML-Id=" << device_id_nvml << ", NVAPI-Id=" <<
                device_id_nvapi_ << ", CUDA-Id=" << device_id_cuda_ << std::endl;
        //set default min and max frequencies
        //##############################

        int min_mem_border = 0, max_mem_border = 0;
        int min_graph_border = 0, max_graph_border = 0;
        if (nvml_supported_)
            nvml_register_gpu(device_id_nvml);
        if (nvapi_supported_) {
            nvapi_register_gpu(device_id_nvapi_);
            const nvapi_clock_info &nvapi_ci_mem = nvapiGetMemClockInfo(device_id_nvapi_);
            min_mem_border = nvapi_ci_mem.min_oc_;
            max_mem_border = (max_mem_oc == default_max_flag) ? std::min(900, nvapi_ci_mem.max_oc_)
                                                              : nvapi_ci_mem.max_oc_;
            //determine min_mem_oc
            min_mem_oc_ = (min_mem_oc == default_min_flag) ? min_mem_border :
                          std::min(std::max(min_mem_oc, min_mem_border), max_mem_border);
            //determine max_mem_oc
            max_mem_oc_ = (max_mem_oc == default_max_flag) ? max_mem_border :
                          std::min(std::max(max_mem_oc, min_mem_border), max_mem_border);

            const nvapi_clock_info &nvapi_ci_graph = nvapiGetGraphClockInfo(device_id_nvapi_);
            min_graph_border = (!nvml_supported_) ? nvapi_ci_graph.min_oc_ :
                               nvmlGetAvailableGraphClocks(device_id_nvml).back() - nvapi_default_graph_clock_;
            max_graph_border = (max_graph_oc == default_max_flag) ? std::min(100, nvapi_ci_graph.max_oc_)
                                                                  : nvapi_ci_graph.max_oc_;
            //determine min_graph_oc
            min_graph_oc_ = (min_graph_oc == default_min_flag) ? min_graph_border :
                            std::min(std::max(min_graph_oc, min_graph_border), max_graph_border);
            //determine max_graph_oc
            max_graph_oc_ = (max_graph_oc == default_max_flag) ? max_graph_border :
                            std::min(std::max(max_graph_oc, min_graph_border), max_graph_border);
        } else {
            min_graph_border = (!nvml_supported_) ? 0 :
                               nvmlGetAvailableGraphClocks(device_id_nvml).back() - nvapi_default_graph_clock_;
            min_mem_oc_ = max_mem_oc_ = 0;
            min_graph_oc_ = (min_graph_oc == default_min_flag) ? min_graph_border :
                            std::min(std::max(min_graph_oc, min_graph_border), max_graph_border);
			max_graph_oc_ = (max_graph_oc == default_max_flag) ? max_graph_border :
				std::min(std::max(max_graph_oc, min_graph_border), max_graph_border);
        }
        //should not happen
        if (min_mem_oc_ > max_mem_oc_)
            THROW_RUNTIME_ERROR("max_mem_oc >= min_mem_oc violated");
        if (min_graph_oc_ > max_graph_oc_)
            THROW_RUNTIME_ERROR("max_graph_oc >= min_graph_oc violated");
        //log
        VLOG(0)
        << log_utils::gpu_log_prefix(device_id_nvml) << "Default clocks: mem=" << nvapi_default_mem_clock_ << ",graph="
        << nvapi_default_graph_clock_ << std::endl;
        VLOG(0) << log_utils::gpu_log_prefix(device_id_nvml) << "Used clock range: min_mem="
                << nvapi_default_mem_clock_ + min_mem_oc_ << ",max_mem=" << nvapi_default_mem_clock_ + max_mem_oc_ <<
                ",min_graph=" << nvapi_default_graph_clock_ + min_graph_oc_ <<
                ",max_graph=" << nvapi_default_graph_clock_ + max_graph_oc_ << std::endl;
        //fill frequency vectors
        //###############################
        int oc_interval = 15;
        std::list<int> all_available_graph_clocks;
        if (nvml_supported_) {
            if (nvapi_supported_) {
                for (int graph_oc = max_graph_border; graph_oc >= oc_interval; graph_oc -= oc_interval) {
                    all_available_graph_clocks.push_back(nvapi_default_graph_clock_ + graph_oc);
                }
            }
            for (int graph_clock : nvmlGetAvailableGraphClocks(device_id_nvml))
                all_available_graph_clocks.push_back(graph_clock);
        } else if (nvapi_supported_) {
            //fake nvml vectors
            for (int graph_oc = max_graph_border; graph_oc >= min_graph_border; graph_oc -= oc_interval) {
                all_available_graph_clocks.push_back(nvapi_default_graph_clock_ + graph_oc);
            }
        }
        //cleanup
        if (all_available_graph_clocks.empty())
            nvml_graph_clocks_.push_back(nvapi_default_graph_clock_);
        else {
            int min_graph_clock = nvapi_default_graph_clock_ + min_graph_oc_;
            int max_graph_clock = nvapi_default_graph_clock_ + max_graph_oc_;
            for (auto it = all_available_graph_clocks.begin(); it != all_available_graph_clocks.end();) {
                if (*it > max_graph_clock)
                    it = all_available_graph_clocks.erase(it);
                else
                    ++it;
            }
            for (auto it = all_available_graph_clocks.end(); it != all_available_graph_clocks.begin();) {
                if (all_available_graph_clocks.size() == 1)
                    break;
                if (*(--it) < min_graph_clock)
                    it = all_available_graph_clocks.erase(it);
            }
            nvml_graph_clocks_.insert(nvml_graph_clocks_.begin(), all_available_graph_clocks.begin(),
                                      all_available_graph_clocks.end());
        }
    }

    device_clock_info device_clock_info::create_dci(int device_id_nvml, int min_mem_clock, int max_mem_clock,
                                                    int min_graph_clock, int max_graph_clock) {
        //get default clocks from nvml (no oc support required)
        int default_graph_clock = nvmlGetDefaultGraphClock(device_id_nvml);
        int default_mem_clock = nvmlGetDefaultMemClock(device_id_nvml);
        int min_mem_oc = (min_mem_clock < 0) ? std::numeric_limits<int>::max() : min_mem_clock - default_mem_clock;
        int min_graph_oc = (min_graph_clock < 0) ? std::numeric_limits<int>::max() : min_graph_clock -
                                                                                     default_graph_clock;
        int max_mem_oc = (max_mem_clock < 0) ? std::numeric_limits<int>::min() : max_mem_clock - default_mem_clock;
        int max_graph_oc = (max_graph_clock < 0) ? std::numeric_limits<int>::min() : max_graph_clock -
                                                                                     default_graph_clock;
        return device_clock_info(device_id_nvml, default_mem_clock, default_graph_clock, min_mem_oc, max_mem_oc,
                                 min_graph_oc, max_graph_oc, std::numeric_limits<int>::max(),
                                 std::numeric_limits<int>::min());
    }

    bool device_clock_info::is_graph_oc_supported() const {
        return nvml_graph_clocks_.size() > 1;
    }

    bool device_clock_info::is_mem_oc_supported() const {
        return max_mem_oc_ - min_mem_oc_ > 0;
    }

    int device_clock_info::find_graph_idx(int graph_clock) const {
        //sorted descending
        for (int i = 0; i < nvml_graph_clocks_.size(); i++) {
            if (nvml_graph_clocks_[i] <= graph_clock) {
                return i;
            }
        }
        return nvml_graph_clocks_.size() - 1;
    }

    int device_clock_info::get_mem_oc(int mem_clock) const {
        return mem_clock - nvapi_default_mem_clock_;
    }

    measurement::measurement() : mem_clock_(0), graph_clock_(0), power_(0),
                                 hashrate_(0), energy_hash_(0),
                                 nvml_graph_clock_idx_(-1), mem_oc_(0), graph_oc_(0) {}

    measurement::measurement(int mem_clock, int graph_clock, double power, double hashrate) :
            mem_clock_(mem_clock), graph_clock_(graph_clock), power_(power),
            hashrate_(hashrate),
            energy_hash_((power == 0) ? std::numeric_limits<double>::lowest() : hashrate / power),
            nvml_graph_clock_idx_(-1), mem_oc_(0), graph_oc_(0) {}


    void measurement::update_power(double power, int power_measure_dur_ms) {
        power_ = power;
        power_measure_dur_ms_ = power_measure_dur_ms;
        energy_hash_ = (power == 0) ? std::numeric_limits<double>::lowest() : hashrate_ / power;
    }

    void measurement::update_hashrate(double hashrate, int hashrate_measure_dur_ms) {
        hashrate_ = hashrate;
        hashrate_measure_dur_ms_ = hashrate_measure_dur_ms;
        energy_hash_ = hashrate / power_;
    }

    void measurement::update_freq_config(const measurement &other) {
        mem_clock_ = other.mem_clock_;
        graph_clock_ = other.graph_clock_;
        mem_oc_ = other.mem_oc_;
        graph_oc_ = other.graph_oc_;
        nvml_graph_clock_idx_ = other.nvml_graph_clock_idx_;
    }

    bool measurement::self_check() const {
        if (hashrate_ <= 0 || !std::isfinite(hashrate_))
            return false;
        if (power_ <= 0 || !std::isfinite(power_))
            return false;
        return true;
    }


    static measurement run_benchmark_script(const currency_type &ct, const device_clock_info &dci,
                                            int graph_clock, int mem_clock) {
        {
            VLOG(1) << log_utils::gpu_log_prefix(ct, dci.device_id_nvml_) <<
                    "Running offline benchmark with clocks: mem=" << mem_clock << ",graph=" << graph_clock
                    << std::endl;
            if (!log_utils::check_file_existance(
                    log_utils::get_offline_bench_filename(ct, dci.device_id_nvml_))) {
                std::ofstream logfile(log_utils::get_offline_bench_filename(ct, dci.device_id_nvml_));
                if (!logfile)
                    THROW_IO_ERROR(
                            "Cannot open " + log_utils::get_offline_bench_filename(ct, dci.device_id_nvml_));
                logfile << "#mem_clock,graph_clock,power,hashrate,energy_hash,bench_duration,timestamp"
                        << std::endl;
            }
            //run benchmark script to get measurement
            char cmd[BUFFER_SIZE];
            if (ct.use_ccminer_) {
                snprintf(cmd, BUFFER_SIZE, "bash %s %i %i %i %i %s %s %s",
                         ct.bench_script_path_.c_str(), dci.device_id_nvml_, dci.device_id_cuda_,
                         mem_clock, graph_clock, log_utils::get_logdir_name().c_str(),
                         ct.currency_name_.c_str(), ct.ccminer_algo_.c_str());
            } else {
                snprintf(cmd, BUFFER_SIZE, "bash %s %i %i %i %i %s",
                         ct.bench_script_path_.c_str(), dci.device_id_nvml_, dci.device_id_cuda_,
                         mem_clock, graph_clock, log_utils::get_logdir_name().c_str());
            }

            process_management::gpu_start_process(cmd, dci.device_id_nvml_, process_type::MINER, false);
        }

        //get last measurement from data file
        std::vector<double> data;
        {
            std::ifstream file(log_utils::get_offline_bench_filename(ct, dci.device_id_nvml_),
                               std::ios_base::ate);
            if (!file)
                THROW_IO_ERROR("Cannot open " + log_utils::get_offline_bench_filename(ct, dci.device_id_nvml_));
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
        //
        if (data.size() < 6) {
            LOG(ERROR) << log_utils::gpu_log_prefix(ct, dci.device_id_nvml_) <<
                       "Offline benchmark with clocks : mem=" << mem_clock << ",graph=" << graph_clock
                       << " failed"
                       << std::endl;
            //return invalid measurement
            return measurement(mem_clock, graph_clock, 0, 0);
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
        if (process_management::gpu_start_process(cmd, device_id, process_type::POWER_MONITOR, true)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(3 * interval_sleep_ms));
            return true;
        }
        return false;
    }

    bool stop_power_monitoring_script(int device_id) {
        //stop power monitoring
        return process_management::gpu_kill_background_process(device_id, process_type::POWER_MONITOR);
    }

    double get_avg_power_usage(int device_id, long long int system_timestamp_start_ms,
                               long long int system_timestamp_end_ms) {
        std::ifstream file(log_utils::get_power_log_filename(device_id));
        if (!file)
            THROW_IO_ERROR("Cannot open " + log_utils::get_power_log_filename(device_id));
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
        return (count == 0) ? 0 : res / count;
    }

    void change_gpu_clocks(const device_clock_info &dci,
                           int mem_oc, int nvml_graph_clock_idx) {
        if (dci.nvapi_supported_) {
            int graph_clock = dci.nvml_graph_clocks_[nvml_graph_clock_idx];
            if (!dci.nvml_supported_ || graph_clock > dci.nvapi_default_graph_clock_) {
                int graph_oc = graph_clock - dci.nvapi_default_graph_clock_;
                nvapiOC(dci.device_id_nvapi_, graph_oc, mem_oc);
            } else {
                nvapiOC(dci.device_id_nvapi_, 0, mem_oc);
                //setting of mem clock has no effect with nvml
                nvmlOC(dci.device_id_nvml_, graph_clock, dci.nvapi_default_mem_clock_);
            }
        } else if (dci.nvml_supported_) {
            int graph_clock = dci.nvml_graph_clocks_[nvml_graph_clock_idx];
            if (mem_oc != 0)
                THROW_RUNTIME_ERROR("mem_oc != 0 althought nvapi not supported");
            nvmlOC(dci.device_id_nvml_, graph_clock, dci.nvapi_default_mem_clock_);
        } else {
            if (mem_oc != 0 || nvml_graph_clock_idx != 0)
                THROW_RUNTIME_ERROR(
                        "mem_oc != 0 or graph_clock_idx != 0 although nvapi and nvml not supported");
        }
    }


    measurement run_benchmark_mining_offline(const currency_type &ct, const device_clock_info &dci,
                                             int mem_oc, int nvml_graph_clock_idx) {
        //change graph and mem clocks
        change_gpu_clocks(dci, mem_oc, nvml_graph_clock_idx);
        int mem_clock = dci.nvapi_default_mem_clock_ + mem_oc;
        int graph_clock = dci.nvml_graph_clocks_[nvml_graph_clock_idx];
        //run script_running
        measurement &&m = run_benchmark_script(ct, dci, graph_clock, mem_clock);
        m.nvml_graph_clock_idx_ = nvml_graph_clock_idx;
        m.mem_oc_ = mem_oc;
        m.graph_oc_ = graph_clock - dci.nvapi_default_graph_clock_;
        return m;
    }
}
