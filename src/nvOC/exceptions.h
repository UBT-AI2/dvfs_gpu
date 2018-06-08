#pragma once

#include <stdexcept>
#include <string>

namespace frequency_scaling {

    class optimization_error : public std::runtime_error {
    public:
        optimization_error(const std::string &msg) : std::runtime_error(msg) {}
    };

    class nvml_error : public optimization_error {
    public:
        nvml_error(const std::string &msg) : optimization_error(msg) {}
    };

    class nvapi_error : public optimization_error {
    public:
        nvapi_error(const std::string &msg) : optimization_error(msg) {}
    };

    class network_error : public std::runtime_error {
    public:
        network_error(const std::string &msg) : std::runtime_error(msg) {}
    };

    class process_error : public std::runtime_error {
    public:
        process_error(const std::string &msg) : std::runtime_error(msg) {}
    };
}