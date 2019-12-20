#include "DeviceState.hpp"
#include "config.hpp"


bool DeviceState::isActive(const Device &device, uint8_t state) {
	if (state == this->state /*|| state == Device::VALUE*/)
		return true;

	// check if pseudo-states are active
	switch (device.type) {
	case Device::Type::SWITCH:
	case Device::Type::LIGHT:
		break;
	case Device::Type::DIMMER:
	case Device::Type::BLIND:
		if (this->mode == 0) {
			// stopped
			return state == Device::STOP;
		} else {
			// transition is in progress
			int s = int(this->state) << 16;
			return state == Device::DIM
				|| (state == Device::LIGHTEN && this->timer < s)
				|| (state == Device::DARKEN && this->timer > s);
		}
	case Device::Type::HANDLE:
		return (state == Device::SHUT && (this->state == Device::LOCKED || this->state == Device::CLOSED))
			|| (state == Device::UNSHUT && (this->state == Device::OPEN || this->state == Device::TILT))
			|| (state == Device::UNLOCKED && this->state != Device::LOCKED);
	}
	return false;
}

void DeviceState::setState(const Device &device, uint8_t state) {
	uint8_t currentState = this->state;
	switch (device.type) {
	case Device::Type::SWITCH:
	case Device::Type::LIGHT:
		this->lastState = currentState;
		this->state = state;
		
		// start delay
		this->mode = DELAY;
		this->timer = device.getDelay();
		break;
	case Device::Type::DIMMER:
		if (state == Device::OFF || state == Device::ON) {
			// set immediately
			this->lastState = currentState;
			this->state = state;

			// start timeout
			this->mode = 0;
			this->timer = device.getTimeout();
			return;
		}
		// fall through
	case Device::Type::BLIND:
		if (state == Device::STOP || this->mode == TRANSITION) {
			// stop
			if (this->mode == TRANSITION) {
				this->state = (this->timer + 0x8000) >> 16;
				this->mode = 0;
			}
		} else {
			// dim
			if (state == Device::DIM) {
				if ((this->lastState > currentState && currentState < 100) || currentState == 0) {
					state = 100;
				} else {
					state = 0;
				}
			} else if (state == Device::LIGHTEN)
				state = 100;
			else if (state == Device::DARKEN)
				state = 0;

			// start transition
			this->lastState = currentState;
			this->state = state;
			this->mode = TRANSITION;
			this->timer = currentState << 16;
		}
		break;
	case Device::Type::HANDLE:
		this->lastState = currentState;
		this->state = state;
		break;
	}
}

int DeviceState::update(const Device &device, int ticks, int temperature) {
	uint8_t currentState = this->state;

	// flags for internal outputs
	int outputs = 0;
	
	// update device state
	switch (device.type) {
	case Device::Type::SWITCH:
	case Device::Type::LIGHT:
		if (this->mode == DELAY) {
			// delay in progress
			this->timer -= ticks;
			if (this->timer <= 0) {
				// delay reached end
				this->mode = 0;

				// start timeout
				this->timer = device.getTimeout();
			}
		} else if (this->timer > 0) {
			// timeout in progress
			this->timer -= ticks;
			if (this->timer <= 0) {
				// reset to default state
				this->lastState = currentState;
				this->state = Device::OFF;
			}
		}
		outputs = (this->mode == DELAY ? this->lastState : currentState) == 100 ? 1 : 0;
		break;
	case Device::Type::DIMMER:
	case Device::Type::BLIND:
		if (this->mode == TRANSITION) {
			// transition is in progress
			int s = int(currentState) << 16;
			int speed = device.getSpeed();
			int step = ticks * speed;
			if (this->timer < s) {
				// up
				this->timer = min(this->timer + step, s);
				outputs = 1;
			} else {
				// down
				this->timer = max(this->timer - step, s);
				outputs = 2;
			}
			if (speed == 0 || this->timer == s) {
				// transition reached end
				this->mode = 0;
				outputs = 0;
				
				// start timeout
				this->timer = device.getTimeout();
			}
		} else if (this->timer > 0) {
			// timeout in progress
			this->timer -= ticks;
			if (this->timer <= 0) {
				// reset to default state
				this->lastState = currentState;
				this->state = Device::OFF;
				this->timer = int(currentState) << 16;
			}
		}
		break;
	case Device::Type::HANDLE:
		// can't drive internal output
		break;
	}

	if (device.output < OUTPUT_COUNT)
		return outputs << device.output;
	return 0;
}
