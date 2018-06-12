//
// Created by Alex on 06.06.2018.
//
#include "currency_type.h"
#include <stdexcept>

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
                throw std::runtime_error("Invalid enum value");
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
            throw std::runtime_error("Currency " + str + " not available");
    }

}

