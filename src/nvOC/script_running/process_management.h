//
// Created by alex on 30.05.18.
//
#pragma once

#include <cstdio>
#include <string>
#include <map>
#include <mutex>

namespace frequency_scaling{

    enum class process_type{
        MINER, POWER_MONITOR, count
    };


    class process_management {
    public:
        static FILE *popen_file(const std::string &cmd);
        static std::string popen_string(const std::string &cmd);

        static bool gpu_has_background_process(int device_id, process_type pt);
        static bool gpu_kill_background_process(int device_id, process_type pt);
        static bool gpu_execute_shell_script(const std::string &filename,
                                             int device_id, process_type pt, bool background);
        static bool gpu_execute_shell_command(const std::string &command,
                                              int device_id, process_type pt, bool background);

        static void kill_process(int pid);
        static int execute_shell_script(const std::string& filename, bool background);
        static int execute_shell_command(const std::string& command, bool background);
    private:
        static int start_process(const std::string& cmd, bool background);
        static std::map<std::pair<int,process_type>, int> background_processes_;
        static std::mutex background_processes_mutex_;
    };

}
