#pragma once

#include <stdint.h>


/**
 * Action to execute for button or timer event
 * id is device or scenario id
 * state < 128: Set device state
 * state 128 - 253: Do device state transition (state & 0x7f is device transition index)
 * state 254: Set scenario
 * state 255: Empty entry
 */
struct Action {
	// special states and state ranges
	static const int TRANSITION_START = 128;
	static const int SCENARIO = 254;

	uint8_t id;
	uint8_t state;
	
	bool isValid() const {return this->id != 0xff || this->state != 0xff;}
};

struct Actions {
	static const int ACTION_COUNT = 8;

	// list of actions
	Action actions[ACTION_COUNT];

	int getActionCount() const {
		for (int i = 0; i < ACTION_COUNT; ++i) {
			if (!this->actions[i].isValid())
				return i;
		}
		return ACTION_COUNT;
	}
};
