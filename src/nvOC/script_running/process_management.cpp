//
// Created by alex on 30.05.18.
//
#include "process_management.h"
#include <signal.h>
#include <condition_variable>
#include <atomic>
#include <glog/logging.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <psapi.h>

#else

#include <unistd.h>
#include <sys/wait.h>

#endif

#include "../nvapi/nvapiOC.h"
#include "../nvml/nvmlOC.h"
#include "../common_header/exceptions.h"
#include "../common_header/constants.h"


namespace frequency_scaling {

	std::condition_variable glob_cond_var;
	std::atomic_bool glob_terminate = ATOMIC_VAR_INIT(false);

	namespace process_management {

		static std::vector<std::pair<int, bool>> all_processes_;
		static std::mutex all_processes_mutex_;
		static std::map<std::pair<int, process_type>, int> gpu_background_processes_;
		static std::mutex gpu_background_processes_mutex_;

		static int start_process(const std::string &cmd, bool background,
			bool is_kill);

		static void remove_pid(int pid);

		static void sig_handler(int signo) {
			LOG(ERROR) << "Catched signal " << signo << std::endl;
			if (signo != SIGTERM) {
				LOG(ERROR) << "Perform cleanup and exit..." << std::endl;
				kill_all_processes(false);
				nvapiUnload(1);
				nvmlShutdown_(true);
				exit(1);
			}
			else {
				LOG(ERROR) << "Wait for program to terminate normally..." << std::endl;
				glob_terminate = true;
				glob_cond_var.notify_all();
			}
		}

#ifdef _WIN32
		static void CALLBACK OnProcessExited(void* context, BOOLEAN isTimeOut)
		{
			HANDLE hProcess = context;
			int pid = GetProcessId(hProcess);
			remove_pid(pid);
			DWORD exit_code;
			GetExitCodeProcess(hProcess, &exit_code);
			VLOG(0) << "Process " << pid << " terminated with exit code " << exit_code << std::endl;
			CloseHandle(hProcess);
		}
#else
		static void sig_child_handler(int sig) {
			pid_t pid;
			int status;
			while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
				remove_pid(pid);
				if (!WIFEXITED(status)) {
					VLOG(0) << "Process " << pid << " terminated unnormally" << std::endl;
				}
				else {
					int exit_code = WEXITSTATUS(status);
					VLOG(0) << "Process " << pid << " terminated with exit code " << exit_code << std::endl;
				}
			}
		}
#endif

		bool register_process_cleanup_sighandler() {
			//setup signal handler
			if ((signal(SIGINT, &sig_handler) == SIG_ERR) |
				(signal(SIGABRT, &sig_handler) == SIG_ERR) |
				(signal(SIGTERM, &sig_handler) == SIG_ERR) |
				(signal(SIGSEGV, &sig_handler) == SIG_ERR)) {
				LOG(ERROR) << "Failed to register signal handler" << std::endl;
				return false;
			}
#ifndef _WIN32
			if (signal(SIGCHLD, &sig_child_handler) == SIG_ERR)
				LOG(ERROR) << "Failed to register sig child handler" << std::endl;
#endif
			VLOG(0) << "Registered cleanup signal handlers." << std::endl;
			return true;
		}


		FILE *popen_file(const std::string &cmd) {
			FILE *fp;
#ifdef _MSC_VER
			fp = _popen(cmd.c_str(), "r");
#else
			fp = popen(cmd.c_str(), "r");
#endif
			return fp;
		}


		std::string popen_string(const std::string &cmd) {
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

		bool gpu_has_background_process(int device_id, process_type pt) {
			std::lock_guard<std::mutex> lock(gpu_background_processes_mutex_);
			auto it = gpu_background_processes_.find(std::make_pair(device_id, pt));
			if (it == gpu_background_processes_.end())
				return false;
			return has_process(it->second);
		}

		bool gpu_kill_background_process(int device_id, process_type pt) {
			if (!gpu_has_background_process(device_id, pt))
				return false;
			int pid;
			{
				std::lock_guard<std::mutex> lock(gpu_background_processes_mutex_);
				pid = gpu_background_processes_.at(std::make_pair(device_id, pt));
			}
			kill_process(pid);
			return true;
		}

		bool gpu_start_process(const std::string &filename,
			int device_id, process_type pt, bool background) {
			if (gpu_has_background_process(device_id, pt))
				return false;
			int pid = start_process(filename, background);
			if (background) {
				std::lock_guard<std::mutex> lock(gpu_background_processes_mutex_);
				gpu_background_processes_.emplace(std::make_pair(device_id, pt), pid);
			}
			return true;
		}


		bool has_process(int pid) {
#ifdef _WIN32
			
			if (HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, pid)){
				// do a wait, if the handle is signaled: not running
				DWORD wait = WaitForSingleObject(h, 0);
				if (wait == WAIT_OBJECT_0) {
					CloseHandle(h);
					return false;
				}
				CloseHandle(h);
				return true;
			}
			return false;

			/*DWORD aProcesses[BUFFER_SIZE], cbNeeded;
			if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
				return false;
			}

			DWORD numProcesses = cbNeeded / sizeof(DWORD);
			for (int i = 0; i < numProcesses; i++) {
				if (aProcesses[i] == pid)
					return true;
			}
			return false;*/
#else
			return kill(pid, 0) == 0;
#endif
		}


