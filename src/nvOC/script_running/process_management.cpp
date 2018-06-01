//
// Created by alex on 30.05.18.
//

#include "process_management.h"
#include <iostream>
#include <algorithm>

#ifdef _WIN32

#include <windows.h>

#endif

#include "../exceptions.h"

namespace frequency_scaling {

    static const int BUFFER_SIZE = 4096;


    FILE *process_management::popen_file(const std::string &cmd) {
        FILE *fp;
#ifdef _MSC_VER
        fp = _popen(cmd.c_str(), "r");
#else
        fp = popen(cmd.c_str(), "r");
#endif
        return fp;
    }


    std::string process_management::popen_string(const std::string &cmd) {
        FILE *fp = popen_file(cmd);
        if (fp == NULL)
            return "";

        std::string res;
        char line[BUFFER_SIZE];
        /* Read the output a line at a time - output it. */
        while (fgets(line, BUFFER_SIZE, fp) != NULL) {
            std::string linestr(line);
            if (!std::all_of(linestr.begin(), linestr.end(), isspace))
                res += linestr;
        }
        fclose(fp);
        return res;
    }

    bool process_management::gpu_has_process(int device_id, process_type pt) {
        auto it = process_management::process_pids_.find(std::make_pair(device_id, pt));
        return it != process_management::process_pids_.end();
    }

    bool process_management::gpu_kill_process(int device_id, process_type pt) {
        if (!process_management::gpu_has_process(device_id, pt))
            return false;
        int pid = process_management::process_pids_.at(std::make_pair(device_id, pt));
        process_management::kill_process(pid);
        return true;
    }

    bool process_management::gpu_execute_shell_script(const std::string &filename,
                                                      int device_id, process_type pt) {
        if (!process_management::gpu_has_process(device_id, pt))
            return false;
        int pid = process_management::execute_shell_script(filename, false);
        process_management::process_pids_.emplace(std::make_pair(device_id, pt), pid);
        return true;
    }

    bool process_management::gpu_execute_shell_command(const std::string &command,
                                                       int device_id, process_type pt) {
        if (!process_management::gpu_has_process(device_id, pt))
            return false;
        int pid = process_management::execute_shell_command(command, false);
        process_management::process_pids_.emplace(std::make_pair(device_id, pt), pid);
        return true;
    }

    void process_management::kill_process(int pid) {
        process_management::execute_shell_command("kill " + std::to_string(pid), true);
    }

    int process_management::execute_shell_script(const std::string &filename, bool blocking) {
        return process_management::start_process("sh " + filename, blocking);
    }


    int process_management::execute_shell_command(const std::string &command, bool blocking) {
        return process_management::start_process("sh -c '" + command + "'", blocking);
    }


    int process_management::start_process(const std::string &cmd, bool blocking) {
#ifdef _WIN32
        STARTUPINFO startup_info = {sizeof(startup_info)};
        PROCESS_INFORMATION pi;
        std::string cmd_copy(cmd);

        if (CreateProcess(NULL,
                          &cmd_copy[0],
                          NULL,
                          NULL,
                          FALSE,
                          0,
                          NULL,
                          NULL,
                          &startup_info,
                          &pi)) {
            int dwPid = GetProcessId(pi.hProcess);
            std::cout << "Started process with PID: " << dwPid << std::endl;
            if (blocking) {
                WaitForSingleObject(pi.hProcess, INFINITE);
            }
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return dwPid;
        } else {
            throw process_error("CreateProcess() failed: " + cmd);
        }
#else

#endif
    }

}