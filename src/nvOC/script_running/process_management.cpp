//
// Created by alex on 30.05.18.
//

#include "process_management.h"
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else

#include <unistd.h>
#include <sys/wait.h>

#endif

#include "../exceptions.h"

namespace frequency_scaling {

    static const int BUFFER_SIZE = 4096;
    std::map<std::pair<int, process_type>, int> process_management::background_processes_;
    std::mutex process_management::background_processes_mutex_;


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

    bool process_management::gpu_has_background_process(int device_id, process_type pt) {
        std::lock_guard<std::mutex> lock(process_management::background_processes_mutex_);
        auto it = process_management::background_processes_.find(std::make_pair(device_id, pt));
        return it != process_management::background_processes_.end();
    }

    bool process_management::gpu_kill_background_process(int device_id, process_type pt) {
        if (!process_management::gpu_has_background_process(device_id, pt))
            return false;
        int pid;
        {
            std::lock_guard<std::mutex> lock(process_management::background_processes_mutex_);
            pid = process_management::background_processes_.at(std::make_pair(device_id, pt));
        }
        process_management::kill_process(pid);
        std::lock_guard<std::mutex> lock(process_management::background_processes_mutex_);
        process_management::background_processes_.erase(std::make_pair(device_id, pt));
        return true;
    }

    bool process_management::gpu_start_process(const std::string &filename,
                                               int device_id, process_type pt, bool background) {
        if (process_management::gpu_has_background_process(device_id, pt))
            return false;
        int pid = process_management::start_process(filename, background);
        if (background) {
            std::lock_guard<std::mutex> lock(process_management::background_processes_mutex_);
            process_management::background_processes_.emplace(std::make_pair(device_id, pt), pid);
        }
        return true;
    }


    void process_management::kill_process(int pid) {
        process_management::start_process("kill " + std::to_string(pid), false);
    }


    int process_management::start_process(const std::string &cmd, bool background) {
#ifdef _WIN32
        STARTUPINFO startup_info = {sizeof(startup_info)};
        PROCESS_INFORMATION pi;
        std::string full_cmd = "bash -c '" + cmd + "'";

        if (CreateProcess(NULL,
                          &full_cmd[0],
                          NULL,
                          NULL,
                          FALSE,
                          0,
                          NULL,
                          NULL,
                          &startup_info,
                          &pi)) {
            int dwPid = GetProcessId(pi.hProcess);
            std::cout << "Started process: " << cmd << " (PID: " << dwPid << ")" << std::endl;
            if (!background) {
                WaitForSingleObject(pi.hProcess, INFINITE);
            }
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            return dwPid;
        } else {
            throw process_error("CreateProcess() failed: " + cmd);
        }
#else
        int pid = vfork();
        if (pid < 0) {
            throw process_error("fork() failed");
        } else if (pid == 0) {
            //child
            execl("/bin/bash", "bash", "-c", cmd.c_str(), NULL);
            //should not get here
            _exit(127);
        } else {
            //parent
            std::cout << "Started process: " << cmd << " (PID: " << pid << ")" << std::endl;
            if (!background) {
                waitpid(pid, NULL, 0);
            }
        }
        return pid;
#endif
    }

}