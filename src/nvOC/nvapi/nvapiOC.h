#pragma once

namespace frequency_scaling {

    void nvapiInit();

    void nvapiUnload(int restoreClocks);

    bool checkNvapiSupport(int device_id_nvml);

    int nvapiGetDeviceIndexByBusId(int busId);

    int nvapiGetCurrentMemClock(int deviceId);

    int nvapiGetCurrentGraphClock(int deviceId);

    void nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz);

}
