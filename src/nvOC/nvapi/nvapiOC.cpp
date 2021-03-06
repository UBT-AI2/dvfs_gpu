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
#include <set>
#include <glog/logging.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#else

#include <X11/Xlib.h>
#include <NVCtrl.h>
#include <NVCtrlLib.h>

#endif

#include "../common_header/constants.h"
#include "../common_header/exceptions.h"

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

    static void __nvapiOC(int device_id_nvapi, int graphOCMHz, int memOCMHz);

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

    static void checkDeviceId(int device_id_nvapi, int nGPU) {
        if (device_id_nvapi >= nGPU) {
            THROW_NVAPI_ERROR("Device with index " + std::to_string(device_id_nvapi) + " doesnt exist");
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

    bool nvapi_register_gpu(int device_id_nvapi) {
        int *hdlGPU[64] = {0}, nGPU;
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        checkDeviceId(device_id_nvapi, nGPU);
        if (!nvapiCheckSupport(device_id_nvapi)) {
            THROW_NVAPI_ERROR("NVAPI register failed. GPU " + std::to_string(device_id_nvapi) +
                              " doesnt support OC");
        }
        auto res = registered_gpus.emplace(device_id_nvapi);
        if (!res.second)
            return false;
        //reset clocks
        nvapiOC(device_id_nvapi, 0, 0);
        //print gpu info
        int memsize = 0, memtype = 0, busId = 0;
        char sysname[64] = {0}, biosname[64] = {0};
        safeNVAPICall(NvGetName(hdlGPU[device_id_nvapi], sysname));
        safeNVAPICall(NvGetMemSize(hdlGPU[device_id_nvapi], &memsize));
        safeNVAPICall(NvGetMemType(hdlGPU[device_id_nvapi], &memtype));
        safeNVAPICall(NvGetBiosName(hdlGPU[device_id_nvapi], biosname));
        safeNVAPICall(NvAPI_GetBusId(hdlGPU[device_id_nvapi], &busId));

        VLOG(0) << "Registered NVAPI GPU " << device_id_nvapi << ", Name: " << sysname <<
                ", VRAM: " << (memsize / 1024) << "MB GDDR" << ((memtype <= 7) ? 3 : 5) <<
                ", BIOS: " << biosname << ", BUS: " << busId << std::endl;
        return true;
    }

    void nvapiUnload(int restoreClocks) {
        //unload should not throw
        VLOG(0) << "NVAPI unload..." << std::endl;
        if (restoreClocks) {
            for (int idxGPU : registered_gpus) {
                try {
                    nvapiOC(idxGPU, 0, 0);
                    VLOG(0) << "NVAPI restored clocks for GPU " << idxGPU << std::endl;
                }
                catch (const nvapi_error &ex) {
                    LOG(ERROR) << "NVAPI failed to restore clocks for GPU " << idxGPU << std::endl;
                }
            }
        }
        //
        try {
            safeNVAPICall(NvUnload());
        }
        catch (const nvapi_error &ex) {
            LOG(ERROR) << "NVAPI unload failed" << std::endl;
        }
    }

    bool nvapiCheckSupport(int device_id_nvapi) {
        try {
            __nvapiOC(device_id_nvapi, 0, 0);
            return true;
        } catch (const nvapi_error &err) {
            return false;
        }
    }

    int nvapiGetDeviceIndexByBusId(int busId) {
        int *hdlGPU[64] = {0}, nGPU;
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        for (int idxGPU = 0; idxGPU < nGPU; idxGPU++) {
            int curBusId;
            safeNVAPICall(NvAPI_GetBusId(hdlGPU[idxGPU], &curBusId));
            if (busId == curBusId)
                return idxGPU;
        }
        return -1;
    }

    nvapi_clock_info nvapiGetMemClockInfo(int device_id_nvapi) {
        int *hdlGPU[64] = {0}, nGPU;
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        checkDeviceId(device_id_nvapi, nGPU);
        NV_GPU_PERF_PSTATES20_INFO_V1 pstates_info;
        pstates_info.version = NV_GPU_PERF_PSTATES20_INFO_VER;
        safeNVAPICall(NvGetPstates(hdlGPU[device_id_nvapi], &pstates_info));
        nvapi_clock_info ci;
        ci.current_freq_ = (int) ((pstates_info.pstates[0].clocks[1]).data.single.freq_kHz) / 1000;
        ci.current_oc_ = (int) ((pstates_info.pstates[0].clocks[1]).freqDelta_kHz.value) / 1000;
        ci.min_oc_ = (int) ((pstates_info.pstates[0].clocks[1]).freqDelta_kHz.valueRange.mindelta) / 1000;
        ci.max_oc_ = (int) ((pstates_info.pstates[0].clocks[1]).freqDelta_kHz.valueRange.maxdelta) / 1000;
        return ci;
    }

    nvapi_clock_info nvapiGetGraphClockInfo(int device_id_nvapi) {
        int *hdlGPU[64] = {0}, nGPU;
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        checkDeviceId(device_id_nvapi, nGPU);
        NV_GPU_PERF_PSTATES20_INFO_V1 pstates_info;
        pstates_info.version = NV_GPU_PERF_PSTATES20_INFO_VER;
        safeNVAPICall(NvGetPstates(hdlGPU[device_id_nvapi], &pstates_info));
        nvapi_clock_info ci;
        ci.current_freq_ = (int) ((pstates_info.pstates[0].clocks[0]).data.range.maxFreq_kHz) / 1000;
        ci.current_oc_ = (int) ((pstates_info.pstates[0].clocks[0]).freqDelta_kHz.value) / 1000;
        ci.min_oc_ = (int) ((pstates_info.pstates[0].clocks[0]).freqDelta_kHz.valueRange.mindelta) / 1000;
        ci.max_oc_ = (int) ((pstates_info.pstates[0].clocks[0]).freqDelta_kHz.valueRange.maxdelta) / 1000;
        return ci;
    }

    void nvapiOC(int device_id_nvapi, int graphOCMHz, int memOCMHz) {
        if (!registered_gpus.count(device_id_nvapi))
            THROW_NVAPI_ERROR("GPU " + std::to_string(device_id_nvapi) + " not registered for OC");
        __nvapiOC(device_id_nvapi, graphOCMHz, memOCMHz);
    }

    static void __nvapiOC(int device_id_nvapi, int graphOCMHz, int memOCMHz) {
        int *hdlGPU[64] = {0}, nGPU;
        safeNVAPICall(NvEnumPhysicalGPUs(hdlGPU, &nGPU));
        checkDeviceId(device_id_nvapi, nGPU);

        //int memtype = 0;
        //safeNVAPICall(NvGetMemType(hdlGPU[idxGPU], &memtype));

        //GPU OC
        NV_GPU_PERF_PSTATES20_INFO_V1 pset1 = {0};
        pset1.version = NV_GPU_PERF_PSTATES20_INFO_VER;
        pset1.numPstates = 1;
        pset1.numClocks = 1;
        pset1.pstates[0].clocks[0].domainId = NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS;
        pset1.pstates[0].clocks[0].freqDelta_kHz.value = graphOCMHz * 1000;
        int error_code = NvSetPstates(hdlGPU[device_id_nvapi], &pset1);
        if (error_code != 0) {
            char msg_buf[BUFFER_SIZE];
            NvAPI_GetErrorMessage(error_code, msg_buf);
            THROW_NVAPI_ERROR("GPU " + std::to_string(device_id_nvapi) +
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
        error_code = NvSetPstates(hdlGPU[device_id_nvapi], &pset2);
        if (error_code != 0) {
            char msg_buf[BUFFER_SIZE];
            NvAPI_GetErrorMessage(error_code, msg_buf);
            THROW_NVAPI_ERROR("GPU " + std::to_string(device_id_nvapi) +
                              " VRAM OC failed: " + std::string(msg_buf));
        }
    }

#else
    static std::set<int> registered_gpus;
    static Display *dpy = NULL;

    static void __nvapiOC(int device_id_nvapi, int graphOCMHz, int memOCMHz);

    static void safeXNVCTRLCall(Bool ret, const std::string &error_msg) {
        if (!ret) {
            THROW_NVAPI_ERROR(error_msg);
        }
    }

    void nvapiInit() {
        VLOG(0) << "XNVCTRL initialization..." << std::endl;
        if (!XInitThreads()) {
            THROW_NVAPI_ERROR("XInitThreads failed");
        }
        //Open a display connection, and make sure the NV-CONTROL X
        //extension is present on the screen we want to use.
        int major, minor;
        dpy = XOpenDisplay(NULL);
        if (!dpy) {
            THROW_NVAPI_ERROR("Cannot open display " + std::string(XDisplayName(NULL)));
        }
        safeXNVCTRLCall(XNVCTRLQueryVersion(dpy, &major, &minor),
                        "The NV-CONTROL X extension does not exist on display " + std::string(XDisplayName(NULL)));
        VLOG(0) << "XNVCTRL initialized. Using NV-CONTROL extension "
                << major << "." << minor << " on display " << XDisplayName(NULL) << std::endl;
    }

    bool nvapi_register_gpu(int device_id_nvapi) {
        if (!nvapiCheckSupport(device_id_nvapi)) {
            THROW_NVAPI_ERROR("XNVCTRL register failed. GPU " + std::to_string(device_id_nvapi) +
                              " doesnt support OC");
        }
        auto res = registered_gpus.emplace(device_id_nvapi);
        if (!res.second)
            return false;
        //reset clocks
        nvapiOC(device_id_nvapi, 0, 0);
        //print gpu info
        char *gpu_name;
        safeXNVCTRLCall(XNVCTRLQueryTargetStringAttribute(dpy,
                                                          NV_CTRL_TARGET_TYPE_GPU,
                                                          device_id_nvapi, // target_id
                                                          0, // display_mask
                                                          NV_CTRL_STRING_PRODUCT_NAME,
                                                          &gpu_name), "Failed to query gpu product name");
        VLOG(0) << "Registered XNCTRL GPU " << device_id_nvapi << "(" << gpu_name << ")" << std::endl;
        XFree(gpu_name);
        return true;
    }

    void nvapiUnload(int restoreClocks) {
        //unload should not throw
        VLOG(0) << "XNVCTRL unload..." << std::endl;
        if (restoreClocks) {
            for (int gpu : registered_gpus) {
                try {
                    nvapiOC(gpu, 0, 0);
                    VLOG(0) << "XNVCTRL restored clocks for GPU " << gpu << std::endl;
                } catch (const nvapi_error &ex) {
                    LOG(ERROR) << "XNVCTRL failed to restore clocks for GPU " << gpu << std::endl;
                }
            }
        }
        XCloseDisplay(dpy);
    }

    bool nvapiCheckSupport(int device_id_nvapi) {
        try {
            __nvapiOC(device_id_nvapi, 0, 0);
            return true;
        } catch (const nvapi_error &err) {
            return false;
        }
    }

    int nvapiGetDeviceIndexByBusId(int busId) {
        int num_gpus;
        safeXNVCTRLCall(XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_GPU, &num_gpus),
                        "Failed to query number of gpus");
        for (int gpu = 0; gpu < num_gpus; gpu++) {
            int current_busid;
            safeXNVCTRLCall(XNVCTRLQueryTargetAttribute(dpy,
                                                        NV_CTRL_TARGET_TYPE_GPU,
                                                        gpu, // target_id
                                                        0, // display_mask
                                                        NV_CTRL_PCI_BUS,
                                                        &current_busid),
                            "Failed to query bus id for GPU " + std::to_string(gpu));
            if (current_busid == busId)
                return gpu;
        }
        return -1;
    }

    nvapi_clock_info nvapiGetMemClockInfo(int device_id_nvapi) {
        nvapi_clock_info res;
        NVCTRLAttributeValidValuesRec valid_values;
        int current_clock_freqs, current_oc;
        safeXNVCTRLCall(XNVCTRLQueryValidTargetAttributeValues(dpy,
                                                               NV_CTRL_TARGET_TYPE_GPU,
                                                               device_id_nvapi,
                                                               3,//performance level
                                                               NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET,
                                                               &valid_values),
                        "Unable to query the valid range of values for "
                        "NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET on GPU " +
                        std::to_string(device_id_nvapi));
        safeXNVCTRLCall(XNVCTRLQueryTargetAttribute(dpy,
                                                    NV_CTRL_TARGET_TYPE_GPU,
                                                    device_id_nvapi,
                                                    3,//performance level
                                                    NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET,
                                                    &current_oc), "Unable to query the current value for "
                                                                  "NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET on GPU " +
                                                                  std::to_string(device_id_nvapi));
        safeXNVCTRLCall(XNVCTRLQueryTargetAttribute(dpy,
                                                    NV_CTRL_TARGET_TYPE_GPU,
                                                    device_id_nvapi,
                                                    0,
                                                    NV_CTRL_GPU_CURRENT_CLOCK_FREQS,
                                                    &current_clock_freqs), "Unable to query the current value for "
                                                                           "NV_CTRL_GPU_CURRENT_CLOCK_FREQS on GPU " +
                                                                           std::to_string(device_id_nvapi));


        res.min_oc_ = valid_values.u.range.min / 2;
        res.max_oc_ = valid_values.u.range.max / 2;
        res.current_oc_ = current_oc;
        res.current_freq_ = (current_clock_freqs & 0xffff);
        return res;
    }

    nvapi_clock_info nvapiGetGraphClockInfo(int device_id_nvapi) {
        nvapi_clock_info res;
        NVCTRLAttributeValidValuesRec valid_values;
        int current_clock_freqs, current_oc;
        safeXNVCTRLCall(XNVCTRLQueryValidTargetAttributeValues(dpy,
                                                               NV_CTRL_TARGET_TYPE_GPU,
                                                               device_id_nvapi,
                                                               3,//performance level
                                                               NV_CTRL_GPU_NVCLOCK_OFFSET,
                                                               &valid_values),
                        "Unable to query the valid range of values for "
                        "NV_CTRL_GPU_NVCLOCK_OFFSET on GPU " +
                        std::to_string(device_id_nvapi));
        safeXNVCTRLCall(XNVCTRLQueryTargetAttribute(dpy,
                                                    NV_CTRL_TARGET_TYPE_GPU,
                                                    device_id_nvapi,
                                                    3,//performance level
                                                    NV_CTRL_GPU_NVCLOCK_OFFSET,
                                                    &current_oc), "Unable to query the current value for "
                                                                  "NV_CTRL_GPU_NVCLOCK_OFFSET on GPU " +
                                                                  std::to_string(device_id_nvapi));
        safeXNVCTRLCall(XNVCTRLQueryTargetAttribute(dpy,
                                                    NV_CTRL_TARGET_TYPE_GPU,
                                                    device_id_nvapi,
                                                    0,
                                                    NV_CTRL_GPU_CURRENT_CLOCK_FREQS,
                                                    &current_clock_freqs), "Unable to query the current value for "
                                                                           "NV_CTRL_GPU_CURRENT_CLOCK_FREQS on GPU " +
                                                                           std::to_string(device_id_nvapi));


        res.min_oc_ = valid_values.u.range.min;
        res.max_oc_ = valid_values.u.range.max;
        res.current_oc_ = current_oc;
        res.current_freq_ = (current_clock_freqs >> 16);
        return res;
    }

    void nvapiOC(int device_id_nvapi, int graphOCMHz, int memOCMHz) {

        if (!registered_gpus.count(device_id_nvapi))
            THROW_NVAPI_ERROR("GPU " + std::to_string(device_id_nvapi) + " not registered for OC");

        __nvapiOC(device_id_nvapi, graphOCMHz, memOCMHz);
    }

    static void __nvapiOC(int device_id_nvapi, int graphOCMHz, int memOCMHz) {
        //Set mem clock offset
        safeXNVCTRLCall(XNVCTRLSetTargetAttributeAndGetStatus(dpy,
                                                              NV_CTRL_TARGET_TYPE_GPU,
                                                              device_id_nvapi,
                                                              3,//performance level
                                                              NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET,
                                                              memOCMHz * 2),
                        "Unable to set value " + std::to_string(memOCMHz) +
                        " for NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET on GPU " +
                        std::to_string(device_id_nvapi));
        //Set graph clock offset
        safeXNVCTRLCall(XNVCTRLSetTargetAttributeAndGetStatus(dpy,
                                                              NV_CTRL_TARGET_TYPE_GPU,
                                                              device_id_nvapi,
                                                              3,//performance level
                                                              NV_CTRL_GPU_NVCLOCK_OFFSET,
                                                              graphOCMHz),
                        "Unable to set value " + std::to_string(graphOCMHz) +
                        " for NV_CTRL_GPU_NVCLOCK_OFFSET on GPU " +
                        std::to_string(device_id_nvapi));
    }

#endif

}
