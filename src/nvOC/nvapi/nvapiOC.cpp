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
#include <string>

#ifdef _WIN32

#include <set>
#include <sstream>
#include <glog/logging.h>
#include <windows.h>
#include "../common_header/constants.h"
#include "../common_header/exceptions.h"

#endif

namespace frequency_scaling {

#ifdef _WIN32

    static const int NVAPI_MAX_GPU_PUBLIC_CLOCKS = 32;
    static const int NVAPI_MAX_GPU_PSTATE20_PSTATES = 16;
    static const int NVAPI_MAX_GPU_PSTATE20_CLOCKS = 8;
    static const int NVAPI_MAX_GPU_PSTATE20_BASE_VOLTAGES = 4;
    static const int NV_GPU_CLOCK_FREQUENCIES_VER = 196872;
    static const int NV_GPU_PERF_PSTATES20_INFO_VER = 0x11c94;

    typedef unsigned long NvU32;

    typedef enum _NV_GPU_PUBLIC_CLOCK_ID {
        NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS = 0,
        NVAPI_GPU_PUBLIC_CLOCK_MEMORY = 4,
        NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR = 7,
        NVAPI_GPU_PUBLIC_CLOCK_VIDEO = 8,
        NVAPI_GPU_PUBLIC_CLOCK_UNDEFINED = NVAPI_MAX_GPU_PUBLIC_CLOCKS,
    } NV_GPU_PUBLIC_CLOCK_ID;

    typedef enum {
        NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ = 0,
        NV_GPU_CLOCK_FREQUENCIES_BASE_CLOCK = 1,
        NV_GPU_CLOCK_FREQUENCIES_BOOST_CLOCK = 2,
        NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE_NUM = 3
    } NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE;

    typedef struct {
        NvU32 version;
        NvU32 ClockType : 2;
        NvU32 reserved : 22;
        NvU32 reserved1 : 8;
        struct {
            NvU32 bIsPresent : 1;
            NvU32 reserved : 31;
            NvU32 frequency;
        } domain[NVAPI_MAX_GPU_PUBLIC_CLOCKS];
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
        NvU32 bIsEditable : 1;
        NvU32 reserved : 31;
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
        NvU32 bIsEditable : 1;
        NvU32 reserved : 31;
        NvU32 volt_uV;
        int voltDelta_uV;
    } NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1;

    typedef struct {
        NvU32 version;
        NvU32 bIsEditable : 1;
        NvU32 reserved : 31;
        NvU32 numPstates;
        NvU32 numClocks;
        NvU32 numBaseVoltages;
        struct {
            NvU32 pstateId;
            NvU32 bIsEditable : 1;
            NvU32 reserved : 31;
            NV_GPU_PSTATE20_CLOCK_ENTRY_V1 clocks[NVAPI_MAX_GPU_PSTATE20_CLOCKS];
            NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1 baseVoltages[NVAPI_MAX_GPU_PSTATE20_BASE_VOLTAGES];
        } pstates[NVAPI_MAX_GPU_PSTATE20_PSTATES];
    } NV_GPU_PERF_PSTATES20_INFO_V1;


    typedef void *(*NvAPI_QueryInterface_t)(unsigned int offset);

    typedef int(*NvAPI_Initialize_t)();

    typedef int(*NvAPI_Unload_t)();

    typedef int(*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);

    typedef int(*NvAPI_GetBusId_t)(int *handle, int *busId);

    typedef int(*NvAPI_GPU_GetSystemType_t)(int *handle, int *systype);

    typedef int(*NvAPI_GPU_GetFullName_t)(int *handle, char *sysname);

    typedef int(*NvAPI_GPU_GetPhysicalFrameBufferSize_t)(int *handle, int *memsize);

    typedef int(*NvAPI_GPU_GetRamType_t)(int *handle, int *memtype);

    typedef int(*NvAPI_GPU_GetVbiosVersionString_t)(int *handle, char *biosname);

    typedef int(*NvAPI_GPU_GetAllClockFrequencies_t)(int *handle,
                                                     NV_GPU_CLOCK_FREQUENCIES_V2 *clock_info);

    typedef int(*NvAPI_GPU_GetPstates20_t)(int *handle,
                                           NV_GPU_PERF_PSTATES20_INFO_V1 *pstates_info);

    typedef int(*NvAPI_GPU_SetPstates20_t)(int *handle,
                                           NV_GPU_PERF_PSTATES20_INFO_V1 *pstates_info);

    typedef int(*NvAPI_GetErrorMessage_t)(int error_code, char *error_string);


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

