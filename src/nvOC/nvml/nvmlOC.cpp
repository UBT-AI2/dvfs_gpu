#include "nvmlOC.h"

#include <nvml.h>
#include <set>
#include "../common_header/constants.h"
#include "../common_header/exceptions.h"

namespace frequency_scaling {

    static std::set<int> registered_gpus;

    static void safeNVMLCall(nvmlReturn_t result) {
        if (result != NVML_SUCCESS) {
            std::string msg("NVML call failed: ");
            msg.append(nvmlErrorString(result));
            msg.append("\n");
            THROW_NVML_ERROR(msg);
        }
    }

    void nvmlInit_() {
        printf("NVML initialization...\n");
        safeNVMLCall(nvmlInit());
    }

    bool nvml_register_gpu(int device_id) {
        auto res = registered_gpus.emplace(device_id);
        if (!res.second)
            return false;

        nvmlDevice_t device;
        char name[BUFFER_SIZE];
        nvmlPciInfo_t pci;
        // Query for device handle to perform operations on a device
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        safeNVMLCall(nvmlDeviceGetName(device, name, BUFFER_SIZE));
        // pci.busId is very useful to know which device physically you're talking to
        // Using PCI identifier you can also match nvmlDevice handle to CUDA device.
        safeNVMLCall(nvmlDeviceGetPciInfo(device, &pci));
        printf("NVML GPU %d: %s [%s]\n", device_id, name, pci.busId);

        //enable persistence mode
        //safeNVMLCall(nvmlDeviceSetPersistenceMode(device, 1));
        //disable auto boosting of clocks (for hardware < pascal)
        //safeNVMLCall(nvmlDeviceSetDefaultAutoBoostedClocksEnabled(device, 0, 0));
        //safeNVMLCall(nvmlDeviceSetAutoBoostedClocksEnabled(device, 0));

        safeNVMLCall(nvmlDeviceResetApplicationsClocks(device));
        return true;
    }

    void nvmlShutdown_(bool restoreClocks) {
        //shutdown should never throw
        try {
            printf("NVML shutdown...\n");
            if (restoreClocks) {
                for (int device_id : registered_gpus) {
                    nvmlDevice_t device;
                    // Query for device handle to perform operations on a device
                    safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
                    safeNVMLCall(nvmlDeviceResetApplicationsClocks(device));
                    printf("NVML restored clocks for device %i\n", device_id);
                    //safeNVMLCall(nvmlDeviceSetDefaultAutoBoostedClocksEnabled(device, 1, 0));
                    //safeNVMLCall(nvmlDeviceSetAutoBoostedClocksEnabled(device, 1));
                }
            }
        } catch (const nvml_error &ex) {}
        try {
            safeNVMLCall(nvmlShutdown());
        } catch (const nvml_error &ex) {}
    }

    bool nvmlCheckOCSupport(int device_id) {
        nvmlDevice_t device;
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        nvmlReturn_t res = nvmlDeviceResetApplicationsClocks(device);
        return res == NVML_SUCCESS;
    }

    std::vector<int> nvmlGetAvailableMemClocks(int device_id) {
        nvmlDevice_t device;
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        unsigned int num_mem_clocks = BUFFER_SIZE;
        unsigned int avail_mem_clocks[BUFFER_SIZE];
        safeNVMLCall(nvmlDeviceGetSupportedMemoryClocks(device, &num_mem_clocks, avail_mem_clocks));
        std::vector<int> res;
        res.reserve(num_mem_clocks);
        for (int i = 0; i < num_mem_clocks; i++)
            res.push_back(avail_mem_clocks[i]);
        return res;
    }

    std::vector<int> nvmlGetAvailableGraphClocks(int device_id, int mem_clock) {
        nvmlDevice_t device;
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        unsigned int num_graph_clocks = BUFFER_SIZE;
        unsigned int avail_graph_clocks[BUFFER_SIZE];
        safeNVMLCall(nvmlDeviceGetSupportedGraphicsClocks(device, mem_clock,
                                                          &num_graph_clocks, avail_graph_clocks));
        std::vector<int> res;
        res.reserve(num_graph_clocks);
        for (int i = 0; i < num_graph_clocks; i++)
            res.push_back(avail_graph_clocks[i]);
        return res;
    }

    int nvmlGetNumDevices() {
        unsigned int deviceCount;
        safeNVMLCall(nvmlDeviceGetCount(&deviceCount));
        return deviceCount;
    }

    int nvmlGetBusId(int device_id) {
        nvmlDevice_t device;
        nvmlPciInfo_t pci;
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        safeNVMLCall(nvmlDeviceGetPciInfo(device, &pci));
        return pci.bus;
    }

    std::string nvmlGetBusIdString(int device_id) {
        nvmlDevice_t device;
        nvmlPciInfo_t pci;
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        safeNVMLCall(nvmlDeviceGetPciInfo(device, &pci));
        return pci.busId;
    }

    std::string nvmlGetDeviceName(int device_id) {
        nvmlDevice_t device;
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        char name[BUFFER_SIZE];
        safeNVMLCall(nvmlDeviceGetName(device, name, BUFFER_SIZE));
        return name;
    }


    int nvmlGetTemperature(int device_id) {
        nvmlDevice_t device;
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        unsigned int res;
        safeNVMLCall(nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &res));
        return res;
    }

    double nvmlGetPowerUsage(int device_id) {
        nvmlDevice_t device;
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        unsigned int res;
        safeNVMLCall(nvmlDeviceGetPowerUsage(device, &res));
        return res/1000.0;
    }

    void nvmlOC(int device_id, int graphClock, int memClock) {
        nvmlDevice_t device;
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        //change graph and mem clocks
        safeNVMLCall(nvmlDeviceSetApplicationsClocks(device, memClock, graphClock));
    }

}