#pragma once

namespace frequency_scaling {

    struct clock_info{
        int current_freq_, current_oc_;
        int min_oc_, max_oc_;
    };

    void nvapiInit();

    bool nvapi_register_gpu(int device_id);

    void nvapiUnload(int restoreClocks);

    bool nvapiCheckSupport();

    int nvapiGetDeviceIndexByBusId(int busId);

    clock_info nvapiGetMemClockInfo(int deviceIdNvapi);

    clock_info nvapiGetGraphClockInfo(int deviceIdNvapi);

    void nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz);

}
