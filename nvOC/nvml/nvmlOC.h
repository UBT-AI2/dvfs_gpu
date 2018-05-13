#pragma once

#include <vector>

namespace frequency_scaling {

    void nvmlInit_();

    void nvmlShutdown_();

    std::vector<int> nvmlGetAvailableMemClocks(int device_id);

    std::vector<int> nvmlGetAvailableGraphClocks(int device_id, int mem_clock);

    int nvmlGetBusId(int device_id);

    int nvmlGetNumDevices();

    void nvmlOC(int device_id, int graphClock, int memClock);

}
