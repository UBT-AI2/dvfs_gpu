#include <stdlib.h>
#include <stdio.h>
#include "nvapi/nvapiOC.h"
#include "nvml/nvmlOC.h"

#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s device_id", argv[0]);
        return 1;
    }
    unsigned int device_id = atoi(argv[1]);
    //init apis
    nvapiInit();
    nvmlInit_();

    //start power monitoring in background process
    char cmd1[BUFFER_SIZE];
    snprintf(cmd1, BUFFER_SIZE, "sh ../scripts/start_pm.sh %i", device_id);
    system(cmd1);

    int nvml_default_mem_clock = getDefaultMemClock(device_id);
    int nvml_default_graph_clock = getDefaultGraphClock(device_id);
    const std::vector<int> &nvml_mem_clocks = getAvailableMemClocks(device_id);
    const std::vector<int> &nvml_graph_clocks = getAvailableGraphClocks(device_id, nvml_default_mem_clock);

    int interval = 50;
    int min_mem_oc = -1000, max_mem_oc = 950;
    int min_graph_oc = interval, max_graph_oc = 1000;

    //for all supported mem clocks (nvapi)
    for (int mem_oc = min_mem_oc; mem_oc <= max_mem_oc; mem_oc += interval) {
        int mem_freq = nvml_default_mem_clock + mem_oc;

        //for all supported graph clocks (nvapi) -> overclocking
        for (int graph_oc = max_graph_oc; graph_oc >= min_graph_oc; graph_oc -= interval) {
            int graph_freq = nvml_default_graph_clock + graph_oc;
            //change graph and mem clocks
            nvapiOC(device_id, graph_freq, mem_freq);
            printf("\t Changed device clocks: mem:%i,graph:%i\n", mem_freq, graph_freq);
            //run benchmark command
            char cmd2[BUFFER_SIZE];
            snprintf(cmd2, BUFFER_SIZE, "sh ../scripts/run_benchmark_ethminer.sh %i %i %i", device_id, mem_freq,
                     graph_freq);
            system(cmd2);
        }

        //for all supported graph clocks (nvml) -> underclocking
        for (int j = 0; j < nvml_graph_clocks.size(); j += 2) {
            int graph_freq = nvml_graph_clocks[j];
            //change graph and mem clocks
            nvapiOC(device_id, 0, mem_freq);
            nvmlOC(device_id, graph_freq, nvml_default_mem_clock);
            printf("\t Changed device clocks: mem:%i,graph:%i\n", mem_freq, graph_freq);
            //run benchmark command
            char cmd2[BUFFER_SIZE];
            snprintf(cmd2, BUFFER_SIZE, "sh ../scripts/run_benchmark_ethminer.sh %i %i %i", device_id, mem_freq, graph_freq);
            system(cmd2);
        }
    }

    //stop power monitoring
    char cmd3[BUFFER_SIZE];
    snprintf(cmd3, BUFFER_SIZE, "sh ../scripts/stop_pm.sh %i", device_id);
    system(cmd3);

    nvapiUnload();
    nvmlShutdown_();
    return 0;
}
