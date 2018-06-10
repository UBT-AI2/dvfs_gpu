#pragma once

namespace frequency_scaling {

    void nvapiInit();

    void nvapiUnload(int restoreClocks);

    int nvapiGetDeviceIndexByBusId(int busId);

    int nvapiGetCurrentMemClock(int deviceId);

    int nvapiGetCurrentGraphClock(int deviceId);

    void nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz);

}
