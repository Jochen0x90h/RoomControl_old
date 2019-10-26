#include "DeviceState.hpp"


bool DeviceState::isActive(const Device &device, uint8_t state) {
	if (state == this->state)
		return true;
		
	switch (device.type) {
	case Device::Type::SWITCH:
	case Device::Type::LIGHT:
		break;
	case Device::Type::DIMMER:
	case Device::Type::BLIND:
		{
			int s = int(this->state) << 16;
			return state == Device::VALUE
				|| (state == Device::STOP && this->interpolator == s)
				|| (state == Device::LIGHTEN && this->interpolator < s)
				|| (state == Device::DARKEN && this->interpolator > s)
				|| (state == Device::DIM && this->interpolator != s);
		}
	case Device::Type::HANDLE:
		return (state == Device::SHUT && (this->state == Device::LOCKED || this->state == Device::CLOSED))
			|| (state == Device::UNSHUT && (this->state == Device::OPEN || this->state == Device::TILT))
			|| (state == Device::UNLOCKED && this->state != Device::LOCKED);
	}
	return false;
}

void DeviceState::setState(const Device &device, uint8_t state) {
	switch (device.type) {
	case Device::Type::SWITCH:
	case Device::Type::LIGHT:
		this->lastState = this->state;
		this->state = state;
		if (device.speed == 0)
			this->interpolator = state << 16;
		break;
	case Device::Type::DIMMER:
		if (state <= 100) {
			// set immediately
			this->lastState = this->state;
			this->state = state;
			this->interpolator = state;
			break;
		}
		// fall through
	case Device::Type::BLIND:
		if (state == Device::STOP) {
			// stop
			this->state = (this->interpolator + 0x8000) >> 16;
			this->interpolator = this->state << 16;
		} else {
			// dim
			if (state == Device::DIM) {
				if ((this->lastState > this->state && this->state < 100) || this->state == 0) {
					state = 100;
				} else {
					state = 0;
				}
			} else if (state == Device::LIGHTEN)
				state = 100;
			else if (state == Device::DARKEN)
				state = 0;

			this->lastState = this->state;
			this->state = state;
		}
		break;
	case Device::Type::HANDLE:
		this->lastState = this->state;
		this->state = state;
		this->interpolator = state << 16;
		break;
	}
}

int DeviceState::update(const Device &device, int ticks) {
	int s = int(this->state) << 16;
	if (this->interpolator != s) {
		int step = device.speed * ticks;
		if (this->interpolator < s) {
			// up
			this->interpolator = min(this->interpolator + step, s);
		} else {
			// down
			this->interpolator = max(this->interpolator - step, s);
		}
	}
	
	// check if device drives an internal output
	if (device.binding < 16) {
		switch (device.type) {
		case Device::Type::SWITCH:
		case Device::Type::LIGHT:
			return (this->state == 100 ? 1 : 0) << device.binding;
		case Device::Type::DIMMER:
			// can't drive internal output
			break;
		case Device::Type::BLIND:
			return ((this->interpolator < s ? 1 : 0) | (this->interpolator > s ? 2 : 0)) << device.binding;
		case Device::Type::HANDLE:
			// can't drive internal output
			break;
		}
	}
	return 0;
}
