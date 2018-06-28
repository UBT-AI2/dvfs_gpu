#include <cstdlib>
#include "../common_header/fullexpr_accum.h"
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "../script_running/process_management.h"
#include "freq_hill_climbing.h"

using namespace frequency_scaling;

int main(int argc, char **argv) {
    if (argc < 10) {
        full_expression_accumulator(std::cout) << "Usage: " << argv[0] << " <currency_type> <device_id> <max_iterations> <mem_step> "
                                             "<graph_idx_step> <min_mem_oc> <max_mem_oc> <min_graph_oc> "
                                             "<max_graph_oc> [<min_hashrate>]" << std::endl;
        return 1;
    }
    currency_type ct = string_to_currency_type(argv[1]);
    unsigned int device_id = atoi(argv[2]);
    int max_iterations = atoi(argv[3]);
    int mem_step = atoi(argv[4]);
    int graph_idx_step = atoi(argv[5]);
    int min_mem_oc = atoi(argv[6]);
    int max_mem_oc = atoi(argv[7]);
    int min_graph_oc = atoi(argv[8]);
    int max_graph_oc = atoi(argv[9]);
    double min_hashrate = -1.0;
    if (argc > 10)
        min_hashrate = atof(argv[10]);

    try {
        //init apis
        nvapiInit();
        nvmlInit_();
        process_management::register_process_cleanup_sighandler();

        //start power monitoring
        start_power_monitoring_script(device_id);

        //
        device_clock_info dci(device_id, min_mem_oc, min_graph_oc, max_mem_oc, max_graph_oc);

        //
        const measurement &m = freq_hill_climbing(&run_benchmark_script_nvml_nvapi, ct, dci, max_iterations, mem_step,
                                                  graph_idx_step, min_hashrate);
        full_expression_accumulator(std::cout) << "Best energy-hash value: " << m.energy_hash_ << std::endl;

        //stop power monitoring
        stop_power_monitoring_script(device_id);

        //unload apis
        nvapiUnload(1);
        nvmlShutdown_(true);
    } catch (const std::exception &ex) {
        full_expression_accumulator(std::cerr) << "Main caught exception: " << ex.what() << std::endl;
        full_expression_accumulator(std::cerr) << "Perform cleanup and exit..." << std::endl;
        process_management::kill_all_processes(false);
        nvapiUnload(1);
        nvmlShutdown_(true);
        return 1;
    }

    return 0;
}
