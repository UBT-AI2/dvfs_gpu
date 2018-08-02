//
// Created by Alex on 16.07.2018.
//

#include "log_utils.h"
#include <glog/logging.h>

#ifdef _WIN32

#include <windows.h>

#else

#include <sys/types.h>
#include <sys/stat.h>

#endif

#include "../common_header/constants.h"

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
        log_utils::logdir_name_ = logdir_to_create + "_" + date_str;
#ifdef _WIN32
        CreateDirectory(&log_utils::logdir_name_[0], NULL);
#else
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

    std::string log_utils::get_power_log_filename(int device_id_nvml) {
        return "power_results_" + std::to_string(device_id_nvml) + ".txt";
    }

    std::string log_utils::get_hash_log_filename(currency_type ct, int device_id_nvml) {
        return "hash_log_" + enum_to_string(ct) + "_" + std::to_string(device_id_nvml) + ".txt";
    }

    std::string log_utils::get_offline_bench_filename(currency_type ct, int device_id_nvml) {
        return "offline_bench_result_gpu" + std::to_string(device_id_nvml) + "_" + enum_to_string(ct) + ".dat";
    }

    std::string log_utils::get_online_bench_filename(currency_type ct, int device_id_nvml) {
        return "online_bench_result_gpu" + std::to_string(device_id_nvml) + "_" + enum_to_string(ct) + ".dat";
    }

}