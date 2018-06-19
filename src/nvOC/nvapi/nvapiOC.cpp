/*
 DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 Version 2, December 2004

 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

 DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
 TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

 0. You just DO WHAT THE FUCK YOU WANT TO.
 */

//https://1vwjbxf1wko0yhnr.wordpress.com/author/2pkaqwtuqm2q7djg/
#include "nvapiOC.h"


#include <stdio.h>
#include <stdlib.h>
#include <string>

#ifdef _WIN32

#include <windows.h>

#endif

#include "../exceptions.h"

namespace frequency_scaling {

#ifdef _WIN32

    typedef unsigned long NvU32;

    typedef struct {
        NvU32 version;
        NvU32 ClockType :2;
        NvU32 reserved :22;
        NvU32 reserved1 :8;
        struct {
            NvU32 bIsPresent :1;
            NvU32 reserved :31;
            NvU32 frequency;
        } domain[32];
    } NV_GPU_CLOCK_FREQUENCIES_V2;

    typedef struct {
        int value;
        struct {
            int mindelta;
            int maxdelta;
        } valueRange;
    } NV_GPU_PERF_PSTATES20_PARAM_DELTA;

    typedef struct {
        NvU32 domainId;
        NvU32 typeId;
        NvU32 bIsEditable :1;
        NvU32 reserved :31;
        NV_GPU_PERF_PSTATES20_PARAM_DELTA freqDelta_kHz;
        union {
            struct {
                NvU32 freq_kHz;
            } single;
            struct {
                NvU32 minFreq_kHz;
                NvU32 maxFreq_kHz;
                NvU32 domainId;
                NvU32 minVoltage_uV;
                NvU32 maxVoltage_uV;
            } range;
        } data;
    } NV_GPU_PSTATE20_CLOCK_ENTRY_V1;

    typedef struct {
        NvU32 domainId;
        NvU32 bIsEditable :1;
        NvU32 reserved :31;
        NvU32 volt_uV;
        int voltDelta_uV;
    } NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1;

    typedef struct {
        NvU32 version;
        NvU32 bIsEditable :1;
        NvU32 reserved :31;
        NvU32 numPstates;
        NvU32 numClocks;
        NvU32 numBaseVoltages;
        struct {
            NvU32 pstateId;
            NvU32 bIsEditable :1;
            NvU32 reserved :31;
            NV_GPU_PSTATE20_CLOCK_ENTRY_V1 clocks[8];
            NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1 baseVoltages[4];
        } pstates[16];
    } NV_GPU_PERF_PSTATES20_INFO_V1;

    typedef void *(*NvAPI_QueryInterface_t)(unsigned int offset);

    typedef int (*NvAPI_Initialize_t)();

    typedef int (*NvAPI_Unload_t)();

    typedef int (*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);

    typedef int (*NvAPI_GetBusId_t)(int *handle, int *busId);

    typedef int (*NvAPI_GPU_GetSystemType_t)(int *handle, int *systype);

    typedef int (*NvAPI_GPU_GetFullName_t)(int *handle, char *sysname);

    typedef int (*NvAPI_GPU_GetPhysicalFrameBufferSize_t)(int *handle, int *memsize);

    typedef int (*NvAPI_GPU_GetRamType_t)(int *handle, int *memtype);

    typedef int (*NvAPI_GPU_GetVbiosVersionString_t)(int *handle, char *biosname);

    typedef int (*NvAPI_GPU_GetAllClockFrequencies_t)(int *handle,
                                                      NV_GPU_PERF_PSTATES20_INFO_V1 *pstates_info);

    typedef int (*NvAPI_GPU_GetPstates20_t)(int *handle,
                                            NV_GPU_PERF_PSTATES20_INFO_V1 *pstates_info);

    typedef int (*NvAPI_GPU_SetPstates20_t)(int *handle, int *pstates_info);

    typedef int (*NvAPI_GetErrorMessage_t)(int error_code, char *error_string);


    static NvAPI_QueryInterface_t NvQueryInterface = 0;
    static NvAPI_Initialize_t NvInit = 0;
    static NvAPI_Unload_t NvUnload = 0;
    static NvAPI_EnumPhysicalGPUs_t NvEnumPhysicalGPUs = 0;
    static NvAPI_GetBusId_t NvAPI_GetBusId = 0;
    static NvAPI_GPU_GetSystemType_t NvGetSysType = 0;
    static NvAPI_GPU_GetFullName_t NvGetName = 0;
    static NvAPI_GPU_GetPhysicalFrameBufferSize_t NvGetMemSize = 0;
    static NvAPI_GPU_GetRamType_t NvGetMemType = 0;
    static NvAPI_GPU_GetVbiosVersionString_t NvGetBiosName = 0;
    static NvAPI_GPU_GetAllClockFrequencies_t NvGetFreq = 0;
    static NvAPI_GPU_GetPstates20_t NvGetPstates = 0;
    static NvAPI_GPU_SetPstates20_t NvSetPstates = 0;
    static NvAPI_GetErrorMessage_t NvAPI_GetErrorMessage = 0;

