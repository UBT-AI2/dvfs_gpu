#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int nvapiInit();

int nvapiUnload(int restoreClocks);

int nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz);

int nvapiGetDeviceIndexByBusId(int busId);

int nvapiGetCurrentMemClock(int deviceId);

int nvapiGetCurrentGraphClock(int deviceId);

#ifdef __cplusplus
}
#endif
