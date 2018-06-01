//
// Created by alex on 30.05.18.
//
#pragma once

#include <cstdio>
#include <string>
#include <map>

namespace frequency_scaling{

    enum class process_type{
        MINER, POWER_MONITOR, count
    };


    class process_management {
    public:
        static FILE *popen_file(const std::string &cmd);
        static std::string popen_string(const std::string &cmd);

        static bool gpu_has_process(int device_id, process_type pt);
        static bool gpu_kill_process(int device_id, process_type pt);
        static bool gpu_execute_shell_script(const std::string &filename,
                                             int device_id, process_type pt);
        static bool gpu_execute_shell_command(const std::string &command,
                                              int device_id, process_type pt);

        static void kill_process(int pid);
        static int execute_shell_script(const std::string& filename, bool blocking);
        static int execute_shell_command(const std::string& command, bool blocking);
    private:
        static int start_process(const std::string& cmd, bool blocking);
        static std::map<std::pair<int,process_type>, int> process_pids_;
    };

}
