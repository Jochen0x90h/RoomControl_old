#pragma once

#include "Device.hpp"
#include "util.hpp"


class DeviceState {
public:
	// mode that indicates that a transition is in progress
	static const uint8_t TRANSITION = 1;

	// mode that indicates that a delay is pending
	static const uint8_t DELAY = TRANSITION;


	void init(const Device &device) {
		this->state = this->lastState = 0;
		this->mode = 0;
		this->condition = 0;
		this->timer = 0;
	}

	/**
	 * Check if a state is active or there is a transition in progress towards the state
	 */
	bool isActive(const Device &device, uint8_t state);

	void setState(const Device &device, uint8_t state);
	
	/**
	 * Update device state
	 * @device device configuration
	 * @ticks ticks in milliseconds since last call
	 */
	int update(const Device &device, int ticks, int temperature);

	// last state, needed if we want to alternate direction of movement
	uint8_t lastState;

	// current state or target state in case of delay e.g. for dimmers or blinds
	uint8_t state;
	
	// mode such as TRANSITION or ACTIVE
	uint8_t mode;
	
	// currently active condition, used to trigger it only once when it becomes active
	uint8_t condition;

	// used to interpolate to target state or measure timeout
	int timer;
	
};
