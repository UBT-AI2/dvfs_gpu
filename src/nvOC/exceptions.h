#pragma once

#include <stdexcept>
#include <string>

namespace frequency_scaling {

    class nvml_exception : public std::runtime_error {
    public:
        nvml_exception(const std::string &msg) : std::runtime_error(msg) {}
    };

    class nvapi_exception : public std::runtime_error {
    public:
        nvapi_exception(const std::string &msg) : std::runtime_error(msg) {}
    };

    class io_exception : public std::runtime_error {
    public:
        io_exception(const std::string &msg) : std::runtime_error(msg) {}
    };

}