#pragma once

#include <vector>

void nvmlInit_();

void nvmlShutdown_();

std::vector<int> getAvailableMemClocks(int device_id);

std::vector<int> getAvailableGraphClocks(int device_id, int mem_clock);

int getDefaultMemClock(int device_id);

int getDefaultGraphClock(int device_id);

int getNumDevices();

void nvmlOC(int device_id, int graphClock, int memClock);
