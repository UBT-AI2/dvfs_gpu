//
// Created by Alex on 16.07.2018.
//
#pragma once

#include <string>
#include <atomic>
#include "currency_type.h"

namespace frequency_scaling {
    namespace log_utils {

        bool init_logging(const std::string &logdir_to_create, const std::string &glog_file_prefix,
                          int glog_verbosity, const char *argv0);

        std::string get_logdir_name();

        bool check_file_existance(const std::string &filename, bool logdir_prefix = false);

        std::string get_power_log_filename(int device_id_nvml, bool logdir_prefix = true);

        std::string get_hash_log_filename(const currency_type &ct, int device_id_nvml, bool logdir_prefix = true);

        std::string get_offline_bench_filename(const currency_type &ct, int device_id_nvml, bool logdir_prefix = true);

        std::string get_online_bench_filename(const currency_type &ct, int device_id_nvml, bool logdir_prefix = true);

        std::string get_profit_stats_filename(const currency_type &ct, int device_id_nvml, bool logdir_prefix = true);

        std::string get_profit_stats_filename(int device_id_nvml, bool logdir_prefix = true);

        std::string get_profit_stats_filename(bool logdir_prefix = true);

        std::string get_autosave_user_config_filename(bool logdir_prefix = true);

        std::string get_autosave_currency_config_filename(bool logdir_prefix = true);

        std::string get_autosave_opt_result_optphase_filename(bool logdir_prefix = true);

        std::string get_autosave_opt_result_monitorphase_filename(bool logdir_prefix = true);

        std::string gpu_log_prefix(int device_id_nvml);

        std::string gpu_log_prefix(const frequency_scaling::currency_type &ct, int device_id_nvml);
    }
}