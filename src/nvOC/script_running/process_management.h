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


    class process_management {
    public:
        static bool register_process_cleanup_sighandler();

        static FILE *popen_file(const std::string &cmd);

        static std::string popen_string(const std::string &cmd);

        static bool gpu_has_background_process(int device_id, process_type pt);

        static bool gpu_kill_background_process(int device_id, process_type pt);

        static bool gpu_start_process(const std::string &filename,
                                      int device_id, process_type pt, bool background);

        static void kill_all_processes(bool only_background);

        static void kill_process(int pid);

        static int start_process(const std::string &cmd, bool background);

    private:
        static int start_process(const std::string &cmd, bool background,
                                 bool is_kill, int pid_to_kill);
        static std::vector<std::pair<int, bool>> all_processes_;
        static std::mutex all_processes_mutex_;
        static std::map<std::pair<int, process_type>, int> gpu_background_processes_;
        static std::mutex gpu_background_processes_mutex_;
    };

}
