#pragma once

#include "Action.hpp"


struct Event {
	// event input such as local motion detector or enocean switch (e.g. Eltako FT55)
	uint32_t input;

	// extra value, e.g. switch button
	uint8_t value;

	// the event executes the first executable action when it triggers
	// note: must be last in struct!
	Actions actions;

	int byteSize() const {return offsetof(Event, actions) + this->actions.byteSize();}
};
