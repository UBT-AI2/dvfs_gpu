//
// Created by alex on 30.05.18.
//
#pragma once

#include <cstdio>
#include <string>
#include <vector>

namespace frequency_scaling{

    enum class process_type{
        MINER, POWER_MONITOR, count
    };

    struct process_info{
        int pid_;
        int device_id_;
        process_type process_type_;
    };

    class process_management {
    public:
        static FILE *popen_file(const std::string &cmd);
        static std::string popen_string(const std::string &cmd);

    private:
        std::vector<process_info> process_infos_;
    };

}
