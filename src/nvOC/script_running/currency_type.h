#pragma once

#include <string>

namespace frequency_scaling {

    enum class currency_type {
        ETH, ZEC, XMR, count
    };

    std::string enum_to_string(currency_type ct);

    currency_type string_to_currency_type(const std::string &str);

    std::string gpu_log_prefix(currency_type ct, int device_id_nvml);
}