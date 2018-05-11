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
#include <windows.h>

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

NvAPI_QueryInterface_t NvQueryInterface = 0;
NvAPI_Initialize_t NvInit = 0;
NvAPI_Unload_t NvUnload = 0;
NvAPI_EnumPhysicalGPUs_t NvEnumGPUs = 0;
NvAPI_GPU_GetSystemType_t NvGetSysType = 0;
NvAPI_GPU_GetFullName_t NvGetName = 0;
NvAPI_GPU_GetPhysicalFrameBufferSize_t NvGetMemSize = 0;
NvAPI_GPU_GetRamType_t NvGetMemType = 0;
NvAPI_GPU_GetVbiosVersionString_t NvGetBiosName = 0;
NvAPI_GPU_GetAllClockFrequencies_t NvGetFreq = 0;
NvAPI_GPU_GetPstates20_t NvGetPstates = 0;
NvAPI_GPU_SetPstates20_t NvSetPstates = 0;


int nvapiInit() {
	NvQueryInterface = (void*) GetProcAddress(LoadLibrary("nvapi64.dll"),
			"nvapi_QueryInterface");
	if (NvQueryInterface == NULL) {
		printf("Failed to load nvapi.dll\n");
		return 1;
	}
	NvInit = NvQueryInterface(0x0150E828);
	NvUnload = NvQueryInterface(0xD22BDD7E);
	NvEnumGPUs = NvQueryInterface(0xE5AC921F);
	NvGetSysType = NvQueryInterface(0xBAAABFCC);
	NvGetName = NvQueryInterface(0xCEEE8E9F);
	NvGetMemSize = NvQueryInterface(0x46FBEB03);
	NvGetMemType = NvQueryInterface(0x57F7CAAC);
	NvGetBiosName = NvQueryInterface(0xA561FD7D);
	NvGetFreq = NvQueryInterface(0xDCB616C3);
	NvGetPstates = NvQueryInterface(0x6FF81213);
	NvSetPstates = NvQueryInterface(0x0F4DAE6B);
	//
	int nGPU = 0, systype = 0, memsize = 0, memtype = 0;
	int *hdlGPU[64] = { 0 }, *buf = 0;
	char sysname[64] = { 0 }, biosname[64] = { 0 };

	NvInit();
	NvEnumGPUs(hdlGPU, &nGPU);
	printf("Number of GPUs: %i\n", nGPU);

	for (int idxGPU = 0; idxGPU < nGPU; idxGPU++) {
		NvGetSysType(hdlGPU[idxGPU], &systype);
		NvGetName(hdlGPU[idxGPU], sysname);
		NvGetMemSize(hdlGPU[idxGPU], &memsize);
		NvGetMemType(hdlGPU[idxGPU], &memtype);
		NvGetBiosName(hdlGPU[idxGPU], biosname);

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
	}

	return 0;
}

int nvapiUnload(){
	NvUnload();
	return 0;
}

int nvapiOC(int idxGPU, int graphOCMHz, int memOCMHz) {

	int nGPU = 0, memsize = 0, memtype = 0;
	int *hdlGPU[64] = { 0 }, *buf = 0;
	char sysname[64] = { 0 };
	NV_GPU_PERF_PSTATES20_INFO_V1 pstates_info;
	pstates_info.version = 0x11c94;

	NvEnumGPUs(hdlGPU, &nGPU);
	printf("Number of GPUs: %i\n", nGPU);
	if (idxGPU >= nGPU) {
		printf("Device with index %i doesnt exist\n", idxGPU);
		return 1;
	}

	NvGetName(hdlGPU[idxGPU], sysname);
	NvGetMemSize(hdlGPU[idxGPU], &memsize);
	NvGetMemType(hdlGPU[idxGPU], &memtype);

	printf("Overclocking GPU %i: %s\n", idxGPU, sysname);

	//GPU OC
	buf = malloc(0x1c94);
	memset(buf, 0, 0x1c94);
	buf[0] = 0x11c94;
	buf[2] = 1;
	buf[3] = 1;
	buf[10] = graphOCMHz * 1000;
	NvSetPstates(hdlGPU[idxGPU], buf) ?
			printf("\nGPU OC failed!\n") :
			printf("\nGPU OC OK: %d MHz\n", graphOCMHz);
	free(buf);

	//VRAM OC
	buf = malloc(0x1c94);
	memset(buf, 0, 0x1c94);
	buf[0] = 0x11c94;
	buf[2] = 1;
	buf[3] = 1;
	buf[7] = 4;
	//buf[10] = memtype <= 7 ? memOCMHz * 1000 : memOCMHz * 1000 * 2;
    buf[10] = memOCMHz * 1000;

    NvSetPstates(hdlGPU[idxGPU], buf) ?
			printf("VRAM OC failed!\n") :
			printf("RAM OC OK: %d MHz\n", memOCMHz);
	free(buf);

    NvGetPstates(hdlGPU[idxGPU], &pstates_info);
    printf("\nGPU: %dMHz\n",
           (int) ((pstates_info.pstates[0].clocks[0]).data.range.maxFreq_kHz)
           / 1000);
    printf("RAM: %dMHz\n",
           (int) ((pstates_info.pstates[0].clocks[1]).data.single.freq_kHz)
           / 1000);
    printf("\nCurrent GPU OC: %dMHz\n",
           (int) ((pstates_info.pstates[0].clocks[0]).freqDelta_kHz.value)
           / 1000);
    printf("Current RAM OC: %dMHz\n",
           (int) ((pstates_info.pstates[0].clocks[1]).freqDelta_kHz.value)
           / 1000);

	return 0;
}
