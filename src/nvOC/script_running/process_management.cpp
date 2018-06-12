//
// Created by alex on 30.05.18.
//
#include "process_management.h"

#include <iostream>
#include <signal.h>

#ifdef _WIN32

#include <windows.h>

#else
#include <unistd.h>
#include <sys/wait.h>
#endif

#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../exceptions.h"

namespace frequency_scaling {

    static const int BUFFER_SIZE = 4096;
    std::vector<std::pair<int, bool>> process_management::all_processes_;
    std::mutex process_management::all_processes_mutex_;
    std::map<std::pair<int, process_type>, int> process_management::gpu_background_processes_;
    std::mutex process_management::gpu_background_processes_mutex_;

    static void sig_handler(int signo) {
        if (signo == SIGINT) {
            process_management::kill_all_processes(false);
            nvapiUnload(1);
            nvmlShutdown_(true);
            exit(1);
        }
    }


    bool process_management::register_process_cleanup_sighandler() {
        //setup sigint handler
        if (signal(SIGINT, &sig_handler) == SIG_ERR) {
            return false;
        }
        return true;
    }


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
            res += line;
        }
        fclose(fp);
        return res;
    }

    bool process_management::gpu_has_background_process(int device_id, process_type pt) {
        std::lock_guard<std::mutex> lock(process_management::gpu_background_processes_mutex_);
        auto it = process_management::gpu_background_processes_.find(std::make_pair(device_id, pt));
        return it != process_management::gpu_background_processes_.end();
    }

    bool process_management::gpu_kill_background_process(int device_id, process_type pt) {
        if (!process_management::gpu_has_background_process(device_id, pt))
            return false;
        int pid;
        {
            std::lock_guard<std::mutex> lock(process_management::gpu_background_processes_mutex_);
            pid = process_management::gpu_background_processes_.at(std::make_pair(device_id, pt));
        }
        process_management::kill_process(pid);
        return true;
    }

    bool process_management::gpu_start_process(const std::string &filename,
                                               int device_id, process_type pt, bool background) {
        if (process_management::gpu_has_background_process(device_id, pt))
            return false;
        int pid = process_management::start_process(filename, background);
        if (background) {
            std::lock_guard<std::mutex> lock(process_management::gpu_background_processes_mutex_);
            process_management::gpu_background_processes_.emplace(std::make_pair(device_id, pt), pid);
        }
        return true;
    }

    void process_management::kill_all_processes(bool only_background) {
        std::vector<int> pids_to_kill;
        {
            std::lock_guard<std::mutex> lock(process_management::all_processes_mutex_);
            for (auto &process : process_management::all_processes_) {
                if (only_background) {
                    if (process.second)
                        pids_to_kill.push_back(process.first);
                } else {
                    pids_to_kill.push_back(process.first);
                }
            }
        }
        //
        for (int pid : pids_to_kill)
            process_management::kill_process(pid);
    }

    void process_management::kill_process(int pid) {
        process_management::start_process("kill " + std::to_string(pid), false, true, pid);
        //
        {
            std::lock_guard<std::mutex> lock(process_management::all_processes_mutex_);
            for (auto it = process_management::all_processes_.begin();
                 it != process_management::all_processes_.end();) {
                if (it->first == pid) {
                    it = process_management::all_processes_.erase(it);
                    break;
                } else
                    ++it;
            }
        }
        {
            std::lock_guard<std::mutex> lock(process_management::gpu_background_processes_mutex_);
            for (auto it = process_management::gpu_background_processes_.begin();
                 it != process_management::gpu_background_processes_.end();) {
                if (it->second == pid) {
                    it = process_management::gpu_background_processes_.erase(it);
                    break;
                } else
                    ++it;
            }
        }
    }

    int process_management::start_process(const std::string &cmd, bool background) {
        return process_management::start_process(cmd, background, false, -1);
    }

    int process_management::start_process(const std::string &cmd, bool background,
                                          bool is_kill, int pid_to_kill) {
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
            if (!is_kill) {
                std::lock_guard<std::mutex> lock(process_management::all_processes_mutex_);
                process_management::all_processes_.emplace_back(dwPid, background);
            }
            std::cout << "Started process: " << cmd << " (PID: " << dwPid << ")" << std::endl;
            if (!background) {
                WaitForSingleObject(pi.hProcess, INFINITE);
                DWORD exit_code;
                GetExitCodeProcess(pi.hProcess, &exit_code);
                if (exit_code != 0) {
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                    throw process_error("Process " + cmd + " returned invalid exit code: " + std::to_string(exit_code));
                }
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
            if(!is_kill){
                std::lock_guard<std::mutex> lock(process_management::all_processes_mutex_);
                process_management::all_processes_.emplace_back(pid, background);
            }
            std::cout << "Started process: " << cmd << " (PID: " << pid << ")" << std::endl;
            if (!background) {
                int status;
                waitpid(pid, &status, 0);
                if(!WIFEXITED(status))
                    throw process_error("Process " + cmd + " terminated unnormally");
                int exit_code = WEXITSTATUS(status);
                if(exit_code != 0)
                    throw process_error("Process " + cmd + " returned invalid exit code: " + std::to_string(exit_code));
            }
        }
        return pid;
#endif
    }

}