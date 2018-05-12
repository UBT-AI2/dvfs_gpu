#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "nvapi/nvapiOC.h"
#include "nvml/nvmlOC.h"

#define BUFFER_SIZE 1024

struct device_clock_info {
    int device_id_nvml, device_id_nvapi;
    int nvml_default_mem_clock, nvml_default_graph_clock;
    std::vector<int> nvml_mem_clocks, nvml_graph_clocks;
    int min_mem_oc, min_graph_oc;
    int max_mem_oc, max_graph_oc;
};

struct measurement {
    measurement() : mem_clock_(0), graph_clock_(0), power_(0),
            hashrate_(0), energy_hash_(0) {}
    measurement(int mem_clock, int graph_clock, float power, float hashrate) :
            mem_clock_(mem_clock), graph_clock_(graph_clock), power_(power),
            hashrate_(hashrate), energy_hash_(hashrate / power) {}

    int mem_clock_, graph_clock_;
    float power_, hashrate_, energy_hash_;
    int nvml_graph_clock_idx, mem_oc;
};

measurement run_benchmark(const device_clock_info& dci, int mem_oc, int nvml_graph_clock_idx){
    //change graph and mem clocks
    int graph_clock = dci.nvml_graph_clocks[nvml_graph_clock_idx];
    nvapiOC(dci.device_id_nvapi, 0, mem_oc);
    nvmlOC(dci.device_id_nvml, graph_clock, dci.nvml_default_mem_clock);
    int mem_clock = dci.nvml_default_mem_clock + mem_oc;
    printf("\t Changed device clocks: mem:%i,graph:%i\n", mem_clock, graph_clock);
    //run benchmark command
    char cmd2[BUFFER_SIZE];
    snprintf(cmd2, BUFFER_SIZE, "sh ../scripts/run_benchmark_ethminer.sh %i %i %i",
             dci.device_id_nvml, mem_clock, graph_clock);
    system(cmd2);
    //
    float data[5] = {0};
    FILE *pipe = popen("type result.dat", "r");
    if (pipe) {
        char buffer[BUFFER_SIZE] = {0};
        if (fgets(buffer, BUFFER_SIZE, pipe)) {
            char *pt;
            pt = strtok(buffer, ",");
            int idx = 0;
            while (pt != NULL) {
                data[idx++] = atof(pt);
                pt = strtok(NULL, ",");
            }

            printf("%s\n", buffer);
        }
        pclose(pipe);
    }
    measurement m((int) data[0], (int) data[1], data[2], data[3]);
    m.nvml_graph_clock_idx = nvml_graph_clock_idx;
    m.mem_oc = mem_oc;
    return m;
}


std::vector<measurement> explore_neighborhood(const device_clock_info& dci, const measurement& current_node,
                                              int mem_step, int graph_step_idx){
    std::vector<measurement> neighbor_nodes;
    int mem_plus = current_node.mem_oc + mem_step;
    int mem_minus = current_node.mem_oc - mem_step;
    int graph_plus_idx = current_node.nvml_graph_clock_idx + graph_step_idx;
    int graph_minus_idx = current_node.nvml_graph_clock_idx - graph_step_idx;
    //oc memory
    if(mem_plus <= dci.max_mem_oc)
        neighbor_nodes.push_back(run_benchmark(dci, mem_plus, current_node.nvml_graph_clock_idx));
    if(mem_minus >= dci.min_mem_oc)
        neighbor_nodes.push_back(run_benchmark(dci, mem_minus, current_node.nvml_graph_clock_idx));

    //oc graphics
    if(graph_plus_idx < dci.nvml_graph_clocks.size())
        neighbor_nodes.push_back(run_benchmark(dci, current_node.mem_oc, graph_plus_idx));
    if(graph_minus_idx >= 0)
        neighbor_nodes.push_back(run_benchmark(dci, current_node.mem_oc, graph_minus_idx));

    return neighbor_nodes;
}


int main(int argc, char **argv) {
    if (argc < 7) {
        printf("Usage: %s device_id use_nvmlUC min_mem_oc max_mem_oc min_graph_oc max_graph_oc", argv[0]);
        return 1;
    }
    unsigned int device_id = atoi(argv[1]);
    int interval = 50;
    int use_nvmlUC = atoi(argv[2]);
    int min_mem_oc = atoi(argv[3]), max_mem_oc = atoi(argv[4]);
    int min_graph_oc = atoi(argv[5]), max_graph_oc = atoi(argv[6]);
    if (use_nvmlUC)
        min_graph_oc = interval;


    //init apis
    nvapiInit();
    nvmlInit_();

    //start power monitoring in background process
    char cmd1[BUFFER_SIZE];
    snprintf(cmd1, BUFFER_SIZE, "sh ../scripts/start_pm.sh %i", device_id);
    system(cmd1);

    device_clock_info dci;
    dci.device_id_nvml = device_id;
    dci.device_id_nvapi = getNumDevices() - 1 - device_id;
    dci.nvml_default_mem_clock = getDefaultMemClock(device_id);
    dci.nvml_mem_clocks = getAvailableMemClocks(device_id);
    dci.nvml_graph_clocks = getAvailableGraphClocks(device_id, dci.nvml_default_mem_clock);
    dci.nvml_default_graph_clock = dci.nvml_graph_clocks[0];
    dci.min_graph_oc = min_graph_oc;
    dci.min_mem_oc = min_mem_oc;
    dci.max_graph_oc = max_graph_oc;
    dci.max_mem_oc = max_mem_oc;

    //hill climbing params
    int max_iterations = 10;
    int mem_step = 100;
    int graph_idx_step = 4;
    //initial guess
    measurement current_node = run_benchmark(dci, (max_mem_oc - min_mem_oc) / 2,
            dci.nvml_graph_clocks.size()/2);
    measurement best_node = current_node;
    //exploration
    for(int i = 0; i<max_iterations; i++){
        const std::vector<measurement>& neighbors = explore_neighborhood(dci, current_node, mem_step, graph_idx_step);
        float tmp_val = -1e37;
        for(measurement n : neighbors){
            if(n.energy_hash_ > tmp_val) {
                current_node = n;
                tmp_val = n.energy_hash_;
            }
        }
        if(current_node.energy_hash_ > best_node.energy_hash_){
            best_node = current_node;
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