    static const int BUFFER_SIZE = 1024;

    static void safeNVAPICall(int result) {
        if (result != 0) {
            std::string msg("NVAPI call failed: ");
            char msg_buf[BUFFER_SIZE];
            NvAPI_GetErrorMessage(result, msg_buf);
            msg.append(msg_buf);
            msg.append("\n");
            throw nvapi_error(msg);
        }
    }


    void nvapiInit() {
        NvQueryInterface = (NvAPI_QueryInterface_t) GetProcAddress(LoadLibrary("nvapi64.dll"),
                                                                   "nvapi_QueryInterface");
        if (NvQueryInterface == NULL) {
            throw nvapi_error("Failed to load nvapi.dll");
        }
        NvInit = (NvAPI_Initialize_t) NvQueryInterface(0x0150E828);
        NvUnload = (NvAPI_Unload_t) NvQueryInterface(0xD22BDD7E);
        NvEnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t) NvQueryInterface(0xE5AC921F);
        NvAPI_GetBusId = (NvAPI_GetBusId_t) NvQueryInterface(0x1BE0B8E5);

        NvGetSysType = (NvAPI_GPU_GetSystemType_t) NvQueryInterface(0xBAAABFCC);
        NvGetName = (NvAPI_GPU_GetFullName_t) NvQueryInterface(0xCEEE8E9F);
        NvGetMemSize = (NvAPI_GPU_GetPhysicalFrameBufferSize_t) NvQueryInterface(0x46FBEB03);
        NvGetMemType = (NvAPI_GPU_GetRamType_t) NvQueryInterface(0x57F7CAAC);
        NvGetBiosName = (NvAPI_GPU_GetVbiosVersionString_t) NvQueryInterface(0xA561FD7D);
        NvGetFreq = (NvAPI_GPU_GetAllClockFrequencies_t) NvQueryInterface(0xDCB616C3);
        NvGetPstates = (NvAPI_GPU_GetPstates20_t) NvQueryInterface(0x6FF81213);
        NvSetPstates = (NvAPI_GPU_SetPstates20_t) NvQueryInterface(0x0F4DAE6B);
        NvAPI_GetErrorMessage = (NvAPI_GetErrorMessage_t) NvQueryInterface(0x6C2D048C);
        //
        int nGPU = 0, systype = 0, memsize = 0, memtype = 0, busId = 0;
        int *hdlGPU[64] = {0};
        char sysname[64] = {0}, biosname[64] = {0};

        safeNVAPICall(NvInit());
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        printf("NVAPI initialization...\n");
        printf("Number of GPUs: %i\n", nGPU);

