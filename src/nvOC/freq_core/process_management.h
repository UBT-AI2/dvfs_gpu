//
// Created by alex on 30.05.18.
//
#pragma once

#include <cstdio>
#include <string>
#include <map>
#include <vector>
#include <mutex>

namespace frequency_scaling {

    enum class process_type {
        MINER, POWER_MONITOR, count
    };


    namespace process_management {
        void register_process_cleanup_sighandler();

        FILE *popen_file(const std::string &cmd);

        std::string popen_string(const std::string &cmd);

        bool gpu_has_background_process(int device_id, process_type pt);

        bool gpu_kill_background_process(int device_id, process_type pt);

        bool gpu_start_process(const std::string &filename,
                               int device_id, process_type pt, bool background);

        bool has_process(int pid);

        void kill_all_processes(bool only_background);

        void kill_process(int pid);

        int start_process(const std::string &cmd, bool background);
    }

}
