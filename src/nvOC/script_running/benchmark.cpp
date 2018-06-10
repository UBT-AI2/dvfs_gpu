//
// Created by Alex on 12.05.2018.
//
#include "benchmark.h"

#include <string.h>
#include <fstream>
#include <cuda.h>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "process_management.h"

namespace frequency_scaling {

    static const int BUFFER_SIZE = 1024;

    static measurement run_benchmark_script(currency_type ct, const device_clock_info &dci,
                                            int graph_clock, int mem_clock) {
        {
            printf("Running %s benchmark script with clocks: mem:%i,graph:%i\n",
                   enum_to_string(ct).c_str(), mem_clock, graph_clock);
            //run script_running command
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
                    throw std::runtime_error("Invalid enum value");
            }
            process_management::gpu_start_process(cmd2, dci.device_id_nvml, process_type::MINER, false);
        }

        //get last measurement from data file
        float data[5] = {0};
        {
            std::string filename = "result_" + std::to_string(dci.device_id_nvml) + ".dat";
            std::ifstream file(filename, std::ios_base::ate);//open file
            if (file) {
                std::string tmp;
                int c = 0;
                int length = file.tellg();//Get file size
                // loop backward over the file
                for (int i = length - 2; i > 0; i--) {
                    file.seekg(i);
                    c = file.get();
                    if (c == '\r' || c == '\n')//new line?
                        break;
                }
                std::getline(file, tmp);//read last line
                char *pt;
                pt = strtok(&tmp[0], ",");
                int idx = 0;
                while (pt != nullptr) {
                    data[idx++] = atof(pt);
                    pt = strtok(nullptr, ",");
                }
            }
        }
        measurement m((int) data[0], (int) data[1], data[2], data[3]);
        return m;
    }


    void start_power_monitoring_script(int device_id) {
        //start power monitoring in background process
        char cmd[BUFFER_SIZE];
        snprintf(cmd, BUFFER_SIZE, "./gpu_power_monitor %i", device_id);
        process_management::gpu_start_process(cmd, device_id, process_type::POWER_MONITOR, true);
    }

    void stop_power_monitoring_script(int device_id) {
        //stop power monitoring
        process_management::gpu_kill_background_process(device_id, process_type::POWER_MONITOR);
    }

    void change_clocks_nvml_nvapi(const device_clock_info &dci,
                                  int mem_oc, int nvml_graph_clock_idx) {
        int graph_clock = dci.nvml_graph_clocks[nvml_graph_clock_idx];
        nvapiOC(dci.device_id_nvapi, 0, mem_oc);
        nvmlOC(dci.device_id_nvml, graph_clock, dci.nvml_mem_clocks[1]);
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

    device_clock_info::device_clock_info(int device_id_nvml, int min_mem_oc, int min_graph_oc, int max_mem_oc,
                                         int max_graph_oc) : device_id_nvml(device_id_nvml), min_mem_oc(min_mem_oc),
                                                             min_graph_oc(min_graph_oc), max_mem_oc(max_mem_oc),
                                                             max_graph_oc(max_graph_oc) {
        device_id_nvapi = nvapiGetDeviceIndexByBusId(nvmlGetBusId(device_id_nvml));
        CUresult res = cuDeviceGetByPCIBusId(&device_id_cuda, nvmlGetBusIdString(device_id_nvml).c_str());
        if (res == CUDA_ERROR_NOT_INITIALIZED) {
            cuInit(0);
            cuDeviceGetByPCIBusId(&device_id_cuda, nvmlGetBusIdString(device_id_nvml).c_str());
        }
        nvml_mem_clocks = nvmlGetAvailableMemClocks(device_id_nvml);
        nvml_graph_clocks = nvmlGetAvailableGraphClocks(device_id_nvml, nvml_mem_clocks[1]);
        nvapi_default_mem_clock = nvml_mem_clocks[0];
        nvapi_default_graph_clock = nvml_graph_clocks[0];
    }


}