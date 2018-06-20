#pragma once

#include <vector>
#include <string>

namespace frequency_scaling {

    void nvmlInit_();

    void nvmlShutdown_(bool restoreClocks);

    bool nvmlCheckOCSupport(int device_id);

    std::vector<int> nvmlGetAvailableMemClocks(int device_id);

    std::vector<int> nvmlGetAvailableGraphClocks(int device_id, int mem_clock);

    int nvmlGetBusId(int device_id);

    std::string nvmlGetBusIdString(int device_id);

    std::string nvmlGetDeviceName(int device_id);

    int nvmlGetNumDevices();

    int nvmlGetTemperature(int device_id);

    int nvmlGetPowerUsage(int device_id);

    void nvmlOC(int device_id, int graphClock, int memClock);

}
