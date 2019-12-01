#pragma once

#include "Action.hpp"


struct Scenario {
	// unique id of scenario, used by buttons and timers
	uint8_t id;
	
	// name
	char name[16];

	// the scenario executes all actions when it is triggered
	// note: must be last in struct!
	Actions actions;

	int byteSize() const {return offsetof(Scenario, actions) + this->actions.byteSize();}
};
