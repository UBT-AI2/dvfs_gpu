#include "nvmlOC.h"

#include <stdlib.h>
#include <stdio.h>
#include <nvml.h>
#include "../exceptions.h"

#define BUFFER_SIZE 1024

static void safeNVMLCall(nvmlReturn_t result) {
    if (result != NVML_SUCCESS) {
        std::string msg("NVML call failed: ");
        msg.append(nvmlErrorString(result));
        msg.append("\n");
        throw nvml_exception(msg);
    }
}

void nvmlInit_() {
    safeNVMLCall(nvmlInit());

    unsigned int deviceCount;
    safeNVMLCall(nvmlDeviceGetCount(&deviceCount));
    for (int device_id = 0; device_id < deviceCount; device_id++) {
        nvmlDevice_t device;
        char name[BUFFER_SIZE];
        nvmlPciInfo_t pci;
        // Query for device handle to perform operations on a device
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        safeNVMLCall(nvmlDeviceGetName(device, name, BUFFER_SIZE));
        // pci.busId is very useful to know which device physically you're talking to
        // Using PCI identifier you can also match nvmlDevice handle to CUDA device.
        safeNVMLCall(nvmlDeviceGetPciInfo(device, &pci));
        printf("%d. %s [%s]\n", device_id, name, pci.busId);

        //enable persistence mode
        //safeNVMLCall(nvmlDeviceSetPersistenceMode(device, 1));
        //disable auto boosting of clocks (for hardware < pascal)
        //safeNVMLCall(nvmlDeviceSetDefaultAutoBoostedClocksEnabled(device, 0, 0));
        //safeNVMLCall(nvmlDeviceSetAutoBoostedClocksEnabled(device, 0));
    }
}

void nvmlShutdown_() {
    unsigned int deviceCount;
    safeNVMLCall(nvmlDeviceGetCount(&deviceCount));
    for (int device_id = 0; device_id < deviceCount; device_id++) {
        nvmlDevice_t device;
        // Query for device handle to perform operations on a device
        safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
        safeNVMLCall(nvmlDeviceResetApplicationsClocks(device));
        //safeNVMLCall(nvmlDeviceSetDefaultAutoBoostedClocksEnabled(device, 1, 0));
        //safeNVMLCall(nvmlDeviceSetAutoBoostedClocksEnabled(device, 1));
        printf("Restored clocks for device %i\n", device_id);
    }
    safeNVMLCall(nvmlShutdown());
}

std::vector<int> getAvailableMemClocks(int device_id) {
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

std::vector<int> getAvailableGraphClocks(int device_id, int mem_clock) {
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

int getDefaultMemClock(int device_id) {
    unsigned int res;
    nvmlDevice_t device;
    safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
    safeNVMLCall(nvmlDeviceGetDefaultApplicationsClock(device, NVML_CLOCK_MEM, &res));
    return res;
}

int getDefaultGraphClock(int device_id) {
    unsigned int res;
    nvmlDevice_t device;
    safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
    safeNVMLCall(nvmlDeviceGetDefaultApplicationsClock(device, NVML_CLOCK_GRAPHICS, &res));
    return res;
}

void nvmlOC(int device_id, int graphClock, int memClock) {
    nvmlDevice_t device;
    safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
    //change graph and mem clocks
    safeNVMLCall(nvmlDeviceSetApplicationsClocks(device, memClock, graphClock));
    //printf("\t Changed device clocks: mem:%i,graph:%i\n", memClock, graphClock);
}
