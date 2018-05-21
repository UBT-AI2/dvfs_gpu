//
// Created by Alex on 12.05.2018.
//
#include "benchmark.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <fstream>
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"

namespace frequency_scaling {

    static const int BUFFER_SIZE = 1024;

    static measurement run_benchmark_script(miner_script ms, const device_clock_info &dci,
                                            int graph_clock, int mem_clock) {
        {
            printf("Running script_running with clocks: mem:%i,graph:%i\n", mem_clock, graph_clock);
            //run script_running command
            char cmd2[BUFFER_SIZE];
            switch (ms) {
                case miner_script::ETHMINER:
                    snprintf(cmd2, BUFFER_SIZE, "sh ../scripts/run_benchmark_ethminer.sh %i %i %i",
                             dci.device_id_nvml, mem_clock, graph_clock);
                    break;
                case miner_script::EXCAVATOR:
                    snprintf(cmd2, BUFFER_SIZE, "sh ../scripts/run_benchmark_excavator.sh %i %i %i",
                             dci.device_id_nvml, mem_clock, graph_clock);
                    break;
                case miner_script::XMRSTAK:
                    snprintf(cmd2, BUFFER_SIZE, "sh ../scripts/run_benchmark_xmrstak.sh %i %i %i",
                             dci.device_id_nvml, mem_clock, graph_clock);
                    break;
            }
            system(cmd2);
        }

        //get last measurement from data file
        float data[5] = {0};
        {
            std::ifstream file("result.dat", std::ios_base::ate);//open file
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
        char cmd1[BUFFER_SIZE];
        snprintf(cmd1, BUFFER_SIZE, "sh ../scripts/start_pm.sh %i", device_id);
        system(cmd1);
    }

    void stop_power_monitoring_script(int device_id) {
        //stop power monitoring
        char cmd3[BUFFER_SIZE];
        snprintf(cmd3, BUFFER_SIZE, "sh ../scripts/kill_process.sh %s", "gpu_power_monitor");
        system(cmd3);
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

    measurement run_benchmark_script_nvml_nvapi(miner_script ms, const device_clock_info &dci,
                                                int mem_oc, int nvml_graph_clock_idx) {
        //change graph and mem clocks
        change_clocks_nvml_nvapi(dci, mem_oc, nvml_graph_clock_idx);
        int mem_clock = dci.nvapi_default_mem_clock + mem_oc;
        int graph_clock = dci.nvml_graph_clocks[nvml_graph_clock_idx];
        //run script_running
        measurement &&m = run_benchmark_script(ms, dci, graph_clock, mem_clock);
        m.nvml_graph_clock_idx = nvml_graph_clock_idx;
        m.mem_oc = mem_oc;
        m.graph_oc = 0;
        return m;
    }

    measurement run_benchmark_script_nvapi_only(miner_script ms, const device_clock_info &dci,
                                                int mem_oc, int graph_oc) {
        //change graph and mem clocks
        change_clocks_nvapi_only(dci, mem_oc, graph_oc);
        int mem_clock = dci.nvapi_default_mem_clock + mem_oc;
        int graph_clock = dci.nvapi_default_graph_clock + graph_oc;
        //run script_running
        measurement &&m = run_benchmark_script(ms, dci, graph_clock, mem_clock);
        m.nvml_graph_clock_idx = -1;
        m.mem_oc = mem_oc;
        m.graph_oc = graph_oc;
        return m;
    }

}