#pragma once

namespace frequency_scaling {

    struct nvapi_clock_info {
        int current_freq_, current_oc_;
        int min_oc_, max_oc_;
    };

    void nvapiInit();

    bool nvapi_register_gpu(int device_id_nvapi);

    void nvapiUnload(int restoreClocks);

    bool nvapiCheckSupport(int device_id_nvapi);

    int nvapiGetDeviceIndexByBusId(int busId);

    nvapi_clock_info nvapiGetMemClockInfo(int device_id_nvapi);

    nvapi_clock_info nvapiGetGraphClockInfo(int device_id_nvapi);

    void nvapiOC(int device_id_nvapi, int graphOCMHz, int memOCMHz);

}
