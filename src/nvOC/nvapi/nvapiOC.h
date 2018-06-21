#pragma once

namespace frequency_scaling {

    void nvapiInit();

    bool nvapi_register_gpu(int device_id);

    void nvapiUnload(int restoreClocks);

    bool nvapiCheckSupport();

    int nvapiGetDeviceIndexByBusId(int busId);

    int nvapiGetCurrentMemClock(int deviceId);

    int nvapiGetCurrentGraphClock(int deviceId);

    void nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz);

}
