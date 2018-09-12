//
// Created by Alex on 16.07.2018.
//

#include "log_utils.h"
#include <glog/logging.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <io.h>

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#endif

#include "../common_header/constants.h"
#include "../nvml/nvmlOC.h"

namespace frequency_scaling {

    std::string log_utils::logdir_name_ = ".";
    std::atomic_bool log_utils::logging_initialized_ = ATOMIC_VAR_INIT(false);

    bool log_utils::init_logging(const std::string &logdir_to_create, const std::string &glog_file_prefix,
                                 int glog_verbosity, const char *argv0) {
        if (log_utils::logging_initialized_)
            return false;
        //create logging directory
        char date_str[BUFFER_SIZE];
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(date_str, BUFFER_SIZE, "%Y-%m-%d_%H-%M-%S", timeinfo);
#ifdef _WIN32
        log_utils::logdir_name_ = logdir_to_create + "_windows_" + date_str;
        CreateDirectory(&log_utils::logdir_name_[0], NULL);
#else
        log_utils::logdir_name_ = logdir_to_create + "_linux_" + date_str;
        mkdir(log_utils::logdir_name_.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
        //init google logging
        FLAGS_v = glog_verbosity;
        google::SetLogDestination(google::GLOG_INFO,
                                  (log_utils::logdir_name_ + "/" + glog_file_prefix).c_str());
        google::SetStderrLogging(0);
        google::InitGoogleLogging(argv0);
        log_utils::logging_initialized_ = true;
        return true;
    }

    std::string log_utils::get_logdir_name() {
        return log_utils::logdir_name_;
    }

    bool log_utils::check_file_existance(const std::string &filename, bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
#ifdef _WIN32
        return _access((prefix_str + filename).c_str(), 0) != -1;
#else
        return access( (prefix_str + filename).c_str(), 0 ) != -1;
#endif
    }

    std::string log_utils::get_power_log_filename(int device_id_nvml, bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
        return prefix_str + "power_results_" + std::to_string(device_id_nvml) + ".txt";
    }

    std::string log_utils::get_hash_log_filename(const currency_type &ct, int device_id_nvml, bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
        return prefix_str + "hash_log_" + ct.currency_name_ + "_" + std::to_string(device_id_nvml) + ".txt";
    }

    std::string
    log_utils::get_offline_bench_filename(const currency_type &ct, int device_id_nvml, bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
        return prefix_str + "offline_bench_result_gpu" + std::to_string(device_id_nvml) + "_" + ct.currency_name_ +
               ".dat";
    }

    std::string
    log_utils::get_online_bench_filename(const currency_type &ct, int device_id_nvml, bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
        return prefix_str + "online_bench_result_gpu" + std::to_string(device_id_nvml) + "_" + ct.currency_name_ +
               ".dat";
    }

    std::string
    log_utils::get_profit_stats_filename(const currency_type &ct, int device_id_nvml, bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
        return prefix_str + "profit_stats_local_gpu" + std::to_string(device_id_nvml) + "_" + ct.currency_name_ +
               ".dat";
    }

    std::string log_utils::get_profit_stats_filename(int device_id_nvml, bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
        return prefix_str + "best_profit_stats_local_gpu" + std::to_string(device_id_nvml) + ".dat";
    }

    std::string log_utils::get_profit_stats_filename(bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
        return prefix_str + "best_profit_stats_global.dat";
    }

    std::string log_utils::get_autosave_user_config_filename(bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
        return prefix_str + "autosave_user_config.json";
    }

    std::string log_utils::get_autosave_currency_config_filename(bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
        return prefix_str + "autosave_currency_config.json";
    }

    std::string log_utils::get_autosave_opt_result_optphase_filename(bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
        return prefix_str + "autosave_opt_result_optimizationphase.json";
    }

    std::string log_utils::get_autosave_opt_result_monitorphase_filename(bool logdir_prefix) {
        const std::string &prefix_str = (logdir_prefix) ? log_utils::get_logdir_name() + "/" : "";
        return prefix_str + "autosave_opt_result_monitoringphase.json";
    }

    std::string log_utils::log_utils::gpu_log_prefix(int device_id_nvml) {
        return "GPU " + std::to_string(device_id_nvml) + " [" +
               nvmlGetDeviceName(device_id_nvml) + "]: ";
    }

    std::string log_utils::log_utils::gpu_log_prefix(const frequency_scaling::currency_type &ct, int device_id_nvml) {
        return "GPU " + std::to_string(device_id_nvml) + " [" +
               nvmlGetDeviceName(device_id_nvml) + "]: " + ct.currency_name_ + ": ";
    }

}