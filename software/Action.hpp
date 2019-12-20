#pragma once

#include "util.hpp"
#include <stdint.h>
#include <stddef.h>


/**
 * Action to execute for event or timer
 */
struct Action {
	// special states and state ranges
	static const uint8_t TRANSITION_START = 128;
	static const uint8_t TRANSITION_END = 253;
	static const uint8_t SCENARIO = 254;
	
	/**
	 * device or scenario id
	 */
	uint8_t id;
	
	/**
	 * state < 128: Device state
	 * state 128 - 253: Device state transition (state - 128 is device state transition index)
	 * state 254: Scenario
	 * state 255: Empty entry
	 */
	uint8_t state;
	
	bool isValid() const {return this->id != 0xff || this->state != 0xff;}
};

struct Actions {
	static const int ACTION_COUNT = 32;

	uint8_t count;

	// list of actions
	Action actions[ACTION_COUNT];

	void clear() {
		this->count = 0;
	}

	int size() const {
		return this->count;
	}
	
	void set(int index, Action action) {
		assert(index >= 0 && index <= this->count);
		if (index == this->count)
			++this->count;
		this->actions[index] = action;
	}
	
	void erase(int index) {
		assert(index >= 0 && index < this->count);
		--this->count;
		for (int i = index; i < this->count; ++i) {
			this->actions[i] = this->actions[i + 1];
		}
	}
	
	const Action operator[](int index) const {
		assert(index >= 0 && index < this->count);
		return this->actions[index];
	}

	bool deleteId(uint8_t id, uint8_t firstState, uint8_t lastState);
	
	const Action *begin() const {return this->actions;}
	const Action *end() const {return this->actions + this->count;}

	int byteSize() const {return offsetof(Actions, actions) + this->count * sizeof(Action);}
};
