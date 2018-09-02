#pragma once

#include <stdexcept>
#include <string>

namespace frequency_scaling {

    class custom_error : public std::runtime_error {
    public:
        custom_error(const std::string &msg) : std::runtime_error(msg) {}
    };

    class optimization_error : public custom_error {
    public:
        optimization_error(const std::string &msg) : custom_error(msg) {}
    };

    class nvml_error : public custom_error {
    public:
        nvml_error(const std::string &msg) : custom_error(msg) {}
    };

    class nvapi_error : public custom_error {
    public:
        nvapi_error(const std::string &msg) : custom_error(msg) {}
    };

    class network_error : public custom_error {
    public:
        network_error(const std::string &msg) : custom_error(msg) {}
    };

    class process_error : public custom_error {
    public:
        process_error(const std::string &msg) : custom_error(msg) {}
    };

    class io_error : public custom_error {
    public:
        io_error(const std::string &msg) : custom_error(msg) {}
    };

#define __FILE_INFO_STR__ (std::string("[") + __FILE__ + ", " + __FUNCTION__ + ", " + std::to_string(__LINE__) + "] ")
#define THROW_RUNTIME_ERROR(msg) throw std::runtime_error("RUNTIME_ERROR " + __FILE_INFO_STR__ + (msg))
#define THROW_OPTIMIZATION_ERROR(msg) throw optimization_error("OPTIMIZATION_ERROR " + __FILE_INFO_STR__ + (msg))
#define THROW_NVML_ERROR(msg) throw nvml_error("NVML_ERROR " + __FILE_INFO_STR__ + (msg))
#define THROW_NVAPI_ERROR(msg) throw nvapi_error("NVAPI_ERROR " + __FILE_INFO_STR__ + (msg))
#define THROW_NETWORK_ERROR(msg) throw network_error("NETWORK_ERROR " + __FILE_INFO_STR__ + (msg))
#define THROW_PROCESS_ERROR(msg) throw process_error("PROCESS_ERROR " + __FILE_INFO_STR__ + (msg))
#define THROW_IO_ERROR(msg) throw process_error("IO_ERROR " + __FILE_INFO_STR__ + (msg))

}