//
// Created by Alex on 06.06.2018.
//
#include "currency_type.h"
#include "../common_header/exceptions.h"
#include "../nvml/nvmlOC.h"

namespace frequency_scaling {

    std::string enum_to_string(currency_type ct) {
        switch (ct) {
            case currency_type::ETH:
                return "ETH";
            case currency_type::ZEC:
                return "ZEC";
            case currency_type::XMR:
                return "XMR";
            default:
                THROW_RUNTIME_ERROR("Invalid enum value");
        }
    }

    currency_type string_to_currency_type(const std::string &str) {
        if (str == "eth" || str == "ETH")
            return currency_type::ETH;
        else if (str == "zec" || str == "ZEC")
            return currency_type::ZEC;
        else if (str == "xmr" || str == "XMR")
            return currency_type::XMR;
        else
            THROW_RUNTIME_ERROR("Currency " + str + " not available");
    }

    std::string gpu_log_prefix(int device_id_nvml) {
        return "GPU " + std::to_string(device_id_nvml) + " [" +
               nvmlGetDeviceName(device_id_nvml) + "]: ";
    }

    std::string gpu_log_prefix(currency_type ct, int device_id_nvml) {
        return "GPU " + std::to_string(device_id_nvml) + " [" +
               nvmlGetDeviceName(device_id_nvml) + "]: " + enum_to_string(ct) + ": ";
    }

}

