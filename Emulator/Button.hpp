#pragma once

#include "Action.hpp"


struct Button {
	// maximum number of actions per button
	static const int ACTION_COUNT = 8;

	// enocean id of button
	uint32_t id;
	
	// button state
	uint8_t state;
	
	// list of actions, the first feasible action gets executed
	Action actions[ACTION_COUNT];

	int getActionCount() const {
		for (int i = 0; i < ACTION_COUNT; ++i) {
			if (!this->actions[i].isValid())
				return i;
		}
		return ACTION_COUNT;
	}
};
