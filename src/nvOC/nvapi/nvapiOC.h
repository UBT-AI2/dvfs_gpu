#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int nvapiInit();

int nvapiUnload(int restoreClocks);

int nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz);

int nvapiGetDeviceIndexByBusId(int busId);

int nvapiGetDefaultMemClock(int deviceId);

int nvapiGetDefaultGraphClock(int deviceId);

#ifdef __cplusplus
}
#endif
