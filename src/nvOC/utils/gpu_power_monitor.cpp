#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <chrono>
#include <thread>
#include "../nvml/nvmlOC.h"

static const int BUFFER_SIZE = 4096;

static void power_monitor_nvml(int device_id, int sleep_time_ms, std::ofstream &data_file);

static int power_monitor_popen(int device_id, int sleep_time_ms, std::ofstream &data_file);


int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <device_id> <interval_sleep_ms> [<log_dir>]\n", argv[0]);
        return 1;
    }
    int device_id = atoi(argv[1]);
    int sleep_time_ms = atoi(argv[2]);
    std::string log_dir = ".";
    if (argc > 3)
        log_dir = argv[3];

    std::string data_filename = log_dir + "/power_results_" + std::to_string(device_id) + ".txt";
    std::ofstream data_file(data_filename, std::ofstream::app);
    if (!data_file) {
        puts("Could not open result file");
        return 1;
    }
    //
    power_monitor_nvml(device_id, sleep_time_ms, data_file);
    return 0;
}


static void power_monitor_nvml(int device_id, int sleep_time_ms, std::ofstream &data_file) {
    frequency_scaling::nvmlInit_();
    while (true) {
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch());
        data_file << dur.count() << " " << frequency_scaling::nvmlGetPowerUsage(device_id) << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
    }
    frequency_scaling::nvmlShutdown_(false);
}


static int power_monitor_popen(int device_id, int sleep_time_ms, std::ofstream &data_file) {
    char cmd[BUFFER_SIZE];
    snprintf(cmd, BUFFER_SIZE,
             "nvidia-smi -a -i %d | grep \"Power Draw\" | cut -d ':' -f 2 | cut -d 'W' -f 1 | awk '{$1=$1};1'",
             device_id);

    while (true) {
        FILE *fp;
        /* Open the command for reading. */
#ifdef _MSC_VER
        fp = _popen(cmd, "r");
#else
        fp = popen(cmd, "r");
#endif
        if (fp == NULL) {
            printf("Failed to run command\n");
            return 1;
        }

        /* Read the output a line at a time - output it. */
        char path[BUFFER_SIZE];
        while (fgets(path, sizeof(path) - 1, fp) != NULL) {
            //printf("%s", path);
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
            data_file << dur.count() << " " << atof(path) << std::endl;
        }

        /* close */
#ifdef _MSC_VER
        _pclose(fp);
#else
        pclose(fp);
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
    }

    return 0;
}
