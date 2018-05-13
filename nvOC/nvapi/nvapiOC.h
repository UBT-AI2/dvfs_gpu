#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int nvapiInit();

int nvapiUnload();

int nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz);

int nvapiGetDeviceIndexByBusId(int busId);

#ifdef __cplusplus
}
#endif
