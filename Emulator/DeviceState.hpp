#pragma once

#include "Device.hpp"
#include "util.hpp"


class DeviceState {
public:

	void init(const Device &device) {
		this->state = this->lastState = 0;//device.state;
		this->interpolator = 0;//device.state << 16;
		this->condition = 0;
	}

	/**
	 * Check if a state is active
	 */
	bool isActive(const Device &device, uint8_t state);

	void setState(const Device &device, uint8_t state);
	
	int update(const Device &device, int ticks);

	uint8_t state;
	uint8_t lastState;
	int interpolator;
	
	// currently active condition, used to trigger it only once when it becomes active
	uint8_t condition;
};