    static std::set<int> registered_gpus;

    static void safeNVAPICall(int result) {
        if (result != 0) {
            std::string msg("NVAPI call failed: ");
            char msg_buf[BUFFER_SIZE];
            NvAPI_GetErrorMessage(result, msg_buf);
            msg.append(msg_buf);
            msg.append("\n");
            THROW_NVAPI_ERROR(msg);
        }
    }

    void nvapiInit() {
        VLOG(0) << "NVAPI initialization..." << std::endl;

        NvQueryInterface = (NvAPI_QueryInterface_t) GetProcAddress(LoadLibrary("nvapi64.dll"),
                                                                   "nvapi_QueryInterface");
        if (NvQueryInterface == NULL) {
            THROW_NVAPI_ERROR("Failed to load nvapi.dll");
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

        safeNVAPICall(NvInit());
    }

    bool nvapi_register_gpu(int device_id) {
        if (!nvapiCheckSupport(device_id))
            THROW_NVAPI_ERROR("NVAPI register failed. GPU " + std::to_string(device_id) + " doesnt support OC");
        auto res = registered_gpus.emplace(device_id);
        if (!res.second)
            return false;

        int nGPU = 0, systype = 0, memsize = 0, memtype = 0, busId = 0;
        int *hdlGPU[64] = {0};
        char sysname[64] = {0}, biosname[64] = {0};
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));

        int idxGPU = device_id;
        safeNVAPICall(NvGetSysType(hdlGPU[idxGPU], &systype));
        safeNVAPICall(NvGetName(hdlGPU[idxGPU], sysname));
        safeNVAPICall(NvGetMemSize(hdlGPU[idxGPU], &memsize));
        safeNVAPICall(NvGetMemType(hdlGPU[idxGPU], &memtype));
        safeNVAPICall(NvGetBiosName(hdlGPU[idxGPU], biosname));
        safeNVAPICall(NvAPI_GetBusId(hdlGPU[idxGPU], &busId));

