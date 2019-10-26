#pragma once

#include "Action.hpp"


struct Scenario {
	// maximum number of actions per scenario
	static const int ACTION_COUNT = 8;

	// unique id of scenario, used by buttons and timers
	uint8_t id;
	
	// name
	char name[16];

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