        for (int idxGPU = 0; idxGPU < nGPU; idxGPU++) {
            safeNVAPICall(NvGetSysType(hdlGPU[idxGPU], &systype));
            safeNVAPICall(NvGetName(hdlGPU[idxGPU], sysname));
            safeNVAPICall(NvGetMemSize(hdlGPU[idxGPU], &memsize));
            safeNVAPICall(NvGetMemType(hdlGPU[idxGPU], &memtype));
            safeNVAPICall(NvGetBiosName(hdlGPU[idxGPU], biosname));
            safeNVAPICall(NvAPI_GetBusId(hdlGPU[idxGPU], &busId));

            printf("GPU index %i\n", idxGPU);
            switch (systype) {
                case 1:
                    printf("Type: Laptop\n");
                    break;
                case 2:
                    printf("Type: Desktop\n");
                    break;
                default:
                    printf("Type: Unknown\n");
                    break;
            }
            printf("Name: %s\n", sysname);
            printf("VRAM: %dMB GDDR%d\n", memsize / 1024, memtype <= 7 ? 3 : 5);
            printf("BIOS: %s\n", biosname);
            //
            //nvapiOC(idxGPU, 0, 0);
        }
    }

    void nvapiUnload(int restoreClocks) {
        printf("NVAPI unload...\n");
        if (restoreClocks) {
            int *hdlGPU[64] = {0};
            int nGPU;
            safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
            for (int idxGPU = 0; idxGPU < nGPU; idxGPU++) {
                //nvapiOC(idxGPU, 0, 0);
                printf("NVAPI restored clocks for device %i\n", idxGPU);
            }
        }
        //
        safeNVAPICall(NvUnload());
    }

    int nvapiGetDeviceIndexByBusId(int busId) {
        int *hdlGPU[64] = {0};
        int nGPU;
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        for (int idxGPU = 0; idxGPU < nGPU; idxGPU++) {
            int curBusId;
            safeNVAPICall(NvAPI_GetBusId(hdlGPU[idxGPU], &curBusId));
            if (busId == curBusId)
                return idxGPU;
        }
        return -1;
    }

    int nvapiGetCurrentMemClock(int deviceIdNvapi) {
        int *hdlGPU[64] = {0};
        int nGPU;
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        NV_GPU_PERF_PSTATES20_INFO_V1 pstates_info;
        pstates_info.version = 0x11c94;
        safeNVAPICall(NvGetPstates(hdlGPU[deviceIdNvapi], &pstates_info));
        int curRam = (int) ((pstates_info.pstates[0].clocks[1]).data.single.freq_kHz) / 1000;
        //int curRamOC = (int) ((pstates_info.pstates[0].clocks[1]).freqDelta_kHz.value) / 1000;
        return curRam;
    }

    int nvapiGetCurrentGraphClock(int deviceIdNvapi) {
        int *hdlGPU[64] = {0};
        int nGPU;
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        NV_GPU_PERF_PSTATES20_INFO_V1 pstates_info;
        pstates_info.version = 0x11c94;
        safeNVAPICall(NvGetPstates(hdlGPU[deviceIdNvapi], &pstates_info));
        int curGraph = (int) ((pstates_info.pstates[0].clocks[0]).data.range.maxFreq_kHz) / 1000;
        //int curGraphOC = (int) ((pstates_info.pstates[0].clocks[0]).freqDelta_kHz.value) / 1000;
        return curGraph;
    }

    void nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz) {

        int nGPU = 0;
        int *hdlGPU[64] = {0}, *buf = 0;

        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        if (idxGPU >= nGPU) {
            throw nvapi_error("Device with index " + std::to_string(idxGPU) + " doesnt exist");
        }

        //int memtype = 0;
        //safeNVAPICall(NvGetMemType(hdlGPU[idxGPU], &memtype));

        //GPU OC
        buf = (int *) malloc(0x1c94);
        memset(buf, 0, 0x1c94);
        buf[0] = 0x11c94;
        buf[2] = 1;
        buf[3] = 1;
        buf[10] = graphOCMHz * 1000;
        int error_code = NvSetPstates(hdlGPU[idxGPU], buf);
        free(buf);
        if (error_code != 0) {
            char msg_buf[BUFFER_SIZE];
            NvAPI_GetErrorMessage(error_code, msg_buf);
            throw nvapi_error("GPU " + std::to_string(idxGPU) +
                              " CORE OC failed: " + std::string(msg_buf));
        }

        //VRAM OC
        buf = (int *) malloc(0x1c94);
        memset(buf, 0, 0x1c94);
        buf[0] = 0x11c94;
        buf[2] = 1;
        buf[3] = 1;
        buf[7] = 4;
        //buf[10] = memtype <= 7 ? memOCMHz * 1000 : memOCMHz * 1000 * 2;
        buf[10] = memOCMHz * 1000;
        error_code = NvSetPstates(hdlGPU[idxGPU], buf);
        free(buf);
        if (error_code != 0) {
            char msg_buf[BUFFER_SIZE];
            NvAPI_GetErrorMessage(error_code, msg_buf);
            throw nvapi_error("GPU " + std::to_string(idxGPU) +
                              " VRAM OC failed: " + std::string(msg_buf));
        }
    }

#else

    void nvapiInit() {
        throw nvapi_error("NVAPI not supported");
    }

    void nvapiUnload(int restoreClocks) {
        throw nvapi_error("NVAPI not supported");
    }

    int nvapiGetDeviceIndexByBusId(int busId) {
        throw nvapi_error("NVAPI not supported");
    }

    int nvapiGetCurrentMemClock(int deviceId) {
        throw nvapi_error("NVAPI not supported");
    }

    int nvapiGetCurrentGraphClock(int deviceId) {
        throw nvapi_error("NVAPI not supported");
    }

    void nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz) {
        throw nvapi_error("NVAPI not supported");
    }

#endif

}