		void kill_all_processes(bool only_background) {
			std::vector<int> pids_to_kill;
			{
				std::lock_guard<std::mutex> lock(all_processes_mutex_);
				for (auto &process : all_processes_) {
					if (only_background) {
						if (process.second)
							pids_to_kill.push_back(process.first);
					}
					else {
						pids_to_kill.push_back(process.first);
					}
				}
			}
			//
			for (int pid : pids_to_kill) {
				if (has_process(pid))
					kill_process(pid);
			}
		}


		void kill_process(int pid) {
			try {
#ifdef _WIN32
				//use taskkill to kill process and its childprocesses
				start_process("taskkill //f //t //pid " + std::to_string(pid), false, true);
#else
				//use pkill to kill process and its childprocesses
				start_process("pkill -P " + std::to_string(pid), false, true);
#endif
			}
			catch (const process_error &ex) {
			}
			//
			//remove_pid(pid);
			VLOG(0) << "Killed process with pid " + std::to_string(pid) << std::endl;
		}

		int start_process(const std::string &cmd, bool background) {
			return start_process(cmd, background, false);
		}

		static int start_process(const std::string &cmd, bool background,
			bool is_kill) {
#ifdef _WIN32
			STARTUPINFO startup_info = { sizeof(startup_info) };
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
					std::lock_guard<std::mutex> lock(all_processes_mutex_);
					all_processes_.emplace_back(dwPid, background);
				}
				VLOG(0) << "Started process: " << cmd << " (PID: " << dwPid << ")"
					<< std::endl;
				HANDLE hWait;
				RegisterWaitForSingleObject(&hWait, pi.hProcess, &OnProcessExited, pi.hProcess, INFINITE, WT_EXECUTEONLYONCE);
				if (!background) {
					WaitForSingleObject(pi.hProcess, INFINITE);
					DWORD exit_code;
					GetExitCodeProcess(pi.hProcess, &exit_code);
					if (exit_code != 0) {
						THROW_PROCESS_ERROR(
							"Process " + cmd + " returned invalid exit code: " + std::to_string(exit_code));
					}
				}
				CloseHandle(pi.hThread);
				return dwPid;
			}
			else {
				THROW_PROCESS_ERROR("CreateProcess() failed: " + cmd);
			}
#else
			int pid = vfork();
			if (pid < 0) {
				THROW_PROCESS_ERROR("fork() failed");
			}
			else if (pid == 0) {
				//child
				execl("/bin/bash", "bash", "-c", cmd.c_str(), NULL);
				//should not get here
				_exit(127);
			}
			else {
				//parent
				if (!is_kill) {
					std::lock_guard<std::mutex> lock(all_processes_mutex_);
					all_processes_.emplace_back(pid, background);
				}
				VLOG(0) << "Started process: " << cmd << " (PID: " << pid << ")"
					<< std::endl;
				if (!background) {
					int status;
					waitpid(pid, &status, 0);
					if (!WIFEXITED(status))
						THROW_PROCESS_ERROR("Process " + cmd + " terminated unnormally");
					int exit_code = WEXITSTATUS(status);
					if (exit_code != 0)
						THROW_PROCESS_ERROR("Process " + cmd + " returned invalid exit code: " + std::to_string(exit_code));
				}
			}
			return pid;
#endif
		}


		static void remove_pid(int pid) {
			{
				std::lock_guard<std::mutex> lock(all_processes_mutex_);
				for (auto it = all_processes_.begin();
					it != all_processes_.end();) {
					if (it->first == pid) {
						it = all_processes_.erase(it);
						break;
					}
					else
						++it;
				}
			}
			{
				std::lock_guard<std::mutex> lock(gpu_background_processes_mutex_);
				for (auto it = gpu_background_processes_.begin();
					it != gpu_background_processes_.end();) {
					if (it->second == pid) {
						it = gpu_background_processes_.erase(it);
						break;
					}
					else
						++it;
				}
			}
		}

	}
}