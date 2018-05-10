#include <stdlib.h>
#include <stdio.h>
#include <nvml.h>

#define BUFFER_SIZE 1024

void safeNVMLCall(nvmlReturn_t result) {
	if (result != NVML_SUCCESS) {
		printf("NVML call failed: %s\n", nvmlErrorString(result));
		nvmlShutdown();
		exit(1);
	}
}

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("Usage: %s device_id", argv[0]);
		return 1;
	}
	unsigned int device_id = atoi(argv[1]);
	// First initialize NVML library
	safeNVMLCall(nvmlInit());
	nvmlDevice_t device;
	char name[NVML_DEVICE_NAME_BUFFER_SIZE];
	nvmlPciInfo_t pci;

	// Query for device handle to perform operations on a device
	safeNVMLCall(nvmlDeviceGetHandleByIndex(device_id, &device));
	safeNVMLCall(nvmlDeviceGetName(device, name, NVML_DEVICE_NAME_BUFFER_SIZE));
	// pci.busId is very useful to know which device physically you're talking to
	// Using PCI identifier you can also match nvmlDevice handle to CUDA device.
	safeNVMLCall(nvmlDeviceGetPciInfo(device, &pci));
	printf("%d. %s [%s]\n", device_id, name, pci.busId);

	//enable persistence mode
	//safeNVMLCall(nvmlDeviceSetPersistenceMode(device, 1));

	//start power monitoring
	char cmd1[BUFFER_SIZE];
	snprintf(cmd1, BUFFER_SIZE, "sh ./start_pm.sh %i", device_id);
	system(cmd1);

	//ensure gpu operations mode is compute
	/*nvmlGpuOperationMode_t current, pending;
	safeNVMLCall(nvmlDeviceGetGpuOperationMode (device, &current, &pending));
	if(current != NVML_GOM_COMPUTE){
		safeNVMLCall(nvmlDeviceSetGpuOperationMode(device, NVML_GOM_COMPUTE));
		//changing GOMs requires a reboot.
		printf("Changed GOM to compute. Please reboot for changes to take effect.\n");
		return 0;
	}*/

	//disable auto boosting of clocks (for hardware < pascal)
	//safeNVMLCall(nvmlDeviceSetDefaultAutoBoostedClocksEnabled(device, 0, 0));
	//safeNVMLCall(nvmlDeviceSetAutoBoostedClocksEnabled(device, 0));
	
	int default_mem_clock = 5005;
	int min_oc = -1000, max_oc = 1000;
	int interval = 50;

	//for all supported mem clocks (nvapi)
	for(int oc=min_oc ; oc<= max_oc; oc+= interval){
		int mem_freq = default_mem_clock + oc;
		char cmd2[BUFFER_SIZE];
		snprintf(cmd2, BUFFER_SIZE, "sh ./change_mem_freq.sh %i %i", 1, oc/2);
		system(cmd2);
		system(cmd2);
		//get supported graph clocks (nvml)
		unsigned int num_graph_clocks = BUFFER_SIZE;
		unsigned int avail_graph_clocks[BUFFER_SIZE];
		safeNVMLCall(nvmlDeviceGetSupportedGraphicsClocks(device, default_mem_clock,
				&num_graph_clocks, avail_graph_clocks));
		//for all supported graph clocks (nvml)
		for(int j = 0; j<num_graph_clocks; j+=2){
			unsigned int graph_freq = avail_graph_clocks[j];
			//change graph and mem clocks
			safeNVMLCall (nvmlDeviceSetApplicationsClocks(device, default_mem_clock, graph_freq));
			printf("\t Changed device clocks: mem:%i,graph:%i\n", mem_freq, graph_freq);
			//run benchmark command
			char cmd3[BUFFER_SIZE];
			snprintf(cmd3, BUFFER_SIZE, "sh ./run_benchmark.sh %i %i %i", device_id, mem_freq, graph_freq);
			system(cmd3);
		}
	}

	//stop power monitoring
	char cmd4[BUFFER_SIZE];
	snprintf(cmd4, BUFFER_SIZE, "sh ./stop_pm.sh %i", device_id);
	system(cmd4);

	//Restore states
	safeNVMLCall(nvmlDeviceResetApplicationsClocks(device));
	//safeNVMLCall(nvmlDeviceSetDefaultAutoBoostedClocksEnabled(device, 1, 0));
	//safeNVMLCall(nvmlDeviceSetAutoBoostedClocksEnabled(device, 1));
	printf("Restored clocks for device\n");
	//safeNVMLCall(nvmlDeviceSetGpuOperationMode(device, NVML_GOM_ALL_ON));
	//printf("Restored GOM to all on. Please reboot for changes to take effect.\n");
	safeNVMLCall(nvmlShutdown());
	printf("All done.\n");
	return 0;
}
