#include <cstdlib>
#include <glog/logging.h>
#include <boost/algorithm/string.hpp>
#include "../common_header/fullexpr_accum.h"
#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../script_running/benchmark.h"
#include "../script_running/process_management.h"
#include "freq_hill_climbing.h"

using namespace frequency_scaling;

int main(int argc, char **argv) {
    if (argc < 7) {
        full_expression_accumulator<>(std::cout) << "Usage: " << argv[0]
                                                 << " <currency_config> <currency_type> <device_id> <max_iterations> <mem_step> <graph_idx_step> "
                                                    "[<min_hashrate>] [<min_mem_oc>] [<max_mem_oc>] [<min_graph_oc>] [<max_graph_oc>]"
                                                 << std::endl;
        return 1;
    }
    const std::map<std::string, currency_type> &available_currencies = read_currency_config(argv[1]);
    const currency_type &ct = available_currencies.at(boost::algorithm::to_upper_copy(std::string(argv[2])));
    unsigned int device_id = atoi(argv[3]);
    int max_iterations = atoi(argv[4]);
    int mem_step = atoi(argv[5]);
    int graph_idx_step = atoi(argv[6]);
    double min_hashrate = -1.0;
    int min_mem_oc = 1, max_mem_oc = -1;
    int min_graph_oc = 1, max_graph_oc = -1;
    if (argc > 7)
        min_hashrate = atof(argv[7]);
    if (argc > 8)
        min_mem_oc = atoi(argv[8]);
    if (argc > 9)
        max_mem_oc = atoi(argv[9]);
    if (argc > 10)
        min_graph_oc = atoi(argv[10]);
    if (argc > 11)
        max_graph_oc = atoi(argv[11]);

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
        const measurement &m = freq_hill_climbing(&run_benchmark_mining_offline, ct, dci, max_iterations, mem_step,
                                                  graph_idx_step, min_hashrate);
        VLOG(0) << gpu_log_prefix(ct, dci.device_id_nvml_) << "Computed optimal energy-hash ratio: "
                << m.energy_hash_ << " (mem=" << m.mem_clock_ << ",graph=" << m.graph_clock_ << ")" << std::endl;

        //stop power monitoring
        stop_power_monitoring_script(device_id);

        //unload apis
        nvapiUnload(1);
        nvmlShutdown_(true);
    } catch (const std::exception &ex) {
        LOG(ERROR) << "Main caught exception: " << ex.what() << std::endl;
        LOG(ERROR) << "Perform cleanup and exit..." << std::endl;
        process_management::kill_all_processes(false);
        nvapiUnload(1);
        nvmlShutdown_(true);
        return 1;
    }

    return 0;
}