        std::ostringstream ss;
        ss << "Registered NVAPI GPU " << idxGPU;
        switch (systype) {
            case 1:
                ss << ", Type: Laptop";
                break;
            case 2:
                ss << ", Type: Desktop";
                break;
            default:
                ss << ", Type: Unknown";
                break;
        }
        ss << ", Name: " << sysname;
        ss << ", VRAM: " << (memsize / 1024) << "MB GDDR" << ((memtype <= 7) ? 3 : 5);
        ss << ", BIOS: " << biosname << std::endl;
        VLOG(0) << ss.str() << std::flush;
        //
        nvapiOC(idxGPU, 0, 0);
        return true;
    }

    void nvapiUnload(int restoreClocks) {
        //unload should not throw
        try {
            VLOG(0) << "NVAPI unload..." << std::endl;
            if (restoreClocks) {
                int *hdlGPU[64] = {0};
                int nGPU;
                safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
                for (int idxGPU : registered_gpus) {
                    nvapiOC(idxGPU, 0, 0);
                    VLOG(0) << "NVAPI restored clocks for GPU " << idxGPU << std::endl;
                }
            }
        }
        catch (const nvapi_error &ex) {
            LOG(ERROR) << "NVAPI restore clocks failed" << std::endl;
        }
        //
        try {
            safeNVAPICall(NvUnload());
        }
        catch (const nvapi_error &ex) {
            LOG(ERROR) << "NVAPI unload failed" << std::endl;
        }
    }

    bool nvapiCheckSupport(int device_id) {
        return true;
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

    nvapi_clock_info nvapiGetMemClockInfo(int deviceIdNvapi) {
        int *hdlGPU[64] = {0};
        int nGPU;
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        NV_GPU_PERF_PSTATES20_INFO_V1 pstates_info;
        pstates_info.version = NV_GPU_PERF_PSTATES20_INFO_VER;
        safeNVAPICall(NvGetPstates(hdlGPU[deviceIdNvapi], &pstates_info));
        nvapi_clock_info ci;
        ci.current_freq_ = (int) ((pstates_info.pstates[0].clocks[1]).data.single.freq_kHz) / 1000;
        ci.current_oc_ = (int) ((pstates_info.pstates[0].clocks[1]).freqDelta_kHz.value) / 1000;
        ci.min_oc_ = (int) ((pstates_info.pstates[0].clocks[1]).freqDelta_kHz.valueRange.mindelta) / 1000;
        ci.max_oc_ = (int) ((pstates_info.pstates[0].clocks[1]).freqDelta_kHz.valueRange.maxdelta) / 1000;
        return ci;
    }

    nvapi_clock_info nvapiGetGraphClockInfo(int deviceIdNvapi) {
        int *hdlGPU[64] = {0};
        int nGPU;
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        NV_GPU_PERF_PSTATES20_INFO_V1 pstates_info;
        pstates_info.version = NV_GPU_PERF_PSTATES20_INFO_VER;
        safeNVAPICall(NvGetPstates(hdlGPU[deviceIdNvapi], &pstates_info));
        nvapi_clock_info ci;
        ci.current_freq_ = (int) ((pstates_info.pstates[0].clocks[0]).data.range.maxFreq_kHz) / 1000;
        ci.current_oc_ = (int) ((pstates_info.pstates[0].clocks[0]).freqDelta_kHz.value) / 1000;
        ci.min_oc_ = (int) ((pstates_info.pstates[0].clocks[0]).freqDelta_kHz.valueRange.mindelta) / 1000;
        ci.max_oc_ = (int) ((pstates_info.pstates[0].clocks[0]).freqDelta_kHz.valueRange.maxdelta) / 1000;
        return ci;
    }

    void nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz) {

        int nGPU = 0;
        int *hdlGPU[64] = {0};

        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        if (idxGPU >= nGPU) {
            THROW_NVAPI_ERROR("Device with index " + std::to_string(idxGPU) + " doesnt exist");
        }

        if (!registered_gpus.count(idxGPU))
            THROW_NVAPI_ERROR("GPU " + std::to_string(idxGPU) + " not registered for OC");

        //int memtype = 0;
        //safeNVAPICall(NvGetMemType(hdlGPU[idxGPU], &memtype));

        //GPU OC
        NV_GPU_PERF_PSTATES20_INFO_V1 pset1 = {0};
        pset1.version = NV_GPU_PERF_PSTATES20_INFO_VER;
        pset1.numPstates = 1;
        pset1.numClocks = 1;
        pset1.pstates[0].clocks[0].domainId = NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS;
        pset1.pstates[0].clocks[0].freqDelta_kHz.value = graphOCMHz * 1000;
        int error_code = NvSetPstates(hdlGPU[idxGPU], &pset1);
        if (error_code != 0) {
            char msg_buf[BUFFER_SIZE];
            NvAPI_GetErrorMessage(error_code, msg_buf);
            THROW_NVAPI_ERROR("GPU " + std::to_string(idxGPU) +
                              " CORE OC failed: " + std::string(msg_buf));
        }

        //VRAM OC
        NV_GPU_PERF_PSTATES20_INFO_V1 pset2 = {0};
        pset2.version = NV_GPU_PERF_PSTATES20_INFO_VER;
        pset2.numPstates = 1;
        pset2.numClocks = 1;
        pset2.pstates[0].clocks[0].domainId = NVAPI_GPU_PUBLIC_CLOCK_MEMORY;
        pset2.pstates[0].clocks[0].freqDelta_kHz.value = memOCMHz * 1000;
        //pset2.pstates[0].clocks[0].freqDelta_kHz.value = memtype <= 7 ? memOCMHz * 1000 : memOCMHz * 1000 * 2;
        error_code = NvSetPstates(hdlGPU[idxGPU], &pset2);
        if (error_code != 0) {
            char msg_buf[BUFFER_SIZE];
            NvAPI_GetErrorMessage(error_code, msg_buf);
            THROW_NVAPI_ERROR("GPU " + std::to_string(idxGPU) +
                              " VRAM OC failed: " + std::string(msg_buf));
        }
    }

#else

    void nvapiInit() {
        THROW_NVAPI_ERROR("NVAPI not available on this platform");
    }

    bool nvapi_register_gpu(int device_id) {
        THROW_NVAPI_ERROR("NVAPI not available on this platform");
    }

    void nvapiUnload(int restoreClocks) {
        THROW_NVAPI_ERROR("NVAPI not available on this platform");
    }

    bool nvapiCheckSupport(int device_id) {
        return false;
    }

    int nvapiGetDeviceIndexByBusId(int busId) {
        THROW_NVAPI_ERROR("NVAPI not available on this platform");
    }

    nvapi_clock_info nvapiGetMemClockInfo(int deviceIdNvapi) {
        THROW_NVAPI_ERROR("NVAPI not available on this platform");
    }

    nvapi_clock_info nvapiGetGraphClockInfo(int deviceIdNvapi) {
        THROW_NVAPI_ERROR("NVAPI not available on this platform");
    }

    void nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz) {
        THROW_NVAPI_ERROR("NVAPI not available on this platform");
    }

#endif

}
